//
// Created by leoul on 2026-11-09.
//

#include "NetworldCore.h"
#include "NetComponent.h"
#include <cstdio>
#include <algorithm>
#include "ClientRole.h"
#include "../../thirdparty/flecs/flecs.h"
#include "ServerRole.h"

namespace networld {


    void NetWorldCore::bind_entity_view(uint64_t entity_id, uint64_t host_handle) {
        flecs::entity e = ecs_.entity(entity_id);
        if (e.is_alive() && host_handle != 0) {
            e.set<ExternalView>({host_handle});
        }
    }

    NetWorldCore::NetWorldCore() {
        registry_.register_component(ecs_.component<Position3D>().id(), 10);
        registry_.register_component(ecs_.component<Velocity3D>().id(), 11);
        registry_.register_component(ecs_.component<Networked>(), 12);
        registry_.register_component(ecs_.component<Rotation3D>().id(), 13);

        ecs_.component<Vec3>()
                .member<float>("x")
                .member<float>("y")
                .member<float>("z");

        ecs_.component<Quat>()
            .member<float>("x")
            .member<float>("y")
            .member<float>("z")
            .member<float>("w");

        ecs_.component<NetType>().member<uint32_t>("type_id");
        ecs_.component<NetOwnership>().member<uint32_t>("owner_peer_id");
        ecs_.component<NetEntityID>().member<uint64_t>("server_id");

        ecs_.component<Rotation3D>().member<Quat>("value");
        ecs_.component<Position3D>().member<Vec3>("value");
        ecs_.component<Velocity3D>().member<Vec3>("value");
        ecs_.component<Gravity>().member<Vec3>("force");

        ecs_.set<flecs::Rest>({});

        fixed_sim_phase_ = ecs_.entity("NetFixedSim").add(flecs::Phase);
        sim_pipeline_ = ecs_.pipeline().with(flecs::System).with(fixed_sim_phase_).build();

        // step pour l'interpolation (disable par defaut)
        ecs_.system<InterpolationState>("InterpolateViews")
            .kind(flecs::OnUpdate)
            .each([](flecs::iter& it, size_t /*index*/, InterpolationState& interp) {
                interp.elapsed += it.delta_time();
                float t = interp.duration > 0.0f ?
                    std::clamp(interp.elapsed / interp.duration, 0.0f, 1.0f)
                    : 1.0f;
                interp.visual_pos = Vec3::lerp(interp.prev_pos, interp.target_pos, t);
                interp.visual_rot = Quat::nlerp(interp.prev_rot, interp.target_rot, t);
            });

        // collect view intepolates (un peu pareil que collect view)
        ecs_.system<const InterpolationState, const ExternalView>("CollectInterpolatedViewUpdates")
            .kind(flecs::PostUpdate)
            .each([this](flecs::entity e, const InterpolationState& interp, const ExternalView& v) {
                if (v.host_handle != 0) {
                    view_updates_.push_back({e.id(), v.host_handle, interp.visual_pos, interp.visual_rot});
                }
            });

        ecs_.system<const Position3D, const Rotation3D, const ExternalView>("CollectPredictedViewUpdates")
            .without<InterpolationState>()
            .kind(flecs::PostUpdate)
            .each([this](flecs::entity e, const Position3D& p, const Rotation3D& r, const ExternalView& v) {
                if (v.host_handle != 0) {
                    view_updates_.push_back({e.id(), v.host_handle, p.value, r.value});
                }
            });
    }

    NetWorldCore::~NetWorldCore() {
        stop();
    }

    void NetWorldCore::set_transport(std::unique_ptr<INetTransport> transport) {
        transport_ = std::move(transport);
    }

    bool NetWorldCore::start_host(uint16_t port, HostMode mode) {
        if (!transport_) return false;
        stop();
        packet_dispatcher_ = PacketDispatcher{};
        current_tick_ = 0;
        if (!transport_->open_as_host(port)) return false;
        role_ = std::make_unique<ServerRole>();
        role_->on_start(*this);
        return true;
    }

    bool NetWorldCore::start_client(const std::string &host_ip, uint16_t port) {
        if (!transport_) return false;
        stop();
        packet_dispatcher_ = PacketDispatcher{};
        current_tick_ = 0;
        if (!transport_->open_as_client(host_ip, port)) return false;
        role_ = std::make_unique<ClientRole>();
        role_->on_start(*this);
        return true;
    }

    void NetWorldCore::stop() {
        if (role_) {
            role_->on_stop(*this);
            packet_dispatcher_ = PacketDispatcher{};
            role_.reset();
        }
        if (transport_) {
            transport_->close();
        }

        connected_peers_.clear();
        disconnected_peers_.clear();
    }

