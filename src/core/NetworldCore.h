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
#include "NetBatch.h"
#include "INetTransport.h"

namespace networld {

class NetWorldCore {

    flecs::world ecs_;
    NetRegistry registry_;
    std::unique_ptr<INetTransport> transport_;
    bool is_server_ = false;
    uint32_t current_tick_ = 0;

    // output stamp contenant les transformations à appliquer aux nœuds visuels du moteur
    std::vector<ViewSyncEntry> view_updates_;

    // pre-Query pour l'Egress réseau
    flecs::query<Position3D, Velocity3D> net_query_;

    // Méthodes internes de traduction Flecs <-> NetBatch
    NetBatch extract_batch(flecs::iter& it) const;
    void apply_batches(const std::vector<NetBatch>& batches);

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

    // je tente un truc avec les templates ?
    // TODO : Est ce un bon choix ?
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

    void bind_external_view(const char* name, uint64_t host_handle);
    void set_entity_position(uint64_t entity_id, const Vec3& pos);
    void set_entity_velocity(uint64_t entity_id, const Vec3& vel);

    // lecture des états
    const std::vector<ViewSyncEntry>& get_view_updates() const { return view_updates_; }

    // pour des tests ou  d'autre systèmes au cas ou
    flecs::world& get_ecs() { return ecs_; }
};

} // namespace networld

#endif //NETWORLD_NETWORLDCORE_H
