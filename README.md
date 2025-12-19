# SDL3CPlusPlus
A minimal SDL3 + Vulkan spinning cube demo.

## Build

1. Install or update [Conan 2.x](https://docs.conan.io/en/latest/installation.html) and run
   ```
   conan profile detect
   conan install . -of build -b missing -s compiler=msvc -s compiler.version=194 -s compiler.cppstd=17 -c tools.cmake.cmaketoolchain:generator="Visual Studio 17 2022"
   ```
   so that Conan brings in `lua`, SDL3, and the Vulkan loader + headers.
2. Configure the project with CMake using the generated Conan toolchain:
   ```
   cmake -B build -S .
   ```
3. Build the demo:
   ```
   cmake --build build --config Release --target sdl3_app
   ```

Shaders are copied into `build/shaders` during configuration, so the demo can load the precompiled `cube.{vert,frag}.spv`.

## Run

```
cmake --build build --target spinning_cube
./build/spinning_cube
```

If you need the Conan runtime environment (e.g., because dependencies set env vars), source `build/conanrun.sh` before launching the binary on Linux/macOS or run `build\\conanrun.bat` on Windows.

## Runtime configuration

`main.cpp` now uses a JSON-driven entrypoint. Use `sdl3_app --json-file-in <path>` to load a configuration that points at the Lua script and captures window dimensions, or run `sdl3_app --create-seed-json config/seed_runtime.json` to write a starter JSON file (based on the executable’s `scripts/cube_logic.lua` location). You can also use `sdl3_app --set-default-json` (optionally followed by an existing JSON path) to copy the runtime JSON to the platform default directory (APPDATA on Windows, `XDG_CONFIG_HOME`/`$HOME/.config` elsewhere); when that default file exists, the app picks it up automatically when launched without extra CLI options. If no JSON input is provided and no default exists, the app falls back to discovering `scripts/cube_logic.lua` next to the binary.

### GUI Demo
`scripts/gui_demo.lua` shows off the Lua GUI framework (buttons, text boxes, list views, and SVG icons) rendered on top of the Vulkan scene. Launch it with `./build/sdl3_app --json-file-in config/gui_runtime.json` (or use the new config as input to `sdl3_app --set-default-json`) to run the interactive overlay in the window.

## Dependency automation

This project ships a `renovate.json` configuration so Renovate can open PRs that bump the Conan packages listed in `conanfile.py`. Either install Renovate locally (`npm install -g renovate` or `npx renovate`) and run it from the repo root, or enable the Renovate bot on your GitHub/GitLab install to pick up the configuration automatically.
