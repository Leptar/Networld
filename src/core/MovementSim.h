//
// Created by leoul on 2026-11-19.
//

#ifndef NETWORLD_MOVEMENTSIM_H
#define NETWORLD_MOVEMENTSIM_H
#include <algorithm>
#include <cmath>
#include "NetComponent.h"

namespace networld {

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

} // networld

#endif //NETWORLD_MOVEMENTSIM_H
