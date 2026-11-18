//
// Created by leoul on 2026-11-09.
//

#ifndef NETWORLD_NETCOMPONENT_H
#define NETWORLD_NETCOMPONENT_H

#pragma once
#include <cstdint>

namespace networld {

    // vu que component utiliser vector3 de godot faut les separer de la couche godot
    struct Vec3 {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
        Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
        Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }

        Vec3() = default;

        Vec3(float i, float i1, float i2) : x(i), y(i1), z(i2) {}
    };

    // composants POD de simulation (Sérialisables en SoA par std::memcpy)
    struct Position3D { Vec3 value; };
    struct Velocity3D { Vec3 value; };
    struct Gravity    { Vec3 force; };

    // tag
    struct Networked {};

    // je stocke un ID a la place, plutot qu'un node.
    struct ExternalView {
        uint64_t host_handle = 0;
    };

    // output pour le moteur qui accueillira la lib (creer en fin de tick)
    struct ViewSyncEntry {
        uint64_t entity_id = 0;
        uint64_t host_handle = 0;
        Vec3 position;
    };

    struct EntitySpawnEntry {
        uint64_t entity_id = 0;
        Vec3 position;
        bool is_local = false;
    };

} // namespace networld

#endif //NETWORLD_NETCOMPONENT_H
