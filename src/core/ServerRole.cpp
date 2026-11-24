//
// Created by leoul on 2026-11-18.
//

#include "ServerRole.h"
#include <cmath>
#include <ranges>
#include "NetMessage.h"
#include "NetworldCore.h"

namespace networld {

    NetBatch ServerRole::extract_batch(const NetWorldCore& core, const flecs::iter& it) const {
        NetBatch batch;
        const flecs::iter_t* c_it = it.c_ptr();
        const auto entity_count = static_cast<uint16_t>(c_it->count);

        // Copie des IDs d'entités Flecs vers le vecteur uint64_t neutre
        batch.entities.resize(entity_count);
        std::memcpy(batch.entities.data(), c_it->entities, entity_count * sizeof(uint64_t));

        for (int i = 0; i < c_it->field_count; ++i) {
            flecs::id field_id = it.id(i);
            const flecs::id_t comp_id = field_id.raw_id();

            const uint16_t net_id = core.get_registry().get_net_id(comp_id);
            const size_t comp_size = ecs_field_size(c_it, i);

            if (net_id == 0 || comp_size == 0) continue;

            NetColumn col;
            col.net_id = net_id;
            col.element_size = static_cast<uint16_t>(comp_size);

            const void* column_data = ecs_field_w_size(c_it, comp_size, i);
            size_t payload_size = entity_count * comp_size;

            col.payload.resize(payload_size);
            std::memcpy(col.payload.data(), column_data, payload_size);

            batch.columns.push_back(std::move(col));
        }

        return batch;
    }

    void ServerRole::on_start(NetWorldCore &core) {

        PacketDispatcher& packet_dispatcher = core.get_dispatcher();
        packet_dispatcher.register_handler<KnockMsg>([](uint32_t /*peer_id*/, const KnockMsg&) {
            // printf("NetWorldCore::start_host : peer %u ready\n", peer_id);
        });

        packet_dispatcher.register_handler<InputMsg>([this](uint32_t peer_id, const InputMsg& msg) {
            if (!connected_peers_.contains(peer_id)) return;

            auto it_seq = last_received_seq_.find(peer_id);
            if (it_seq != last_received_seq_.end() && msg.seq <= it_seq->second) return;

            last_received_seq_[peer_id] = msg.seq;

            auto& queue = peer_input_queues_[peer_id];
            if (queue.size() >= MAX_PAYLOAD_INPUTS) {
                queue.pop_front();
            }
            queue.push_back(PendingServerInput{msg.seq, msg.target_entity_id, msg.payload});
        });

        // build query
        sync_query_ = core.get_ecs()
            .query_builder<Position3D, Rotation3D, Velocity3D>()
            .with<Networked>()
            .cached()
            .build();

        sync_accumulator_ = 0.0f;
        if (mode_ == HostMode::LISTEN) {
            core.push_connected_peers(PeerEvent{PEER_SERVER, true});
        }
    }

    void ServerRole::on_stop(NetWorldCore &core) {
        std::vector<uint64_t> to_destroy;
        auto query = core.get_ecs().query_builder().with<Networked>().build();
        query.each([&](flecs::entity e) {
            to_destroy.push_back(e.id());
        });
        for (uint64_t entity_id : to_destroy) {
            core.push_despawned_entity(entity_id);
            core.destroy_entity(entity_id);
        }
        connected_peers_.clear();
        peer_input_queues_.clear();
        last_received_seq_.clear();
        last_ack_seq_.clear();
        host_local_inputs_.clear();
        sync_accumulator_ = 0.0f;
    }

    void ServerRole::on_peer_connected(NetWorldCore & core, uint32_t peer_id) {
        if (!connected_peers_.insert(peer_id).second) return;

        auto welcome_bytes = encode_packet(WelcomeMsg{peer_id, core.get_current_tick()});
        core.get_transport()->send(peer_id, NetChannel::RELIABLE_EVENTS, welcome_bytes.data(), welcome_bytes.size());

        auto query = core.get_ecs()
            .query_builder<const Position3D, const Rotation3D, const NetType, const NetOwnership>()
            .with<Networked>()
            .build();

        query.each([&](flecs::entity e, const Position3D& pos, const Rotation3D& rot,
                            const NetType& type,const NetOwnership& ownership) {
            const SpawnEntityMsg msg{
                e.id(),
                type.type_id,
                ownership.owner_peer_id,
                pos.value,
                rot.value,
                false // un client qui arrive ne possède pas encore d'entité pré-existante, donc pas de prediction pour l'instant
            };
            auto bytes = encode_packet(msg);
            core.get_transport()->send(peer_id, NetChannel::RELIABLE_EVENTS, bytes.data(), bytes.size());
        });

        core.push_connected_peers(PeerEvent{peer_id, false});
    }

