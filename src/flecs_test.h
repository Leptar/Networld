#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "flecs.h"
#include "components.cpp"
#include "godot_cpp/classes/engine.hpp"

namespace godot {

class FlecsManager3D : public Node3D {
    GDCLASS(FlecsManager3D, Node3D)
private:
    flecs::world ecs;

protected:
    static void _bind_methods() {}

public:
    FlecsManager3D() {
        ecs.component<Vector3>()
            .member<float>("x")
            .member<float>("y")
            .member<float>("z");

        ecs.component<Position3D>().member<Vector3>("value");
        ecs.component<Velocity3D>().member<Vector3>("value");
        ecs.component<Gravity>().member<Vector3>("force");

        ecs.set<flecs::Rest>({});
        // Système 1 : Appliquer la gravité (modifie la vélocité)
        ecs.system<Velocity3D, const Gravity>("ApplyGravity")
            .each([](flecs::iter& it, size_t index, Velocity3D& v, const Gravity& g) {
                v.value += g.force * it.delta_time();
            });

        // Système 2 : Appliquer le mouvement (modifie la position)
        ecs.system<Position3D, const Velocity3D>("ApplyMovement")
            .each([](flecs::iter& it, size_t index, Position3D& p, const Velocity3D& v) {
                p.value += v.value * it.delta_time();
            });

        // Système 3 : Synchroniser les données ECS vers le Scene Tree Godot
        ecs.system<const Position3D, GodotView>("SyncGodotView")
            .each([](const Position3D& p, GodotView& view) {
                if (view.node != nullptr) {
                    view.node->set_position(p.value);
                }
            });
    }

    void _ready() override {
        if (Engine::get_singleton()->is_editor_hint()) return; // Ne pas exécuter dans l'éditeur

        MeshInstance3D* ball_mesh = memnew(MeshInstance3D);
        Ref<SphereMesh> sphere;
        sphere.instantiate();
        ball_mesh->set_mesh(sphere);

        add_child(ball_mesh);

        ecs.entity("FallingBall")
            .set<Position3D>({Vector3(0, 10.0f, 0)})
            .set<Velocity3D>({Vector3(0, 0, 0)})
            .set<Gravity>({Vector3(0, -9.81f, 0)})
            .set<GodotView>({ball_mesh});            \
    }

    void _process(double delta) override {
        if (Engine::get_singleton()->is_editor_hint()) return;

        ecs.progress(delta);
    }
};

} // namespace godot