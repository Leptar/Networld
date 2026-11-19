//
// Created by leoul on 2026-11-18.
//

#ifndef NETWORLD_CLIENTROLE_H
#define NETWORLD_CLIENTROLE_H
#include <unordered_map>

#include "INetRole.h"
#include "NetBatch.h"

namespace networld {

    constexpr float SNAP_DIST = 100.0f;

    class ClientRole : public INetRole {
        std::unordered_map<uint64_t, uint64_t> net_to_local_entities_; // (server_entity_id -> local_entity_id)
        Vec3 input_client_{};
        uint64_t local_player_entity_id_ = 0;
        uint32_t last_received_tick_ = 0;

        void apply_batches(NetWorldCore& core, const std::vector<NetBatch>& batches) const;
    public:
        void on_start(NetWorldCore &core) override;
        void on_stop(NetWorldCore &core) override;

        void on_egress(NetWorldCore &core, float delta_time) override;
        void set_client_input(const Vec3 &) override;
        uint64_t get_local_entity_id() const override;
    };
} // networld

#endif //NETWORLD_CLIENTROLE_H
