//
// Created by leoul on 2026-11-02.
//

#ifndef NETWORLD_NETWORLD_H
#define NETWORLD_NETWORLD_H

#pragma once
#include <godot_cpp/classes/node.hpp>
#include <memory>
#include "flecs.h"
#include "NetRegistry.h"
#include "NetworldCore.h"
#include "../core/INetTransport.h"

namespace godot {
    class Node3D;

    class NetWorld : public Node {
        GDCLASS(NetWorld, Node);

        static NetWorld *networld_;

        networld::NetWorldCore core_;

    protected:
        static void _bind_methods();

    public:
        static NetWorld *get_networld();
        NetWorld();
        ~NetWorld();

        void host(int port);
        void join(String ip, int port);

        uint64_t spawn_local_entity(Vector3 pos, Node3D* visual_node);
        void bind_view_to_server_player(Node3D* visual_node);

        void _process(double delta) override;
    };

} // namespace godot

#endif //NETWORLD_NETWORLD_H
