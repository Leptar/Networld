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

    void ClientRole::handle_spawn_entity(NetWorldCore &core, const SpawnEntityMsg &msg) {
        if (net_to_local_entities_.contains(msg.entity_Id)) return;

        SpawnParams spawn_params;
        spawn_params.type_id = msg.type_id;
        spawn_params.owner_peer_id = msg.owner_peer_id;
        spawn_params.position = msg.position;
        spawn_params.rotation = msg.rotation.normalized();
        spawn_params.predicted = msg.predicted;

        uint64_t local_id = core.create_local_entity(spawn_params);
        net_to_local_entities_[msg.entity_Id] = local_id;
        local_to_net_entities_[local_id] = msg.entity_Id;

        flecs::entity e = core.get_ecs().entity(local_id);
        e.set<NetEntityID>({msg.entity_Id});

        const bool is_local = (local_peer_id_ != PEER_NONE && msg.owner_peer_id == local_peer_id_);
        if (is_local && msg.predicted) {
            e.add<LocalPredicted>();
        } else {
            e.set<InterpolationState>({
            spawn_params.position, spawn_params.position, spawn_params.position,
            spawn_params.rotation, spawn_params.rotation, spawn_params.rotation,
            0.0f, NET_SYNC_INTERVAL
            });
        }

        core.push_spawned_entity(EntitySpawnEntry{
            spawn_params.type_id, spawn_params.owner_peer_id, local_id,
            spawn_params.position, is_local, spawn_params.rotation
        });
    }

    void ClientRole::on_start(NetWorldCore &core) {
        pending_inputs_.clear();
        staged_local_inputs_.clear();
        deferred_spawns_.clear();
        welcome_received_ = false;
        local_peer_id_ = PEER_NONE;
        next_input_seq = 0;
        last_received_tick_ = 0;

        PacketDispatcher& packet_dispatcher = core.get_dispatcher();

        packet_dispatcher.register_handler<WelcomeMsg>([this, &core](uint32_t , const WelcomeMsg& msg) {
            if (welcome_received_) return;

            local_peer_id_ = msg.assigned_peer_id;
            welcome_received_ = true;
            core.push_connected_peers(PeerEvent{local_peer_id_, true});

            for (const SpawnEntityMsg& pending_spawn : deferred_spawns_) {
                handle_spawn_entity(core, pending_spawn);
            }
            deferred_spawns_.clear();
        });

        packet_dispatcher.register_handler<SpawnEntityMsg>([this, &core](uint32_t, const SpawnEntityMsg& msg) {
            if (!welcome_received_) {
                deferred_spawns_.push_back(msg);
                return;
            }
            handle_spawn_entity(core, msg);
        });

        packet_dispatcher.register_handler<DespawnEntityMsg>([this, &core](uint32_t, const DespawnEntityMsg& msg) {
            auto it = net_to_local_entities_.find(msg.entity_Id);
            if (it == net_to_local_entities_.end()) return;

            const uint64_t to_despawn_id = it->second;
            core.push_despawned_entity(to_despawn_id);
            staged_local_inputs_.erase(to_despawn_id);
            std::erase_if(pending_inputs_, [to_despawn_id](const PendingInput& input) {
               return input.local_entity_id == to_despawn_id;
            });
            local_to_net_entities_.erase(to_despawn_id);
            net_to_local_entities_.erase(it);
            core.destroy_entity(to_despawn_id);
        });

        packet_dispatcher.register_handler<StateSyncMsg>([this, &core](uint32_t /*peer_id*/, const StateSyncMsg& msg) {
            if (msg.tick <= last_received_tick_) return;
            last_received_tick_ = msg.tick;
            apply_batches(core, msg.batches);

            while (!pending_inputs_.empty() && pending_inputs_.front().seq <= msg.ack_input_seq) pending_inputs_.pop_front();

            for (const PendingInput& input : pending_inputs_) {
                flecs::entity e = core.get_ecs().entity(input.local_entity_id);
                if (!e.is_alive() || !e.has<LocalPredicted>()) continue;
                SimContext ctx{core, FIXED_DT, input.seq, true};
                core.run_entity_simulator(input.local_entity_id, input.payload, ctx);
            }

            if (msg.tick % 60 == 0 || last_received_tick_ == 0) {
                /*printf("[CLIENT] StateSync tick=%u batches=%zu\n", msg.tick, msg.batches.size());
                fflush(stdout);*/
            }

            core.get_ecs().each([](const Position3D& pos, const Rotation3D& rot, InterpolationState& interp) {
                Vec3 diff = pos.value - interp.visual_pos;
                float norm = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

                if (norm > SNAP_DIST) {
                    interp.prev_pos = pos.value;
                    interp.target_pos = pos.value;
                    interp.visual_pos = pos.value;
                    interp.prev_rot = rot.value;
                    interp.target_rot = rot.value;
                    interp.visual_rot = rot.value;
                } else {
                    interp.prev_pos = interp.visual_pos;
                    interp.target_pos = pos.value;
                    interp.prev_rot = interp.visual_rot;
                    interp.target_rot = rot.value;
                }
                interp.elapsed = 0.0f;
                interp.duration = NET_SYNC_INTERVAL;
            });
        });

        auto knock = encode_packet(KnockMsg{});
        core.get_transport()->send(PEER_SERVER, NetChannel::RELIABLE_EVENTS, knock.data(), knock.size());
    }

    void ClientRole::on_stop(NetWorldCore &core) {
        for (uint64_t local_id : net_to_local_entities_ | std::views::values) {
            core.push_despawned_entity(local_id);
            core.destroy_entity(local_id);
        }

        pending_inputs_.clear();
        staged_local_inputs_.clear();
        net_to_local_entities_.clear();
        local_to_net_entities_.clear();
        deferred_spawns_.clear();
        local_peer_id_ = PEER_NONE;
        welcome_received_ = false;
        last_received_tick_ = 0;
        next_input_seq = 0;
    }

    void ClientRole::set_local_input(uint64_t target_local_entity_id, const InputPayload &payload) {
        if (target_local_entity_id == 0) return;
        staged_local_inputs_[target_local_entity_id] = payload;
    }

    void ClientRole::on_egress(NetWorldCore &core, float delta_time) {
        const uint8_t check_tick = !welcome_received_ ? 30 : 60;
        if (core.get_current_tick() % check_tick == 0){
            auto knock_pkt = encode_packet(KnockMsg{});
            core.get_transport()->send(PEER_SERVER, NetChannel::UNRELIABLE_STATE, knock_pkt.data(), knock_pkt.size());
        }
    }

    void ClientRole::on_fixed_tick(NetWorldCore &core, float fixed_dt) {
        if (!welcome_received_) {
            staged_local_inputs_.clear();
            return;
        }

        for (const auto& [local_id, payload] : staged_local_inputs_) {
            auto it_net = local_to_net_entities_.find(local_id);
            if (it_net == local_to_net_entities_.end()) continue;

            flecs::entity e = core.get_ecs().entity(local_id);
            if (!e.is_alive()) continue;

            const auto* own = e.try_get<NetOwnership>();
            if (!own || own->owner_peer_id != local_peer_id_) continue;

            const uint64_t seq = ++next_input_seq;
            auto input_pkt = encode_packet(InputMsg{seq, it_net->second, payload});
            core.get_transport()->send(PEER_SERVER, NetChannel::UNRELIABLE_STATE, input_pkt.data(), input_pkt.size());

            if (e.has<LocalPredicted>()) {
                if (pending_inputs_.size() >= 128) pending_inputs_.pop_front();
                pending_inputs_.push_back({seq, local_id, payload});

                SimContext ctx{core, fixed_dt, seq, false};
                core.run_entity_simulator(local_id, payload, ctx);
            }
        }

        staged_local_inputs_.clear();
    }
} // networld