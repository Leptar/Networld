//
// Created by leoul on 2026-11-02.
//

#include "GodotUdpTransport.h"
#include <godot_cpp/variant/utility_functions.hpp>

#include "godot_cpp/classes/time.hpp"

std::string GodotUdpTransport::make_endpoint_key(const String &ip, uint16_t port) {
    return std::string(ip.utf8().get_data()) + ":" + std::to_string(port);
}

GodotUdpTransport::GodotUdpTransport() {
    udp.instantiate();
}

bool GodotUdpTransport::open_as_host(uint16_t port) {
    is_host = true;
    next_peer_id = 2;
    clients_by_id.clear();
    clients_to_id.clear();

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
    clients_by_id.clear();
    clients_to_id.clear();
    udp->close();
}

void GodotUdpTransport::poll() {
    if (!udp.is_valid()) return;

    while (udp->get_available_packet_count() > 0) {
        PackedByteArray pkt = udp->get_packet();
        NetPacket p;
        String ip = udp->get_packet_ip();
        uint16_t port = udp->get_packet_port();

        if (is_host) {
            std::string key = make_endpoint_key(ip, port);
            uint64_t timestamp = Time::get_singleton()->get_ticks_msec();
            auto it = clients_to_id.find(key);

            if (it == clients_to_id.end()) {

                // nouveau client
                uint32_t assigned_id = next_peer_id++;
                ClientInfo client_info;
                client_info.peer_id = assigned_id;
                client_info.ip = ip;
                client_info.port = port;
                client_info.last_seen_ms = timestamp;

                //paquet de connection
                NetPacket conn_pkt;
                conn_pkt.sender_peer_id = assigned_id;
                conn_pkt.event = NetEvent::CONNECTED;
                conn_pkt.channel = NetChannel::RELIABLE_EVENTS;
                rx_queue.push(conn_pkt);
                UtilityFunctions::print("[UDP] Nouveau client ", assigned_id, " depuis ", ip, ":", port);

                // ajout dans les listes
                clients_by_id[assigned_id] = client_info;
                clients_to_id[key] = assigned_id;
                p.sender_peer_id = assigned_id;
            } else {
                uint32_t client_id = it->second;
                clients_by_id[client_id].last_seen_ms = timestamp;
                p.sender_peer_id = client_id;
            }
        } else {
            p.sender_peer_id = PEER_SERVER;
        }

        p.channel = NetChannel::UNRELIABLE_STATE;
        p.payload.assign(pkt.ptr(), pkt.ptr() + pkt.size());

        rx_queue.push(p);
    }

    // gestion timeout
    if (is_host) {
        uint64_t timestamp = Time::get_singleton()->get_ticks_msec();
        std::vector<uint32_t> to_disconnect;

        for (auto & pair: clients_by_id) {
            if (timestamp - clients_by_id[pair.first].last_seen_ms > PEER_TIMEOUT) {
                to_disconnect.push_back(pair.first);
            }
        }

        for (auto id: to_disconnect) {
            String ip = clients_by_id[id].ip;
            uint32_t port = clients_by_id[id].port;
            std::string key = make_endpoint_key(ip, port);

            clients_to_id.erase(key);
            clients_by_id.erase(id);

            NetPacket disc_pkt;
            disc_pkt.sender_peer_id = id;
            disc_pkt.event = NetEvent::DISCONNECTED;
            disc_pkt.channel = NetChannel::RELIABLE_EVENTS;
            rx_queue.push(disc_pkt);
        }
    }
}

bool GodotUdpTransport::send(uint32_t target_peer_id, NetChannel channel, const uint8_t* data, size_t size) {
    if (data == nullptr || size == 0) return false;

    // Préparer le PackedByteArray Godot une seule fois
    PackedByteArray godot_array;
    godot_array.resize(size);
    std::memcpy(godot_array.ptrw(), data, size);

    // client : toujours vers l'hôte
    if (!is_host) {
        udp->set_dest_address(target_ip, target_port);
        return udp->put_packet(godot_array) == OK;
    }
    // serveur broadcast
    if (target_peer_id == PEER_BROADCAST) {
        bool all_ok = true;
        for (const auto& pair : clients_by_id) {
            const ClientInfo& client = pair.second;
            udp->set_dest_address(client.ip, client.port);
            if (udp->put_packet(godot_array) != OK) {
                all_ok = false;
            }
        }
        return all_ok;
    }
    // Serveur cible un client (target_peer_id > 1)
    auto it = clients_by_id.find(target_peer_id);
    if (it != clients_by_id.end()) {
        const ClientInfo& client = it->second;
        udp->set_dest_address(client.ip, client.port);
        return udp->put_packet(godot_array) == OK;
    }

    // Client introuvable
    return false;
}

bool GodotUdpTransport::recv(NetPacket& out_packet) {
    if (rx_queue.empty()) return false;
    out_packet = rx_queue.front();
    rx_queue.pop();
    return true;
}