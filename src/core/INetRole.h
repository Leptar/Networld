//
// Created by leoul on 2026-11-18.
//

#ifndef NETWORLD_INETROLE_H
#define NETWORLD_INETROLE_H
#include <cstdint>
#include "NetComponent.h"

namespace networld {

    class NetWorldCore;

    inline constexpr float FIXED_DT = 1.0f / 60.0f;
    inline constexpr float NET_SYNC_INTERVAL = 1.0f / 20.0f;

    enum class HostMode : uint8_t {
        DEDICATED,
        LISTEN
    };

    // va me permettre de séparer les deux networld (client/server)
    class INetRole {
        public:
        virtual ~INetRole() = default;

        virtual void on_start(NetWorldCore& core) = 0;
        virtual void on_stop(NetWorldCore& core) {}
        virtual void on_peer_connected(NetWorldCore& core, uint32_t peer_id) {}
        virtual void on_peer_disconnected(NetWorldCore& core, uint32_t peer_id) {}
        virtual void on_egress(NetWorldCore& core, float delta_time) = 0;

        virtual void on_fixed_tick(NetWorldCore& core, float fixed_dt) = 0;
        virtual void on_entity_spawned(NetWorldCore& core, uint64_t entity_id, const SpawnParams& params){}
        virtual void on_entity_despawned(NetWorldCore& core, uint64_t entity_id){}
        virtual void set_local_input(uint64_t target_local_entity_id, const InputPayload& payload) {}
        [[nodiscard]] virtual uint32_t get_local_peer_id() const { return 0; }
    };
} // networld

#endif //NETWORLD_INETROLE_H
