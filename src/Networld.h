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
#include "INetTransport.h"

namespace godot {

    class NetWorld : public Node {
        GDCLASS(NetWorld, Node);

    private:
        flecs::world ecs;
        NetRegistry registry;
        std::unique_ptr<INetTransport> transport;
        bool is_server = false;

    protected:
        static void _bind_methods();

    public:
        NetWorld();
        ~NetWorld();

        void host(int port);
        void join(String ip, int port);

        void _process(double delta) override;
    };

} // namespace godot

#endif //NETWORLD_NETWORLD_H
