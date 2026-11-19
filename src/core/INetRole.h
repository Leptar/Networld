//
// Created by leoul on 2026-11-18.
//

#ifndef NETWORLD_INETROLE_H
#define NETWORLD_INETROLE_H
#include <cstdint>

#include "NetComponent.h"

namespace networld {

    class NetWorldCore;

    inline constexpr float NET_SYNC_INTERVAL = 1.0f / 20.0f;

    // va me permettre de séparer les deux networld (client/server)
    class INetRole {
        public:
        virtual ~INetRole() = default;

        virtual void on_start(NetWorldCore& core) = 0;
        virtual void on_stop(NetWorldCore& core) {}
        virtual void on_peer_connected(NetWorldCore& core, uint32_t peer_id) {}
        virtual void on_peer_disconnected(NetWorldCore& core, uint32_t peer_id) {}
        virtual void on_egress(NetWorldCore& core, float delta_time) = 0;

        virtual void set_client_input(const Vec3&) {}
        [[nodiscard]] virtual uint64_t get_local_entity_id() const { return 0; }
    };
} // networld

#endif //NETWORLD_INETROLE_H
