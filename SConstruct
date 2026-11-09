#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

VariantDir("build/src", "src", duplicate=0)
VariantDir("build/thirdparty", "thirdparty", duplicate=0)

env.Append(CPPPATH=["src/*"])
sources = Glob("build/src/*.cpp")

env.Append(CPPPATH=["thirdparty/flecs"])
sources.append("build/thirdparty/flecs/flecs.c")

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "demo/bin/Networld.{}.{}.framework/Networld.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
elif env["platform"] == "ios":
    if env["ios_simulator"]:
        library = env.StaticLibrary(
            "demo/bin/Networld.{}.{}.simulator.a".format(env["platform"], env["target"]),
            source=sources,
        )
    else:
        library = env.StaticLibrary(
            "demo/bin/Networld.{}.{}.a".format(env["platform"], env["target"]),
            source=sources,
        )
else:
    library = env.SharedLibrary(
        "demo/bin/Networld{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

if env["platform"] == "windows":
    env.Append(LIBS=["ws2_32"])

Default(library)