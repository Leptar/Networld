//
// Created by leoul on 2026-11-14.
//

#include "PacketDispatcher.h"

namespace networld {

    bool PacketDispatcher::dispatch(uint32_t peer_id, std::span<const uint8_t> raw_packet) const {
        if (raw_packet.empty()) return false;

        StreamReader reader(raw_packet);
        auto id_opt = reader.read<uint8_t>();
        if (!id_opt.has_value()) return false;

        const RawHandler& handler = _handlers[*id_opt];
        if (!handler) return false;


        handler(peer_id, reader);
        return true;
    }
} // networld