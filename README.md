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
   - Ninja (clang/MinGW): configure into a clean folder, e.g., `cmake -G Ninja -B build-ninja -S .`
     * CMake already appends `C:\ProgramData\chocolatey\bin` to `CMAKE_PROGRAM_PATH`, so Ninja and Chocolatey LLVM tools are discoverable without extra `PATH` edits.
     * Ninja is a single-config generator. When no multi-config generator has been used, the project defaults `CMAKE_BUILD_TYPE=Release`; override it with `-DCMAKE_BUILD_TYPE=Debug` if you need another configuration in that folder.
   - Ninja with MSVC: open a Developer Command Prompt (call `"%VSINSTALLDIR%VC\\Auxiliary\\Build\\vcvarsall.bat" x64` or run that batch file from PowerShell) and configure with
     ```
     cmake -G Ninja -B build-ninja-msvc -S . -DCMAKE_BUILD_TYPE=Release
     ```
      Keep using the same prompt (so `cl.exe` carries the SDK/CRT paths) and build with `cmake --build build-ninja-msvc --config Release`.
      * Quick one-liner that initializes the MSVC environment and runs the Release build from a regular shell:
        ```
        cmd /c "call \"%VSINSTALLDIR%VC\Auxiliary\Build\vcvarsall.bat\" x64 && cmake --build build-ninja-msvc --config Release"
        ```
   - Optional clang-tidy: add `-DENABLE_CLANG_TIDY=ON`
     * CMake searches both `C:\Program Files\LLVM\bin` and `C:\ProgramData\chocolatey\bin`, so standard LLVM/Chocolatey installs are found automatically.
     * The wrapped clang-tidy binary prints `[clang-tidy] starting .` and `[clang-tidy] finished (exit .)` markers so you can spot linting in verbose logs even when no diagnostics appear.

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

1. `main.cpp` now uses a JSON-driven entrypoint.
   - Pass `sdl3_app --json-file-in <path>` to load runtime settings such as the Lua script path, window size, and `lua_debug`.
   - The loader enforces valid types and resolves relative scripts next to the configuration file.
2. Generate a starter JSON with the shipped script locations:
   ```
   sdl3_app --create-seed-json config/seed_runtime.json
   ```
   The helper assumes `scripts/cube_logic.lua` lives beside the executable and writes normalized paths.
3. Store or override the app-wide config with `sdl3_app --set-default-json [path]`.
   - Windows uses `%APPDATA%/sdl3cpp`; other platforms use `$XDG_CONFIG_HOME/sdl3cpp/default_runtime.json`, falling back to `~/.config/sdl3cpp`.
   - Providing a `path` duplicates that JSON into the platform default without editing the runtime values.
   - When the default file exists the executable reads it automatically; otherwise it falls back to the built-in script discovery logic.

### GUI Demo
`scripts/gui_demo.lua` shows off the Lua GUI framework (buttons, text boxes, list views, and SVG icons) rendered on top of the Vulkan scene. Launch it with `./build/sdl3_app --json-file-in config/gui_runtime.json` (or use that config via `sdl3_app --set-default-json`) to run the interactive overlay in the window.

## Dependency automation

This project ships a `renovate.json` configuration so Renovate can open PRs that bump the Conan packages listed in `conanfile.py`. Either install Renovate locally (`npm install -g renovate` or `npx renovate`) and run it from the repo root, or enable the Renovate bot on your GitHub/GitLab install to pick up the configuration automatically.
