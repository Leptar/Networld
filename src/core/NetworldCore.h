//
// Created by leoul on 2026-11-09.
//

#ifndef NETWORLD_NETWORLDCORE_H
#define NETWORLD_NETWORLDCORE_H
#pragma once
#include <memory>
#include <string>
#include <vector>
#include "flecs.h"
#include "NetComponent.h"
#include "NetRegistry.h"
#include "INetRole.h"
#include "NetBatch.h"
#include "INetTransport.h"
#include "PacketDispatcher.h"

namespace networld {

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


public:
    NetWorldCore();
    ~NetWorldCore();

    // interface et pont setup
    void set_transport(std::unique_ptr<INetTransport> transport);
    bool start_host(uint16_t port);
    bool start_client(const std::string& host_ip, uint16_t port);
    void stop();

    // boucle systeme, ca change pas
    // Exécute : Ingress Réseau -> Systèmes Flecs -> Collecte des Vues -> Egress Réseau
    void step(float delta_time);

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
    uint64_t create_networked_entity(const Vec3& initial_pos, uint64_t host_handle = 0, const char* name = nullptr);
    void destroy_entity(uint64_t entity_id);
    uint64_t find_entity(const char* name) const;

    void bind_entity_view(uint64_t entity_id, uint64_t host_handle);

    void set_entity_position(uint64_t entity_id, const Vec3& pos);
    void set_entity_velocity(uint64_t entity_id, const Vec3& vel);
    void set_simulation_systems_enabled(bool enabled);
    void set_interpolation_systems_enabled(bool enabled);
    void set_client_input(const Vec3& input) { if (role_) role_->set_client_input(input); }
    bool set_system_enabled(const char* name, bool enabled);

    // lecture des états
    [[nodiscard]] uint64_t get_local_entity_id() const { return role_ ? role_->get_local_entity_id() : 0; }
    [[nodiscard]] const std::vector<ViewSyncEntry>& get_view_updates() const { return view_updates_; }
    [[nodiscard]] const std::vector<EntitySpawnEntry>& get_spawned_entities() const { return spawned_entities_; }
    [[nodiscard]] const std::vector<uint64_t>& get_despawned_entities() const { return despawned_entities_; }
    INetTransport* get_transport() const { return transport_.get(); }
    PacketDispatcher& get_dispatcher() { return packet_dispatcher_; }
    const NetRegistry& get_registry() const { return registry_; }
    uint32_t get_current_tick() const { return current_tick_; }

    void push_spawned_entity(const EntitySpawnEntry& entry) {spawned_entities_.push_back(entry);}
    void push_despawned_entity(uint64_t entity_id) { despawned_entities_.push_back(entity_id); }

    void clear_despawned_entities() { despawned_entities_.clear(); }
    void clear_spawned_entities() { spawned_entities_.clear(); }

    // pour des tests ou  d'autre systèmes au cas ou
    flecs::world& get_ecs() { return ecs_; }
};

} // namespace networld

#endif //NETWORLD_NETWORLDCORE_H
