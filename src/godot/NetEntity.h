//
// Created by leoul on 2026-11-03.
//

#ifndef NETWORLD_NETENTITY_H
#define NETWORLD_NETENTITY_H
#pragma once
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>

#include "Networld.h"

namespace godot {

    class NetEntity : public Node {
        GDCLASS(NetEntity, Node);

    private:
        // Pointeur vers le parent visuel
        Node3D* parent_node3d = nullptr;

        uint64_t flecs_entity_id_ = 0;

        // Le chemin (NodePath) vers le singleton NetWorld dans l'arbre de scène Godot
        NetWorld* networld;

    protected:
        static void _bind_methods();
        void _notification(int p_what);

    public:
        NetEntity();
        ~NetEntity();

        NetWorld* get_networld() const;

        void bake();
    };

} // namespace godot

#endif //NETWORLD_NETENTITY_H