    void ServerRole::on_peer_disconnected(NetWorldCore &core, uint32_t peer_id) {
        if (connected_peers_.erase(peer_id) == 0) return;

        peer_input_queues_.erase(peer_id);
        last_received_seq_.erase(peer_id);
        last_ack_seq_.erase(peer_id);

        core.push_disconnected_peers(peer_id);
    }

    void ServerRole::on_egress(NetWorldCore &core, float delta_time) {
        if (!sync_query_) return;

        sync_accumulator_ += delta_time;
        if (sync_accumulator_ < NET_SYNC_INTERVAL) return;
        sync_accumulator_ = std::fmod(sync_accumulator_, NET_SYNC_INTERVAL);

        // broadcast le sync state msg pour les autres clients
        StateSyncMsg sync_msg;
        sync_msg.tick = core.get_current_tick();

        if (connected_peers_.empty()) return;

        sync_query_.run([&](flecs::iter& it) {
            while (it.next()) {
                NetBatch batch = extract_batch(core, it);
                if (!batch.entities.empty()) {
                    sync_msg.batches.push_back(std::move(batch));
                }
            }
        });

        if (sync_msg.batches.empty()) return;

        for (uint32_t peer_id: connected_peers_) {
            auto it = last_ack_seq_.find(peer_id);
            sync_msg.ack_input_seq = (it != last_ack_seq_.end()) ? it->second : 0;

            std::vector<uint8_t> out = encode_packet(sync_msg);
            core.get_transport()->send(peer_id, NetChannel::UNRELIABLE_STATE, out.data(), out.size());
        }
    }

    void ServerRole::on_fixed_tick(NetWorldCore &core, float fixed_dt) {
        for (auto& [peer_id, queue] : peer_input_queues_) {
            if (queue.empty()) continue;

            PendingServerInput item = queue.front();
            queue.pop_front();

            flecs::entity e = core.get_ecs().entity(item.target_entity_id);
            if (e.is_alive()) {
                const auto* own = e.try_get<NetOwnership>();
                if (own && own->owner_peer_id == peer_id) {
                    SimContext ctx{core, fixed_dt, item.seq, false};
                    core.run_entity_simulator(item.target_entity_id, item.payload, ctx);
                }
            }

            last_ack_seq_[peer_id] = item.seq;
        }

        if (mode_ == HostMode::LISTEN) {
            for (const auto& [entity_id, payload] : host_local_inputs_) {
                flecs::entity e = core.get_ecs().entity(entity_id);
                if (!e.is_alive()) continue;
                const auto* own = e.try_get<NetOwnership>();
                if (own && own->owner_peer_id == PEER_SERVER) {
                    SimContext ctx{core,fixed_dt, 0, false};
                    core.run_entity_simulator(entity_id, payload, ctx);
                }
            }
            host_local_inputs_.clear();
        }
    }

    void ServerRole::on_entity_spawned(NetWorldCore &core, uint64_t entity_id, const SpawnParams &params) {
        if (connected_peers_.empty()) return;

        SpawnEntityMsg msg {
            entity_id,
            params.type_id,
            params.owner_peer_id,
            params.position,
            params.rotation.normalized(),
            params.predicted
        };
        std::vector<uint8_t> out = encode_packet(msg);
        for (const uint32_t& peer_id: connected_peers_) {
            core.get_transport()->send(peer_id, NetChannel::RELIABLE_EVENTS, out.data(), out.size());
        }
    }

    void ServerRole::on_entity_despawned(NetWorldCore &core, uint64_t entity_id) {
        if (connected_peers_.empty()) return;

        DespawnEntityMsg msg {
            entity_id
        };

        std::vector<uint8_t> out = encode_packet(msg);
        for (const uint32_t& peer_id: connected_peers_) {
            core.get_transport()->send(peer_id, NetChannel::RELIABLE_EVENTS, out.data(), out.size());
        }
    }

    void ServerRole::set_local_input(uint64_t target_local_entity_id, const InputPayload &payload) {
        if (mode_ != HostMode::LISTEN || target_local_entity_id == 0) return;
        host_local_inputs_[target_local_entity_id] = payload;
    }
} // networld