//
// Created by leoul on 2026-11-14.
//

#ifndef NETWORLD_NETSTREAM_H
#define NETWORLD_NETSTREAM_H
#include <concepts>
#include <optional>
#include <type_traits>

namespace networld {
    // NetIntegrale doit etre un int different d'un booleen
    template <typename T>
    concept NetIntegral = std::integral<T> && !std::same_as<T, bool>;

    // NetEnum doit etre la valeur d'une enum
    template <typename T>
    concept NetEnum = std::is_enum_v<T>;

    // NetSerializable est vrai si on peut le serializer et le deserializer avec son type de paquet
    template <typename T, typename Reader, typename Writer>
    concept NetSerializable = requires(const T& obj, Reader& r, Writer& w){
        { obj.serialize(w) } -> std::same_as<void>;
        { T::deserialize(r) } -> std::same_as<std::optional<T>>;
    };

} // networld

#endif //NETWORLD_NETSTREAM_H
