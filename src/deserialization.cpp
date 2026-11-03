//
// Created by leoul on 2026-11-02.
//
#include "deserialization.h"
#include "flecs.h"
#include "NetRegistry.h"

void deserialize_buffer(flecs::world& ecs, const uint8_t* data, size_t size, const NetRegistry& registry) {
    if (size < 3) return;
    size_t offset = 0;

    uint16_t entity_count = (data[offset] << 8) | data[offset+1];
    offset += 2;

    std::vector<flecs::entity_t> entities(entity_count);
    std::memcpy(entities.data(), data + offset, entity_count * sizeof(flecs::entity_t));
    offset += entity_count * sizeof(flecs::entity_t);

    uint8_t column_count = data[offset++];

    for (uint8_t c = 0; c < column_count; ++c) {
        uint16_t net_id = (data[offset] << 8) | data[offset+1];
        offset += 2;

        uint16_t comp_size = (data[offset] << 8) | data[offset+1];
        offset += 2;

        flecs::id_t local_id = registry.get_local_id(net_id);
        const uint8_t* column_payload = data + offset;

        if (local_id != 0 && comp_size > 0) {
            std::vector<uint8_t> aligned_comp(comp_size);

            for (uint16_t i = 0; i < entity_count; ++i) {
                if (!ecs.is_alive(entities[i])) {
                    continue;
                }
                std::memcpy(aligned_comp.data(), column_payload + (i * comp_size), comp_size);
                ecs_set_id(ecs.c_ptr(), entities[i], local_id, comp_size, aligned_comp.data());
            }
        }
        offset += entity_count * comp_size;
    }
}