//
// Created by leoul on 2026-11-03.
//

#include "NetEntity.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "Networld.h"
#include "godot_cpp/classes/canvas_item.hpp"

using namespace godot;

void NetEntity::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_networld"), &NetEntity::get_networld);
    ClassDB::bind_method(D_METHOD("bake"), &NetEntity::bake);
}

NetEntity::NetEntity() {
    if (NetWorld* net_world = NetWorld::get_networld()) {
        networld = net_world;
    }
}

NetEntity::~NetEntity() {
    networld = nullptr;
}

NetWorld* NetEntity::get_networld() const {
    return networld;
}

void NetEntity::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_READY :
            if (Engine::get_singleton()->is_editor_hint()) {
                return;
            }
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
    parent_node3d = cast_to<Node3D>(get_parent());
    if (!parent_node3d) {
        UtilityFunctions::printerr("[NetEntity] Erreur : Le parent de NetEntity doit etre un Node3D !");
        return;
    }

    if (!networld) {
        UtilityFunctions::printerr("[NetEntity] Erreur : NetWorld est nullptr");
        return;
    }

    Vector3 initial_pos = parent_node3d->get_global_position();
    networld->spawn_local_entity(initial_pos, parent_node3d);
}
