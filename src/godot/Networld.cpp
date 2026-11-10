//
// Created by leoul on 2026-11-02.
//

#include "NetWorld.h"
#include "flecs.h"
#include "../components.h"
#include "GodotUDPTransport.h"

using namespace godot;

void NetWorld::_bind_methods() {
    ClassDB::bind_method(D_METHOD("host", "port"), &NetWorld::host);
    ClassDB::bind_method(D_METHOD("join", "ip", "port"), &NetWorld::join);
    ClassDB::bind_method(
        D_METHOD(
            "bind_view_to_server_player",
            "visual_node"),
        &NetWorld::bind_view_to_server_player);
}

void NetWorld::bind_view_to_server_player(Node3D* visual_node) {
    if (!visual_node) return;
    uint64_t handle = visual_node->get_instance_id();
    core_.bind_external_view("ServerPlayer", handle);
}

NetWorld::NetWorld() {
}

NetWorld::~NetWorld() {
}

void NetWorld::host(int port) {
    core_.set_transport(std::make_unique<GodotUdpTransport>());
    core_.start_host(port);
}

void NetWorld::join(String ip, int port) {
    core_.set_transport(std::make_unique<GodotUdpTransport>());
    core_.start_client(ip.utf8().get_data(), port);
}

uint64_t NetWorld::spawn_local_entity(Vector3 pos, Node3D *visual_node) {
    uint64_t handle = visual_node ? visual_node->get_instance_id() : 0;
    return core_.create_networked_entity({pos.x, pos.y, pos.z}, handle);

}

void NetWorld::_process(double delta) {
    core_.step(static_cast<float>(delta));

    for (const auto& update : core_.get_view_updates()) {
        Object* obj = ObjectDB::get_instance(update.host_handle);
        auto* node = cast_to<Node3D>(obj);
        if (node) {
            node->set_global_position(Vector3(update.position.x, update.position.y, update.position.z));
        }
    }
}