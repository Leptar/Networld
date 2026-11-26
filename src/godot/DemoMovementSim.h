//
// Created by leoul on 2026-11-25.
//

#ifndef NETWORLD_DEMOMOVEMENTSIM_H
#define NETWORLD_DEMOMOVEMENTSIM_H
#include <algorithm>
#include <cmath>

#include "../../thirdparty/flecs/flecs.h"
#include "../core/NetComponent.h"

namespace networld {

    struct DemoMoveInput {
        float dir_x = 0.0f;
        float dir_z = 0.0f;
    };

    inline constexpr float PLAYER_MOVE_SPEED = 5.0f;
    inline constexpr float PLAYER_TURN_SPEED = 12.0f;

    inline void simulate_player_step(Vec3& pos, Quat& rot, const Vec3& raw_dir, float dt) {
        if (!std::isfinite(raw_dir.x) || !std::isfinite(raw_dir.z)) return;
        float norm = std::sqrt(raw_dir.x * raw_dir.x + raw_dir.z * raw_dir.z);
        if (norm <= 0.001f) return;
        Vec3 dir{};
        dir.x = (norm > 1.0f) ? (raw_dir.x / norm) : raw_dir.x;
        dir.z = (norm > 1.0f) ? (raw_dir.z / norm) : raw_dir.z;
        pos += dir * (PLAYER_MOVE_SPEED * dt);
        float yaw = std::atan2(dir.x, dir.z);
        float rot_t = std::min(1.0f, PLAYER_TURN_SPEED * dt);
        rot = Quat::nlerp(rot, Quat::from_yaw(yaw), rot_t);
    }

    inline void simulate_demo_player (flecs::entity e, const InputPayload& input, const SimContext& sim) {
        DemoMoveInput move{};
        if (!input.read_as(move)) return;

        auto* pos = e.try_get_mut<Position3D>();
        auto* rot = e.try_get_mut<Rotation3D>();
        if (!pos || !rot) return;

        simulate_player_step(pos->value, rot->value, Vec3{move.dir_x, 0.0f, move.dir_z}, sim.dt);
    }

} // networld


#endif // NETWORLD_DEMOMOVEMENTSIM_H
