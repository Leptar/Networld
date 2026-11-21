//
// Created by leoul on 2026-11-09.
//

#ifndef NETWORLD_NETCOMPONENT_H
#define NETWORLD_NETCOMPONENT_H

#pragma once
#include <array>
#include <cstring>
#include <span>
#include <type_traits>
#include <cmath>
#include <optional>
#include <string>
#include "INetTransport.h"

namespace networld {

    class NetWorldCore;
    // vu que component utiliser vector3 de godot faut les separer de la couche godot
    struct Vec3 {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
        Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
        Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
        Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }

        Vec3() = default;

        Vec3(float i, float i1, float i2) : x(i), y(i1), z(i2) {}

        static Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
            return a + (b - a) * t;
        }
    };

    struct Quat {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 1.0f;

        Quat() = default;
        Quat(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

        [[nodiscard]] Quat normalized() const {
            float len = std::sqrt(x * x + y * y + z * z + w * w);
            if (len <= 0.0f) return {};
            return {x / len, y / len, z / len, w / len};
        }

        static Quat from_yaw(float radians) {
            float half = radians * 0.5f;
            return {0.0f, std::sin(half), 0.0f, std::cos(half)};
        }

        static Quat from_pitch(float radians) {
            float half = radians * 0.5f;
            return {std::sin(half), 0.0f, 0.0f, std::cos(half)};
        }

        static Quat from_roll(float radians) {
            float half = radians * 0.5f;
            return {0.0f, 0.0f, std::sin(half), std::cos(half)};
        }

        static Quat nlerp(const Quat& a, const Quat& b, float t) {
            float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
            Quat target = dot < 0.0f ? Quat{-b.x, -b.y, -b.z, -b.w} : b;
            Quat res {
                a.x + (target.x - a.x) * t,
                a.y + (target.y - a.y) * t,
                a.z + (target.z - a.z) * t,
                a.w + (target.w - a.w) * t
            };
            return res.normalized();
        }
    };

    // composants POD de simulation (Sérialisables en SoA par std::memcpy)
    struct Position3D { Vec3 value; };
    struct Rotation3D { Quat value; };
    struct Velocity3D { Vec3 value; };
    struct Gravity    { Vec3 force; };
    struct NetType { uint32_t type_id = 0; };
    struct NetOwnership { uint32_t owner_peer_id = PEER_NONE; };
    struct NetEntityID { uint64_t server_id = 0; };

    // tag
    struct Networked {};
    struct LocalPredicted {};

    // je stocke un ID a la place, plutot qu'un node.
    struct ExternalView {
        uint64_t host_handle = 0;
    };

    // output pour le moteur qui accueillira la lib (creer en fin de tick)
    struct ViewSyncEntry {
        uint64_t entity_id = 0;
        uint64_t host_handle = 0;
        Vec3 position;
        Quat rotation;
    };

    struct EntitySpawnEntry {
        uint32_t type_id = 0;
        uint32_t owner_peer_id = 0;
        uint64_t entity_id = 0;
        Vec3 position;
        bool is_local = false;
        Quat rotation;
    };

    struct InterpolationState { // Client only
        // position
        Vec3 prev_pos;
        Vec3 target_pos;
        Vec3 visual_pos;
        // rotation
        Quat prev_rot;
        Quat target_rot;
        Quat visual_rot;
        
        float elapsed = 0.0f;
        float duration = 0.05f;
    };

    struct SpawnParams {
        uint32_t type_id = 0;
        uint32_t owner_peer_id = 0;
        Vec3 position;
        Quat rotation;
        bool predicted = false;
        uint64_t host_handle = 0;
        std::string debug_name;
    };

    struct PeerEvent {
        uint32_t peer_id = 0;
        bool is_local = false;
    };

    struct SimContext {
        NetWorldCore& core;
        float dt = 0.0f;
        uint64_t input_seq = 0;
        bool is_replay = false;
    };

    // input
    struct InputPayload {
        static constexpr size_t MAX_SIZE = 64; // pas besoin de plus je pense
        uint8_t size = 0;
        std::array<uint8_t, MAX_SIZE> data{};

        template <typename T>
        static InputPayload from(const T& value) {
            static_assert(std::is_trivially_copyable_v<T>);
            static_assert(sizeof(T) <= MAX_SIZE);

            InputPayload payload;
            payload.size = static_cast<uint8_t>(sizeof(T));
            std::memcpy(payload.data.data(), &value, payload.size);
            return payload;
        }

        // TODO Note : utiliser requires ptetre, ca collerait un peu plus cpp 20
        template <typename T>
        bool read_as(T& value) const{
            static_assert(std::is_trivially_copyable_v<T>);
            static_assert(sizeof(T) <= MAX_SIZE);

            if (size != sizeof(T)) return false;

            std::memcpy(&value, data.data(), sizeof(T));
            return true;
        }

        [[nodiscard]] std::span<const uint8_t> bytes() const {
            return {data.data(), size};
        }

        static std::optional<InputPayload> from_bytes(std::span<const uint8_t> raw) {
            if (raw.size() > MAX_SIZE ) return std::nullopt;
            InputPayload payload;
            payload.size = static_cast<uint8_t>(raw.size());
            if (!raw.empty()) std::memcpy(payload.data.data(), raw.data(), payload.size);
            return payload;
        }

        [[nodiscard]] bool empty() const { return size == 0; }
    };
} // namespace networld

#endif //NETWORLD_NETCOMPONENT_H
