//
// Created by leoul on 2026-11-18.
//

#ifndef NETWORLD_SERVERROLE_H
#define NETWORLD_SERVERROLE_H
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include "flecs.h"
#include "INetRole.h"
#include "NetBatch.h"

namespace networld {

    struct PendingServerInput {
        uint64_t seq = 0;
        uint64_t target_entity_id = 0;
        InputPayload payload{};
    };
    static constexpr size_t MAX_PAYLOAD_INPUTS = 16;

    class ServerRole : public INetRole {

        HostMode mode_ = HostMode::LISTEN;
        std::unordered_set<uint32_t> connected_peers_;

        std::unordered_map<uint32_t, std::deque<PendingServerInput>> peer_input_queues_;
        std::unordered_map<uint32_t, uint64_t> last_received_seq_; // anti doublon
        std::unordered_map<uint32_t, uint64_t> last_ack_seq_; // last seq simulé

        std::unordered_map<uint64_t, InputPayload> host_local_inputs_; // LISTEN MODE ONLY

        NetBatch extract_batch(const NetWorldCore& core, const flecs::iter& it) const;

        // pre-Query pour l'Egress réseau
        flecs::query<Position3D, Rotation3D, Velocity3D> sync_query_;

        float sync_accumulator_ = 0.0f;

    public:
        explicit ServerRole(HostMode mode = HostMode::LISTEN) : mode_(mode) {}

        void on_start(NetWorldCore &core) override;
        void on_stop(NetWorldCore &core) override;
        void on_peer_connected(NetWorldCore &core, uint32_t peer_id) override;
        void on_peer_disconnected(NetWorldCore &core, uint32_t peer_id) override;
        void on_egress(NetWorldCore &core, float delta_time) override;
        void on_fixed_tick(NetWorldCore& core, float fixed_dt) override;
        void on_entity_spawned(NetWorldCore &core, uint64_t entity_id, const SpawnParams &params) override;
        void on_entity_despawned(NetWorldCore &core, uint64_t entity_id) override;
        void set_local_input(uint64_t target_local_entity_id, const InputPayload &payload) override;
        [[nodiscard]] uint32_t get_local_peer_id() const override {
            return mode_ == HostMode::LISTEN ? PEER_SERVER : PEER_NONE;
        }
    };
} // networld

#endif //NETWORLD_SERVERROLE_H
