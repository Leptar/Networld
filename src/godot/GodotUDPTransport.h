//
// Created by leoul on 2026-11-02.
//

#ifndef NETWORLD_GODOTUDPTRANSPORT_H
#define NETWORLD_GODOTUDPTRANSPORT_H

#pragma once
#include "../core/INetTransport.h"
#include <godot_cpp/classes/packet_peer_udp.hpp>
#include <queue>
#include <unordered_map>
#include <chrono>

using namespace godot;

constexpr uint64_t PEER_TIMEOUT = 5000; // ms

struct ClientInfo {
    uint32_t peer_id;
    String ip;
    uint16_t port;
    uint64_t last_seen_ms;
};

class GodotUdpTransport : public INetTransport {

    Ref<PacketPeerUDP> udp;

    uint32_t next_peer_id = 2;
    std::unordered_map<uint32_t, ClientInfo> clients_by_id;
    std::unordered_map<std::string, uint32_t> clients_to_id;
    bool is_host = false;
    std::queue<NetPacket> rx_queue;
    String target_ip;
    uint16_t target_port;

    std::string make_endpoint_key(const String& ip, uint16_t port);

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
