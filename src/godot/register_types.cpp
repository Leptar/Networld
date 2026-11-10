#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "../flecs_test.h"
#include "NetEntity.h"
#include "NetWorld.h"

using namespace godot;

void initialize_networld_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    GDREGISTER_RUNTIME_CLASS(FlecsManager3D)
    ClassDB::register_class<NetWorld>();
    ClassDB::register_class<NetEntity>();
}

void uninitialize_networld_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

extern "C" {
// Initialization.
GDExtensionBool GDE_EXPORT Networld(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_networld_module);
    init_obj.register_terminator(uninitialize_networld_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}
}