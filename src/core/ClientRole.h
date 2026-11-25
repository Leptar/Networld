//
// Created by leoul on 2026-11-18.
//

#ifndef NETWORLD_CLIENTROLE_H
#define NETWORLD_CLIENTROLE_H
#include <unordered_map>
#include <deque>
#include <set>
#include <vector>
#include "NetMessage.h"
#include "INetRole.h"
#include "NetBatch.h"

namespace networld {

    constexpr float SNAP_DIST = 100.0f;

    class ClientRole : public INetRole {

        struct PendingInput {
            uint64_t seq = 0;
            uint64_t local_entity_id = 0;
            InputPayload payload{};
        };
        uint32_t local_peer_id_ = PEER_NONE;
        bool welcome_received_ = false;
        std::vector<SpawnEntityMsg> deferred_spawns_; // utile pour l'ordre udp

        std::deque<PendingInput> pending_inputs_;
        uint64_t next_input_seq = 0;
        std::unordered_map<uint64_t, InputPayload> staged_local_inputs_; // local_entity_id -> payload du tick

        std::unordered_map<uint64_t, uint64_t> net_to_local_entities_; // (server_entity_id -> local_entity_id)
        std::unordered_map<uint64_t, uint64_t> local_to_net_entities_; // (local_entity_id -> server_entity_id)
        uint32_t last_received_tick_ = 0;

        void apply_batches(NetWorldCore& core, const std::vector<NetBatch>& batches) const;
        void handle_spawn_entity(NetWorldCore& core, const SpawnEntityMsg& msg);

    public:
        void on_start(NetWorldCore &core) override;
        void on_stop(NetWorldCore &core) override;
        void set_local_input(uint64_t target_local_entity_id, const InputPayload &payload) override;
        void on_egress(NetWorldCore &core, float delta_time) override;
        void on_fixed_tick(NetWorldCore &core, float fixed_dt) override;
        [[nodiscard]] uint32_t get_local_peer_id() const override { return local_peer_id_; }
    };
} // networld

#endif //NETWORLD_CLIENTROLE_H
