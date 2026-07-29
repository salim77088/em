# NEXUS Engine

A modern, lightweight, cross-platform game engine built on top of [bgfx](https://github.com/bkaradzic/bgfx) (rendering), [SDL2](https://www.libsdl.org/) (platform layer), [EnTT](https://github.com/skypjack/entt) (ECS), and [Dear ImGui](https://github.com/ocornut/imgui) (editor UI).

## Features

- **Modern rendering** via bgfx — supports Vulkan, Metal, DirectX 11, and OpenGL backends automatically
- **Cross-platform** — Windows, Linux, macOS
- **Custom scripting language** — EZScript, a simple Python-like language with a stack-based VM
- **Node-based visual scripting** — NodeFlow, a complete node editor for visual logic
- **Entity Component System** — Built on EnTT for high-performance scene management
- **Scene serialization** — JSON-based save/load
- **Asset pipeline** — Texture (PNG/JPG/BMP/TGA) and mesh (OBJ) loading with caching
- **Editor UI** — ImGui-based editor with Hierarchy, Inspector, Viewport, Console, Node Editor, and Asset Browser
- **Audio** — miniaudio for cross-platform audio playback

## Building

### Requirements

- CMake 3.16+
- C++17 compiler (MSVC 2019+, GCC 9+, Clang 10+)

### Build Instructions

```bash
git clone https://github.com/salim77088/em.git
cd em
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
```

### Platform-Specific Notes

- **Windows**: Requires Windows 10+ and Visual Studio 2019+ Build Tools
- **Linux**: Install `libgl1`, `libx11-dev`, `libwayland-dev`, `libxkbcommon-dev`
- **macOS**: Requires macOS 11.0+, Xcode 13+

## Architecture

```
src/
├── core/           Engine, Window, Logger
├── math/           GLM-based math utilities
├── graphics/       bgfx renderer, Mesh, Shader, Texture, Camera
├── scene/          Scene, Entity, Components (Tag, Transform, MeshRenderer, Light, Camera3D)
├── scripting/      EZScript: Lexer, Parser, VM
├── nodes/          NodeFlow visual scripting graph
├── editor/         ImGui-based editor (Hierarchy, Inspector, Viewport, Console, Node Editor)
├── audio/          miniaudio wrapper
├── input/          Input manager
└── assets/         Asset manager (textures, meshes, files)
```

## EZScript Language

A simple, easy-to-learn scripting language:

```ezscript
// Variables
var name = "Player"
var health = 100
let pi = 3.14159

// Functions
func add(a, b) {
    return a + b
}

// Control flow
if health > 50 {
    print("Healthy")
} else {
    print("Low health")
}

// Loops
for i in 10 {
    print("Counter:", i)
}

// Data structures
var arr = [1, 2, 3]
var obj = { name: "Hero", level: 1 }
```

## Node Editor

The built-in node editor (`NodeFlow`) supports:
- Visual scripting with input/output pins
- Custom node types with properties
- Link management with type checking
- JSON save/load for node graphs

## License

MIT License — see LICENSE file for details.

## Acknowledgments

- [bgfx](https://github.com/bkaradzic/bgfx) — cross-platform rendering library (BSD-2-Clause)
- [SDL2](https://www.libsdl.org/) — Simple DirectMedia Layer (zlib)
- [EnTT](https://github.com/skypjack/entt) — ECS for modern C++ (MIT)
- [Dear ImGui](https://github.com/ocornut/imgui) — Immediate-mode GUI (MIT)
- [nlohmann/json](https://github.com/nlohmann/json) — JSON for Modern C++ (MIT)
- [glm](https://github.com/g-truc/glm) — OpenGL Mathematics (MIT)
- [stb](https://github.com/nothings/stb) — single-file libraries (MIT/Public Domain)
- [miniaudio](https://github.com/mackron/miniaudio) — single-file audio playback library (MIT)
