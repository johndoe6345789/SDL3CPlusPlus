#!/usr/bin/env python3
"""
Improved build helper CLI.

This version addresses Windows quoting issues with the `msvc-quick` command and
provides a cleaner approach for executing `vcvarsall.bat` followed by a CMake
build. The rest of the commands remain platform-neutral and avoid using
`shell=True` wherever possible for greater safety and consistency.

Key changes:

* Simplified the construction of the one-liner used to call `vcvarsall.bat`
  and run the subsequent command, avoiding nested quoting that caused errors
  under PowerShell. The `cmd.exe` invocation now looks like:

    cmd.exe /d /s /c call "<bat>" <arch> && <then command>

  where `<then command>` is properly quoted using `subprocess.list2cmdline`.

* Updated `msvc-quick` to use the above construction, while still allowing
  users to override the follow-on command via positional arguments after `--`.

* Other commands continue to build argument lists rather than shell strings,
  preventing injection and ensuring predictable behavior across platforms.

* Added descriptive comments throughout for maintainability.

Use this script as a drop-in replacement for the original `dev_commands.py`.
"""

from __future__ import annotations

import argparse
import os
import platform
import subprocess
from pathlib import Path
from typing import Iterable, Sequence

IS_WINDOWS = platform.system() == "Windows"

DEFAULT_GENERATOR = "ninja-msvc" if IS_WINDOWS else "ninja"
GENERATOR_DEFAULT_DIR = {
    "vs": "build",
    "ninja": "build-ninja",
    "ninja-msvc": "build-ninja-msvc",
}
CMAKE_GENERATOR = {
    "vs": "Visual Studio 17 2022",
    "ninja": "Ninja",
    "ninja-msvc": "Ninja",
}

DEFAULT_BUILD_DIR = GENERATOR_DEFAULT_DIR[DEFAULT_GENERATOR]
TRACE_ENV_VAR = "DEV_COMMANDS_TRACE"

DEFAULT_VCVARSALL = (
    "C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional"
    "\\VC\\Auxiliary\\Build\\vcvarsall.bat"
)
VITA_ENV_VAR = "VITASDK"
DEFAULT_VITA_SDK_PATH = "/usr/local/vitasdk"
VITA_PRESETS = {"vita-release"}


def _sh_quote(s: str) -> str:
    """Minimal POSIX-style quoting for display purposes on non-Windows."""
    if not s:
        return "''"
    safe = set(
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
        "._-/:@=+"
    )
    if all(c in safe for c in s):
        return s
    return "'" + s.replace("'", "'\"'\"'") + "'"


def _print_cmd(argv: Sequence[str]) -> None:
    """
    Print a command list in a way that approximates how it would appear on the
    command line. Uses Windows-specific quoting on Windows via
    `subprocess.list2cmdline`, and POSIX-style quoting elsewhere.
    """
    if IS_WINDOWS:
        rendered = subprocess.list2cmdline(list(argv))
    else:
        rendered = " ".join(_sh_quote(a) for a in argv)
    print("\n> " + rendered)


def _trace(message: str) -> None:
    if os.environ.get(TRACE_ENV_VAR) == "1":
        print(f"[trace] {message}")


def _strip_leading_double_dash(args: Sequence[str] | None) -> list[str]:
    """Drop a leading `--` that argparse keeps with REMAINDER arguments."""
    if not args:
        return []
    args_list = list(args)
    if args_list and args_list[0] == "--":
        return args_list[1:]
    return args_list


def _has_runtime_config_arg(args: Sequence[str] | None) -> bool:
    if not args:
        return False
    for arg in args:
        if arg in {"-j", "--json-file-in"}:
            return True
        if arg.startswith("--json-file-in="):
            return True
        if arg.startswith("-j") and len(arg) > 2:
            return True
    return False


def _merge_env(env_overrides: dict[str, str] | None) -> dict[str, str] | None:
    if not env_overrides:
        return None
    merged = os.environ.copy()
    merged.update(env_overrides)
    return merged


def run_argvs(
    argvs: Iterable[Sequence[str]],
    dry_run: bool,
    cwd: str | None = None,
    env_overrides: dict[str, str] | None = None,
) -> None:
    """
    Run a sequence of commands represented as lists of arguments. Each command
    is printed before execution. If `dry_run` is True, commands are printed
    but not executed.
    """
    merged_env = _merge_env(env_overrides)
    for argv in argvs:
        _print_cmd(argv)
        if dry_run:
            continue
        subprocess.run(list(argv), check=True, cwd=cwd, env=merged_env)


def _as_build_dir(path_str: str | None, fallback: str) -> str:
    """Return the provided path if not None, otherwise the fallback."""
    return path_str or fallback


def _has_cache_arg(cmake_args: Sequence[str] | None, name: str) -> bool:
    """Return True if the CMake args already define a cache variable."""
    if not cmake_args:
        return False
    key = f"-D{name}"
    prefix = f"-D{name}="
    for arg in cmake_args:
        if arg == key or arg.startswith(prefix):
            return True
    return False


def _cmake_cache_arg_value(cmake_args: Sequence[str] | None, name: str) -> str | None:
    if not cmake_args:
        return None
    key = f"-D{name}"
    for arg in cmake_args:
        if not arg.startswith(key):
            continue
        if "=" not in arg:
            return ""
        return arg.split("=", 1)[1]
    return None


def _cmake_cache_enabled(cmake_args: Sequence[str] | None, name: str) -> bool:
    value = _cmake_cache_arg_value(cmake_args, name)
    if value is None:
        return False
    return value.strip().upper() in {"ON", "TRUE", "1", "YES"}


def _read_cmake_cache_value(build_dir: str, name: str) -> str | None:
    cache_path = Path(build_dir) / "CMakeCache.txt"
    if not cache_path.is_file():
        return None
    try:
        content = cache_path.read_text(encoding="utf-8", errors="ignore")
    except OSError as exc:
        _trace(f"Unable to read CMake cache at {cache_path}: {exc}")
        return None
    needle = f"{name}:"
    for line in content.splitlines():
        if not line.startswith(needle):
            continue
        if "=" not in line:
            return ""
        return line.split("=", 1)[1].strip()
    return None


def _is_vita_build_dir(build_dir: str) -> bool:
    value = _read_cmake_cache_value(build_dir, "ENABLE_VITA")
    if not value:
        return False
    return value.upper() in {"ON", "TRUE", "1", "YES"}


def _vita_env_overrides(reason: str) -> dict[str, str]:
    _trace(f"Setting {VITA_ENV_VAR} for {reason}: {DEFAULT_VITA_SDK_PATH}")
    return {VITA_ENV_VAR: DEFAULT_VITA_SDK_PATH}


def _resolve_vita_env_for_configure(
    preset: str | None,
    cmake_args: Sequence[str] | None,
) -> dict[str, str] | None:
    if preset in VITA_PRESETS:
        return _vita_env_overrides(f"preset {preset}")
    if _cmake_cache_enabled(cmake_args, "ENABLE_VITA"):
        return _vita_env_overrides("explicit ENABLE_VITA cache arg")
    return None


def _resolve_vita_env_for_build(build_dir: str) -> dict[str, str] | None:
    if _is_vita_build_dir(build_dir):
        return _vita_env_overrides(f"build dir {build_dir}")
    return None


def _find_conan_toolchain(build_type: str) -> Path | None:
    """
    Look for the Conan toolchain file in common output locations.

    The default `conan install -of build-ninja` + `cmake_layout()` layout produces
    `build-ninja/build/<build_type>/generators/conan_toolchain.cmake`.
    """
    candidates = [
        Path("build-ninja") / "build" / build_type / "generators" / "conan_toolchain.cmake",
        Path("build-ninja") / build_type / "generators" / "conan_toolchain.cmake",
        Path("build-ninja") / "generators" / "conan_toolchain.cmake",
        Path("build-ninja") / "conan_toolchain.cmake",
    ]
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    return None


def _has_cmake_cache(build_dir: str) -> bool:
    """Return True if the build directory already has a CMake cache."""
    return (Path(build_dir) / "CMakeCache.txt").is_file()


