//
// Created by leoul on 2026-11-18.
//

#ifndef NETWORLD_SERVERROLE_H
#define NETWORLD_SERVERROLE_H
#include <unordered_map>
#include "flecs.h"
#include "INetRole.h"
#include "NetBatch.h"

namespace networld {
    class ServerRole : public INetRole {
        std::unordered_map<uint32_t, uint64_t> peer_entities_; // (peer_id -> server_entity_id)
        std::unordered_map<uint32_t, uint64_t> peer_inputs; // (peer_id -> last_tick_input)

        NetBatch extract_batch(const NetWorldCore& core, const flecs::iter& it) const;

        // pre-Query pour l'Egress réseau
        flecs::query<Position3D, Rotation3D, Velocity3D> sync_query_;

        uint64_t server_player_entity_id_ = 0;

        static constexpr float PLAYER_MOVE_SPEED = 5.0f;
        float sync_accumulator_ = 0.0f;

    public:
        void on_start(NetWorldCore &core) override;
        void on_stop(NetWorldCore &core) override;
        void on_peer_connected(NetWorldCore &core, uint32_t peer_id) override;
        void on_peer_disconnected(NetWorldCore &core, uint32_t peer_id) override;
        void on_egress(NetWorldCore &core, float delta_time) override;
    };
} // networld

#endif //NETWORLD_SERVERROLE_H
