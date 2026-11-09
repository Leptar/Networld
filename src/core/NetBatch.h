//
// Created by leoul on 2026-11-03.
//

#ifndef NETWORLD_NETBATCH_H
#define NETWORLD_NETBATCH_H

#pragma once
#include <cstdint>
#include <vector>

struct NetColumn {
    uint16_t net_id = 0;
    uint16_t element_size = 0;
    std::vector<uint8_t> payload; // Taille = entities.size() * element_size
};

struct NetBatch {
    std::vector<uint64_t> entities;
    std::vector<NetColumn> columns;
};

#endif //NETWORLD_NETBATCH_H
