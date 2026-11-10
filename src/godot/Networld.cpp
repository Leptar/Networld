//
// Created by leoul on 2026-11-02.
//

#include "NetWorld.h"
#include "flecs.h"
#include "../components.h"
#include "serialization.h"
#include "../core/deserialization.h"
#include "GodotUDPTransport.h"
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

// convertit un itérateur Flecs en NetBatch neutre
NetBatch NetWorld::extract_batch(flecs::iter& it) const {
    NetBatch batch;
    const flecs::iter_t* c_it = it.c_ptr();
    uint16_t entity_count = static_cast<uint16_t>(c_it->count);

    // Copie des IDs d'entités Flecs vers le vecteur uint64_t neutre
    batch.entities.resize(entity_count);
    std::memcpy(batch.entities.data(), c_it->entities, entity_count * sizeof(uint64_t));

    for (int i = 0; i < c_it->field_count; ++i) {
        flecs::id field_id = it.id(i);
        flecs::id_t comp_id = field_id.raw_id();

        uint16_t net_id = registry.get_net_id(comp_id);
        size_t comp_size = ecs_field_size(c_it, i);

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

// applique les NetBatch désérialisés au monde Flecs
void NetWorld::apply_batches(const std::vector<NetBatch>& batches) {
    for (const NetBatch& batch : batches) {
        size_t entity_count = batch.entities.size();

        for (const NetColumn& col : batch.columns) {
            flecs::id_t local_id = registry.get_local_id(col.net_id);
            if (local_id == 0 || col.element_size == 0) continue;

            for (size_t i = 0; i < entity_count; ++i) {
                flecs::entity_t entity_id = static_cast<flecs::entity_t>(batch.entities[i]);
                if (!ecs.is_alive(entity_id)) continue;

                const uint8_t* elem_ptr = col.payload.data() + (i * col.element_size);
                ecs_set_id(ecs.c_ptr(), entity_id, local_id, col.element_size, elem_ptr);
            }
        }
    }
}

void NetWorld::_bind_methods() {
    ClassDB::bind_method(D_METHOD("host", "port"), &NetWorld::host);
    ClassDB::bind_method(D_METHOD("join", "ip", "port"), &NetWorld::join);
    ClassDB::bind_method(
        D_METHOD(
            "bind_view_to_server_player",
            "visual_node"),
        &NetWorld::bind_view_to_server_player);
}

void NetWorld::bind_view_to_server_player(Node3D* visual_node) {

    flecs::entity e = ecs.lookup("ServerPlayer");
    if (e.is_alive()) {
        e.set<GodotView>({visual_node});
        UtilityFunctions::print("[NetWorld] Marionnette Godot attachee a ServerPlayer !");
    }
}

NetWorld::NetWorld() {
    set_process(true);
    
    registry.register_component(ecs.component<Position3D>().id(), 10);
    registry.register_component(ecs.component<Velocity3D>().id(), 11);
    ecs.component<Networked>();

    ecs.component<Vector3>()
            .member<float>("x")
            .member<float>("y")
            .member<float>("z");

    ecs.component<Position3D>().member<Vector3>("value");
    ecs.component<Velocity3D>().member<Vector3>("value");
    ecs.component<Gravity>().member<Vector3>("force");

    ecs.set<flecs::Rest>({});
    // Système 1 : Appliquer la gravité (modifie la vélocité)
    ecs.system<Velocity3D, const Gravity>("ApplyGravity")
        .each([](flecs::iter& it, size_t index, Velocity3D& v, const Gravity& g) {
            v.value += g.force * it.delta_time();
        });

    // Système 2 : Appliquer le mouvement (modifie la position)
    ecs.system<Position3D, const Velocity3D>("ApplyMovement")
        .each([](flecs::iter& it, size_t index, Position3D& p, const Velocity3D& v) {
            p.value += v.value * it.delta_time();
        });

    // Système 3 : Synchroniser les données ECS vers le Scene Tree Godot
    ecs.system<const Position3D, const GodotView>("GodotViewSyncSystem")
    .each([](flecs::entity e, const Position3D& p, const GodotView& v) {
        if (v.node != nullptr && Object::cast_to<Node3D>(v.node)) {
            // Mise à jour de la position du nœud graphique
            v.node->set_global_position(p.value);
        }
    });
}

NetWorld::~NetWorld() {
    if (transport) transport->close();
}

void NetWorld::host(int port) {
    is_server = true;
    transport = std::make_unique<GodotUdpTransport>();
    transport->open_as_host(port);

    // Entite serveur de test
    ecs.entity("ServerPlayer")
       .set<Position3D>({Vector3(0, 5, 0)})
       .set<Velocity3D>({Vector3(1, 0, 0)})
       .add<Networked>();
}

void NetWorld::join(String ip, int port) {
    is_server = false;
    transport = std::make_unique<GodotUdpTransport>();
    transport->open_as_client(ip.utf8().get_data(), port);

    ecs.entity("ServerPlayer")
       .set<Position3D>({Vector3(0, 0, 0)})
       .set<Velocity3D>({Vector3(0, 0, 0)})
       .add<Networked>();

    uint8_t knock_packet = 0xFF;
    transport->send(1, NetChannel::UNRELIABLE_STATE, &knock_packet, 1);
}

void NetWorld::spawn_local_entity(Vector3 pos, Node3D *visual_node) {
    auto e = ecs.entity()
       .set<Position3D>({pos})
       .set<Velocity3D>({Vector3(0, 0, 0)}) // Vélocité initiale nulle
       .set<GodotView>({visual_node})       // Le pont pour que GodotViewSyncSystem bouge le nœud
       .add<Networked>();
}

void NetWorld::_process(double delta) {
    if (!transport) return;

    // INGRESS : Réseau -> Désérialisation pure -> Application au NetWorld
    transport->poll();
    NetPacket pkt;
    while (transport->recv(pkt)) {
        std::vector<NetBatch> batches = deserialize_buffer(pkt.payload.data(), pkt.payload.size());
        apply_batches(batches);
    }

    // SIMULATION ECS
    ecs.progress(delta);

    // EGRESS : Extraction NetWorld -> Sérialisation pure -> Réseau
    if (is_server) {
        std::vector<uint8_t> out_buffer;
        auto query = ecs.query_builder<Position3D, Velocity3D>().with<Networked>().build();

        query.run([&](flecs::iter& it) {
            while (it.next()) {
                NetBatch batch = extract_batch(it);
                std::vector<uint8_t> block = serialize_batch(batch);
                out_buffer.insert(out_buffer.end(), block.begin(), block.end());
            }
        });

        if (!out_buffer.empty()) {
            transport->send(0, NetChannel::UNRELIABLE_STATE, out_buffer.data(), out_buffer.size());
        }
    }
}