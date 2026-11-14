//
// Created by leoul on 2026-11-14.
//

#ifndef NETWORLD_PACKETDISPATCHER_H
#define NETWORLD_PACKETDISPATCHER_H
#include <array>
#include <concepts>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "NetStream.h"
#include "StreamReader.h"
#include "StreamWriter.h"

namespace networld {

    template <typename T>
    concept NetMessage = NetSerializable<T, StreamReader, StreamWriter> && requires {
            { static_cast<uint8_t>(T::PACKET_ID) } -> std::same_as<uint8_t>;
    };

    template <NetMessage T>
    std::vector<uint8_t> encode_packet(const T& msg) {
        StreamWriter writer;
        writer.write<uint8_t>(static_cast<uint8_t>(T::PACKET_ID));
        msg.serialize(writer);
        return writer.finish();
    }

    class PacketDispatcher {
        using RawHandler = std::function<void(uint32_t peer_id, StreamReader& reader)>;
        std::array<RawHandler, 256> _handlers{};

    public:
        template <NetMessage T, typename Callback>
        requires std::invocable<Callback, uint32_t, const T&>
        void register_handler(Callback&& cb) {
            constexpr auto id = static_cast<uint8_t>(T::PACKET_ID);

            _handlers[id] = [func = std::forward<Callback>(cb)](uint32_t peer_id, StreamReader& reader) {
                std::optional<T> msg = T::deserialize(reader);
                if (msg.has_value()) {
                    func(peer_id, *msg);
                }
                // paquet ignoré = mal tronqué ou formé
            };
        }

        template <NetMessage T>
        [[nodiscard]] bool has_handler() const {
            return _handlers[static_cast<uint8_t>(T::PACKET_ID)] != nullptr;
        }

        [[nodiscard]] bool dispatch(uint32_t peer_id, std::span<const uint8_t> raw_packet) const;
    };
} // networld

#endif //NETWORLD_PACKETDISPATCHER_H
