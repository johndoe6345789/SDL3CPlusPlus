# SDL3CPlusPlus
A minimal SDL3 + Vulkan spinning cube demo.

## Build

1. **Dependencies**
   - Install or update [Conan 2.x](https://docs.conan.io/en/latest/installation.html).
   - Run:
     ```
     conan profile detect
     conan install . -of build -b missing -s compiler=msvc -s compiler.version=194 -s compiler.cppstd=17 -c tools.cmake.cmaketoolchain:generator="Visual Studio 17 2022"
     ```
     so Conan pulls in `lua`, SDL3, Vulkan, and the rest of the graph defined in `conanfile.py`.

2. **Configure**
   - Default (Visual Studio): `cmake -B build -S .`
     * This uses the Visual Studio 2022 generator.
   - Ninja: configure into a clean folder, e.g., `cmake -G Ninja -B build-ninja -S .`
     * CMake already appends `C:\ProgramData\chocolatey\bin` to `CMAKE_PROGRAM_PATH`, so Ninja and Chocolatey LLVM tools are discoverable without extra `PATH` edits.
     * Ninja is a single-config generator. When no multi-config generator has been used, the project defaults `CMAKE_BUILD_TYPE=Release`; override it with `-DCMAKE_BUILD_TYPE=Debug` if you need another configuration in that folder.
   - Optional clang-tidy: add `-DENABLE_CLANG_TIDY=ON`
     * CMake searches both `C:\Program Files\LLVM\bin` and `C:\ProgramData\chocolatey\bin`, so standard LLVM/Chocolatey installs are found automatically.
     * The wrapped clang-tidy binary prints `[clang-tidy] starting …` and `[clang-tidy] finished (exit …)` markers so you can spot linting in verbose logs even when no diagnostics appear.

3. **Build**
   - `cmake --build build --config Release --target sdl3_app` (or point at `build-ninja` if you used Ninja).
   - Shaders land in `build/shaders` or `build-ninja/shaders`, so the executable can load `cube.{vert,frag}.spv`.

## Run

```
cmake --build build --target spinning_cube
./build/spinning_cube
```

If you rely on the Conan runtime environment (some dependencies export env vars), source `build/conanrun.sh` on Linux/macOS or run `build\\conanrun.bat` on Windows before launching the binary.

## Runtime configuration

1. `main.cpp` now uses a JSON-driven entrypoint. Run `sdl3_app --json-file-in <path>` to load a config that points at the Lua script and defines window dimensions.
2. Generate a starter JSON peppered with the default script locations via `sdl3_app --create-seed-json config/seed_runtime.json` (it assumes `scripts/cube_logic.lua` is next to the executable).
3. Copy a JSON into the platform default by running `sdl3_app --set-default-json [path]` (APPDATA on Windows, `XDG_CONFIG_HOME`/`~/.config` elsewhere). When the default file exists, the app picks it up automatically; otherwise it discovers `scripts/cube_logic.lua` next to the binary.

### GUI Demo
`scripts/gui_demo.lua` shows off the Lua GUI framework (buttons, text boxes, list views, and SVG icons) rendered on top of the Vulkan scene. Launch it with `./build/sdl3_app --json-file-in config/gui_runtime.json` (or use that config via `sdl3_app --set-default-json`) to run the interactive overlay in the window.

## Dependency automation

This project ships a `renovate.json` configuration so Renovate can open PRs that bump the Conan packages listed in `conanfile.py`. Either install Renovate locally (`npm install -g renovate` or `npx renovate`) and run it from the repo root, or enable the Renovate bot on your GitHub/GitLab install to pick up the configuration automatically.
