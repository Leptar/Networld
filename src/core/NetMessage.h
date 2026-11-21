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
        INPUT = 5,
        WELCOME = 6,
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
        uint32_t type_id = 0;
        uint32_t owner_peer_id = 0;
        Vec3 position{};
        Quat rotation{};
        bool predicted = false;

        void serialize(StreamWriter& stream) const {
            stream.write<uint64_t>(entity_Id);
            stream.write<uint32_t>(type_id);
            stream.write<uint32_t>(owner_peer_id);
            stream.write_vec3(position);
            stream.write_quat(rotation);
            stream.write_bool(predicted);
        }

        static std::optional<SpawnEntityMsg> deserialize(StreamReader& stream) {
            auto entity_id = stream.read<uint64_t>();
            auto type_id = stream.read<uint32_t>();
            auto owner_peer_id = stream.read<uint32_t>();
            auto pos = stream.read_vec3();
            auto rot = stream.read_quat();
            auto predicted = stream.read_bool();
            if (!entity_id || !type_id || !owner_peer_id || !pos || !rot || !predicted) {
                return std::nullopt;
            }
            return SpawnEntityMsg{*entity_id, *type_id, *owner_peer_id, *pos, *rot, *predicted};
        }
    };
    static_assert(NetMessage<SpawnEntityMsg>);

    struct DespawnEntityMsg {
        static constexpr PacketId PACKET_ID = PacketId::DESPAWN_ENTITY;
        uint64_t entity_Id = 0;
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
        uint32_t tick = 0;
        uint64_t ack_input_seq = 0;
        std::vector<NetBatch> batches;

        void serialize(StreamWriter& stream) const {
            stream.write<uint32_t>(tick);
            stream.write<uint64_t>(ack_input_seq);

            std::vector<uint8_t> soa_buffer;
            for (const NetBatch& batch : batches) {
                std::vector<uint8_t> block = serialize_batch(batch);
                soa_buffer.insert(soa_buffer.end(), block.begin(), block.end());
            }
            stream.write_bytes(soa_buffer);
        }

        static std::optional<StateSyncMsg> deserialize(StreamReader& stream) {
            auto tick = stream.read<uint32_t>();
            auto ack_input_seq = stream.read<uint64_t>();
            auto raw_opt = stream.read_bytes();
            if (!tick || !raw_opt || !ack_input_seq) {
                return std::nullopt;
            }

            StateSyncMsg msg;
            msg.tick = *tick;
            msg.ack_input_seq = *ack_input_seq;
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

    struct InputMsg {
        static constexpr PacketId PACKET_ID = PacketId::INPUT;
        uint64_t seq = 0;
        uint64_t target_entity_id = 0;
        InputPayload payload{};

        void serialize(StreamWriter& stream) const {
            stream.write<uint64_t>(seq);
            stream.write<uint64_t>(target_entity_id);
            stream.write<uint8_t>(payload.size);
            if (!payload.empty()) stream.write_raw_bytes(payload.bytes());
        }

        static std::optional<InputMsg> deserialize(StreamReader& stream) {
            const auto seq = stream.read<uint64_t>();
            const auto target_entity_id = stream.read<uint64_t>();
            auto sz = stream.read<uint8_t>();
            if (!sz || *sz > InputPayload::MAX_SIZE) return std::nullopt;

            const auto raw = stream.read_raw_bytes(*sz);
            if (!seq || !target_entity_id || !raw) {
                return std::nullopt;
            }

            return *sz == 0 ? InputMsg{*seq, *target_entity_id, InputPayload{}}
                    : InputMsg{*seq, *target_entity_id, *InputPayload::from_bytes(*raw)};
        }
    };
    static_assert(NetMessage<InputMsg>);

    struct WelcomeMsg {
        static constexpr PacketId PACKET_ID = PacketId::WELCOME;
        uint32_t assigned_peer_id = 0;
        uint32_t tick = 0;

        void serialize(StreamWriter& stream) const {
            stream.write<uint32_t>(assigned_peer_id);
            stream.write<uint32_t>(tick);
        }

        static std::optional<WelcomeMsg> deserialize(StreamReader& stream) {
            auto const assigned_peer_id = stream.read<uint32_t>();
            auto const tick = stream.read<uint32_t>();
            if (!assigned_peer_id || !tick) {
                return std::nullopt;
            }

            return WelcomeMsg{*assigned_peer_id, *tick};
        }
    };
    static_assert(NetMessage<WelcomeMsg>);
} // networld

#endif //NETWORLD_NETMESSAGE_H
