# SDL3CPlusPlus
SDL3 + bgfx demo app with Lua-driven runtime configuration, audio playback, and a small GUI sample.

## Overview
- Renders a spinning cube with bgfx (Vulkan renderer by default) via SDL3.
- Lua scripts control behavior under `scripts/`.
- JSON runtime configs live in `config/`.
- Optional helpers for audio asset generation and workflow diagnostics.

## Prerequisites
- Python 3 for helper scripts.
- Conan for dependency resolution.
- CMake and a C++ compiler.
- Ninja (default generator) or Visual Studio 2022 on Windows.
- Vulkan runtime/driver for the default bgfx renderer (or set `bgfx.renderer` in config).

## Quick start
1. `python scripts/dev_commands.py dependencies`
2. `python scripts/dev_commands.py configure`
3. `python scripts/dev_commands.py build`
4. If you used Conan, load the runtime env vars:
   - Linux/macOS: `source build/conanrun.sh`
   - Windows (cmd.exe): `build\conanrun.bat`
5. `python scripts/dev_commands.py run`
6. Use the guardrail prefix helper before invoking Codex if you want to prepend standard instructions: `python scripts/auto_prompt_prefixer.py --prompt "Describe the package layout"`. It defaults to `config/prompt_prefix.txt`, or pass `--prefix-file`/`--prefix-text` to change it.

## Build helper commands
- `python scripts/dev_commands.py configure` uses Ninja by default (Ninja+MSVC on Windows) and writes to `build-ninja`/`build-ninja-msvc`; override with `--generator` and `--build-dir`.
- `python scripts/dev_commands.py build` runs `cmake --build` for that folder (use `--build-dir` and `--target` as needed).
- `python scripts/dev_commands.py msvc-quick` runs `vcvarsall.bat` plus a build command (Windows only).
- Prefix any subcommand with `--dry-run` to print the shell command without executing it.

## Running
- Default build dir: `build-ninja` on Linux/macOS, `build-ninja-msvc` on Windows.
- `python scripts/dev_commands.py run --build-dir <path>` to run from a custom build directory.
- `python scripts/dev_commands.py run --target spinning_cube` to launch another executable.
- `python scripts/dev_commands.py run -- --json-file-in config/gui_runtime.json` to pass app flags (use `--` before app args).

### JSON config examples
- `python scripts/dev_commands.py run -- --json-file-in config/seed_runtime.json`
- `python scripts/dev_commands.py run -- --json-file-in config/soundboard_runtime.json`
- `python scripts/dev_commands.py run -- --json-file-in config/gui_runtime.json`

## Runtime configuration
- `sdl3_app --json-file-in <path>` loads JSON configs (script path, window size, `lua_debug`, etc.).
- `sdl3_app --create-seed-json config/seed_runtime.json` writes a starter file assuming `scripts/cube_logic.lua` sits beside the binary.
- `sdl3_app --set-default-json [path]` stores or overrides the runtime JSON; Windows writes `%APPDATA%/sdl3cpp`, other OSes use `$XDG_CONFIG_HOME/sdl3cpp/default_runtime.json` (fallback `~/.config/sdl3cpp`).

### Input bindings
`config/seed_runtime.json` includes an `input_bindings` section that maps keyboard keys and gamepad inputs to action names consumed by the Lua script (see `scripts/cube_logic.lua`).

Keyboard bindings use SDL key names (e.g. `W`, `Space`, `Left Shift`). Gamepad bindings use SDL gamepad names (e.g. `start`, `south`, `dpad_up`, `leftx`).

Example:
```
"input_bindings": {
  "move_forward": "W",
  "move_back": "S",
  "move_left": "A",
  "move_right": "D",
  "music_toggle": "M",
  "music_toggle_gamepad": "start",
  "gamepad_move_x_axis": "leftx",
  "gamepad_move_y_axis": "lefty",
  "gamepad_look_x_axis": "rightx",
  "gamepad_look_y_axis": "righty",
  "gamepad_dpad_up": "dpad_up",
  "gamepad_dpad_down": "dpad_down",
  "gamepad_dpad_left": "dpad_left",
  "gamepad_dpad_right": "dpad_right",
  "gamepad_button_actions": {
    "south": "gamepad_a",
    "east": "gamepad_b",
    "west": "gamepad_x",
    "north": "gamepad_y",
    "left_shoulder": "gamepad_lb",
    "right_shoulder": "gamepad_rb",
    "left_stick": "gamepad_ls",
    "right_stick": "gamepad_rs",
    "back": "gamepad_back",
    "start": "gamepad_start"
  },
  "gamepad_axis_actions": {
    "left_trigger": "gamepad_lt",
    "right_trigger": "gamepad_rt"
  },
  "gamepad_axis_action_threshold": 0.5
}
```

## GUI demo
`scripts/gui_demo.lua` exercises the Lua GUI framework; rendering hooks are currently stubbed while bgfx GUI integration is wired up. Launch it as `python scripts/dev_commands.py run -- --json-file-in config/gui_runtime.json` or register that config via `sdl3_app --set-default-json`.

## Audio assets
Install the dependencies that power `scripts/generate_audio_assets.py`:
- `python -m pip install --user numpy soundfile pedalboard piper-tts`

Before translating phrases, download a Piper voice (the script defaults to `en_US-lessac-medium`):

```
python -m piper.download_voices en_US-lessac-medium --download-dir scripts/assets/audio/tts/voices
```

Running the generator recreates procedural effects under `scripts/assets/audio/sfx/` and voice clips under `scripts/assets/audio/tts/`. Use `--force` to rebuild every file and `--skip-sfx` / `--skip-tts` if you only need one subset; add `--verbose` to see the internal logging as the files are created. Override the voice files with `--piper-voice-model <path>` and (optionally) `--piper-voice-config <path>` if you downloaded a different voice or location. Pass `--download-voice` to have the script invoke `piper.download_voices` automatically before rendering (requires `piper-tts` and network access).

## GitHub Actions workflow diagnostics
- `python -m pip install pyyaml` installs the YAML dependency for the workflow analyzer.
- `python scripts/workflow_doctor.py [--workflows-dir .github/workflows]` inspects workflows for missing permissions, floating action references, and other reproducibility/security hints.
