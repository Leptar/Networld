//
// Created by leoul on 2026-11-02.
//

#ifndef NETWORLD_DESERIALIZATION_H
#define NETWORLD_DESERIALIZATION_H
#pragma once
#include <cstdint>
#include "flecs.h"
#include "NetRegistry.h"

void deserialize_buffer(flecs::world& ecs, const uint8_t* data, size_t size, const NetRegistry& registry);
#endif //NETWORLD_DESERIALIZATION_H
