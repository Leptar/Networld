//
// Created by leoul on 2026-11-14.
//

#include "StreamReader.h"

#include <bit>

namespace networld {

    std::optional<bool> StreamReader::read_bool() {
        auto raw = read<uint8_t>();
        if (!raw.has_value()) return std::nullopt;
        return *raw != 0;
    }

    std::optional<float> StreamReader::read_float() {
        auto raw = read<uint32_t>();
        if (!raw.has_value()) return std::nullopt;
        return std::bit_cast<float>(*raw);
    }

    std::optional<Vec3> StreamReader::read_vec3() {
        auto x = read_float();
        auto y = read_float();
        auto z = read_float();
        if (!x || !y || !z) return std::nullopt;
        return Vec3{*x, *y, *z};
    }

    std::optional<std::span<const uint8_t>> StreamReader::read_raw_bytes(size_t count) {
        if (_cursor.size() < count) {
            return std::nullopt;
        }
        auto slice = _cursor.subspan(0, count);
        _cursor = _cursor.subspan(count);
        return slice;
    }

    std::optional<std::span<const uint8_t>> StreamReader::read_bytes() {
        auto len = read<uint32_t>();
        if (!len.has_value()) return std::nullopt;
        return read_raw_bytes(*len);
    }

    bool StreamReader::empty() const {
        return _cursor.empty();
    }
} // networld