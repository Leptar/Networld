//
// Created by leoul on 2026-11-14.
//

#include "StreamWriter.h"

#include <bit>

namespace networld {

    void StreamWriter::write_bool(bool value) {
        uint8_t byte = value ? 1 : 0;
        write<uint8_t>(byte);
    }

    void StreamWriter::write_float(float value) {
        uint32_t convert_value = std::bit_cast<uint32_t>(value);
        write<uint32_t>(convert_value);
    }

    void StreamWriter::write_vec3(const Vec3 &value) {
        write_float(value.x);
        write_float(value.y);
        write_float(value.z);
    }

    void StreamWriter::write_quat(const Quat &value) {
        write_float(value.x);
        write_float(value.y);
        write_float(value.z);
        write_float(value.w);
    }

    void StreamWriter::write_raw_bytes(std::span<const uint8_t> bytes) {
        _buffer.insert(_buffer.end(), bytes.begin(), bytes.end());
    }

    void StreamWriter::write_bytes(std::span<const uint8_t> bytes) {
        write<uint32_t>(static_cast<uint32_t>(bytes.size()));
        write_raw_bytes(bytes);
    }

    std::vector<uint8_t> StreamWriter::finish() {
        std::vector<uint8_t> result = std::move(_buffer);
        _buffer.clear();
        return result;
    }
} // networld