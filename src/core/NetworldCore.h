//
// Created by leoul on 2026-11-09.
//

#ifndef NETWORLD_NETWORLDCORE_H
#define NETWORLD_NETWORLDCORE_H
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include "flecs.h"
#include "NetComponent.h"
#include "NetRegistry.h"
#include "INetRole.h"
#include "NetBatch.h"
#include "INetTransport.h"
#include "PacketDispatcher.h"

namespace networld {

    using EntitySimCallback = std::function<void(flecs::entity entity, const InputPayload& input, const SimContext& sim)>;

    class NetWorldCore {

        flecs::world ecs_;
        NetRegistry registry_;
        std::unique_ptr<INetTransport> transport_;
        std::unique_ptr<INetRole> role_;

        std::vector<EntitySpawnEntry> spawned_entities_;
        std::vector<uint64_t> despawned_entities_;

        // tick for message
        uint32_t current_tick_ = 0;
        PacketDispatcher packet_dispatcher_;

        // output stamp contenant les transformations à appliquer aux nœuds visuels du moteur
        std::vector<ViewSyncEntry> view_updates_;

        flecs::entity sim_pipeline_;
        flecs::entity fixed_sim_phase_;
        std::unordered_map<uint32_t, EntitySimCallback> entity_simulators_;

        std::vector<PeerEvent> connected_peers_;
        std::vector<uint32_t> disconnected_peers_;

    public:
        NetWorldCore();
        ~NetWorldCore();

        // interface et pont setup
        void set_transport(std::unique_ptr<INetTransport> transport);
        bool start_host(uint16_t port, HostMode mode = HostMode::LISTEN);
        bool start_client(const std::string& host_ip, uint16_t port);
        void stop();

        // boucle systeme, ca change pas
        // Exécute : Ingress Réseau -> Systèmes Flecs -> Collecte des Vues -> Egress Réseau
        void step_simulation(float delta_time);
        void step_visual(float delta_time);

        // je tente un truc avec les templates
        template <typename T>
        uint64_t register_component(uint16_t net_id = 0) {
            flecs::id_t comp_id = ecs_.component<T>().id();
            if (net_id != 0) {
                registry_.register_component(comp_id, net_id);
            }
            return comp_id;
        }

        // cycle de vie
        uint64_t create_local_entity(const SpawnParams& params);
        uint64_t spawn_networked_entity(const SpawnParams& params);
        void despawn_networked_entity(uint64_t entity_id);
        void destroy_entity(uint64_t entity_id);
        void destroy_entities_owned_by(uint32_t peer_id);
        uint64_t find_entity(const char* name) const;

        void register_entity_simulator(uint32_t type_id, EntitySimCallback cb);
        void run_entity_simulator(uint64_t entity_id, const InputPayload& input, const SimContext& ctx);
        void set_local_input(uint64_t target_local_entity_id, const InputPayload& payload);

        void bind_entity_view(uint64_t entity_id, uint64_t host_handle);

        void set_entity_position(uint64_t entity_id, const Vec3& pos);
        void set_entity_velocity(uint64_t entity_id, const Vec3& vel);
        void set_entity_rotation(uint64_t entity_id, const Quat& rot);
        bool set_system_enabled(const char* name, bool enabled);

        // lecture des états
        [[nodiscard]] const std::vector<ViewSyncEntry>& get_view_updates() const { return view_updates_; }
        [[nodiscard]] const std::vector<EntitySpawnEntry>& get_spawned_entities() const { return spawned_entities_; }
        [[nodiscard]] const std::vector<uint64_t>& get_despawned_entities() const { return despawned_entities_; }
        INetTransport* get_transport() const { return transport_.get(); }
        PacketDispatcher& get_dispatcher() { return packet_dispatcher_; }
        const NetRegistry& get_registry() const { return registry_; }
        uint32_t get_current_tick() const { return current_tick_; }
        uint32_t get_local_peer_id() const;
        const std::vector<uint32_t>& get_disconnected_peers() const { return disconnected_peers_; }
        const std::vector<PeerEvent>& get_connected_peers() const { return connected_peers_; }
        flecs::entity get_fixed_sim_phase() const { return fixed_sim_phase_; }

        void push_spawned_entity(const EntitySpawnEntry& entry) {spawned_entities_.push_back(entry);}
        void push_despawned_entity(uint64_t entity_id) { despawned_entities_.push_back(entity_id); }
        void push_connected_peers(const PeerEvent& peers) {connected_peers_.push_back(peers);}
        void push_disconnected_peers(uint32_t peer_id) { disconnected_peers_.push_back(peer_id); }

        void clear_despawned_entities() { despawned_entities_.clear(); }
        void clear_spawned_entities() { spawned_entities_.clear(); }
        void clear_connected_peers() { connected_peers_.clear(); }
        void clear_disconnected_peers() { disconnected_peers_.clear(); }

        // pour des tests ou  d'autre systèmes au cas ou
        flecs::world& get_ecs() { return ecs_; }
    };

} // namespace networld

#endif //NETWORLD_NETWORLDCORE_H
