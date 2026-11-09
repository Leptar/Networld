//
// Created by leoul on 2026-11-02.
//

#include "serialization.h"
#include <cstring>

std::vector<uint8_t> serialize_batch(const NetBatch& batch) {
    std::vector<uint8_t> buffer;

    uint16_t entity_count = static_cast<uint16_t>(batch.entities.size());
    if (entity_count == 0) return buffer;

    // nombre d'entités (2 octets Big-Endian)
    buffer.push_back((entity_count >> 8) & 0xFF);
    buffer.push_back(entity_count & 0xFF);

    // copie du tableau d'IDs d'entités (64 bits chacun)
    size_t entity_block_size = entity_count * sizeof(uint64_t);
    size_t current_size = buffer.size();
    buffer.resize(current_size + entity_block_size);
    std::memcpy(buffer.data() + current_size, batch.entities.data(), entity_block_size);

    // nombre de colonnes
    uint8_t column_count = static_cast<uint8_t>(batch.columns.size());
    buffer.push_back(column_count);

    // écriture de chaque colonne (NetID + Taille + Payload brut)
    for (const NetColumn& col : batch.columns) {
        buffer.push_back((col.net_id >> 8) & 0xFF);
        buffer.push_back(col.net_id & 0xFF);

        buffer.push_back((col.element_size >> 8) & 0xFF);
        buffer.push_back(col.element_size & 0xFF);

        current_size = buffer.size();
        buffer.resize(current_size + col.payload.size());
        std::memcpy(buffer.data() + current_size, col.payload.data(), col.payload.size());
    }

    return buffer;
}

// #include "serialization.h"
// #include <vector>
// #include "flecs.h"
// #include "NetRegistry.h"
// #include "godot_cpp/variant/utility_functions.hpp"
//
// std::vector<uint8_t> serialize_query(flecs::iter& it, const NetRegistry& registry) {
//     std::vector<uint8_t> buffer;
//     const flecs::iter_t *c_it = it.c_ptr();
//
//     uint16_t entity_count = static_cast<uint16_t>(c_it->count);
//     buffer.push_back((entity_count >> 8) & 0xFF);
//     buffer.push_back(entity_count & 0xFF);
//
//     const flecs::entity_t* entities = c_it->entities;
//     size_t entity_block_size = entity_count * sizeof(flecs::entity_t);
//     size_t current_size = buffer.size();
//     buffer.resize(current_size + entity_block_size);
//     std::memcpy(buffer.data() + current_size, entities, entity_block_size);
//
//     uint8_t valid_columns = 0;
//     size_t col_count_offset = buffer.size();
//     buffer.push_back(0);
//
//     for (int i = 0; i < c_it->field_count; ++i) {
//         flecs::id field_id = it.id(i);
//         flecs::id_t comp_id = field_id.raw_id(); // ID brut C++
//
//         uint16_t net_id = registry.get_net_id(comp_id);
//         size_t comp_size = ecs_field_size(c_it, i);
//
//         // Affichage du nom réel du composant en C++ (ex: godot::Position3D)
//         const char* comp_name = field_id.str();
//         if (!comp_name) comp_name = "Inconnu";
//
//         /*godot::UtilityFunctions::print("[NetWorld Extract] Champ ", i,
//             " | Nom: ", comp_name,
//             " | ID_Flecs: ", (int64_t)comp_id,
//             " | NetID: ", net_id,
//             " | Size: ", (int64_t)comp_size);*/
//
//         if (net_id == 0 || comp_size == 0) continue;
//
//         valid_columns++;
//
//         buffer.push_back((net_id >> 8) & 0xFF);
//         buffer.push_back(net_id & 0xFF);
//
//         uint16_t size_per_elem = static_cast<uint16_t>(comp_size);
//         buffer.push_back((size_per_elem >> 8) & 0xFF);
//         buffer.push_back(size_per_elem & 0xFF);
//
//         const void* column_data = ecs_field_w_size(c_it, comp_size, i);
//         size_t payload_size = entity_count * comp_size;
//
//         current_size = buffer.size();
//         buffer.resize(current_size + payload_size);
//         std::memcpy(buffer.data() + current_size, column_data, payload_size);
//     }
//
//     buffer[col_count_offset] = valid_columns;
//     return buffer;
// }
