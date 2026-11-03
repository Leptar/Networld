//
// Created by leoul on 2026-11-02.
//
// test_roundtrip_serialize.cpp
#include "test_roundtrip_serialize.h"
#include "components.h"
#include "serialization.h"
#include "deserialization.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include "flecs.h"
#include "NetRegistry.h"

// On utilise le namespace pour simplifier
using namespace godot;

void test_generic_roundtrip(flecs::world& ecs) {
    /*NetRegistry reg_src;
    reg_src.register_component(ecs.id<Position3D>(), 10);
    reg_src.register_component(ecs.id<Velocity3D>(), 11);

    auto e1 = ecs.entity("EntiteClient")
                 .set<Position3D>({Vector3(1.0f, 2.0f, 3.0f)})
                 .set<Velocity3D>({Vector3(0.1f, 0.0f, -0.1f)})
                 .add<Networked>();

    flecs::entity_t network_entity_id = e1.id();

    std::vector<uint8_t> packet;
    auto query = ecs.query_builder<Position3D, Velocity3D>().with<Networked>().build();

    query.run([&](flecs::iter& it) {
        while (it.next()) {
            std::vector<uint8_t> block = serialize_query(it, reg_src);
            packet.insert(packet.end(), block.begin(), block.end());
        }
    });

    flecs::world world_dst;
    world_dst.component<Gravity>();
    flecs::id_t pos_id_dst = world_dst.component<Position3D>().id();
    flecs::id_t vel_id_dst = world_dst.component<Velocity3D>().id();
    world_dst.component<Networked>();

    NetRegistry reg_dst;
    reg_dst.register_component(pos_id_dst, 10);
    reg_dst.register_component(vel_id_dst, 11);

    world_dst.make_alive(network_entity_id);

    world_dst.entity(network_entity_id)
             .set<Position3D>({Vector3(0.0f, 0.0f, 0.0f)})
             .set<Velocity3D>({Vector3(0.0f, 0.0f, 0.0f)})
             .add<Networked>();

    // 5. INJECTION DES MISES A JOUR
    deserialize_buffer(world_dst, packet.data(), packet.size(), reg_dst);

    // 6. Vérifications
    const Position3D* p = &world_dst.entity(network_entity_id).get<Position3D>();
    const Velocity3D* v = &world_dst.entity(network_entity_id).get<Velocity3D>();

    if (p->value == Vector3(0.0f, 0.0f, 0.0f)) {
        UtilityFunctions::printerr("[NetWorld Error] Position non mise a jour ! Paquet incomplet.");
        return;
    }

    if (p->value != Vector3(1.0f, 2.0f, 3.0f) || v->value != Vector3(0.1f, 0.0f, -0.1f)) {
        UtilityFunctions::printerr("[NetWorld Error] Les valeurs sont corrompues apres injection !");
        return;
    }

    UtilityFunctions::print("[NetWorld SUCCESS] Architecture securisee ! Mise a jour Multi-World parfaite !");*/
}