    void NetWorldCore::step_simulation(float delta_time) {
        if (!transport_) return;

        // INGRESS : Réseau -> Désérialisation pure -> Application au NetWorld
        transport_->poll();
        NetPacket pkt;
        while (transport_->recv(pkt)) {
            // je check les events du packet pour voir comment traiter le packet
            switch (pkt.event) {
                case NetEvent::CONNECTED :
                    if (role_) role_->on_peer_connected(*this, pkt.sender_peer_id);
                    break;
                case NetEvent::DISCONNECTED :
                    if (role_) role_->on_peer_disconnected(*this,pkt.sender_peer_id);
                    break;
                case NetEvent::DATA :
                    if (!packet_dispatcher_.dispatch(pkt.sender_peer_id, pkt.payload)) {
                        printf("NetWorldCore::step case Data: error packet dispatcher\n");
                    }
                    break;
                default:
                    printf("Bro wtf ?");
                    break;
            }
        }

        // avance sim
        current_tick_++;
        if (role_) role_->on_fixed_tick(*this, delta_time);
        ecs_.run_pipeline(sim_pipeline_, delta_time);

        // EGRESS : Extraction NetWorld -> Sérialisation pure -> Réseau
        if (role_) role_->on_egress(*this, delta_time);
    }

    void NetWorldCore::step_visual(float delta_time) {
        view_updates_.clear();
        ecs_.progress(delta_time);
    }

    uint64_t NetWorldCore::create_local_entity(const SpawnParams &params) {
        const char* name = params.debug_name.empty() ? nullptr : params.debug_name.c_str();
        auto e = ecs_.entity(name)
            .set<Position3D>(Position3D{params.position})
            .set<Rotation3D>(Rotation3D{params.rotation.normalized()})
            .set<Velocity3D>(Velocity3D{Vec3{0,0,0}})
            .set<NetType>(NetType{params.type_id})
            .set<NetOwnership>(NetOwnership{params.owner_peer_id})
            .add<Networked>();

        if (params.host_handle != 0) {
            e.set<ExternalView>({params.host_handle});
        }
        return e.id();
    }

    uint64_t NetWorldCore::spawn_networked_entity(const SpawnParams &params) {
        uint64_t id = create_local_entity(params);
        if (role_) role_->on_entity_spawned(*this, id, params);

        bool is_local = (params.owner_peer_id != PEER_NONE && params.owner_peer_id == get_local_peer_id());
        push_spawned_entity(
    {params.type_id, params.owner_peer_id, id,
            params.position, is_local, params.rotation.normalized()}
        );
        return id;
    }

    void NetWorldCore::despawn_networked_entity(uint64_t entity_id) {
        flecs::entity e = ecs_.entity(entity_id);
        if (!e.is_alive()) return;

        if (role_) role_->on_entity_despawned(*this, entity_id);
        push_despawned_entity(entity_id);
        e.destruct();
    }

    void NetWorldCore::destroy_entity(uint64_t entity_id) {
        flecs::entity e = ecs_.entity(entity_id);
        e.destruct();
    }

    void NetWorldCore::destroy_entities_owned_by(uint32_t peer_id) {
        std::vector<uint64_t> to_destroy;
        auto query = ecs_.query_builder<const NetOwnership>()
            .with<Networked>()
            .build();
        query.each([&](flecs::entity e, const NetOwnership& ownership) {
            if (ownership.owner_peer_id == peer_id) {
                to_destroy.push_back(e.id());
            }
        });
        for (uint64_t id : to_destroy) {
            despawn_networked_entity(id);
        }
    }

    uint64_t NetWorldCore::find_entity(const char *name) const {
        flecs::entity e = ecs_.lookup(name);
        return e.id();

    }

    void NetWorldCore::register_entity_simulator(uint32_t type_id, EntitySimCallback cb) {
        entity_simulators_[type_id] = std::move(cb);
    }

    void NetWorldCore::run_entity_simulator(uint64_t entity_id, const InputPayload &input, const SimContext &ctx) {
        flecs::entity e = ecs_.entity(entity_id);
        if (!e.is_alive()) return;

        const auto* type = e.try_get<NetType>();
        if (!type) return;
        auto it = entity_simulators_.find(type->type_id);
        if (it != entity_simulators_.end() && it->second) {
            it->second(e, input, ctx);
        }
    }

    void NetWorldCore::set_local_input(uint64_t target_local_entity_id, const InputPayload &payload) {
        if (role_) role_->set_local_input(target_local_entity_id, payload);
    }

    void NetWorldCore::set_entity_position(uint64_t entity_id, const Vec3 &pos) {
        flecs::entity e = ecs_.entity(entity_id);
        if (e.is_alive()) {
            e.set<Position3D>({pos});
        }
    }

    void NetWorldCore::set_entity_velocity(uint64_t entity_id, const Vec3 &vel) {
        flecs::entity e = ecs_.entity(entity_id);
        if (e.is_alive()) {
            e.set<Velocity3D>({vel});
        }
    }

    void NetWorldCore::set_entity_rotation(uint64_t entity_id, const Quat &rot) {
        flecs::entity e = ecs_.entity(entity_id);
        if (e.is_alive()) {
            e.set<Rotation3D>({rot.normalized()});
        }
    }

    bool NetWorldCore::set_system_enabled(const char *name, bool enabled) {
        flecs::entity sys = ecs_.lookup(name);
        if (!sys.is_alive()) return false;

        if (enabled) {
            sys.enable();
        } else {
            sys.disable();
        }
        return true;
    }

    uint32_t NetWorldCore::get_local_peer_id() const {
        return role_ ? role_->get_local_peer_id() : PEER_NONE;
    }
} // networld