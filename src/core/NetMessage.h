//
// Created by leoul on 2026-11-16.
//

#ifndef NETWORLD_NETMESSAGE_H
#define NETWORLD_NETMESSAGE_H
#include <cstdint>
#include <optional>
#include <vector>

#include "deserialization.h"
#include "NetBatch.h"
#include "NetComponent.h"
#include "PacketDispatcher.h"
#include "serialization.h"
#include "StreamReader.h"
#include "StreamWriter.h"

namespace networld {

    enum class PacketId : std::uint8_t {
        KNOCK = 1,
        SPAWN_ENTITY = 2,
        DESPAWN_ENTITY = 3,
        STATE_SYNC = 4,
    };

    struct KnockMsg {
        static constexpr PacketId PACKET_ID = PacketId::KNOCK;

        void serialize(StreamWriter&) const {}

        static std::optional<KnockMsg> deserialize(StreamReader&) {
            return KnockMsg{};
        }
    };
    static_assert(NetMessage<KnockMsg>);

    struct SpawnEntityMsg {
        static constexpr PacketId PACKET_ID = PacketId::SPAWN_ENTITY;

        uint64_t entity_Id = 0;
        Vec3 position{};
        Vec3 rotation{};


        void serialize(StreamWriter& stream) const {
            stream.write<uint64_t>(entity_Id);
            stream.write_vec3(position);
            stream.write_vec3(rotation);
        }

        static std::optional<SpawnEntityMsg> deserialize(StreamReader& stream) {
            auto entity_id = stream.read<uint64_t>();
            auto pos = stream.read_vec3();
            auto rot = stream.read_vec3();
            if (!entity_id || !pos || !rot) {
                return std::nullopt;
            }
            return SpawnEntityMsg{*entity_id, *pos, *rot};
        }
    };
    static_assert(NetMessage<SpawnEntityMsg>);

    struct DespawnEntityMsg {
        static constexpr PacketId PACKET_ID = PacketId::DESPAWN_ENTITY;
        uint64_t entity_Id;
        void serialize(StreamWriter& stream) const {
            stream.write<uint64_t>(entity_Id);
        }

        static std::optional<DespawnEntityMsg> deserialize(StreamReader& stream) {
            auto entity_id = stream.read<uint64_t>();
            if (!entity_id) {
                return std::nullopt;
            }
            return DespawnEntityMsg{*entity_id};
        }
    };
    static_assert(NetMessage<DespawnEntityMsg>);

    struct StateSyncMsg {
        static constexpr PacketId PACKET_ID = PacketId::STATE_SYNC;
        uint32_t tick;
        std::vector<NetBatch> batches;

        void serialize(StreamWriter& stream) const {
            stream.write<uint32_t>(tick);

            std::vector<uint8_t> soa_buffer;
            for (const NetBatch& batch : batches) {
                std::vector<uint8_t> block = serialize_batch(batch);
                soa_buffer.insert(soa_buffer.end(), block.begin(), block.end());
            }
            stream.write_bytes(soa_buffer);
        }

        static std::optional<StateSyncMsg> deserialize(StreamReader& stream) {
            auto tick = stream.read<uint32_t>();
            auto raw_opt = stream.read_bytes();
            if (!tick || !raw_opt) {
                return std::nullopt;
            }

            StateSyncMsg msg;
            msg.tick = *tick;
            if (!raw_opt->empty()) {
                msg.batches = deserialize_buffer(raw_opt->data(), raw_opt->size());
                if (msg.batches.empty()) {
                    return std::nullopt;
                }
            }
            return msg;
        }
    };
    static_assert(NetMessage<StateSyncMsg>);
} // networld

#endif //NETWORLD_NETMESSAGE_H
