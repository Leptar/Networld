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
}

NetWorld::NetWorld() {
    set_process(true);
    
    registry.register_component(ecs.component<Position3D>().id(), 10);
    registry.register_component(ecs.component<Velocity3D>().id(), 11);
    ecs.component<Networked>();
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

    // 1. INGRESS : Pomper le socket et Deserialiser
    transport->poll();
    NetPacket pkt;
    while (transport->recv(pkt)) {
        // Injection directe et securisee en RAM
        deserialize_buffer(ecs, pkt.payload.data(), pkt.payload.size(), registry);

        if (!is_server) {
            UtilityFunctions::print("[Client] Synchronisation reçue : ", pkt.payload.size(), " octets injectés dans Flecs.");
        }
    }

    // 2. SIMULATION : Faire avancer le moteur ECS
    ecs.progress(delta);

    // 3. EGRESS : Extraire et envoyer (Seulement le Host fait autorite ici)
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