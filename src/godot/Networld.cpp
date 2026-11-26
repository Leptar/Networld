//
// Created by leoul on 2026-11-02.
//

#include <utility>

#include "DemoMovementSim.h"
#include "NetWorld.h"
#include "flecs.h"
#include "GodotUDPTransport.h"
#include "godot_cpp/classes/node3d.hpp"
#include "godot_cpp/classes/project_settings.hpp"

using namespace godot;
using namespace networld;

NetWorld *NetWorld::networld_ = nullptr;
std::vector<NetWorld::TransportEntry> NetWorld::transport_registry_;

void NetWorld::_on_settings_changed() {
    ProjectSettings *ps = ProjectSettings::get_singleton();
    String setting_path = "network/networld/default_transport";

    if (ps->has_setting(setting_path)) {
        String new_setting = ps->get_setting(setting_path);
        if (new_setting != current_transport_name_) {
            current_transport_name_ = new_setting;
            core_.set_transport(create_transport());
        }
    }
}

std::unique_ptr<INetTransport> NetWorld::create_transport() const {
    for (const auto& entry : transport_registry_) {
        if (entry.name == current_transport_name_) {
            return entry.factory();
        }
    }
    // sécurité au cas ou le nom n'est pas trouvé
    ERR_PRINT("Transport introuvable: " + current_transport_name_ + ". Fallback sur Godot UDP.");
    return std::make_unique<GodotUdpTransport>();

}

void NetWorld::bind_entity_view(uint64_t id, Node3D *visual_node) {
    ERR_FAIL_NULL_MSG(visual_node, "NetWorld::bind_entity_view : visual_node is null");
    uint64_t handle_id = visual_node->get_instance_id();
    core_.bind_entity_view(id, handle_id);
}

void NetWorld::_bind_methods() {
    ClassDB::bind_method(D_METHOD("host_listen", "port"), &NetWorld::host_listen);
    ClassDB::bind_method(D_METHOD("host_dedicated", "port"), &NetWorld::host_dedicated);
    ClassDB::bind_method(D_METHOD("join", "ip", "port"), &NetWorld::join);
    ClassDB::bind_method(D_METHOD("stop_session"), &NetWorld::stop_session);

    ClassDB::bind_method(
        D_METHOD("bind_entity_view", "id", "visual_node"),
        &NetWorld::bind_entity_view);
    ClassDB::bind_method(D_METHOD("set_local_move_input",  "entity_id", "dir"), &NetWorld::set_local_move_input);
    ClassDB::bind_method(D_METHOD("get_local_peer_id"), &NetWorld::get_local_peer_id);
    ClassDB::bind_method(D_METHOD("spawn_entity", "type_id", "owner_peer_id", "pos", "rot", "predicted"), &NetWorld::spawn_entity);
    ClassDB::bind_method(D_METHOD("despawn_entity", "entity_id"), &NetWorld::despawn_entity);
    ClassDB::bind_method(D_METHOD("destroy_entities_owned_by", "peer_id"), &NetWorld::destroy_entities_owned_by);

    ADD_SIGNAL(MethodInfo("entity_spawned",
        PropertyInfo(Variant::INT, "entity_id"),
        PropertyInfo(Variant::INT, "type_id"),
        PropertyInfo(Variant::INT, "owner_peer_id"),
        PropertyInfo(Variant::VECTOR3, "position"),
        PropertyInfo(Variant::QUATERNION, "rotation"),
        PropertyInfo(Variant::BOOL, "is_local")));
    ADD_SIGNAL(MethodInfo("entity_despawned",
        PropertyInfo(Variant::INT, "entity_id")));
    ADD_SIGNAL(MethodInfo("peer_connected",
        PropertyInfo(Variant::INT, "peer_id"),
        PropertyInfo(Variant::BOOL, "is_local")));
    ADD_SIGNAL(MethodInfo("peer_disconnected",
        PropertyInfo(Variant::INT, "peer_id")));
}

void NetWorld::register_transport(const String &name, TransportFactory factory) {
    transport_registry_.push_back({name, std::move(factory)});
}

bool NetWorld::host_listen(int port) {
    stop_session();
    core_.set_transport(create_transport());
    return core_.start_host(port, networld::HostMode::LISTEN);
}

bool NetWorld::host_dedicated(int port) {
    stop_session();
    core_.set_transport(create_transport());
    return core_.start_host(port, networld::HostMode::DEDICATED);
}

bool NetWorld::join(const String &ip, int port) {
    stop_session();
    core_.set_transport(create_transport());
    const std::string ip_s = ip.utf8().get_data();
    return core_.start_client(ip_s, port);
}

void NetWorld::stop_session() {
    core_.stop();

    core_.clear_connected_peers();
    core_.clear_disconnected_peers();
    core_.clear_despawned_entities();
    core_.clear_spawned_entities();
}

