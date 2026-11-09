//
// Created by leoul on 2026-11-02.
//

#ifndef NETWORLD_GODOTUDPTRANSPORT_H
#define NETWORLD_GODOTUDPTRANSPORT_H

#pragma once
#include "../core/INetTransport.h"
#include <godot_cpp/classes/packet_peer_udp.hpp>
#include <queue>

using namespace godot;

class GodotUdpTransport : public INetTransport {

    Ref<PacketPeerUDP> udp;
    bool is_host = false;
    String target_ip = "";
    uint16_t target_port = 0;
    std::queue<NetPacket> rx_queue;

public:
    GodotUdpTransport();

    bool open_as_host(uint16_t port) override;
    bool open_as_client(const std::string& host_ip, uint16_t port) override;
    void close() override;

    void poll() override;
    bool send(uint32_t target_peer_id, NetChannel channel, const uint8_t* data, size_t size) override;
    bool recv(NetPacket& out_packet) override;
};
#endif //NETWORLD_GODOTUDPTRANSPORT_H
