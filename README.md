# SDL3CPlusPlus
Minimal SDL3 + Vulkan spinning cube demo.

## Cheat sheet

### Dependencies
- `python scripts/dev_commands.py dependencies` installs the Conan graph in `build`.

### Configure & build
- `python scripts/dev_commands.py configure` runs Ninja with the default `build-ninja` directory and `Release` build type; add `--generator vs` for the Visual Studio generator or `--generator ninja-msvc` when you want the MSVC-aware Ninja folder.
- `python scripts/dev_commands.py build` targets `build-ninja` by default (change `--build-dir` if you configured elsewhere).
- `python scripts/dev_commands.py msvc-quick` invokes the VC vars + Ninja one-liner (pass `--bat-path` to point at another Visual Studio layout).
- Combine `--dry-run` with any subcommand to inspect the alias-driven shell invocation without executing it.
 - `python scripts/dev_commands.py run` launches `spinning_cube` from the configured Ninja output; pass `--build-dir` when you configured elsewhere.

### Run
- `python scripts/dev_commands.py run [--build-dir ...]` to launch `spinning_cube`; source `build/conanrun.sh` / `build\\conanrun.bat` if Conan exports env vars before running the helper.

## Runtime configuration
1. sdl3_app --json-file-in <path> loads JSON configs (script path, window size, lua_debug, etc.).
2. sdl3_app --create-seed-json config/seed_runtime.json writes a starter file assuming scripts/cube_logic.lua sits beside the binary.
3. sdl3_app --set-default-json [path] stores or overrides the default runtime JSON; Windows writes %APPDATA%/sdl3cpp, other OSes use $XDG_CONFIG_HOME/sdl3cpp/default_runtime.json (fallback ~/.config/sdl3cpp).

### GUI Demo
scripts/gui_demo.lua paints the Lua GUI framework on top of the Vulkan scene. Launch it as ./build/sdl3_app --json-file-in config/gui_runtime.json or register that config via sdl3_app --set-default-json.
