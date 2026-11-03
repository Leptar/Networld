//
// Created by leoul on 2026-11-02.
//

#include "GodotUdpTransport.h"
#include <godot_cpp/variant/utility_functions.hpp>

GodotUdpTransport::GodotUdpTransport() {
    udp.instantiate();
}

bool GodotUdpTransport::open_as_host(uint16_t port) {
    is_host = true;
    Error err = udp->bind(port);
    if (err == OK) UtilityFunctions::print("[UDP] Host demarre sur le port ", port);
    return err == OK;
}

bool GodotUdpTransport::open_as_client(const std::string& host_ip, uint16_t port) {
    is_host = false;
    target_ip = String(host_ip.c_str());
    target_port = port;

    // Le client se bind sur un port aleatoire et cible le Host
    udp->bind(0);
    udp->set_dest_address(target_ip, target_port);
    UtilityFunctions::print("[UDP] Client connecte a ", target_ip, ":", port);
    return true;
}

void GodotUdpTransport::close() {
    udp->close();
}

void GodotUdpTransport::poll() {
    if (!udp.is_valid()) return;

    while (udp->get_available_packet_count() > 0) {
        PackedByteArray pkt = udp->get_packet();

        // En mode Host (1v1), on sauvegarde l'adresse de l'emetteur pour lui repondre
        if (is_host) {
            target_ip = udp->get_packet_ip();
            target_port = udp->get_packet_port();
            udp->set_dest_address(target_ip, target_port);
        }

        NetPacket p;
        p.sender_peer_id = is_host ? 2 : 1; // 1 = Host, 2 = Client
        p.channel = NetChannel::UNRELIABLE_STATE;
        p.payload.assign(pkt.ptr(), pkt.ptr() + pkt.size());

        rx_queue.push(p);
    }
}

bool GodotUdpTransport::send(uint32_t target_peer_id, NetChannel channel, const uint8_t* data, size_t size) {
    if (target_ip.is_empty() || target_port == 0 || data == nullptr || size == 0) return false;

    PackedByteArray godot_array;
    godot_array.resize(size);
    std::memcpy(godot_array.ptrw(), data, size);

    return udp->put_packet(godot_array) == OK;
}

bool GodotUdpTransport::recv(NetPacket& out_packet) {
    if (rx_queue.empty()) return false;
    out_packet = rx_queue.front();
    rx_queue.pop();
    return true;
}