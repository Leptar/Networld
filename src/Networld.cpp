//
// Created by leoul on 2026-11-02.
//

#include "NetWorld.h"
#include "flecs.h"
#include "components.h"
#include "serialization.h"
#include "deserialization.h"
#include "GodotUdpTransport.h"
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

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

    uint8_t knock_packet = 0xFF;
    transport->send(1, NetChannel::UNRELIABLE_STATE, &knock_packet, 1);
}

void NetWorld::_process(double delta) {
    if (!transport) return;

    // ingress: Pomper le socket et Deserialiser
    transport->poll();
    NetPacket pkt;
    while (transport->recv(pkt)) {
        // Injection directe et securisee en RAM
        deserialize_buffer(ecs, pkt.payload.data(), pkt.payload.size(), registry);

        /* pour debug
        if (!is_server) {
            UtilityFunctions::print("[Client] Synchronisation reçue : ", pkt.payload.size(), " octets injectés dans Flecs.");
        }*/
    }

    // sim: Faire avancer le moteur ECS
    ecs.progress(delta);

    // egress: Extraire et envoyer (seulement host)
    if (is_server) {
        std::vector<uint8_t> out_buffer;

        // On recupere les entites marquees pour le reseau
        auto query = ecs.query_builder<Position3D, Velocity3D>().with<Networked>().build();

        query.run([&](flecs::iter& it) {
            while (it.next()) {
                std::vector<uint8_t> block = serialize_query(it, registry);
                out_buffer.insert(out_buffer.end(), block.begin(), block.end());
            }
        });

        // Envoi binaire compact si le buffer n'est pas vide
        if (!out_buffer.empty()) {
            transport->send(0, NetChannel::UNRELIABLE_STATE, out_buffer.data(), out_buffer.size());
        }
    }
}