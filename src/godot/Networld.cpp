//
// Created by leoul on 2026-11-02.
//

#include <utility>

#include "NetWorld.h"
#include "flecs.h"
#include "GodotUDPTransport.h"
#include "godot_cpp/classes/node3d.hpp"
#include "godot_cpp/classes/project_settings.hpp"

using namespace godot;

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
    ClassDB::bind_method(D_METHOD("host", "port"), &NetWorld::host);
    ClassDB::bind_method(D_METHOD("join", "ip", "port"), &NetWorld::join);
    ClassDB::bind_method(
        D_METHOD("bind_entity_view", "id", "visual_node"),
        &NetWorld::bind_entity_view);
    ClassDB::bind_method(D_METHOD("set_client_input", "input"), &NetWorld::set_client_input);
    ClassDB::bind_method(D_METHOD("get_local_entity_id"), &NetWorld::get_local_entity_id);

    ADD_SIGNAL(MethodInfo("entity_spawned",
        PropertyInfo(Variant::INT, "entity_id"),
        PropertyInfo(Variant::VECTOR3, "position"),
        PropertyInfo(Variant::QUATERNION, "rotation"),
        PropertyInfo(Variant::BOOL, "is_local")));
    ADD_SIGNAL(MethodInfo("entity_despawned",
        PropertyInfo(Variant::INT, "entity_id")));
}

void NetWorld::register_transport(const String &name, TransportFactory factory) {
    transport_registry_.push_back({name, std::move(factory)});
}

NetWorld* NetWorld::get_networld() {
    return networld_;
}

void NetWorld::set_entity_velocity(uint64_t entity_id, const Vector3 &velocity) {
    networld::Vec3 vel = {velocity.x, velocity.y, velocity.z};
    core_.set_entity_velocity(entity_id, vel);
}

void NetWorld::set_entity_position(uint64_t entity_id, const Vector3 &pos) {
    networld::Vec3 position = {pos.x, pos.y, pos.z};
    core_.set_entity_position(entity_id, position);
}

void NetWorld::set_client_input(const Vector3 &input) {
    networld::Vec3 new_input = {input.x, input.y, input.z};
    core_.set_client_input(new_input);
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
}

NetWorld::~NetWorld() {
    if (networld_ == this) {
        networld_ = nullptr;
    }
}

void NetWorld::host(int port) {
    core_.clear_despawned_entities();
    core_.clear_spawned_entities();
    core_.stop();
    core_.set_transport(create_transport());
    core_.start_host(port);
}

void NetWorld::join(String ip, int port) {
    core_.clear_despawned_entities();
    core_.clear_spawned_entities();
    core_.stop();
    core_.set_transport(create_transport());
    core_.start_client(ip.utf8().get_data(), port);
}

uint64_t NetWorld::spawn_local_entity(Vector3 pos, Node3D *visual_node) {
    uint64_t handle = visual_node ? visual_node->get_instance_id() : 0;
    return core_.create_networked_entity({pos.x, pos.y, pos.z}, handle);

}

void NetWorld::_process(double delta) {
    core_.step(static_cast<float>(delta));

    for (const auto &it : core_.get_despawned_entities()) {
        emit_signal("entity_despawned", it);
    }

    for (const auto &it : core_.get_spawned_entities()) {
        Vector3 entity_pos;
        entity_pos.x = it.position.x;
        entity_pos.y = it.position.y;
        entity_pos.z = it.position.z;

        emit_signal("entity_spawned", it.entity_id, entity_pos, it.is_local);
    }

    for (const auto& update : core_.get_view_updates()) {
        Object* obj = ObjectDB::get_instance(update.host_handle);
        auto* node = cast_to<Node3D>(obj);
        if (node) {
            node->set_global_position(Vector3(update.position.x, update.position.y, update.position.z));
            node->set_global_basis(Basis(Quaternion(update.rotation.x, update.rotation.y, update.rotation.z, update.rotation.w)));
        }
    }

    core_.clear_despawned_entities();
    core_.clear_spawned_entities();
}