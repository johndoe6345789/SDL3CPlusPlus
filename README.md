# GameEngine

SDL3 GPU 2D/3D game engine with a JSON-driven workflow system. Built with C++20, using Conan for dependency management and CMake + Ninja for builds.

## Quick Start

### Prerequisites

- C++20 compiler (MSVC, Clang, or GCC)
- [CMake](https://cmake.org/) 3.24+
- [Conan](https://conan.io/) 2.x
- [Ninja](https://ninja-build.org/) (recommended)
- Python 3.9+ (build helper)

### Build & Run (Recommended)

The `dev_commands.py` build helper runs the full pipeline: Conan install, CMake generate, configure, and build.

```bash
# Build and run the seed demo (auto-detects platform bootstrap)
python python/dev_commands.py all --run

# Equivalent explicit form on Windows
python python/dev_commands.py all --run --bootstrap bootstrap_windows --game seed

# macOS
python python/dev_commands.py all --run --bootstrap bootstrap_mac --game seed

# Linux
python python/dev_commands.py all --run --bootstrap bootstrap_linux --game seed
```

### Build Steps (Manual)

```bash
# 1. Install Conan dependencies
python python/dev_commands.py dependencies

# 2. Generate CMakeLists.txt from cmake_config.json
python python/dev_commands.py generate

# 3. Configure CMake
python python/dev_commands.py configure --preset conan-default

# 4. Build
python python/dev_commands.py build

# 5. Run
python python/dev_commands.py run -- --bootstrap bootstrap_windows --game seed
```

### Makefile (Unix)

```bash
make build                              # Build sdl3_app
make build TARGET=sdl3_app              # Build specific target
make test                               # Build and run all tests
make rebuild                            # Clean + build
make list-targets                       # Show available targets
```

## CLI Arguments

The compiled `sdl3_app` binary accepts:

| Flag | Default | Description |
|------|---------|-------------|
| `--bootstrap` | `bootstrap_mac` | Platform bootstrap package |
| `--game` | `standalone_cubes` | Game package to load |
| `--project-root` | Current directory | Path to gameengine root |

## Packages

Game content and platform initialization are defined as JSON packages in `packages/`:

| Package | Type | Description |
|---------|------|-------------|
| `bootstrap_windows` | bootloader | Windows init, SDL3 GPU with D3D12 |
| `bootstrap_mac` | bootloader | macOS init, SDL3 GPU with Metal |
| `bootstrap_linux` | bootloader | Linux init, SDL3 GPU with Vulkan |
| `seed` | game | FPS demo: room with spinning cube, WASD movement, mouse look, jump. Bullet3 physics |
| `standalone_cubes` | game | 11x11 grid of spinning colored cubes with per-cube animation offsets |
| `quake3` | game | Quake 3 BSP map viewer with FPS controls |
| `soundboard` | game | Audio cues + GUI controls |
| `engine_tester` | game | Validation tour with teleport checkpoints, captures, and diagnostics |
| `asset_loader` | library | Universal asset loading with cross-engine unit conversion (JSON-driven) |
| `assets` | library | Shared runtime assets (audio, fonts, images) |
| `materialx` | library | MaterialX integration |

## Architecture

```
src/
  main.cpp                          # Entry point, CLI parsing, workflow bootstrap
  services/
    interfaces/                     # Abstract service interfaces (16 services)
      i_audio_service.hpp
      i_graphics_service.hpp
      i_input_service.hpp
      i_physics_service.hpp
      i_scene_service.hpp
      i_window_service.hpp
      i_workflow_executor.hpp
      ...
    impl/                           # Concrete implementations
      workflow/                     # Workflow step implementations
      diagnostics/                  # Logging
      app/                          # CLI, platform services

packages/                           # JSON-driven game content
  {package}/
    package.json                    # Package metadata, config, shader list
    workflows/*.json                # Workflow definitions (v2.2.0 format)
    shaders/spirv/                  # SPIR-V shaders (Vulkan, D3D12)
    shaders/msl/                    # Metal shaders (macOS)
    scene/*.json                    # Scene definitions
    assets/                         # Package-specific assets

python/
  dev_commands.py                   # Build helper CLI
  generate_cmake.py                 # CMakeLists.txt generator from cmake_config.json
  export_room_gltf.py              # Seed demo room glTF exporter
```

## Dependencies (Conan)

| Library | Version | Purpose |
|---------|---------|---------|
| SDL | 3.2.20 | Windowing, input, GPU API |
| Bullet3 | 3.25 | 3D physics |
| Box2D | 3.1.1 | 2D physics |
| EnTT | 3.16.0 | Entity Component System |
| Assimp | 6.0.2 | 3D model import |
| glm | 1.0.1 | Math library |
| nlohmann_json | 3.11.3 | JSON parsing |
| RapidJSON | cci.20230929 | JSON parsing (performance) |
| FFmpeg | 8.0.1 | Audio/video decoding |
| FreeType | 2.13.2 | Font rendering |
| Cairo | 1.18.0 | 2D vector graphics |
| stb | cci.20230920 | Image loading |
| LunaSVG | 3.0.1 | SVG rendering |
| libzip | 1.10.1 | ZIP archive support |
| cpptrace | 1.0.4 | Stack traces |
| CLI11 | 2.6.0 | Command-line parsing |
| GTest | 1.17.0 | Testing framework |

## Testing

```bash
# Via Makefile
make test

# Via dev_commands.py
python python/dev_commands.py build --target test_exit_step
```
