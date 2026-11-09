//
// Created by leoul on 2026-11-02.
//

#ifndef NETWORLD_INETTRANSPORT_H
#define NETWORLD_INETTRANSPORT_H

#pragma once
#include <cstdint>
#include <vector>
#include <string>

enum class NetChannel : uint8_t {
    UNRELIABLE_STATE = 0,
    RELIABLE_EVENTS = 1
};

struct NetPacket {
    uint32_t sender_peer_id;
    NetChannel channel;
    std::vector<uint8_t> payload;
};

class INetTransport {
public:
    virtual ~INetTransport() = default;

    virtual bool open_as_host(uint16_t port) = 0;
    virtual bool open_as_client(const std::string& host_ip, uint16_t port) = 0;
    virtual void close() = 0;

    virtual void poll() = 0;

    // Remplace std::span par un pointeur brut + taille
    virtual bool send(uint32_t target_peer_id, NetChannel channel, const uint8_t* data, size_t size) = 0;
    virtual bool recv(NetPacket& out_packet) = 0;
};

#endif //NETWORLD_INETTRANSPORT_H