def dependencies(args: argparse.Namespace) -> None:
    """Run Conan profile detection and install dependencies."""
    cmd_detect = ["conan", "profile", "detect", "-f"]
    cmd_install = ["conan", "install", ".", "-of", "build-ninja", "-b", "missing", "-c", "tools.build:cxxflags=[\"-include\",\"cstdint\"]"]
    conan_install_args = _strip_leading_double_dash(args.conan_install_args)
    if conan_install_args:
        cmd_install.extend(conan_install_args)
    run_argvs([cmd_detect, cmd_install], args.dry_run)


def configure(args: argparse.Namespace) -> None:
    """Configure a CMake project based on the chosen generator and options."""
    if args.preset:
        cmake_args = ["cmake", "--preset", args.preset]
        cmake_extra_args = _strip_leading_double_dash(args.cmake_args)
        if cmake_extra_args:
            cmake_args.extend(cmake_extra_args)
        vita_env = _resolve_vita_env_for_configure(args.preset, cmake_extra_args)
        run_argvs([cmake_args], args.dry_run, env_overrides=vita_env)
        return
    generator = args.generator or DEFAULT_GENERATOR
    build_dir = _as_build_dir(
        args.build_dir, GENERATOR_DEFAULT_DIR.get(generator, "build")
    )
    cmake_args: list[str] = ["cmake", "-B", build_dir, "-S", "."]
    conan_toolchain = _find_conan_toolchain(args.build_type)
    if (
        conan_toolchain
        and not _has_cache_arg(args.cmake_args, "CMAKE_TOOLCHAIN_FILE")
        and not _has_cmake_cache(build_dir)
    ):
        cmake_args.append(f"-DCMAKE_TOOLCHAIN_FILE={conan_toolchain}")
    if conan_toolchain and not _has_cache_arg(args.cmake_args, "CMAKE_PREFIX_PATH"):
        conan_generators_dir = conan_toolchain.parent
        cmake_args.append(f"-DCMAKE_PREFIX_PATH={conan_generators_dir}")
    if generator == "vs":
        cmake_args.extend(["-G", CMAKE_GENERATOR["vs"]])
    else:
        cmake_args.extend(["-G", CMAKE_GENERATOR[generator]])
        cmake_args.append(f"-DCMAKE_BUILD_TYPE={args.build_type}")
    cmake_extra_args = _strip_leading_double_dash(args.cmake_args)
    if cmake_extra_args:
        cmake_args.extend(cmake_extra_args)
    vita_env = _resolve_vita_env_for_configure(None, cmake_extra_args)
    run_argvs([cmake_args], args.dry_run, env_overrides=vita_env)


def build(args: argparse.Namespace) -> None:
    """Run the `cmake --build` command for a given build directory."""
    cmd: list[str] = ["cmake", "--build", args.build_dir]
    if args.config:
        cmd.extend(["--config", args.config])
    if args.target:
        cmd.extend(["--target", args.target])
    build_tool_args = _strip_leading_double_dash(args.build_tool_args)
    if build_tool_args:
        cmd.append("--")
        cmd.extend(build_tool_args)
    vita_env = _resolve_vita_env_for_build(args.build_dir)
    run_argvs([cmd], args.dry_run, env_overrides=vita_env)


def tests(args: argparse.Namespace) -> None:
    """Build (optional) and run ctest for a given build directory."""
    build_dir = _as_build_dir(args.build_dir, DEFAULT_BUILD_DIR)
    vita_env = _resolve_vita_env_for_build(build_dir)
    argvs: list[list[str]] = []

    if args.build_first:
        build_cmd: list[str] = ["cmake", "--build", build_dir]
        if args.config:
            build_cmd.extend(["--config", args.config])
        if args.target:
            build_cmd.extend(["--target", args.target])
        build_tool_args = _strip_leading_double_dash(args.build_tool_args)
        if build_tool_args:
            build_cmd.append("--")
            build_cmd.extend(build_tool_args)
        argvs.append(build_cmd)

    ctest_cmd: list[str] = ["ctest", "--output-on-failure", "--test-dir", build_dir]
    if args.config:
        ctest_cmd.extend(["-C", args.config])
    ctest_args = _strip_leading_double_dash(args.ctest_args)
    if ctest_args:
        ctest_cmd.extend(ctest_args)
    argvs.append(ctest_cmd)

    run_argvs(argvs, args.dry_run, env_overrides=vita_env)


def _cmd_one_liner_vcvars_then(bat: str, arch: str, then_parts: Sequence[str]) -> list[str]:
    """
    Construct a command to call a Visual Studio environment setup batch file and
    then run another command. The returned list of arguments can be passed to
    subprocess.run with shell=False.

    On Windows, we use:

        cmd.exe /d /s /c call "<bat>" <arch> && <then...>

    The path to the batch file is quoted to handle spaces. The follow-on
    command (`then_parts`) is converted to a command string using
    `subprocess.list2cmdline`, which properly quotes arguments for cmd.exe.
    """
    then_cmdline = subprocess.list2cmdline(list(then_parts))
    full_cmd = f'call "{bat}" {arch} && {then_cmdline}'
    return ["cmd.exe", "/d", "/s", "/c", full_cmd]


def msvc_quick(args: argparse.Namespace) -> None:
    """
    Set up the Visual Studio environment and build the project.

    On Windows, this command calls `vcvarsall.bat` (or a custom batch file)
    with the specified architecture, then runs a follow-on command. By
    default, the follow-on command is `cmake --build <build_dir>` with
    optional configuration, target, and extra build-tool arguments. Users can
    override the follow-on command entirely by specifying positional arguments
    after `--`.

    On non-Windows platforms, this command will exit with an error, as there is
    no Visual Studio environment to initialize.
    """
    if not IS_WINDOWS:
        raise SystemExit("msvc-quick is only supported on Windows")
    bat = args.bat_path or DEFAULT_VCVARSALL
    arch = args.arch or "x64"
    if args.then_command:
        then_cmd = _strip_leading_double_dash(args.then_command)
    else:
        build_dir = _as_build_dir(args.build_dir, DEFAULT_BUILD_DIR)
        then_cmd = ["cmake", "--build", build_dir]
        if args.config:
            then_cmd.extend(["--config", args.config])
        if args.target:
            then_cmd.extend(["--target", args.target])
        build_tool_args = _strip_leading_double_dash(args.build_tool_args)
        if build_tool_args:
            then_cmd.append("--")
            then_cmd.extend(build_tool_args)
    cmd = _cmd_one_liner_vcvars_then(bat, arch, then_cmd)
    run_argvs([cmd], args.dry_run)


def _sync_assets(build_dir: str, dry_run: bool) -> None:
    """
    Sync asset files (scripts, shaders, models) from the project root to the
    build directory before running the application.
    """
    import shutil

    build_path = Path(build_dir)
    project_root = Path(".")

    # Define asset directories to sync
    asset_dirs = [
        ("scripts", ["*.lua"]),
        ("shaders", ["*.vert", "*.frag", "*.geom", "*.tesc", "*.tese", "*.comp", "*.spv"]),
        ("scripts/models", ["*.stl", "*.obj", "*.fbx"]),
        ("config", ["*.json"]),
    ]
    asset_trees = [
        "MaterialX/libraries",
        "MaterialX/resources",
    ]

    print("\n=== Syncing Assets ===")

    for src_dir, patterns in asset_dirs:
        src_path = project_root / src_dir
        dst_path = build_path / src_dir

        if not src_path.exists():
            continue

        # Create destination directory if needed
        if not dry_run:
            dst_path.mkdir(parents=True, exist_ok=True)

        # Sync files matching patterns
        for pattern in patterns:
            for src_file in src_path.glob(pattern):
                if src_file.is_file() and src_file.name != "dev_commands.py":
                    dst_file = dst_path / src_file.name
                    print(f"  {src_file} -> {dst_file}")
                    if not dry_run:
                        shutil.copy2(src_file, dst_file)

    for src_dir in asset_trees:
        src_path = project_root / src_dir
        dst_path = build_path / src_dir
        if not src_path.exists():
            continue
        print(f"  {src_path} -> {dst_path}")
        if not dry_run:
            shutil.copytree(src_path, dst_path, dirs_exist_ok=True)

    print("=== Assets Synced ===\n")


