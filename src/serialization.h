//
// Created by leoul on 2026-11-02.
//

#ifndef NETWORLD_SERIALIZATION_H
#define NETWORLD_SERIALIZATION_H
#pragma once
#include <vector>
#include <cstdint>
#include "flecs.h"
#include "NetRegistry.h"

std::vector<uint8_t> serialize_query(flecs::iter& it, const NetRegistry& registry);

#endif //NETWORLD_SERIALIZATION_H