uint64_t NetWorld::spawn_entity(uint32_t type_id, uint32_t owner_peer_id, const Vector3 &pos, const Quaternion &rot,
    bool predicted) {
    SpawnParams spawn_params;
    spawn_params.type_id = type_id;
    spawn_params.owner_peer_id = owner_peer_id;
    Vec3 position = {pos.x, pos.y, pos.z};
    Quat rotation = {rot.x, rot.y, rot.z, rot.w};
    spawn_params.position = position;
    spawn_params.rotation = rotation;
    spawn_params.predicted = predicted;

    return core_.spawn_networked_entity(spawn_params);
}

void NetWorld::despawn_entity(uint64_t entity_id) {
    core_.despawn_networked_entity(entity_id);
}

void NetWorld::destroy_entities_owned_by(uint32_t peer_id) {
    core_.destroy_entities_owned_by(peer_id);
}

void NetWorld::set_local_move_input(uint64_t entity_id, const Vector2 &dir) {
    InputPayload input_payload = InputPayload::from(DemoMoveInput{dir.x, dir.y});
    core_.set_local_input(entity_id, input_payload);
}

void NetWorld::_physics_process(double p_delta) {
    Node::_physics_process(p_delta);

    core_.step_simulation(static_cast<float>(p_delta));

    // signal for disconnected peers
    std::vector<uint32_t> disconnected_peers = core_.get_disconnected_peers();
    core_.clear_disconnected_peers();
    for (auto peer : disconnected_peers) {
        emit_signal("peer_disconnected", peer);
    }

    // signal for dispawned entity
    std::vector<uint64_t> despawned_entities = core_.get_despawned_entities();
    core_.clear_despawned_entities();
    for (auto peer : despawned_entities) {
        emit_signal("entity_despawned", peer);
    }

    // signal for peer connected
    std::vector<PeerEvent> connected_peers = core_.get_connected_peers();
    core_.clear_connected_peers();
    for (auto peer : connected_peers) {
        emit_signal("peer_connected", peer.peer_id, peer.is_local);
    }

    //signal for spawned entity
    std::vector<EntitySpawnEntry> spawned_entities = core_.get_spawned_entities();
    core_.clear_spawned_entities();
    for (auto peer : spawned_entities) {
        Vector3 position = {peer.position.x, peer.position.y, peer.position.z};
        Quaternion rotation = {peer.rotation.x, peer.rotation.y, peer.rotation.z, peer.rotation.w};
        emit_signal("entity_spawned", peer.entity_id, peer.type_id, peer.owner_peer_id, position, rotation, peer.is_local);
    }
}

NetWorld* NetWorld::get_networld() {
    return networld_;
}

uint32_t NetWorld::get_local_peer_id() const {
    return core_.get_local_peer_id();
}

void NetWorld::set_entity_velocity(uint64_t entity_id, const Vector3 &velocity) {
    Vec3 vel = {velocity.x, velocity.y, velocity.z};
    core_.set_entity_velocity(entity_id, vel);
}

void NetWorld::set_entity_position(uint64_t entity_id, const Vector3 &pos) {
    Vec3 position = {pos.x, pos.y, pos.z};
    core_.set_entity_position(entity_id, position);
}

NetWorld::NetWorld() {
    ERR_FAIL_COND(networld_ != nullptr); // tout le temps vrai vu que je le set dans register et donc au demarrage de l'editeur
    networld_ = this;

    ProjectSettings *ps = ProjectSettings::get_singleton();
    String setting_path = "network/networld/default_transport";

    if (!ps->has_setting(setting_path)) {
        ps->set_setting(setting_path, "Godot UDP");
    }
    ps->set_initial_value(setting_path, "Godot UDP");

    // options pour le menu déroulant de l'éditeur
    String hint_string = "";
    for (size_t i = 0; i < transport_registry_.size(); ++i) {
        if (i > 0) hint_string += ",";
        hint_string += transport_registry_[i].name;
    }

    Dictionary prop_info;
    prop_info["name"] = setting_path;
    prop_info["type"] = Variant::STRING; // On utilise un String
    prop_info["hint"] = PROPERTY_HINT_ENUM;
    prop_info["hint_string"] = hint_string; // par ex: "Godot UDP,Steam,ENet"
    ps->add_property_info(prop_info);

    current_transport_name_ = ps->get_setting(setting_path);
    core_.set_transport(create_transport());

    if (!ps->is_connected("settings_changed", callable_mp(this, &NetWorld::_on_settings_changed))) {
        ps->connect("settings_changed", callable_mp(this, &NetWorld::_on_settings_changed));
    }

    core_.register_entity_simulator(1, networld::simulate_demo_player);
}

NetWorld::~NetWorld() {
    if (networld_ == this) {
        networld_ = nullptr;
    }
}

void NetWorld::_process(double delta) {
    core_.step_visual(static_cast<float>(delta));

    for (const auto& update : core_.get_view_updates()) {
        Object* obj = ObjectDB::get_instance(update.host_handle);
        auto* node = cast_to<Node3D>(obj);
        if (node) {
            node->set_global_position(Vector3(update.position.x, update.position.y, update.position.z));
            node->set_global_basis(Basis(Quaternion(update.rotation.x, update.rotation.y, update.rotation.z, update.rotation.w)));
        }
    }

}