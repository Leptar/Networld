//
// Created by leoul on 2026-11-02.
//

#ifndef NETWORLD_SERIALIZATION_H
#define NETWORLD_SERIALIZATION_H
#pragma once
#include <vector>
#include <cstdint>
#include "NetBatch.h"

std::vector<uint8_t> serialize_batch(const NetBatch& batch);

#endif //NETWORLD_SERIALIZATION_H
