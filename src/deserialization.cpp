//
// Created by leoul on 2026-11-02.
//
#include "deserialization.h"
#include <cstring>

std::vector<NetBatch> deserialize_buffer(const uint8_t* data, size_t size) {
    std::vector<NetBatch> batches;
    if (!data || size < 3) return batches;

    size_t offset = 0;

    // Boucle pour décoder tous les blocs d'archétypes présents dans le paquet
    while (offset + 3 <= size) {
        NetBatch batch;

        uint16_t entity_count = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
        offset += 2;

        size_t entities_bytes = entity_count * sizeof(uint64_t);
        if (offset + entities_bytes + 1 > size) break; // Protection contre paquet tronqué

        batch.entities.resize(entity_count);
        std::memcpy(batch.entities.data(), data + offset, entities_bytes);
        offset += entities_bytes;

        uint8_t column_count = data[offset++];

        for (uint8_t c = 0; c < column_count; ++c) {
            if (offset + 4 > size) break;

            NetColumn col;
            col.net_id = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
            offset += 2;

            col.element_size = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
            offset += 2;

            size_t payload_bytes = static_cast<size_t>(entity_count) * col.element_size;
            if (offset + payload_bytes > size) break;

            col.payload.resize(payload_bytes);
            std::memcpy(col.payload.data(), data + offset, payload_bytes);
            offset += payload_bytes;

            batch.columns.push_back(std::move(col));
        }

        batches.push_back(std::move(batch));
    }

    return batches;
}