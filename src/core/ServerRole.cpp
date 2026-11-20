//
// Created by leoul on 2026-11-18.
//

#include "ServerRole.h"
#include <cmath>
#include <ranges>
#include "MovementSim.h"
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
        core.set_simulation_systems_enabled(true);
        core.set_interpolation_systems_enabled(false);

        PacketDispatcher& packet_dispatcher = core.get_dispatcher();
        packet_dispatcher.register_handler<KnockMsg>([](uint32_t /*peer_id*/, const KnockMsg&) {
            // printf("NetWorldCore::start_host : peer %u ready\n", peer_id);
        });

        packet_dispatcher.register_handler<InputMsg>([this, &core](uint32_t peer_id, const InputMsg& input_msg) {
            auto it = peer_entities_.find(peer_id);
            if (it == peer_entities_.end()) return;

            auto it_input = peer_inputs.find(peer_id);
            if ( it_input != peer_inputs.end()
                && it_input->second >= input_msg.tick_input) return;

            peer_inputs[peer_id] = input_msg.tick_input;

            flecs::entity e = core.get_ecs().entity(it->second);
            if (!e.is_alive()) return;

            auto* pos = e.try_get_mut<Position3D>();
            auto* rot = e.try_get_mut<Rotation3D>();
            if (!pos || !rot) return;
            simulate_player_step(pos->value, rot->value, input_msg.direction, FIXED_DT);
            /*if (!std::isfinite(input_msg.direction.x) || !std::isfinite(input_msg.direction.z)) return;

            Vec3 dir_input;
            float norme = std::sqrt(input_msg.direction.x*input_msg.direction.x
                                 + input_msg.direction.z*input_msg.direction.z);

            dir_input.x = norme > 1 ? input_msg.direction.x/norme : input_msg.direction.x;
            dir_input.y = 0;
            dir_input.z = norme > 1 ? input_msg.direction.z/norme : input_msg.direction.z;

            uint64_t entity_id = it->second;
            core.set_entity_velocity(entity_id, dir_input * PLAYER_MOVE_SPEED);
            if (norme > 0.001f) {
                float yaw = std::atan2(dir_input.x, dir_input.z);
                core.set_entity_rotation(entity_id, Quat::from_yaw(yaw));
            }*/
        });

        // build query
        sync_query_ = core.get_ecs()
            .query_builder<Position3D, Rotation3D, Velocity3D>()
            .with<Networked>()
            .cached()
            .build();

        // Entite serveur de test
        auto server_player = core.get_ecs().entity("ServerPlayer")
           .set<Position3D>({Vec3(0, 5, 0)})
           .set<Velocity3D>({Vec3(1, 0, 0)})
           .set<Rotation3D>({Quat{}})
           .add<Networked>();

        server_player_entity_id_ = server_player.id();
        core.push_spawned_entity({server_player.id(), Vec3(0, 5, 0)});
        sync_accumulator_ = 0.0f;
    }

    void ServerRole::on_stop(NetWorldCore &core) {
        for (uint64_t entity_id: peer_entities_ | std::views::values) {
            core.push_despawned_entity(entity_id);
            core.destroy_entity(entity_id);
        }

        peer_entities_.clear();
        peer_inputs.clear();
        if (server_player_entity_id_ != 0) {
            core.push_despawned_entity(server_player_entity_id_);
            core.destroy_entity(server_player_entity_id_);
            server_player_entity_id_ = 0;
        }

        sync_accumulator_ = 0.0f;
        // TODO : voir cette version plus tard ptetre (si c'est pertinent)
        /*
         *  std::vector<uint64_t> to_destroy;
            auto query = core.get_ecs().query_builder().with<Networked>().build();
            query.each([&](flecs::entity e) {
                to_destroy.push_back(e.id());
            });
            for (uint64_t entity_id : to_destroy) {
                core.push_despawned_entity(entity_id);
                core.destroy_entity(entity_id);
            }
            peer_entities_.clear();
            peer_inputs_.clear();
         *
         */
    }

    void ServerRole::on_peer_connected(NetWorldCore &core, uint32_t peer_id) {
        // entity du joueur
        uint64_t new_entity_id = core.create_networked_entity(Vec3{0,5,0});
        peer_entities_[peer_id] = new_entity_id;
        core.push_spawned_entity({new_entity_id, Vec3{0, 5, 0}});

        // send au nouveau joueur ce qui existe deja
        auto query = core.get_ecs().query_builder<const Position3D, const Rotation3D>().with<Networked>().build();
        query.each([&](flecs::entity e, const Position3D& pos, const Rotation3D& rot) {
            const bool is_owner = (e.id() == new_entity_id);
            const SpawnEntityMsg msg{e.id(), pos.value, rot.value, is_owner};
            auto bytes = encode_packet(msg);
            core.get_transport()->send(peer_id, NetChannel::RELIABLE_EVENTS, bytes.data(), bytes.size());
        });

        // broadcast sont arrivé
        const SpawnEntityMsg new_msg{new_entity_id, Vec3{0,5,0}, Quat{}, false};
        auto out = encode_packet(new_msg);
        for (const auto &other_peer_id: peer_entities_ | std::views::keys) {
            if (other_peer_id != peer_id) {
                core.get_transport()->send(other_peer_id, NetChannel::RELIABLE_EVENTS, out.data(), out.size());
            }
        }
    }

    void ServerRole::on_peer_disconnected(NetWorldCore &core, uint32_t peer_id) {
        auto it = peer_entities_.find(peer_id);
        if (it == peer_entities_.end()) return;

        uint64_t entity_id = it->second;
        peer_entities_.erase(it);
        core.destroy_entity(entity_id);
        peer_inputs.erase(peer_id);
        core.push_despawned_entity(entity_id);

        auto out = encode_packet(DespawnEntityMsg{entity_id});
        core.get_transport()->send(PEER_BROADCAST, NetChannel::RELIABLE_EVENTS, out.data(), out.size());
    }

    void ServerRole::on_egress(NetWorldCore &core, float delta_time) {
        if (!sync_query_) return;

        sync_accumulator_ += delta_time;
        if (sync_accumulator_ < NET_SYNC_INTERVAL) return;
        sync_accumulator_ = std::fmod(sync_accumulator_, NET_SYNC_INTERVAL);

        // broadcast le sync state msg pour les autres clients
        StateSyncMsg sync_msg;
        sync_msg.tick = core.get_current_tick();

        sync_query_.run([&](flecs::iter& it) {
            while (it.next()) {
                NetBatch batch = extract_batch(core, it);
                if (!batch.entities.empty()) {
                    sync_msg.batches.push_back(std::move(batch));
                }
            }
        });

        if (sync_msg.batches.empty()) return;

        for (uint32_t peer_id: peer_entities_ | std::views::keys) {
            auto it = peer_inputs.find(peer_id);
            sync_msg.ack_input_seq = (it != peer_entities_.end()) ? it->second : 0;

            std::vector<uint8_t> out = encode_packet(sync_msg);
            core.get_transport()->send(peer_id, NetChannel::UNRELIABLE_STATE, out.data(), out.size());
        }

        /*if (!sync_msg.batches.empty()) {
            std::vector<uint8_t> out = encode_packet(sync_msg);
            core.get_transport()->send(PEER_BROADCAST, NetChannel::UNRELIABLE_STATE, out.data(), out.size());
        }*/
    }
} // networld