//
// Created by leoul on 2026-11-09.
//

#include "NetworldCore.h"
#include "NetComponent.h"
#include "deserialization.h"
#include "serialization.h"
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
                    flecs::entity_t entity_id = batch.entities[i];
                    if (!ecs_.is_alive(entity_id)) continue;

                    const uint8_t* elem_ptr = col.payload.data() + (i * col.element_size);
                    ecs_set_id(ecs_.c_ptr(), entity_id, local_id, col.element_size, elem_ptr);
                }
            }
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
        is_server_ = true;
        transport_->open_as_host(port);

        // Entite serveur de test
        ecs_.entity("ServerPlayer")
           .set<Position3D>({Vec3(0, 5, 0)})
           .set<Velocity3D>({Vec3(1, 0, 0)})
           .add<Networked>();

        return true;
    }

    bool NetWorldCore::start_client(const std::string &host_ip, uint16_t port) {
        if (!transport_) return false;
        is_server_ = false;
        transport_->open_as_client(host_ip, port);
        ecs_.entity("ServerPlayer")
            .set<Position3D>({Vec3(0, 0, 0)})
            .set<Velocity3D>({Vec3(0, 0, 0)})
            .add<Networked>();

        uint8_t knock_packet = 0xFF;
        transport_->send(1, NetChannel::UNRELIABLE_STATE, &knock_packet, 1);
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
            std::vector<NetBatch> batches = deserialize_buffer(pkt.payload.data(), pkt.payload.size());
            apply_batches(batches);
        }

        // SIMULATION ECS
        view_updates_.clear();
        ecs_.progress(delta_time);

        // EGRESS : Extraction NetWorld -> Sérialisation pure -> Réseau
        if (is_server_) {
            std::vector<uint8_t> out_buffer;
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
            }
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

    void NetWorldCore::bind_external_view(const char* name, uint64_t host_handle) {
        flecs::entity e = ecs_.entity(name);
        if (e.is_alive()) {
            e.set<ExternalView>({host_handle});
        }
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