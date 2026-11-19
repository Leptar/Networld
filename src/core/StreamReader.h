//
// Created by leoul on 2026-11-14.
//

#ifndef NETWORLD_STREAMREADER_H
#define NETWORLD_STREAMREADER_H
#include <cstdint>
#include <span>
#include <cstring>
#include <optional>
#include <type_traits>
#include "NetComponent.h"
#include "NetStream.h"

namespace networld {

    class StreamReader {
        std::span<const uint8_t> _cursor;
    public:
        explicit StreamReader(std::span<const uint8_t> data) : _cursor(data) {}

        template <NetIntegral T>
        std::optional<T> read(){
            if (_cursor.size() < sizeof(T)) {
                return std::nullopt;
            }

            T value{};
            std::memcpy(&value, _cursor.data(), sizeof(T));
            _cursor = _cursor.subspan(sizeof(T));
            return value;
        }

        std::optional<bool> read_bool();
        std::optional<float> read_float();
        std::optional<Vec3> read_vec3();
        std::optional<Quat> read_quat();

        template <NetEnum T>
        std::optional<T> read_enum() {
            auto raw = read<std::underlying_type_t<T>>();
            if (!raw.has_value()) {
                return std::nullopt;
            }
            return static_cast<T>(*raw);
        }

        std::optional<std::span<const uint8_t>> read_raw_bytes(size_t count);
        std::optional<std::span<const uint8_t>> read_bytes();

        std::span<const uint8_t> remaining() const { return _cursor; }
        bool empty() const;

        template <typename T>
        requires NetReadable<T, StreamReader>
        std::optional<T> read_struct() {
            return T::deserialize(*this);
        }

    };
} // networld

#endif //NETWORLD_STREAMREADER_H