def run_demo(args: argparse.Namespace) -> None:
    """
    Run a compiled demo application from the build directory. The default
    executable is `sdl3_app` (or `sdl3_app.exe` on Windows). Additional
    arguments can be passed to the executable after `--`.

    By default, syncs asset files before running.
    Use --no-sync to skip asset synchronization.
    """
    build_dir = _as_build_dir(args.build_dir, DEFAULT_BUILD_DIR)

    if not args.no_sync:
        _sync_assets(build_dir, args.dry_run)

    exe_name = args.target or ("sdl3_app.exe" if IS_WINDOWS else "sdl3_app")
    binary = str(Path(build_dir).resolve() / exe_name)
    run_args = _strip_leading_double_dash(args.args)
    cmd: list[str] = [binary]
    if run_args:
        cmd.extend(run_args)
    _print_cmd(cmd)
    import os
    os.chdir(build_dir)
    os.execv(binary, cmd)


def gui(args: argparse.Namespace) -> None:
    """
    Launch a PyQt6 GUI launcher similar to Steam's interface for managing builds
    and running demos.
    """
    try:
        from PyQt6.QtWidgets import (
            QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
            QPushButton, QLabel, QTextEdit, QComboBox, QListWidget, QListWidgetItem,
            QSplitter, QMenuBar, QDialog, QDialogButtonBox, QFormLayout, QMessageBox,
            QPlainTextEdit, QTabWidget, QLineEdit
        )
        from PyQt6.QtCore import Qt, QProcess, QProcessEnvironment, QSize, QTimer
        from PyQt6.QtGui import QFont, QPalette, QColor, QAction, QSyntaxHighlighter, QTextCharFormat, QTextCursor, QTextDocument
    except ImportError:
        raise SystemExit(
            "PyQt6 is not installed. Install it with:\n"
            "  pip install PyQt6"
        )

    import sys
    import re

    class LuaSyntaxHighlighter(QSyntaxHighlighter):
        """Simple Lua syntax highlighter"""
        def __init__(self, parent=None):
            super().__init__(parent)

            # Define colors
            keyword_color = QColor("#569cd6")  # Blue
            string_color = QColor("#ce9178")   # Orange
            comment_color = QColor("#6a9955")  # Green
            function_color = QColor("#dcdcaa")  # Yellow
            number_color = QColor("#b5cea8")   # Light green

            # Define formatting
            self.keyword_format = QTextCharFormat()
            self.keyword_format.setForeground(keyword_color)
            self.keyword_format.setFontWeight(700)

            self.string_format = QTextCharFormat()
            self.string_format.setForeground(string_color)

            self.comment_format = QTextCharFormat()
            self.comment_format.setForeground(comment_color)
            self.comment_format.setFontItalic(True)

            self.function_format = QTextCharFormat()
            self.function_format.setForeground(function_color)

            self.number_format = QTextCharFormat()
            self.number_format.setForeground(number_color)

            # Lua keywords
            keywords = [
                'and', 'break', 'do', 'else', 'elseif', 'end', 'false', 'for',
                'function', 'if', 'in', 'local', 'nil', 'not', 'or', 'repeat',
                'return', 'then', 'true', 'until', 'while'
            ]

            # Build highlighting rules
            self.rules = []

            # Keywords
            for word in keywords:
                pattern = f'\\b{word}\\b'
                self.rules.append((re.compile(pattern), self.keyword_format))

            # Numbers
            self.rules.append((re.compile(r'\b\d+\.?\d*\b'), self.number_format))

            # Functions
            self.rules.append((re.compile(r'\b[A-Za-z_][A-Za-z0-9_]*(?=\s*\()'), self.function_format))

            # Strings (double quotes)
            self.rules.append((re.compile(r'"[^"\\]*(\\.[^"\\]*)*"'), self.string_format))

            # Strings (single quotes)
            self.rules.append((re.compile(r"'[^'\\]*(\\.[^'\\]*)*'"), self.string_format))

            # Comments
            self.rules.append((re.compile(r'--[^\n]*'), self.comment_format))

        def highlightBlock(self, text):
            """Apply syntax highlighting to the given block of text"""
            for pattern, fmt in self.rules:
                for match in pattern.finditer(text):
                    start, end = match.span()
                    self.setFormat(start, end - start, fmt)

    class BuildSettingsDialog(QDialog):
        """Dialog for configuring build settings"""
        def __init__(self, parent=None):
            super().__init__(parent)
            self.setWindowTitle("Build Settings")
            self.setMinimumWidth(400)

            layout = QFormLayout(self)

            self.preset_combo = QComboBox()
            self.preset_combo.addItems(["default", "vita-release"])
            self.preset_combo.setCurrentText("default")
            layout.addRow("Preset:", self.preset_combo)

            self.generator_combo = QComboBox()
            self.generator_combo.addItems(["ninja", "ninja-msvc", "vs"])
            self.generator_combo.setCurrentText(DEFAULT_GENERATOR)
            layout.addRow("Generator:", self.generator_combo)

            self.build_type_combo = QComboBox()
            self.build_type_combo.addItems(["Release", "Debug", "RelWithDebInfo"])
            layout.addRow("Build Type:", self.build_type_combo)

            self.target_combo = QComboBox()
            self.target_combo.addItems([
                "sdl3_app",
                "all",
                "script_engine_tests",
                "gxm_backend_tests",
                "bgfx_gui_service_tests",
            ])
            layout.addRow("Target:", self.target_combo)

            buttons = QDialogButtonBox(
                QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel
            )
            buttons.accepted.connect(self.accept)
            buttons.rejected.connect(self.reject)
            layout.addRow(buttons)

    class NewProjectDialog(QDialog):
        """Dialog for creating a new project from templates"""
        def __init__(self, parent=None):
            super().__init__(parent)
            self.setWindowTitle("New Project")
            self.setMinimumWidth(500)
            self.setMinimumHeight(300)

            layout = QVBoxLayout(self)

            # Project name input
            name_layout = QHBoxLayout()
            name_label = QLabel("Project Name:")
            self.name_input = QLineEdit()
            self.name_input.setPlaceholderText("Enter project name")
            name_layout.addWidget(name_label)
            name_layout.addWidget(self.name_input)
            layout.addLayout(name_layout)

            # Template selection
            template_label = QLabel("Template:")
            layout.addWidget(template_label)

            self.template_combo = QComboBox()
            self.template_combo.addItem("Cube Demo - 3D game with physics", "cube")
            self.template_combo.addItem("GUI Demo - ImGui interface demo", "gui")
            self.template_combo.addItem("Soundboard - Audio application", "soundboard")
            layout.addWidget(self.template_combo)

            # Description
            self.description_label = QLabel()
            self.description_label.setWordWrap(True)
            self.description_label.setStyleSheet("color: #666; font-size: 10pt; margin-top: 10px;")
            layout.addWidget(self.description_label)

            # Update description when template changes
            self.template_combo.currentIndexChanged.connect(self.update_description)
            self.update_description()

            layout.addStretch()

            # Buttons
            buttons = QDialogButtonBox(
                QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel
            )
            buttons.accepted.connect(self.accept)
            buttons.rejected.connect(self.reject)
            layout.addWidget(buttons)

        def update_description(self):
            """Update description based on selected template"""
            template = self.template_combo.currentData()
            descriptions = {
                "cube": "A 3D cube room with first-person controls, lanterns, and physics interactions. Includes mouse look, movement controls, and audio.",
                "gui": "An interactive GUI demonstration using ImGui widgets and controls. Showcases various UI elements and interactions.",
                "soundboard": "An audio soundboard application for playing and mixing sound effects. Features multiple audio channels and controls."
            }
            self.description_label.setText(descriptions.get(template, ""))

        def get_project_data(self):
            """Return the project name and template type"""
            return {
                "name": self.name_input.text().strip(),
                "template": self.template_combo.currentData()
            }

    class BuildLauncherGUI(QMainWindow):
        def __init__(self):
            super().__init__()
            self.process = None
            self.current_game = None

            # Build settings
            self.preset = "default"
            self.generator = DEFAULT_GENERATOR
            self.build_type = "Release"
            self.target = "sdl3_app"

            # Load games from config files
            self.games = self.load_games_from_config()

            self.init_ui()

        def load_games_from_config(self):
            """Scan config directory for JSON files with launcher metadata"""
            import json
            from pathlib import Path

            games = []
            config_dir = Path("config")

            if not config_dir.exists():
                # Fallback if config dir doesn't exist
                return [{
                    "id": "sdl3_app",
                    "name": "SDL3 Demo Application",
                    "description": "Main SDL3 demo application showcasing basic functionality",
                    "config_file": "config/seed_runtime.json",
                }]

            # Scan all JSON files in config directory
            for config_file in sorted(config_dir.glob("*.json")):
                try:
                    with open(config_file, 'r') as f:
                        data = json.load(f)

                    # Check if file has launcher metadata
                    if "launcher" in data:
                        launcher = data["launcher"]

                        # Only add if enabled (defaults to True if not specified)
                        if launcher.get("enabled", True):
                            games.append({
                                "id": config_file.stem,
                                "name": launcher.get("name", config_file.stem),
                                "description": launcher.get("description", ""),
                                "config_file": str(config_file),
                            })
                except (json.JSONDecodeError, IOError) as e:
                    # Skip files that can't be parsed
                    print(f"Warning: Could not load {config_file}: {e}")
                    continue

            # If no games found, add a default one
            if not games:
                games.append({
                    "id": "sdl3_app",
                    "name": "SDL3 Demo Application",
                    "description": "Main SDL3 demo application showcasing basic functionality",
                    "config_file": "config/seed_runtime.json",
                })

            return games

        def init_ui(self):
            self.setWindowTitle("SDL3 C++ Launcher")
            self.setMinimumSize(1000, 700)

            # Set dark theme similar to Steam
            self.set_dark_theme()

            # Central widget with horizontal splitter
            central_widget = QWidget()
            self.setCentralWidget(central_widget)
            main_layout = QHBoxLayout(central_widget)
            main_layout.setContentsMargins(0, 0, 0, 0)
            main_layout.setSpacing(0)

            # Left sidebar - Game library
            sidebar = QWidget()
            sidebar.setMaximumWidth(250)
            sidebar.setStyleSheet("background-color: #171a21;")
            sidebar_layout = QVBoxLayout(sidebar)
            sidebar_layout.setContentsMargins(0, 0, 0, 0)

            # Library header
            library_header = QLabel("LIBRARY")
            library_header.setStyleSheet("""
                background-color: #1b2838;
                color: #c6d1db;
                padding: 12px;
                font-weight: bold;
                font-size: 11pt;
            """)
            sidebar_layout.addWidget(library_header)

            # Game list
            self.game_list = QListWidget()
            self.game_list.setStyleSheet("""
                QListWidget {
                    background-color: #171a21;
                    border: none;
                    color: #c6d1db;
                    font-size: 10pt;
                    outline: none;
                }
                QListWidget::item {
                    padding: 12px;
                    border-bottom: 1px solid #0e1216;
                }
                QListWidget::item:selected {
                    background-color: #2a475e;
                }
                QListWidget::item:hover {
                    background-color: #1b2838;
                }
            """)
            self.game_list.currentItemChanged.connect(self.on_game_selected)

            for game in self.games:
                item = QListWidgetItem(game["name"])
                item.setData(Qt.ItemDataRole.UserRole, game)
                self.game_list.addItem(item)

            sidebar_layout.addWidget(self.game_list)
            main_layout.addWidget(sidebar)

            # Right side - Game details and console
            right_panel = QWidget()
            right_layout = QVBoxLayout(right_panel)
            right_layout.setContentsMargins(0, 0, 0, 0)
            right_layout.setSpacing(0)

            # Game detail header
            detail_header = QWidget()
            detail_header.setStyleSheet("background-color: #1b2838;")
            detail_header.setMinimumHeight(200)
            detail_layout = QVBoxLayout(detail_header)
            detail_layout.setContentsMargins(30, 30, 30, 30)

            # Game title
            self.game_title = QLabel("Select a game")
            title_font = QFont()
            title_font.setPointSize(24)
            title_font.setBold(True)
            self.game_title.setFont(title_font)
            self.game_title.setStyleSheet("color: #ffffff;")
            detail_layout.addWidget(self.game_title)

            # Game description
            self.game_description = QLabel("")
            self.game_description.setWordWrap(True)
            self.game_description.setStyleSheet("color: #8f98a0; font-size: 11pt;")
            detail_layout.addWidget(self.game_description)

            # Play button container
            button_container = QHBoxLayout()

            self.play_btn = QPushButton("▶ PLAY")
            self.play_btn.setEnabled(False)
            self.play_btn.setMinimumHeight(50)
            self.play_btn.setMinimumWidth(200)
            play_font = QFont()
            play_font.setPointSize(14)
            play_font.setBold(True)
            self.play_btn.setFont(play_font)
            self.play_btn.setStyleSheet("""
                QPushButton {
                    background-color: #5c7e10;
                    color: white;
                    border: none;
                    border-radius: 4px;
                    padding: 10px 40px;
                }
                QPushButton:hover {
                    background-color: #6a9612;
                }
                QPushButton:pressed {
                    background-color: #4a6a0d;
                }
                QPushButton:disabled {
                    background-color: #3f4e5f;
                    color: #7a8896;
                }
            """)
            self.play_btn.clicked.connect(self.play_game)
            button_container.addWidget(self.play_btn)

            self.stop_btn = QPushButton("⏹ STOP")
            self.stop_btn.setEnabled(False)
            self.stop_btn.setMinimumHeight(50)
            self.stop_btn.setMinimumWidth(120)
            stop_font = QFont()
            stop_font.setPointSize(12)
            stop_font.setBold(True)
            self.stop_btn.setFont(stop_font)
            self.stop_btn.setStyleSheet("""
                QPushButton {
                    background-color: #a83232;
                    color: white;
                    border: none;
                    border-radius: 4px;
                    padding: 10px 20px;
                }
                QPushButton:hover {
                    background-color: #c13838;
                }
                QPushButton:pressed {
                    background-color: #8f2828;
                }
                QPushButton:disabled {
                    background-color: #3f4e5f;
                    color: #7a8896;
                }
            """)
            self.stop_btn.clicked.connect(self.stop_process)
            button_container.addWidget(self.stop_btn)

            button_container.addStretch()
            detail_layout.addLayout(button_container)

            right_layout.addWidget(detail_header)

            # Tabbed interface for code editor and console
            self.tab_widget = QTabWidget()
            self.tab_widget.setStyleSheet("""
                QTabWidget::pane {
                    background-color: #1b2838;
                    border: none;
                }
                QTabBar::tab {
                    background-color: #1b2838;
                    color: #8f98a0;
                    padding: 8px 16px;
                    border: none;
                    margin-right: 2px;
                }
                QTabBar::tab:selected {
                    background-color: #2a475e;
                    color: #c6d1db;
                }
                QTabBar::tab:hover {
                    background-color: #243447;
                }
            """)

            # Lua Code Editor Tab
            editor_container = QWidget()
            editor_layout = QVBoxLayout(editor_container)
            editor_layout.setContentsMargins(10, 10, 10, 10)

            # Editor toolbar
            editor_toolbar = QHBoxLayout()
            self.lua_file_label = QLabel("No file loaded")
            self.lua_file_label.setStyleSheet("color: #8f98a0; font-size: 9pt;")
            editor_toolbar.addWidget(self.lua_file_label)

            # Search box
            self.search_edit = QLineEdit()
            self.search_edit.setPlaceholderText("Search...")
            self.search_edit.setMaximumWidth(150)
            self.search_edit.setStyleSheet("""
                QLineEdit {
                    background-color: #2a475e;
                    color: #c6d1db;
                    border: 1px solid #0e1216;
                    border-radius: 3px;
                    padding: 3px;
                }
            """)
            self.search_edit.returnPressed.connect(lambda: self.search_in_editor(forward=True))
            editor_toolbar.addWidget(self.search_edit)

            self.find_prev_btn = QPushButton("⬆ Prev")
            self.find_prev_btn.setStyleSheet("""
                QPushButton {
                    background-color: #2a475e;
                    color: #c6d1db;
                    border: none;
                    border-radius: 3px;
                    padding: 5px;
                }
                QPushButton:hover {
                    background-color: #3e5c78;
                }
            """)
            self.find_prev_btn.clicked.connect(lambda: self.search_in_editor(forward=False))
            editor_toolbar.addWidget(self.find_prev_btn)

            self.find_next_btn = QPushButton("⬇ Next")
            self.find_next_btn.setStyleSheet("""
                QPushButton {
                    background-color: #2a475e;
                    color: #c6d1db;
                    border: none;
                    border-radius: 3px;
                    padding: 5px;
                }
                QPushButton:hover {
                    background-color: #3e5c78;
                }
            """)
            self.find_next_btn.clicked.connect(lambda: self.search_in_editor(forward=True))
            editor_toolbar.addWidget(self.find_next_btn)

            # Status label for feedback
            self.status_label = QLabel("")
            self.status_label.setStyleSheet("color: #8f98a0; font-size: 9pt;")
            editor_toolbar.addWidget(self.status_label)

            editor_toolbar.addStretch()

            self.save_lua_btn = QPushButton("💾 Save")
            self.save_lua_btn.setEnabled(False)
            self.save_lua_btn.setStyleSheet("""
                QPushButton {
                    background-color: #2a475e;
                    color: #c6d1db;
                    border: none;
                    border-radius: 3px;
                    padding: 5px 15px;
                }
                QPushButton:hover {
                    background-color: #3e5c78;
                }
                QPushButton:disabled {
                    background-color: #1b2838;
                    color: #4e5a66;
                }
            """)
            self.save_lua_btn.clicked.connect(self.save_lua_script)
            editor_toolbar.addWidget(self.save_lua_btn)

            self.reload_lua_btn = QPushButton("🔄 Reload")
            self.reload_lua_btn.setEnabled(False)
            self.reload_lua_btn.setStyleSheet("""
                QPushButton {
                    background-color: #2a475e;
                    color: #c6d1db;
                    border: none;
                    border-radius: 3px;
                    padding: 5px 15px;
                }
                QPushButton:hover {
                    background-color: #3e5c78;
                }
                QPushButton:disabled {
                    background-color: #1b2838;
                    color: #4e5a66;
                }
            """)
            self.reload_lua_btn.clicked.connect(self.reload_lua_script)
            editor_toolbar.addWidget(self.reload_lua_btn)

            editor_layout.addLayout(editor_toolbar)

            # Code editor
            self.lua_editor = QPlainTextEdit()
            self.lua_editor.setPlaceholderText("Select a game to edit its Lua script...")
            editor_font = QFont("Courier New")
            editor_font.setPointSize(10)
            self.lua_editor.setFont(editor_font)
            self.lua_editor.setStyleSheet("""
                QPlainTextEdit {
                    background-color: #1e1e1e;
                    color: #d4d4d4;
                    border: 1px solid #0e1216;
                    border-radius: 3px;
                    padding: 5px;
                }
            """)
            self.lua_editor.setLineWrapMode(QPlainTextEdit.LineWrapMode.NoWrap)
            self.lua_editor.textChanged.connect(self.on_lua_text_changed)

            # Apply syntax highlighting
            self.lua_highlighter = LuaSyntaxHighlighter(self.lua_editor.document())

            editor_layout.addWidget(self.lua_editor)

            self.tab_widget.addTab(editor_container, "Lua Script Editor")

            # Console Output Tab
            console_container = QWidget()
            console_layout = QVBoxLayout(console_container)
            console_layout.setContentsMargins(10, 10, 10, 10)

            console_label = QLabel("OUTPUT")
            console_label.setStyleSheet("color: #8f98a0; font-weight: bold; font-size: 9pt;")
            console_layout.addWidget(console_label)

            self.console = QTextEdit()
            self.console.setReadOnly(True)
            console_font = QFont("Courier New")
            console_font.setPointSize(9)
            self.console.setFont(console_font)
            self.console.setStyleSheet("""
                QTextEdit {
                    background-color: #0e1216;
                    color: #c6d1db;
                    border: 1px solid #0e1216;
                    border-radius: 3px;
                }
            """)

            # Console toolbar
            console_toolbar = QHBoxLayout()

            self.copy_console_btn = QPushButton("📋 Copy")
            self.copy_console_btn.setStyleSheet("""
                QPushButton {
                    background-color: #2a475e;
                    color: #c6d1db;
                    border: none;
                    border-radius: 3px;
                    padding: 5px 15px;
                }
                QPushButton:hover {
                    background-color: #3e5c78;
                }
            """)
            self.copy_console_btn.clicked.connect(self.copy_console)
            console_toolbar.addWidget(self.copy_console_btn)

            self.clear_console_btn = QPushButton("🗑 Clear")
            self.clear_console_btn.setStyleSheet("""
                QPushButton {
                    background-color: #2a475e;
                    color: #c6d1db;
                    border: none;
                    border-radius: 3px;
                    padding: 5px 15px;
                }
                QPushButton:hover {
                    background-color: #3e5c78;
                }
            """)
            self.clear_console_btn.clicked.connect(self.console.clear)
            console_toolbar.addWidget(self.clear_console_btn)

            console_toolbar.addStretch()
            console_layout.addLayout(console_toolbar)

            console_layout.addWidget(self.console)

            self.tab_widget.addTab(console_container, "Console Output")

            right_layout.addWidget(self.tab_widget)

            main_layout.addWidget(right_panel, 1)

            # Create menu bar after all widgets are initialized
            self.create_menu_bar()

            # Select first game by default
            if self.games:
                self.game_list.setCurrentRow(0)

        def create_menu_bar(self):
            """Create menu bar with developer tools"""
            menubar = self.menuBar()
            menubar.setStyleSheet("""
                QMenuBar {
                    background-color: #171a21;
                    color: #c6d1db;
                    padding: 4px;
                }
                QMenuBar::item:selected {
                    background-color: #2a475e;
                }
                QMenu {
                    background-color: #1b2838;
                    color: #c6d1db;
                    border: 1px solid #0e1216;
                }
                QMenu::item:selected {
                    background-color: #2a475e;
                }
            """)

            # File menu
            file_menu = menubar.addMenu("File")

            new_action = QAction("New Project...", self)
            new_action.triggered.connect(self.show_new_project_dialog)
            file_menu.addAction(new_action)

            file_menu.addSeparator()

            exit_action = QAction("Exit", self)
            exit_action.triggered.connect(self.close)
            file_menu.addAction(exit_action)

            # Developer menu
            dev_menu = menubar.addMenu("Developer")

            deps_action = QAction("Install Dependencies", self)
            deps_action.triggered.connect(self.run_dependencies)
            dev_menu.addAction(deps_action)

            config_action = QAction("Configure CMake", self)
            config_action.triggered.connect(self.run_configure)
            dev_menu.addAction(config_action)

            build_action = QAction("Build Project", self)
            build_action.triggered.connect(self.run_build)
            dev_menu.addAction(build_action)

            tests_action = QAction("Run Tests", self)
            tests_action.triggered.connect(self.run_tests)
            dev_menu.addAction(tests_action)

            sync_action = QAction("Sync Assets", self)
            sync_action.triggered.connect(self.sync_assets)
            dev_menu.addAction(sync_action)

            dev_menu.addSeparator()

            settings_action = QAction("Build Settings...", self)
            settings_action.triggered.connect(self.show_settings)
            dev_menu.addAction(settings_action)

            # View menu
            view_menu = menubar.addMenu("View")

            clear_console_action = QAction("Clear Console", self)
            clear_console_action.triggered.connect(self.console.clear)
            view_menu.addAction(clear_console_action)

        def show_settings(self):
            """Show build settings dialog"""
            dialog = BuildSettingsDialog(self)
            dialog.preset_combo.setCurrentText(self.preset)
            dialog.generator_combo.setCurrentText(self.generator)
            dialog.build_type_combo.setCurrentText(self.build_type)
            dialog.target_combo.setCurrentText(self.target)

            if dialog.exec() == QDialog.DialogCode.Accepted:
                self.preset = dialog.preset_combo.currentText()
                self.generator = dialog.generator_combo.currentText()
                self.build_type = dialog.build_type_combo.currentText()
                self.target = dialog.target_combo.currentText()
                self.log(f"Settings updated: Preset={self.preset}, Generator={self.generator}, Build Type={self.build_type}, Target={self.target}")

        def show_new_project_dialog(self):
            """Show new project creation dialog"""
            dialog = NewProjectDialog(self)
            if dialog.exec() == QDialog.DialogCode.Accepted:
                project_data = dialog.get_project_data()
                if project_data["name"]:
                    self.create_new_project(project_data["name"], project_data["template"])
                else:
                    QMessageBox.warning(self, "Invalid Name", "Please enter a valid project name.")

        def create_new_project(self, name, template):
            """Create a new project based on template"""
            import json
            from pathlib import Path

            # Create project ID from name
            project_id = name.lower().replace(" ", "_").replace("-", "_")

            # Template configurations
            templates = {
                "cube": {
                    "config": {
                        "launcher": {
                            "name": name,
                            "description": f"3D {name} project based on cube demo template",
                            "enabled": True
                        },
                        "schema_version": 2,
                        "window": {
                            "title": name,
                            "size": {
                                "width": 1024,
                                "height": 768
                            },
                            "mouse_grab": {
                                "enabled": True,
                                "grab_on_click": True,
                                "release_on_escape": True,
                                "start_grabbed": False,
                                "hide_cursor": True,
                                "relative_mode": True,
                                "grab_mouse_button": "left",
                                "release_key": "escape"
                            }
                        },
                        "scripts": {
                            "entry": f"scripts/{project_id}_logic.lua",
                            "lua_debug": False
                        },
                        "paths": {
                            "project_root": "../",
                            "scripts": "scripts",
                            "shaders": "shaders"
                        },
                        "input": {
                            "bindings": {
                                "move_forward": "W",
                                "move_back": "S",
                                "move_left": "A",
                                "move_right": "D",
                                "fly_up": "Q",
                                "fly_down": "Z",
                                "jump": "Space",
                                "noclip_toggle": "N",
                                "music_toggle": "M"
                            }
                        }
                    },
                    "lua_script": f"""-- {name} Logic Script
-- Generated from cube demo template

local function init()
    -- Initialize your game here
    print("{name} initialized")
end

local function update(dt)
    -- Update game logic here
    -- dt is the time delta in seconds
end

local function render()
    -- Render your game here
end

local function shutdown()
    -- Cleanup resources here
    print("{name} shutdown")
end

-- Return the game interface
return {{
    init = init,
    update = update,
    render = render,
    shutdown = shutdown
}}
"""
                },
                "gui": {
                    "config": {
                        "launcher": {
                            "name": name,
                            "description": f"GUI {name} project based on GUI demo template",
                            "enabled": True
                        },
                        "window_width": 1024,
                        "window_height": 768,
                        "lua_script": f"scripts/{project_id}_gui.lua",
                        "scripts_directory": "scripts",
                        "project_root": "../",
                        "shaders_directory": "shaders",
                        "bgfx": {
                            "renderer": "vulkan"
                        },
                        "mouse_grab": {
                            "enabled": False
                        }
                    },
                    "lua_script": f"""-- {name} GUI Script
-- Generated from GUI demo template

local function init()
    -- Initialize ImGui interface here
    print("{name} GUI initialized")
end

local function update(dt)
    -- Update GUI logic here
end

local function render()
    -- Render ImGui interface here
    if imgui_begin then
        imgui_begin("{name}")

        imgui_text("Welcome to {name}!")

        if imgui_button then
            if imgui_button("Click me!") then
                print("Button clicked!")
            end
        end

        imgui_end()
    end
end

local function shutdown()
    -- Cleanup GUI resources here
    print("{name} GUI shutdown")
end

-- Return the GUI interface
return {{
    init = init,
    update = update,
    render = render,
    shutdown = shutdown
}}
"""
                },
                "soundboard": {
                    "config": {
                        "launcher": {
                            "name": name,
                            "description": f"Soundboard {name} project based on soundboard template",
                            "enabled": True
                        },
                        "window_width": 1024,
                        "window_height": 768,
                        "lua_script": f"scripts/{project_id}_soundboard.lua",
                        "scripts_directory": "scripts",
                        "project_root": "../",
                        "shaders_directory": "shaders",
                        "bgfx": {
                            "renderer": "vulkan"
                        },
                        "config_file": f"config/{project_id}_runtime.json",
                        "mouse_grab": {
                            "enabled": False
                        }
                    },
                    "lua_script": f"""-- {name} Soundboard Script
-- Generated from soundboard template

local sounds = {{
    -- Add your sound files here
    -- "sound1.ogg",
    -- "sound2.ogg"
}}

local function init()
    -- Initialize soundboard here
    print("{name} soundboard initialized")
end

local function update(dt)
    -- Update soundboard logic here
end

local function render()
    -- Render soundboard interface here
    if imgui_begin then
        imgui_begin("{name} Soundboard")

        imgui_text("Soundboard Controls")

        for i, sound in ipairs(sounds) do
            if imgui_button then
                if imgui_button("Play " .. sound) then
                    if audio_play_sound then
                        audio_play_sound(sound)
                    end
                end
            end
        end

        imgui_end()
    end
end

local function shutdown()
    -- Cleanup soundboard resources here
    print("{name} soundboard shutdown")
end

-- Return the soundboard interface
return {{
    init = init,
    update = update,
    render = render,
    shutdown = shutdown
}}
"""
                }
            }

            try:
                # Create config file
                config_path = Path("config") / f"{project_id}_runtime.json"
                with open(config_path, 'w') as f:
                    json.dump(templates[template]["config"], f, indent=2)

                # Create Lua script file
                script_path = Path("scripts") / f"{project_id}_{template}.lua"
                with open(script_path, 'w') as f:
                    f.write(templates[template]["lua_script"])

                self.log(f"Created new project '{name}' based on {template} template")
                self.log(f"Config: {config_path}")
                self.log(f"Script: {script_path}")

                # Refresh the games list
                self.games = self.load_games_from_config()
                self.game_list.clear()
                for game in self.games:
                    item = QListWidgetItem(game["name"])
                    item.setData(Qt.ItemDataRole.UserRole, game)
                    self.game_list.addItem(item)

                QMessageBox.information(self, "Project Created",
                    f"New project '{name}' has been created successfully!\n\n"
                    f"Config: {config_path}\n"
                    f"Script: {script_path}")

            except Exception as e:
                QMessageBox.critical(self, "Error", f"Failed to create project: {str(e)}")

        def on_game_selected(self, current, previous):
            """Handle game selection from library"""
            if current:
                game = current.data(Qt.ItemDataRole.UserRole)
                self.current_game = game
                self.game_title.setText(game["name"])
                self.game_description.setText(game["description"])
                self.play_btn.setEnabled(True)

                # Load the Lua script for this game
                self.load_lua_script()
            else:
                self.current_game = None
                self.game_title.setText("Select a game")
                self.game_description.setText("")
                self.play_btn.setEnabled(False)
                self.lua_editor.clear()
                self.lua_file_label.setText("No file loaded")
                self.save_lua_btn.setEnabled(False)
                self.reload_lua_btn.setEnabled(False)

        def load_lua_script(self):
            """Load the Lua script associated with the current game"""
            if not self.current_game:
                return

            import json

            try:
                # Read the config file to get the lua_script path
                config_file = self.current_game.get("config_file", "")
                with open(config_file, 'r') as f:
                    config_data = json.load(f)

                lua_script_path = config_data.get("lua_script", "")
                if not lua_script_path:
                    scripts_config = config_data.get("scripts", {})
                    if isinstance(scripts_config, dict):
                        lua_script_path = scripts_config.get("entry", "")
                if not lua_script_path:
                    self.lua_editor.setPlainText("# No Lua script specified in config")
                    self.lua_file_label.setText("No Lua script found")
                    self.save_lua_btn.setEnabled(False)
                    self.reload_lua_btn.setEnabled(False)
                    return

                # Load the Lua script
                lua_file = Path(lua_script_path)
                if lua_file.exists():
                    with open(lua_file, 'r') as f:
                        content = f.read()

                    # Block signals to prevent marking as modified
                    self.lua_editor.blockSignals(True)
                    self.lua_editor.setPlainText(content)
                    self.lua_editor.blockSignals(False)

                    self.lua_file_label.setText(str(lua_file))
                    self.current_lua_file = lua_file
                    self.lua_modified = False
                    self.save_lua_btn.setEnabled(False)
                    self.reload_lua_btn.setEnabled(True)
                else:
                    self.lua_editor.setPlainText(f"# Lua script not found: {lua_script_path}")
                    self.lua_file_label.setText(f"File not found: {lua_script_path}")
                    self.save_lua_btn.setEnabled(False)
                    self.reload_lua_btn.setEnabled(False)
            except Exception as e:
                self.lua_editor.setPlainText(f"# Error loading Lua script: {e}")
                self.lua_file_label.setText("Error loading script")
                self.save_lua_btn.setEnabled(False)
                self.reload_lua_btn.setEnabled(False)

        def on_lua_text_changed(self):
            """Handle Lua editor text changes"""
            if hasattr(self, 'current_lua_file'):
                self.lua_modified = True
                self.save_lua_btn.setEnabled(True)

        def save_lua_script(self):
            """Save the current Lua script"""
            if not hasattr(self, 'current_lua_file'):
                return

            try:
                with open(self.current_lua_file, 'w') as f:
                    f.write(self.lua_editor.toPlainText())

                self.lua_modified = False
                self.save_lua_btn.setEnabled(False)
                self.status_label.setText(f"✓ Saved {self.current_lua_file}")
                self.status_label.setStyleSheet("color: #4caf50; font-size: 9pt;")
                QTimer.singleShot(3000, lambda: self.status_label.setText(""))
            except Exception as e:
                self.status_label.setText(f"❌ Error saving {self.current_lua_file}: {e}")
                self.status_label.setStyleSheet("color: #ff6b6b; font-size: 9pt;")
                QTimer.singleShot(5000, lambda: self.status_label.setText(""))

        def reload_lua_script(self):
            """Reload the Lua script from disk"""
            if self.lua_modified:
                # Warn user about unsaved changes
                from PyQt6.QtWidgets import QMessageBox
                reply = QMessageBox.question(
                    self, 'Unsaved Changes',
                    'You have unsaved changes. Reload anyway?',
                    QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
                    QMessageBox.StandardButton.No
                )
                if reply == QMessageBox.StandardButton.No:
                    return

            self.load_lua_script()

        def search_in_editor(self, forward=True):
            """Search for text in the Lua editor"""
            text = self.search_edit.text()
            if text:
                flags = QTextDocument.FindFlag(0)
                if not forward:
                    flags |= QTextDocument.FindFlag.FindBackward
                
                found = self.lua_editor.find(text, flags)
                if not found:
                    # Wrap around
                    cursor = self.lua_editor.textCursor()
                    if forward:
                        cursor.movePosition(QTextCursor.MoveOperation.Start)
                    else:
                        cursor.movePosition(QTextCursor.MoveOperation.End)
                    self.lua_editor.setTextCursor(cursor)
                    found = self.lua_editor.find(text, flags)
                    if not found:
                        self.status_label.setText(f"No matches for '{text}'")
                        self.status_label.setStyleSheet("color: #ff6b6b; font-size: 9pt;")
                        QTimer.singleShot(3000, lambda: self.status_label.setText(""))

        def copy_console(self):
            """Copy console output to clipboard"""
            from PyQt6.QtWidgets import QApplication
            clipboard = QApplication.clipboard()
            selected_text = self.console.textCursor().selectedText()
            if selected_text:
                clipboard.setText(selected_text)
            else:
                # If no selection, copy all text
                clipboard.setText(self.console.toPlainText())

        def play_game(self):
            """Launch the selected game"""
            if not self.current_game:
                return

            build_dir = GENERATOR_DEFAULT_DIR.get(self.generator, DEFAULT_BUILD_DIR)

            # Build the command to run the demo with the specific config file
            exe_name = "sdl3_app.exe" if IS_WINDOWS else "sdl3_app"
            binary = str(Path(build_dir) / exe_name)
            config_file = self.current_game.get("config_file", "config/seed_runtime.json")

            cmd = [binary, "-j", config_file]
            self.run_command(cmd)

        def stop_process(self):
            """Stop the running process"""
            if self.process and self.process.state() == QProcess.ProcessState.Running:
                self.log("\n⏸ Stopping process...")
                self.process.kill()
                self.process.waitForFinished(3000)

        def set_dark_theme(self):
            """Apply a dark theme similar to Steam"""
            palette = QPalette()
            palette.setColor(QPalette.ColorRole.Window, QColor(27, 40, 56))
            palette.setColor(QPalette.ColorRole.WindowText, QColor(198, 209, 219))
            palette.setColor(QPalette.ColorRole.Base, QColor(35, 47, 62))
            palette.setColor(QPalette.ColorRole.AlternateBase, QColor(27, 40, 56))
            palette.setColor(QPalette.ColorRole.Text, QColor(198, 209, 219))
            palette.setColor(QPalette.ColorRole.Button, QColor(42, 71, 94))
            palette.setColor(QPalette.ColorRole.ButtonText, QColor(198, 209, 219))
            palette.setColor(QPalette.ColorRole.Highlight, QColor(91, 124, 153))
            palette.setColor(QPalette.ColorRole.HighlightedText, QColor(255, 255, 255))
            self.setPalette(palette)

        def log(self, message: str):
            """Add a message to the console"""
            self.console.append(message)
            # Auto-scroll to bottom
            self.console.verticalScrollBar().setValue(
                self.console.verticalScrollBar().maximum()
            )

        def run_command(self, args: list[str], env_overrides: dict[str, str] | None = None):
            """Execute a command using QProcess"""
            if self.process and self.process.state() == QProcess.ProcessState.Running:
                self.log("⚠️  A process is already running. Stop it first.")
                return

            self.console.clear()
            self.log(f"▶ Running: {' '.join(args)}\n")

            self.process = QProcess(self)
            if env_overrides:
                env = QProcessEnvironment.systemEnvironment()
                for key, value in env_overrides.items():
                    env.insert(key, value)
                self.process.setProcessEnvironment(env)
            self.process.readyReadStandardOutput.connect(self.handle_stdout)
            self.process.readyReadStandardError.connect(self.handle_stderr)
            self.process.finished.connect(self.process_finished)
            self.process.start(args[0], args[1:])

            self.play_btn.setEnabled(False)
            self.stop_btn.setEnabled(True)

        def handle_stdout(self):
            """Handle stdout from the process"""
            if self.process:
                data = self.process.readAllStandardOutput()
                text = bytes(data).decode('utf-8', errors='replace')
                self.log(text)

        def handle_stderr(self):
            """Handle stderr from the process"""
            if self.process:
                data = self.process.readAllStandardError()
                text = bytes(data).decode('utf-8', errors='replace')
                self.log(text)

        def process_finished(self, exit_code: int, exit_status):
            """Handle process completion"""
            if exit_code == 0:
                self.log(f"\n✓ Process completed successfully")
            else:
                self.log(f"\n❌ Process exited with code {exit_code}")

            self.stop_btn.setEnabled(False)
            if self.current_game:
                self.play_btn.setEnabled(True)

        def run_dependencies(self):
            """Run conan dependencies installation"""
            cmd = [sys.executable, __file__, "dependencies"]
            vita_env = None
            if self.preset == "vita-release":
                cmd.extend(["--conan-install-args", "--profile", "profiles/vita"])
                vita_env = _vita_env_overrides(f"preset {self.preset}")
            self.run_command(cmd, env_overrides=vita_env)

        def run_configure(self):
            """Run CMake configuration"""
            cmd = [sys.executable, __file__, "configure"]
            vita_env = None
            if self.preset != "default":
                cmd.extend(["--preset", self.preset])
                vita_env = _resolve_vita_env_for_configure(self.preset, None)
            else:
                cmd.extend([
                    "--generator", self.generator,
                    "--build-type", self.build_type
                ])
            self.run_command(cmd, env_overrides=vita_env)

        def run_build(self):
            """Run build command"""
            if self.preset != "default":
                build_dir = f"build-{self.preset.split('-')[0]}"  # e.g., build-vita
            else:
                build_dir = GENERATOR_DEFAULT_DIR.get(self.generator, DEFAULT_BUILD_DIR)
            cmd = [
                sys.executable, __file__, "build",
                "--build-dir", build_dir,
                "--target", self.target
            ]
            vita_env = None
            if self.preset in VITA_PRESETS:
                vita_env = _vita_env_overrides(f"preset {self.preset}")
            else:
                vita_env = _resolve_vita_env_for_build(build_dir)
            self.run_command(cmd, env_overrides=vita_env)

        def run_tests(self):
            """Build (optional) and run tests"""
            if self.preset != "default":
                build_dir = f"build-{self.preset.split('-')[0]}"  # e.g., build-vita
            else:
                build_dir = GENERATOR_DEFAULT_DIR.get(self.generator, DEFAULT_BUILD_DIR)
            cmd = [
                sys.executable, __file__, "tests",
                "--build-dir", build_dir,
                "--config", self.build_type,
                "--target", "all"
            ]
            vita_env = None
            if self.preset in VITA_PRESETS:
                vita_env = _vita_env_overrides(f"preset {self.preset}")
            else:
                vita_env = _resolve_vita_env_for_build(build_dir)
            self.run_command(cmd, env_overrides=vita_env)

        def sync_assets(self):
            """Sync assets into the active build directory"""
            if self.preset != "default":
                build_dir = f"build-{self.preset.split('-')[0]}"  # e.g., build-vita
            else:
                build_dir = GENERATOR_DEFAULT_DIR.get(self.generator, DEFAULT_BUILD_DIR)
            self.console.clear()
            self.log("=== Syncing Assets ===\n")
            _sync_assets(build_dir, dry_run=False)
            self.log("\n✓ Asset sync completed")

    app = QApplication(sys.argv)
    window = BuildLauncherGUI()
    window.show()
    sys.exit(app.exec())


