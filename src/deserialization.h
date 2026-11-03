//
// Created by leoul on 2026-11-02.
//

#ifndef NETWORLD_DESERIALIZATION_H
#define NETWORLD_DESERIALIZATION_H
#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include "NetBatch.h"

std::vector<NetBatch> deserialize_buffer(const uint8_t* data, size_t size);

#endif //NETWORLD_DESERIALIZATION_H
