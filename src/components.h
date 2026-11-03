//
// Created by leoul on 2026-10-28.
//
#pragma once

#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/classes/node3d.hpp>

namespace godot {

    // Composants
    struct Position3D { Vector3 value; };
    struct Velocity3D { Vector3 value; };
    struct Gravity    { Vector3 force; };

    // simple tag pour savoir ce qui doit etre envoyer
    struct Networked {};

    // Composant pont pour la synchronisation visuelle
    struct GodotView  { Node3D* node = nullptr; };

}