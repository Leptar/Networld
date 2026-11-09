//
// Created by leoul on 2026-11-03.
//

#ifndef NETWORLD_NETENTITY_H
#define NETWORLD_NETENTITY_H
#pragma once
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include "../NetWorld.h"

namespace godot {

    class NetEntity : public Node {
        GDCLASS(NetEntity, Node);

    private:
        // Pointeur vers le parent visuel
        Node3D* parent_node3d = nullptr;

        // Le chemin (NodePath) vers le singleton NetWorld dans l'arbre de scène Godot
        NodePath networld_path;

    protected:
        static void _bind_methods();

    public:
        NetEntity();
        ~NetEntity();

        void set_networld_path(const NodePath& path);
        NodePath get_networld_path() const;

        void _ready() override;
        void bake();
    };

} // namespace godot

#endif //NETWORLD_NETENTITY_H
