//
// Created by leoul on 2026-11-02.
//

#ifndef NETWORLD_NETWORLD_H
#define NETWORLD_NETWORLD_H

#pragma once
#include <functional>
#include <godot_cpp/classes/node.hpp>
#include <memory>
#include "flecs.h"
#include "NetRegistry.h"
#include "NetworldCore.h"
#include "../core/INetTransport.h"
#include "godot_cpp/classes/node3d.hpp"

namespace godot {

    class NetWorld : public Node {
        GDCLASS(NetWorld, Node);

    protected:
        static void _bind_methods();

    public:
        // singleton
        static NetWorld *get_networld();
        NetWorld();
        ~NetWorld();

        //register transport
        using TransportFactory = std::function<std::unique_ptr<INetTransport>()>;
        static void register_transport(const String& name, TransportFactory factory);

        void host(int port);
        void join(String ip, int port);

        uint64_t spawn_local_entity(Vector3 pos, Node3D* visual_node);
        void set_entity_velocity(uint64_t entity_id, const Vector3& velocity);
        void set_entity_position(uint64_t entity_id, const Vector3& pos);
        void set_client_input(const Vector3& input);

        void bind_entity_view(uint64_t id, Node3D* visual_node);

        void _process(double delta) override;
    private:
        // singleton et core
        static NetWorld *networld_;
        networld::NetWorldCore core_;

        // Transport settings setup
        struct TransportEntry {
            String name;
            TransportFactory factory;
        };

        static std::vector<TransportEntry> transport_registry_;
        String current_transport_name_ = "Godot UDP";
        void _on_settings_changed();
        std::unique_ptr<INetTransport> create_transport() const;
    };

} // namespace godot

#endif //NETWORLD_NETWORLD_H
