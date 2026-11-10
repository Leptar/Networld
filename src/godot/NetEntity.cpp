//
// Created by leoul on 2026-11-03.
//

#include "NetEntity.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "Networld.h"
#include "../components.h"
#include "godot_cpp/classes/canvas_item.hpp"

using namespace godot;

void NetEntity::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_networld_path", "path"), &NetEntity::set_networld_path);
    ClassDB::bind_method(D_METHOD("get_networld_path"), &NetEntity::get_networld_path);
    ClassDB::bind_method(D_METHOD("bake"), &NetEntity::bake);
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "networld_path"), "set_networld_path", "get_networld_path");
}

NetEntity::NetEntity() {
    networld_path = NodePath("/root/NetWorld"); // Chemin par défaut (Singleton)
}

NetEntity::~NetEntity() {}

void NetEntity::set_networld_path(const NodePath& path) {
    networld_path = path;
}

NodePath NetEntity::get_networld_path() const {
    return networld_path;
}

void NetEntity::_ready() {
    if (Engine::get_singleton()->is_editor_hint()) {
        return;
    }

    call_deferred("bake");
}

void NetEntity::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_READY :
            call_deferred("bake");
            break;
        case Node3D::NOTIFICATION_TRANSFORM_CHANGED :
            // TODO Notifier networld que le node a changer de position
            break;
        case NOTIFICATION_EXIT_TREE :
            if (flecs_entity_id_ != 0) {
                // TODO destroy entity. y a encore des trou dans le lot c'est un peu space
                // net_world->destroy_entity(flecs_entity_id_);
                flecs_entity_id_ = 0;
            }
        default:
            break;
    }
}

void NetEntity::bake() {
    parent_node3d = Object::cast_to<Node3D>(get_parent());
    if (!parent_node3d) {
        UtilityFunctions::printerr("[NetEntity] Erreur : Le parent de NetEntity doit etre un Node3D !");
        return;
    }

    Node* networld_node = get_node_or_null(networld_path);
    auto* net_world = cast_to<NetWorld>(networld_node);

    if (!net_world) {
        UtilityFunctions::printerr("[NetEntity] Erreur : NetWorld introuvable au chemin specifie : ", networld_path);
        return;
    }

    Vector3 initial_pos = parent_node3d->get_global_position();
    net_world->spawn_local_entity(initial_pos, parent_node3d);
}
