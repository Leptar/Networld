//
// Created by leoul on 2026-11-14.
//

#ifndef NETWORLD_STREAMWRITER_H
#define NETWORLD_STREAMWRITER_H
#include <cstdint>
#include <span>
#include <vector>
#include <cstring>
#include <type_traits>
#include "NetComponent.h"
#include "NetStream.h"

namespace networld {
    class StreamWriter {
        std::vector<uint8_t> _buffer;
    public:
        template <NetIntegral T>
        void write(T value){
            const size_t offset = _buffer.size();
            _buffer.resize(offset + sizeof(T));
            std::memcpy(_buffer.data() + offset, &value, sizeof(T));
        }

        void write_bool(bool value);
        void write_float(float value);
        void write_vec3(const Vec3& value);

        template <NetEnum T>
        void write_enum(T value) {
            write(static_cast<std::underlying_type_t<T>>(value));
        }

        void write_raw_bytes(std::span<const uint8_t> bytes);
        void write_bytes(std::span<const uint8_t> bytes);

        template <typename T>
        requires NetWritable<T, StreamWriter>
        void write_struct(const T& s) {
            s.serialize(*this);
        }

        std::vector<uint8_t> finish();
    };
} // networld

#endif //NETWORLD_STREAMWRITER_H