def main() -> int:
    parser = argparse.ArgumentParser(description="Run build helper commands.")
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="print commands without executing them",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)
    deps = subparsers.add_parser("dependencies", help="run Conan setup from README")
    deps.add_argument(
        "--conan-install-args",
        nargs=argparse.REMAINDER,
        help=(
            "extra arguments forwarded to `conan install` "
            "(prefix with '--' before conan flags if needed)"
        ),
    )
    deps.set_defaults(func=dependencies)
    conf = subparsers.add_parser("configure", help="configure CMake project")
    conf.add_argument(
        "--preset",
        help="use a CMake preset instead of manual configuration",
    )
    conf.add_argument(
        "--generator",
        choices=["vs", "ninja", "ninja-msvc"],
        help=(
            "which generator to invoke (default: Ninja+MSVC on Windows, Ninja elsewhere)"
        ),
    )
    conf.add_argument("--build-dir", help="override the directory where CMake writes build files")
    conf.add_argument(
        "--build-type",
        default="Release",
        help="single-config builds need an explicit CMAKE_BUILD_TYPE",
    )
    conf.add_argument(
        "--cmake-args",
        nargs=argparse.REMAINDER,
        help=(
            "extra arguments forwarded to `cmake` configure step "
            "(prefix with '--' before cmake flags if needed)"
        ),
    )
    conf.set_defaults(func=configure)
    bld = subparsers.add_parser("build", help="run cmake --build")
    bld.add_argument(
        "--build-dir", default=DEFAULT_BUILD_DIR, help="which directory to build"
    )
    bld.add_argument(
        "--config", default="Release", help="configuration for multi-config generators"
    )
    bld.add_argument(
        "--target",
        default="sdl3_app",
        help="target to build (e.g. sdl3_app, spinning_cube)",
    )
    bld.add_argument(
        "--build-tool-args",
        nargs=argparse.REMAINDER,
        help=(
            "extra args forwarded to the underlying build tool after `--` "
            "(prefix with '--' before the tool args if needed)"
        ),
    )
    bld.set_defaults(func=build)
    tst = subparsers.add_parser("tests", help="build (optional) and run ctest")
    tst.add_argument(
        "--build-dir", default=DEFAULT_BUILD_DIR, help="which directory to test"
    )
    tst.add_argument(
        "--config", default="Release", help="configuration for multi-config generators"
    )
    tst.add_argument(
        "--target",
        default="all",
        help="target to build before tests (use --no-build to skip)",
    )
    tst.add_argument(
        "--no-build",
        action="store_true",
        help="skip build step and only run tests",
    )
    tst.add_argument(
        "--build-tool-args",
        nargs=argparse.REMAINDER,
        help=(
            "extra args forwarded to the underlying build tool after `--` "
            "(prefix with '--' before the tool args if needed)"
        ),
    )
    tst.add_argument(
        "--ctest-args",
        nargs=argparse.REMAINDER,
        help=(
            "extra arguments forwarded to ctest "
            "(prefix with '--' before ctest flags if needed)"
        ),
    )
    tst.set_defaults(func=tests, build_first=True)
    msvc = subparsers.add_parser(
        "msvc-quick", help="run a VS env setup + follow-on command (README one-liner style)"
    )
    msvc.add_argument("--bat-path", help="full path to vcvarsall.bat")
    msvc.add_argument(
        "--arch", default="x64", help="architecture argument passed to vcvarsall.bat"
    )
    msvc.add_argument(
        "--build-dir",
        default=DEFAULT_BUILD_DIR,
        help="build directory (used by default follow-on build command)",
    )
    msvc.add_argument(
        "--config", default="Release", help="configuration for multi-config generators"
    )
    msvc.add_argument(
        "--target",
        default="sdl3_app",
        help="target to build (used by default follow-on build)",
    )
    msvc.add_argument(
        "--build-tool-args",
        nargs=argparse.REMAINDER,
        help=(
            "extra args forwarded to the underlying build tool after `--` "
            "when using the default follow-on build"
        ),
    )
    msvc.add_argument(
        "then_command",
        nargs=argparse.REMAINDER,
        help=(
            "optional command to run after vcvarsall (overrides default build). "
            "Example: msvc-quick -- cmake -B build -S ."
        ),
    )
    msvc.set_defaults(func=msvc_quick)
    runp = subparsers.add_parser(
        "run", help="execute a built binary from the build folder"
    )
    runp.add_argument("--build-dir", help="where the binary lives")
    runp.add_argument(
        "--target",
        help="executable name to run (defaults to `sdl3_app[.exe]`)",
    )
    runp.add_argument(
        "--no-sync",
        action="store_true",
        help="skip asset syncing before running",
    )
    runp.add_argument(
        "args",
        nargs=argparse.REMAINDER,
        help=(
            "arguments forwarded to the executable "
            "(prefix with '--' before positional args when needed)"
        ),
    )
    runp.set_defaults(func=run_demo)
    guip = subparsers.add_parser(
        "gui", help="launch PyQt6 GUI launcher (Steam-like interface)"
    )
    guip.set_defaults(func=gui)
    args = parser.parse_args()
    if hasattr(args, "no_build") and args.no_build:
        args.build_first = False
    try:
        args.func(args)
    except subprocess.CalledProcessError as exc:
        return int(exc.returncode)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
