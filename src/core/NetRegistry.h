//
// Created by leoul on 2026-11-02.
//

#ifndef NETWORLD_NETREGISTRY_H
#define NETWORLD_NETREGISTRY_H

#pragma once
#include <cstdint>
#include <unordered_map>
#include "flecs.h"

class NetRegistry {
public:
    // Associe un ID Flecs local à un ID Réseau fixe (ex: 1 = Position, 2 = Velocity)
    void register_component(flecs::id_t local_id, uint16_t net_id) {
        local_to_net_[local_id] = net_id;
        net_to_local_[net_id] = local_id;
    }

    uint16_t get_net_id(flecs::id_t local_id) const {
        auto it = local_to_net_.find(local_id);
        return it != local_to_net_.end() ? it->second : 0;
    }

    flecs::id_t get_local_id(uint16_t net_id) const {
        auto it = net_to_local_.find(net_id);
        return it != net_to_local_.end() ? it->second : 0;
    }

private:
    std::unordered_map<flecs::id_t, uint16_t> local_to_net_;
    std::unordered_map<uint16_t, flecs::id_t> net_to_local_;
};


#endif //NETWORLD_NETREGISTRY_H
