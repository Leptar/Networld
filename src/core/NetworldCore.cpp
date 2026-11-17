//
// Created by leoul on 2026-11-09.
//

#include "NetworldCore.h"
#include "NetComponent.h"
#include <cstdio>
#include "../../thirdparty/flecs/flecs.h"
#include "NetMessage.h"

namespace networld {

    NetBatch NetWorldCore::extract_batch(const flecs::iter &it) const {
        NetBatch batch;
        const flecs::iter_t* c_it = it.c_ptr();
        const auto entity_count = static_cast<uint16_t>(c_it->count);

        // Copie des IDs d'entités Flecs vers le vecteur uint64_t neutre
        batch.entities.resize(entity_count);
        std::memcpy(batch.entities.data(), c_it->entities, entity_count * sizeof(uint64_t));

        for (int i = 0; i < c_it->field_count; ++i) {
            flecs::id field_id = it.id(i);
            const flecs::id_t comp_id = field_id.raw_id();

            const uint16_t net_id = registry_.get_net_id(comp_id);
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

    void NetWorldCore::apply_batches(const std::vector<NetBatch> &batches) const {
        for (const NetBatch& batch : batches) {
            size_t entity_count = batch.entities.size();

            for (const NetColumn& col : batch.columns) {
                flecs::id_t local_id = registry_.get_local_id(col.net_id);
                if (local_id == 0 || col.element_size == 0) continue;

                for (size_t i = 0; i < entity_count; ++i) {
                    auto it = net_to_local_entities_.find(batch.entities[i]);
                    if (it == net_to_local_entities_.end()) continue;
                    flecs::entity_t entity_id = it->second;
                    if (!ecs_.is_alive(entity_id)) continue;

                    const uint8_t* elem_ptr = col.payload.data() + (i * col.element_size);
                    ecs_set_id(ecs_.c_ptr(), entity_id, local_id, col.element_size, elem_ptr);
                }
            }
        }
    }

    void NetWorldCore::on_peer_connected(uint32_t peer_id) {
        if (!is_server_) return;

        // entity du joueur
        uint64_t new_entity_id = create_networked_entity(Vec3{0,5,0});
        peer_entities_[peer_id] = new_entity_id;
        spawned_entities_.push_back({new_entity_id, Vec3{0, 5, 0}});

        // send au nouveau joueur ce qui existe deja
        auto query = ecs_.query_builder<const Position3D>().with<Networked>().build();
        query.each([&](flecs::entity e, const Position3D& pos) {
            SpawnEntityMsg msg{e.id(), pos.value, Vec3{}};
            auto bytes = encode_packet(msg);
            transport_->send(peer_id, NetChannel::RELIABLE_EVENTS, bytes.data(), bytes.size());
        });

        // broadcast sont arrivé
        SpawnEntityMsg new_msg{new_entity_id, Vec3{0,5,0}, Vec3{}};
        auto out = encode_packet(new_msg);
        for (const auto& [other_peer_id, _] : peer_entities_) {
            if (other_peer_id != peer_id) {
                transport_->send(other_peer_id, NetChannel::RELIABLE_EVENTS, out.data(), out.size());
            }
        }
    }

    void NetWorldCore::on_peer_disconnected(uint32_t peer_id) {
        if (!is_server_) return;

        auto it = peer_entities_.find(peer_id);
        if (it == peer_entities_.end()) return;

        uint64_t entity_id = it->second;
        peer_entities_.erase(it);
        destroy_entity(entity_id);
        despawned_entities_.push_back(entity_id);

        auto out = encode_packet(DespawnEntityMsg{entity_id});
        transport_->send(PEER_BROADCAST, NetChannel::RELIABLE_EVENTS, out.data(), out.size());
    }

    void NetWorldCore::bind_entity_view(uint64_t entity_id, uint64_t host_handle) {
        flecs::entity e = ecs_.entity(entity_id);
        if (e.is_alive() && host_handle != 0) {
            e.set<ExternalView>({host_handle});
        }
    }

    NetWorldCore::NetWorldCore() {
        registry_.register_component(ecs_.component<Position3D>().id(), 10);
        registry_.register_component(ecs_.component<Velocity3D>().id(), 11);
        registry_.register_component(ecs_.component<Networked>(), 12);

        ecs_.component<Vec3>()
                .member<float>("x")
                .member<float>("y")
                .member<float>("z");

        ecs_.component<Position3D>().member<Vec3>("value");
        ecs_.component<Velocity3D>().member<Vec3>("value");
        ecs_.component<Gravity>().member<Vec3>("force");

        ecs_.set<flecs::Rest>({});
        // Système 1 : Appliquer la gravité (modifie la vélocité)
        ecs_.system<Velocity3D, const Gravity>("ApplyGravity")
            .each([](flecs::iter& it, size_t index, Velocity3D& v, const Gravity& g) {
                v.value += g.force * it.delta_time();
            });

        // Système 2 : Appliquer le mouvement (modifie la position)
        ecs_.system<Position3D, const Velocity3D>("ApplyMovement")
            .each([](flecs::iter& it, size_t index, Position3D& p, const Velocity3D& v) {
                p.value += v.value * it.delta_time();
            });

        // Systeme 3 change ducoup
        ecs_.system<const Position3D, const ExternalView>("CollectViewUpdates")
            .each([this](flecs::entity e, const Position3D& p, const ExternalView& v) {
                if (v.host_handle != 0) {
                    // On enregistre la mise à jour dans le tampon de sortie
                    view_updates_.push_back({e.id(), v.host_handle, p.value});
                }
            });
    }

    NetWorldCore::~NetWorldCore() {
        if (transport_) transport_->close();
    }

    void NetWorldCore::set_transport(std::unique_ptr<INetTransport> transport) {
        transport_ = std::move(transport);
    }

    bool NetWorldCore::start_host(uint16_t port) {
        if (!transport_) return false;

        packet_dispatcher_ = PacketDispatcher{};
        packet_dispatcher_.register_handler<KnockMsg>([](uint32_t /*peer_id*/, const KnockMsg&) {
            // printf("NetWorldCore::start_host : peer %u ready\n", peer_id);
        });

        peer_entities_.clear();
        net_to_local_entities_.clear();
        current_tick_ = 0;
        last_received_tick_ = 0;
        is_server_ = true;
        transport_->open_as_host(port);

        // Entite serveur de test
        auto server_player = ecs_.entity("ServerPlayer")
           .set<Position3D>({Vec3(0, 5, 0)})
           .set<Velocity3D>({Vec3(1, 0, 0)})
           .add<Networked>();
        spawned_entities_.push_back({server_player.id(), Vec3(0, 5, 0)});
        return true;
    }

    bool NetWorldCore::start_client(const std::string &host_ip, uint16_t port) {
        if (!transport_) return false;

        packet_dispatcher_ = PacketDispatcher{};

        packet_dispatcher_.register_handler<StateSyncMsg>([this](uint32_t /*peer_id*/, const StateSyncMsg& msg) {
            if (msg.tick <= last_received_tick_) return;
            last_received_tick_ = msg.tick;
            apply_batches(msg.batches);

            if (msg.tick % 60 == 0 || last_received_tick_ == 0) {
                /*printf("[CLIENT] StateSync tick=%u batches=%zu\n", msg.tick, msg.batches.size());
                fflush(stdout);*/
            }
        });

        packet_dispatcher_.register_handler<SpawnEntityMsg>([this](uint32_t , const SpawnEntityMsg& msg) {
            if (net_to_local_entities_.contains(msg.entity_Id)) return;

            uint64_t local_id = create_networked_entity(msg.position);
            net_to_local_entities_[msg.entity_Id] = local_id;

            spawned_entities_.push_back({local_id, msg.position});

            printf("[CLIENT] SpawnEntityMsg server_id=%llu -> local_id=%llu (%.2f, %.2f, %.2f)\n",
                    msg.entity_Id, local_id, msg.position.x, msg.position.y, msg.position.z);
            fflush(stdout);
        });

        packet_dispatcher_.register_handler<DespawnEntityMsg>([this](uint32_t , const DespawnEntityMsg& msg) {
            auto it = net_to_local_entities_.find(msg.entity_Id);
            if (it == net_to_local_entities_.end()) return;

            despawned_entities_.push_back(it->second);

            destroy_entity(it->second);
            net_to_local_entities_.erase(it);
        });

        peer_entities_.clear();
        net_to_local_entities_.clear();
        current_tick_ = 0;
        last_received_tick_ = 0;
        is_server_ = false;
        transport_->open_as_client(host_ip, port);

        auto knock = encode_packet(KnockMsg{});
        transport_->send(PEER_SERVER, NetChannel::RELIABLE_EVENTS, knock.data(), knock.size());
        return true;
    }

    void NetWorldCore::stop() {

    }

    void NetWorldCore::step(float delta_time) {
        if (!transport_) return;

        // INGRESS : Réseau -> Désérialisation pure -> Application au NetWorld
        transport_->poll();
        NetPacket pkt;
        while (transport_->recv(pkt)) {
            // je check les events du packet pour voir comment traiter le packet
            switch (pkt.event) {
                case NetEvent::CONNECTED :
                    on_peer_connected(pkt.sender_peer_id);
                    break;
                case NetEvent::DISCONNECTED :
                    on_peer_disconnected(pkt.sender_peer_id);
                    break;
                case NetEvent::DATA :
                    if (!packet_dispatcher_.dispatch(pkt.sender_peer_id, pkt.payload)) {
                        printf("NetWorldCore::step case Data: error packet dispatcher\n");
                    }
                    break;
                default:
                    printf("Bro wtf ?");
                    break;
            }
        }

        // SIMULATION ECS
        view_updates_.clear();
        current_tick_++;
        ecs_.progress(delta_time);

        // EGRESS : Extraction NetWorld -> Sérialisation pure -> Réseau
        if (is_server_) {
            // broadcast le sync state msg pour les autres clients
            StateSyncMsg sync_msg;
            sync_msg.tick = current_tick_;

            auto query = ecs_.query_builder<Position3D, Velocity3D>().with<Networked>().build();
            query.run([&](flecs::iter& it) {
                while (it.next()) {
                    NetBatch batch = extract_batch(it);
                    if (!batch.entities.empty()) {
                        sync_msg.batches.push_back(std::move(batch));
                    }
                }
            });

            if (!sync_msg.batches.empty()) {
                std::vector<uint8_t> out = encode_packet(sync_msg);
                transport_->send(PEER_BROADCAST, NetChannel::UNRELIABLE_STATE, out.data(), out.size());
            }

            /*std::vector<uint8_t> out_buffer;
            auto query = ecs_.query_builder<Position3D, Velocity3D>().with<Networked>().build();

            query.run([&](flecs::iter& it) {
                while (it.next()) {
                    NetBatch batch = extract_batch(it);
                    std::vector<uint8_t> block = serialize_batch(batch);
                    out_buffer.insert(out_buffer.end(), block.begin(), block.end());
                }
            });

            if (!out_buffer.empty()) {
                transport_->send(0, NetChannel::UNRELIABLE_STATE, out_buffer.data(), out_buffer.size());
            }*/
        } else if (current_tick_ % 60 == 0){
            auto knock = encode_packet(KnockMsg{});
            transport_->send(PEER_SERVER, NetChannel::UNRELIABLE_STATE, knock.data(), knock.size());
        }

    }

    uint64_t NetWorldCore::create_networked_entity(const Vec3 &initial_pos, uint64_t host_handle, const char *name) {
        auto e = ecs_.entity(name)
            .set<Position3D>({initial_pos})
            .set<Velocity3D>({{0, 0, 0}})
            .add<Networked>();
        if (host_handle != 0) {
            e.set<ExternalView>({host_handle});
        }
        return e.id();
    }

    void NetWorldCore::destroy_entity(uint64_t entity_id) {
        flecs::entity e = ecs_.entity(entity_id);
        e.destruct();
    }

    uint64_t NetWorldCore::find_entity(const char *name) const {
        flecs::entity e = ecs_.lookup(name);
        return e.id();

    }

    void NetWorldCore::set_entity_position(uint64_t entity_id, const Vec3 &pos) {
        flecs::entity e = ecs_.entity(entity_id);
        if (e.is_alive()) {
            e.set<Position3D>({pos});
        }
    }

    void NetWorldCore::set_entity_velocity(uint64_t entity_id, const Vec3 &vel) {
        flecs::entity e = ecs_.entity(entity_id);
        if (e.is_alive()) {
            e.set<Velocity3D>({vel});
        }
    }
} // networld