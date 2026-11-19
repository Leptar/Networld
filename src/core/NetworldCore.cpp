//
// Created by leoul on 2026-11-09.
//

#include "NetworldCore.h"
#include "NetComponent.h"
#include <cstdio>
#include <cmath>

#include "ClientRole.h"
#include "../../thirdparty/flecs/flecs.h"
#include "NetMessage.h"
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

        ecs_.component<Vec3>()
                .member<float>("x")
                .member<float>("y")
                .member<float>("z");

        ecs_.component<Position3D>().member<Vec3>("value");
        ecs_.component<Velocity3D>().member<Vec3>("value");
        ecs_.component<Gravity>().member<Vec3>("force");

        ecs_.set<flecs::Rest>({});
        // Système 1 : Appliquer la gravité (modifie la vélocité)
        ecs_.system<Velocity3D, const Gravity>("ApplyGravity")
            .each([](flecs::iter& it, size_t index, Velocity3D& v, const Gravity& g) {
                v.value += g.force * it.delta_time();
            });

        // Système 2 : Appliquer le mouvement (modifie la position)
        ecs_.system<Position3D, const Velocity3D>("ApplyMovement")
            .each([](flecs::iter& it, size_t index, Position3D& p, const Velocity3D& v) {
                p.value += v.value * it.delta_time();
            });

        // Systeme 3 change ducoup
        ecs_.system<const Position3D, const ExternalView>("CollectViewUpdates")
            .each([this](flecs::entity e, const Position3D& p, const ExternalView& v) {
                if (v.host_handle != 0) {
                    // On enregistre la mise à jour dans le tampon de sortie
                    view_updates_.push_back({e.id(), v.host_handle, p.value});
                }
            });
    }

    NetWorldCore::~NetWorldCore() {
        stop();
    }

    void NetWorldCore::set_transport(std::unique_ptr<INetTransport> transport) {
        transport_ = std::move(transport);
    }

    bool NetWorldCore::start_host(uint16_t port) {
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
    }

    void NetWorldCore::step(float delta_time) {
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

        // SIMULATION ECS
        view_updates_.clear();
        current_tick_++;
        ecs_.progress(delta_time);

        // EGRESS : Extraction NetWorld -> Sérialisation pure -> Réseau
        if (role_) role_->on_egress(*this, delta_time);
    }

    uint64_t NetWorldCore::create_networked_entity(const Vec3 &initial_pos, uint64_t host_handle, const char *name) {
        auto e = ecs_.entity(name)
            .set<Position3D>({initial_pos})
            .set<Velocity3D>({{0, 0, 0}})
            .add<Networked>();
        if (host_handle != 0) {
            e.set<ExternalView>({host_handle});
        }
        return e.id();
    }

    void NetWorldCore::destroy_entity(uint64_t entity_id) {
        flecs::entity e = ecs_.entity(entity_id);
        e.destruct();
    }

    uint64_t NetWorldCore::find_entity(const char *name) const {
        flecs::entity e = ecs_.lookup(name);
        return e.id();

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

    void NetWorldCore::set_simulation_systems_enabled(bool enabled) {
        if (enabled) {
            ecs_.lookup("ApplyGravity").enable();
            ecs_.lookup("ApplyMovement").enable();
        } else {
            ecs_.lookup("ApplyGravity").disable();
            ecs_.lookup("ApplyMovement").disable();
        }

    }
} // networld