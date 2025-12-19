# SDL3CPlusPlus
Minimal SDL3 + Vulkan spinning cube demo.

## Cheat sheet

### Dependencies
- python scripts/dev_commands.py dependencies installs the Conan graph in uild.

### Configure & build
- python scripts/dev_commands.py configure defaults to Ninja+MSVC on Windows or Ninja on other platforms, writing into the matching uild-ninja-msvc/uild-ninja folder with the Release type; override the generator or build dir with --generator / --build-dir if needed.
- python scripts/dev_commands.py build runs cmake --build in the same folder (change --build-dir to match a different configure directory).
- python scripts/dev_commands.py msvc-quick (Windows only) runs the VC vars + Ninja build alias; pass --bat-path to target a different Visual Studio installation.
- python scripts/dev_commands.py run launches spinning_cube from the configured Ninja output.
- Prefix any subcommand with --dry-run to print the alias-driven shell command instead of executing it.

### Run
- python scripts/dev_commands.py run [--build-dir ...] (source uild/conanrun.sh / uild\\conanrun.bat first if Conan exports env vars).

## Runtime configuration
1. sdl3_app --json-file-in <path> loads JSON configs (script path, window size, lua_debug, etc.).
2. sdl3_app --create-seed-json config/seed_runtime.json writes a starter file assuming scripts/cube_logic.lua sits beside the binary.
3. sdl3_app --set-default-json [path] stores or overrides the runtime JSON; Windows writes %APPDATA%/sdl3cpp, other OSes use $XDG_CONFIG_HOME/sdl3cpp/default_runtime.json (fallback ~/.config/sdl3cpp).

### GUI Demo
scripts/gui_demo.lua paints the Lua GUI framework on top of the Vulkan scene. Launch it as ./build/sdl3_app --json-file-in config/gui_runtime.json or register that config via sdl3_app --set-default-json.
