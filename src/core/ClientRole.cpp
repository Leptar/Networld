//
// Created by leoul on 2026-11-18.
//

#include "ClientRole.h"
#include <ranges>
#include "NetMessage.h"
#include "NetworldCore.h"

namespace networld {
    void ClientRole::apply_batches(NetWorldCore &core, const std::vector<NetBatch> &batches) const {
        for (const NetBatch& batch : batches) {
            size_t entity_count = batch.entities.size();

            for (const NetColumn& col : batch.columns) {
                flecs::id_t local_id = core.get_registry().get_local_id(col.net_id);
                if (local_id == 0 || col.element_size == 0) continue;

                for (size_t i = 0; i < entity_count; ++i) {
                    auto it = net_to_local_entities_.find(batch.entities[i]);
                    if (it == net_to_local_entities_.end()) continue;
                    flecs::entity_t entity_id = it->second;
                    if (!core.get_ecs().is_alive(entity_id)) continue;

                    const uint8_t* elem_ptr = col.payload.data() + (i * col.element_size);
                    ecs_set_id(core.get_ecs().c_ptr(), entity_id, local_id, col.element_size, elem_ptr);
                }
            }
        }
    }

    void ClientRole::on_start(NetWorldCore &core) {
        core.set_simulation_systems_enabled(false);

        PacketDispatcher& packet_dispatcher = core.get_dispatcher();

        packet_dispatcher.register_handler<StateSyncMsg>([this, &core](uint32_t /*peer_id*/, const StateSyncMsg& msg) {
            if (msg.tick <= last_received_tick_) return;
            last_received_tick_ = msg.tick;
            apply_batches(core, msg.batches);

            if (msg.tick % 60 == 0 || last_received_tick_ == 0) {
                /*printf("[CLIENT] StateSync tick=%u batches=%zu\n", msg.tick, msg.batches.size());
                fflush(stdout);*/
            }
        });

        packet_dispatcher.register_handler<SpawnEntityMsg>([this, &core](uint32_t , const SpawnEntityMsg& msg) {
            if (net_to_local_entities_.contains(msg.entity_Id)) return;

            uint64_t local_id = core.create_networked_entity(msg.position);
            net_to_local_entities_[msg.entity_Id] = local_id;

            if (msg.is_owner) {
                local_player_entity_id_ = local_id;
            }
            core.push_spawned_entity({local_id, msg.position, msg.is_owner});

            printf("[CLIENT] SpawnEntityMsg server_id=%llu -> local_id=%llu (%.2f, %.2f, %.2f)\n",
                    msg.entity_Id, local_id, msg.position.x, msg.position.y, msg.position.z);
            fflush(stdout);
        });

        packet_dispatcher.register_handler<DespawnEntityMsg>([this, &core](uint32_t , const DespawnEntityMsg& msg) {
            auto it = net_to_local_entities_.find(msg.entity_Id);
            if (it == net_to_local_entities_.end()) return;

            core.push_despawned_entity(it->second);

            if (it->second == local_player_entity_id_) {
                local_player_entity_id_ = 0;
            }
            core.destroy_entity(it->second);
            net_to_local_entities_.erase(it);
        });

        auto knock = encode_packet(KnockMsg{});
        core.get_transport()->send(PEER_SERVER, NetChannel::RELIABLE_EVENTS, knock.data(), knock.size());
    }

    void ClientRole::on_stop(NetWorldCore &core) {
        for (uint64_t local_id : net_to_local_entities_ | std::views::values) {
            core.push_despawned_entity(local_id);
            core.destroy_entity(local_id);
        }

        net_to_local_entities_.clear();
        local_player_entity_id_ = 0;
        last_received_tick_ = 0;
        input_client_ = Vec3{};
    }

    void ClientRole::on_egress(NetWorldCore &core, float delta_time) {
        if (core.get_current_tick() % 60 == 0){
            auto knock_pkt = encode_packet(KnockMsg{});
            core.get_transport()->send(PEER_SERVER, NetChannel::UNRELIABLE_STATE, knock_pkt.data(), knock_pkt.size());
        }

        InputMsg input_msg;
        input_msg.tick_input = core.get_current_tick();
        input_msg.direction = input_client_;
        auto input_pkt = encode_packet(input_msg);
        core.get_transport()->send(PEER_SERVER, NetChannel::UNRELIABLE_STATE, input_pkt.data(), input_pkt.size());
    }

    void ClientRole::set_client_input(const Vec3 &input) {
        input_client_ = input;
    }

    uint64_t ClientRole::get_local_entity_id() const {
        return local_player_entity_id_;
    }
} // networld