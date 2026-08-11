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
import sys
from pathlib import Path
from typing import Iterable, Sequence

IS_WINDOWS = platform.system() == "Windows"


def _conan_cmd() -> list[str]:
    """Return the command prefix for invoking Conan.

    Searches for the ``conan`` executable using several strategies so the
    script works regardless of how Python/Conan were installed.

    **Windows coverage**:

    - Windows Store Python (``pip install --user``) — ``nt_user`` sysconfig
    - python.org installer (global or per-user) — interpreter scripts dir
    - python.org / Chocolatey ``pip install --user`` — ``%APPDATA%\\Python``
    - Chocolatey (global) — typically on PATH or interpreter scripts dir

    **macOS coverage**:

    - Homebrew Python — on PATH (``/opt/homebrew/bin`` or ``/usr/local/bin``)
    - System Python ``pip install --user`` — ``~/Library/Python/X.Y/bin``
    - pyenv / conda / virtualenv — on PATH when activated, scripts dir otherwise

    **Linux coverage**:

    - apt / dnf system Python — on PATH (``/usr/bin`` or ``/usr/local/bin``)
    - ``pip install --user`` — ``~/.local/bin``
    - pyenv / conda / virtualenv — on PATH when activated, scripts dir otherwise

    Strategy order:

    1. ``shutil.which`` — anything already on PATH.
    2. Python-adjacent Scripts/bin dirs — interpreter-level, user-level,
       ``%APPDATA%\\Python`` (Windows), ``~/Library/Python`` (macOS),
       ``~/.local/bin`` (Linux), and site-packages siblings.
    3. ``sys.executable -m conans.conan`` — package importable but no script.

    Raises ``FileNotFoundError`` with install instructions if nothing works.
    """
    import shutil
    import sysconfig
    import site

    # 1. Already on PATH (system install, brew, pyenv, conda, activated venv,
    #    choco with PATH, apt/dnf)
    if shutil.which("conan"):
        return ["conan"]

    # 2. Search Scripts/bin dirs adjacent to the running interpreter
    exe_name = "conan.exe" if IS_WINDOWS else "conan"
    bin_dir = "Scripts" if IS_WINDOWS else "bin"
    search_dirs: list[str] = []

    # Interpreter-level scripts (venv/Scripts, C:\PythonXXX\Scripts,
    # C:\Users\X\AppData\Local\Programs\Python\PythonXXX\Scripts,
    # /usr/local/bin, /opt/homebrew/bin)
    interp_scripts = sysconfig.get_path("scripts")
    if interp_scripts:
        search_dirs.append(interp_scripts)

    # User-level scripts via sysconfig (Windows Store Python, ~/.local/bin)
    scheme = "nt_user" if IS_WINDOWS else "posix_user"
    try:
        user_scripts = sysconfig.get_path("scripts", scheme)
        if user_scripts:
            search_dirs.append(user_scripts)
    except KeyError:
        pass

    if IS_WINDOWS:
        # python.org / Chocolatey "pip install --user" puts scripts under
        # %APPDATA%\Python\PythonXYZ\Scripts — not covered by nt_user when
        # running from Windows Store Python
        appdata = os.environ.get("APPDATA", "")
        if appdata:
            ver = f"Python{sys.version_info.major}{sys.version_info.minor}"
            search_dirs.append(os.path.join(appdata, "Python", ver, "Scripts"))
            # Also check unversioned (older pip layouts)
            search_dirs.append(os.path.join(appdata, "Python", "Scripts"))
    else:
        # macOS system Python: ~/Library/Python/X.Y/bin
        if platform.system() == "Darwin":
            ver = f"{sys.version_info.major}.{sys.version_info.minor}"
            search_dirs.append(
                os.path.expanduser(f"~/Library/Python/{ver}/bin")
            )
        # Linux/macOS: ~/.local/bin (pip install --user default)
        search_dirs.append(os.path.expanduser("~/.local/bin"))

    # Site-packages sibling dirs (covers additional layouts)
    try:
        for sp in site.getsitepackages():
            search_dirs.append(os.path.join(os.path.dirname(sp), bin_dir))
    except AttributeError:
        pass

    for d in dict.fromkeys(search_dirs):  # dedupe, preserve order
        candidate = os.path.join(d, exe_name)
        if os.path.isfile(candidate):
            return [candidate]

    # 3. Module invocation (conan is importable but no script on disk)
    try:
        from importlib.metadata import distribution
        distribution("conan")  # raises PackageNotFoundError if absent
        return [sys.executable, "-m", "conans.conan"]
    except Exception:
        pass

    raise FileNotFoundError(
        "Could not find Conan. Install it with:  pip install conan"
    )


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

def _find_vcvarsall() -> str:
    """Auto-detect vcvarsall.bat across VS editions and versions."""
    if not IS_WINDOWS:
        return ""
    base = "C:\\Program Files\\Microsoft Visual Studio"
    # Search newest VS version first, then editions
    for version in ["18", "2022", "2019"]:
        for edition in ["Community", "Professional", "Enterprise", "BuildTools"]:
            bat = f"{base}\\{version}\\{edition}\\VC\\Auxiliary\\Build\\vcvarsall.bat"
            if os.path.isfile(bat):
                return bat
    return ""


DEFAULT_VCVARSALL = _find_vcvarsall()

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
    """Run Conan profile detection and install dependencies with C++20."""
    conan = _conan_cmd()
    cmd_detect = [*conan, "profile", "detect", "-f"]
    cmd_install = [*conan, "install", ".", "-of", "build-ninja", "-b", "missing",
                   "-s", "compiler.cppstd=20"]
    conan_install_args = _strip_leading_double_dash(args.conan_install_args)
    if conan_install_args:
        cmd_install.extend(conan_install_args)
    run_argvs([cmd_detect, cmd_install], args.dry_run)


def generate(args: argparse.Namespace) -> None:
    """Generate CMakeLists.txt from cmake_config.json using Jinja2."""
    env = {"PYTHONIOENCODING": "utf-8"}
    cmd = [
        "python", "generate_cmake.py",
        "--config", args.config,
        "--output", args.output,
    ]
    if args.template:
        cmd.extend(["--template", args.template])
    if args.validate:
        cmd.append("--validate")
    run_argvs([cmd], args.dry_run, env_overrides=env)

    # Fix CMakeUserPresets.json to only include existing preset files
    if not args.dry_run and not args.validate:
        _fix_cmake_user_presets()


def _fix_cmake_user_presets() -> None:
    """Ensure CMakeUserPresets.json only includes existing, non-conflicting preset files.

    Conan regenerates preset files when the build layout changes (e.g.
    ``build-ninja/build/generators/`` vs ``build-ninja/build/Release/generators/``).
    If the old file still exists, CMake fails with "Duplicate preset".  This
    function removes missing includes *and* detects preset name collisions,
    keeping only the newest file when duplicates are found.
    """
    import json as json_mod
    presets_path = Path("CMakeUserPresets.json")
    if not presets_path.exists():
        return
    try:
        data = json_mod.loads(presets_path.read_text())
        includes = data.get("include", [])

        # Drop missing files
        valid = [p for p in includes if Path(p).exists()]

        # Detect and resolve duplicate preset names across included files
        seen_names: dict[str, str] = {}  # preset_name -> include_path
        duplicates: set[str] = set()
        for inc_path in valid:
            try:
                inc_data = json_mod.loads(Path(inc_path).read_text())
            except (json_mod.JSONDecodeError, OSError):
                continue
            for key in ("configurePresets", "buildPresets", "testPresets"):
                for preset in inc_data.get(key, []):
                    name = preset.get("name", "")
                    if name in seen_names and seen_names[name] != inc_path:
                        # Keep the newer file, drop the older one
                        older = seen_names[name]
                        newer = inc_path
                        if Path(older).stat().st_mtime > Path(newer).stat().st_mtime:
                            older, newer = newer, older
                        duplicates.add(older)
                        seen_names[name] = newer
                    else:
                        seen_names[name] = inc_path

        deduped = [p for p in valid if p not in duplicates]

        if deduped != includes:
            data["include"] = deduped
            presets_path.write_text(json_mod.dumps(data, indent=4) + "\n")
            removed = len(includes) - len(deduped)
            print(f"  Fixed CMakeUserPresets.json: removed {removed} stale/duplicate include(s)")
    except (json_mod.JSONDecodeError, OSError):
        pass


def full_build(args: argparse.Namespace) -> None:
    """Run the full build pipeline: dependencies + generate + configure + build."""
    print("=== Step 1/4: Installing dependencies ===")
    deps_args = argparse.Namespace(
        dry_run=args.dry_run,
        conan_install_args=None,
    )
    dependencies(deps_args)

    print("\n=== Step 2/4: Generating CMakeLists.txt ===")
    gen_args = argparse.Namespace(
        dry_run=args.dry_run,
        config="cmake_config.json",
        template=None,
        output="CMakeLists.txt",
        validate=False,
    )
    generate(gen_args)

    print("\n=== Step 3/4: Configuring CMake ===")
    conf_args = argparse.Namespace(
        dry_run=args.dry_run,
        preset="conan-default",
        generator=None,
        build_dir=None,
        build_type=args.build_type,
        cmake_args=["-DBUILD_SDL3_APP=ON", "-DSDL_VERSION=SDL3"],
    )
    configure(conf_args)

    print("\n=== Step 4/4: Building ===")
    bld_args = argparse.Namespace(
        dry_run=args.dry_run,
        build_dir="build-ninja/build",
        config=args.build_type,
        target=args.target,
        build_tool_args=None,
    )
    build(bld_args)

    if args.run:
        print("\n=== Running ===")
        run_args = argparse.Namespace(
            dry_run=args.dry_run,
            build_dir="build-ninja/build/" + args.build_type,
            target=None,
            no_sync=False,
            args=["--bootstrap", args.bootstrap, "--game", args.game],
        )
        run_demo(run_args)


def configure(args: argparse.Namespace) -> None:
    """Configure a CMake project based on the chosen generator and options."""
    if args.preset:
        cmake_args = ["cmake", "--preset", args.preset]
        cmake_extra_args = _strip_leading_double_dash(args.cmake_args)
        if cmake_extra_args:
            cmake_args.extend(cmake_extra_args)
        run_argvs([cmake_args], args.dry_run)
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
    run_argvs([cmake_args], args.dry_run)


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
    run_argvs([cmd], args.dry_run)


def tests(args: argparse.Namespace) -> None:
    """Build (optional) and run ctest for a given build directory."""
    build_dir = _as_build_dir(args.build_dir, DEFAULT_BUILD_DIR)
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

    run_argvs(argvs, args.dry_run)


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
    Sync asset files (packages/, shaders, workflows, MaterialX) from the project
    root to the build directory before running the application.  Uses copytree
    so subdirectories and binary assets (*.spv, *.dxil, textures) are included.
    """
    import shutil

    build_path = Path(build_dir)
    project_root = Path(".")

    asset_trees = [
        "packages",
        "MaterialX/libraries",
        "MaterialX/resources",
    ]

    print("\n=== Syncing Assets ===")

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

    # Detect Steam-owned game data and export as env vars for ${env:VAR}
    # substitution in workflow JSON. Silent no-op if Steam isn't installed.
    try:
        from steam_detector import detect_and_export
        detected = detect_and_export()
        if detected:
            print("=== Detected game data ===")
            for k, v in detected.items():
                print(f"  {k}={v}")
                os.environ[k] = v
            print()
    except NotImplementedError as e:
        print(f"[steam_detector] {e}")
    except Exception as e:
        print(f"[steam_detector] detection skipped: {e}")

    exe_name = args.target or ("sdl3_app.exe" if IS_WINDOWS else "sdl3_app")
    binary = str(Path(build_dir).resolve() / exe_name)
    run_args = _strip_leading_double_dash(args.args)
    cmd: list[str] = [binary]
    if run_args:
        cmd.extend(run_args)
    _print_cmd(cmd)
    os.chdir(build_dir)
    if IS_WINDOWS:
        # os.execv is buggy on Windows with paths containing spaces;
        # use subprocess and propagate the exit code instead.
        import subprocess
        sys.exit(subprocess.call(cmd))
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
            QDialog, QDialogButtonBox, QFormLayout, QMessageBox,
        )
        from PyQt6.QtCore import Qt, QProcess, QProcessEnvironment
        from PyQt6.QtGui import QFont, QPalette, QColor, QAction
    except ImportError:
        raise SystemExit(
            "PyQt6 is not installed. Install it with:\n"
            "  pip install PyQt6"
        )

    import sys

    class BuildSettingsDialog(QDialog):
        """Dialog for configuring build settings"""
        def __init__(self, parent=None):
            super().__init__(parent)
            self.setWindowTitle("Build Settings")
            self.setMinimumWidth(400)

            layout = QFormLayout(self)

            self.preset_combo = QComboBox()
            self.preset_combo.addItems(["default"])
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
                "gpu_gui_service_tests",
            ])
            layout.addRow("Target:", self.target_combo)

            buttons = QDialogButtonBox(
                QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel
            )
            buttons.accepted.connect(self.accept)
            buttons.rejected.connect(self.reject)
            layout.addRow(buttons)

    class BuildLauncherGUI(QMainWindow):
        def __init__(self):
            super().__init__()
            self.process = None
            self.current_game = None
            self.current_bootloader = None
            self.current_game_package = None

            # Build settings
            self.preset = "default"
            self.generator = DEFAULT_GENERATOR
            self.build_type = "Release"
            self.target = "sdl3_app"

            # Load bootloader packages
            self.bootloaders = self.load_bootloader_packages()
            # Load game packages
            self.game_packages = self.load_game_packages()

            self.init_ui()

        def _project_root(self) -> "Path":
            return Path(__file__).resolve().parent.parent

        def _candidate_build_dirs(self) -> "list[Path]":
            """All plausible build directories, ordered newest-configured first."""
            root = self._project_root()
            build_types = ["Release", "Debug", "RelWithDebInfo", "MinSizeRel"]
            generator_dirs = list(GENERATOR_DEFAULT_DIR.values()) + ["build"]
            candidates: list[tuple[float, Path]] = []
            for gen_dir in dict.fromkeys(generator_dirs):
                base = root / gen_dir
                # Conan nested layout: <gen>/build/<type>/
                for bt in build_types:
                    nested = base / "build" / bt
                    cache = nested / "CMakeCache.txt"
                    if cache.is_file():
                        candidates.append((cache.stat().st_mtime, nested))
                # Flat layout: <gen>/
                cache = base / "CMakeCache.txt"
                if cache.is_file():
                    candidates.append((cache.stat().st_mtime, base))
            candidates.sort(key=lambda x: x[0], reverse=True)
            return [p for _, p in candidates]

        def _find_build_dir(self) -> "Path | None":
            """Return the most recently configured build directory."""
            candidates = self._candidate_build_dirs()
            return candidates[0] if candidates else None

        def _find_binary(self) -> "str | None":
            """Return the path to the most recently built sdl3_app binary, or None."""
            exe_name = "sdl3_app.exe" if IS_WINDOWS else "sdl3_app"
            best: tuple[float, str] | None = None
            for build_dir in self._candidate_build_dirs():
                exe = build_dir / exe_name
                if exe.is_file():
                    mtime = exe.stat().st_mtime
                    if best is None or mtime > best[0]:
                        best = (mtime, str(exe))
            return best[1] if best else None

        def load_bootloader_packages(self):
            """Load bootloader packages from packages/ directory"""
            import json
            from pathlib import Path

            bootloaders = []
            packages_dir = Path(__file__).resolve().parent.parent / "packages"

            if not packages_dir.exists():
                return []

            # Look for bootstrap_* packages
            for pkg_dir in sorted(packages_dir.glob("bootstrap_*")):
                if not pkg_dir.is_dir():
                    continue
                package_json = pkg_dir / "package.json"
                if not package_json.exists():
                    continue
                try:
                    with open(package_json, 'r') as f:
                        data = json.load(f)
                    pkg_type = data.get("type", "bootloader")
                    pkg_category = data.get("category", "")
                    display_name = data.get("name", pkg_dir.name)
                    if pkg_category:
                        display_name = f"{display_name} ({pkg_category})"
                    bootloaders.append({
                        "id": pkg_dir.name,
                        "name": display_name,
                        "description": data.get("description", ""),
                        "type": pkg_type,
                        "category": pkg_category,
                        "package_dir": str(pkg_dir),
                    })
                except (json.JSONDecodeError, IOError) as e:
                    print(f"Warning: Could not load bootloader {package_json}: {e}")
                    continue

            return bootloaders

        def load_game_packages(self):
            """Load game packages from packages/ directory (exclude bootstraps and libraries)"""
            import json
            from pathlib import Path

            games = []
            packages_dir = Path(__file__).resolve().parent.parent / "packages"

            if not packages_dir.exists():
                return []

            # Load all packages except bootstrap_* packages and libraries
            for pkg_dir in sorted(packages_dir.glob("*")):
                if not pkg_dir.is_dir():
                    continue
                if pkg_dir.name.startswith("bootstrap_"):
                    continue
                package_json = pkg_dir / "package.json"
                if not package_json.exists():
                    continue
                try:
                    with open(package_json, 'r') as f:
                        data = json.load(f)
                    pkg_type = data.get("type", "game")
                    # Only show game packages, not libraries
                    if pkg_type != "game":
                        continue
                    pkg_category = data.get("category", "")
                    display_name = data.get("name", pkg_dir.name)
                    if pkg_category:
                        display_name = f"{display_name} ({pkg_category})"
                    games.append({
                        "id": pkg_dir.name,
                        "name": display_name,
                        "description": data.get("description", ""),
                        "type": pkg_type,
                        "category": pkg_category,
                        "package_dir": str(pkg_dir),
                    })
                except (json.JSONDecodeError, IOError) as e:
                    print(f"Warning: Could not load game package {package_json}: {e}")
                    continue

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

            for game_pkg in self.game_packages:
                item = QListWidgetItem(game_pkg["name"])
                item.setData(Qt.ItemDataRole.UserRole, game_pkg)
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

            # Package selection lists side by side
            packages_row = QHBoxLayout()

            # Bootloader list
            bootloader_column = QVBoxLayout()
            bootloader_label = QLabel("BOOTLOADER")
            bootloader_label.setStyleSheet("color: #8f98a0; font-weight: bold; font-size: 9pt;")
            bootloader_column.addWidget(bootloader_label)

            list_style = """
                QListWidget {
                    background-color: #171a21;
                    border: 1px solid #0e1216;
                    border-radius: 3px;
                    color: #c6d1db;
                    font-size: 10pt;
                    outline: none;
                }
                QListWidget::item {
                    padding: 6px 8px;
                    border-bottom: 1px solid #0e1216;
                }
                QListWidget::item:selected {
                    background-color: #2a475e;
                }
                QListWidget::item:hover {
                    background-color: #1b2838;
                }
            """

            self.bootloader_list = QListWidget()
            self.bootloader_list.setStyleSheet(list_style)
            self.bootloader_list.setMaximumHeight(120)
            for bootloader in self.bootloaders:
                item = QListWidgetItem(bootloader["name"])
                item.setData(Qt.ItemDataRole.UserRole, bootloader)
                self.bootloader_list.addItem(item)
            if self.bootloaders:
                self.current_bootloader = self.bootloaders[0]
                self.bootloader_list.setCurrentRow(0)
            self.bootloader_list.currentItemChanged.connect(self.on_bootloader_selected)
            bootloader_column.addWidget(self.bootloader_list)
            packages_row.addLayout(bootloader_column)

            packages_row.addSpacing(10)

            # Game package list
            game_pkg_column = QVBoxLayout()
            game_pkg_label = QLabel("GAME PACKAGE")
            game_pkg_label.setStyleSheet("color: #8f98a0; font-weight: bold; font-size: 9pt;")
            game_pkg_column.addWidget(game_pkg_label)

            self.game_package_list = QListWidget()
            self.game_package_list.setStyleSheet(list_style)
            self.game_package_list.setMaximumHeight(120)
            for game_pkg in self.game_packages:
                item = QListWidgetItem(game_pkg["name"])
                item.setData(Qt.ItemDataRole.UserRole, game_pkg)
                self.game_package_list.addItem(item)
            if self.game_packages:
                self.current_game_package = self.game_packages[0]
                self.game_package_list.setCurrentRow(0)
            self.game_package_list.currentItemChanged.connect(self.on_game_package_selected)
            game_pkg_column.addWidget(self.game_package_list)
            packages_row.addLayout(game_pkg_column)

            detail_layout.addLayout(packages_row)

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

            # Console output panel
            console_container = QWidget()
            console_container.setStyleSheet("background-color: #1b2838;")
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

            self.copy_console_btn = QPushButton("Copy")
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

            self.clear_console_btn = QPushButton("Clear")
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

            right_layout.addWidget(console_container)

            main_layout.addWidget(right_panel, 1)

            # Create menu bar after all widgets are initialized
            self.create_menu_bar()

            # Select first game by default
            if self.game_packages:
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

        def on_bootloader_selected(self, current, previous):
            """Handle bootloader selection"""
            if current:
                self.current_bootloader = current.data(Qt.ItemDataRole.UserRole)
                self.log(f"Selected bootloader: {self.current_bootloader['name']}")

        def on_game_package_selected(self, current, previous):
            """Handle game package selection"""
            if current:
                self.current_game_package = current.data(Qt.ItemDataRole.UserRole)
                self.log(f"Selected game package: {self.current_game_package['name']}")

        def on_game_selected(self, current, previous):
            """Handle game selection from library"""
            if current:
                game = current.data(Qt.ItemDataRole.UserRole)
                self.current_game = game
                self.game_title.setText(game["name"])
                self.game_description.setText(game["description"])
                self.play_btn.setEnabled(True)
            else:
                self.current_game = None
                self.game_title.setText("Select a game")
                self.game_description.setText("")
                self.play_btn.setEnabled(False)

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
            """Launch the selected game with chosen bootloader and game package"""
            if not self.current_game:
                return

            binary = self._find_binary()
            if not binary:
                self.log("❌ Could not find sdl3_app binary. Build the project first (Developer → Build Project).")
                return

            self.log(f"Binary: {binary}")
            cmd = [binary]
            if self.current_bootloader:
                cmd.extend(["--bootstrap", self.current_bootloader["id"]])
            if self.current_game:
                cmd.extend(["--game", self.current_game["id"]])

            self.log(f"Bootloader: {self.current_bootloader.get('name', 'default') if self.current_bootloader else 'default'}")
            self.log(f"Game: {self.current_game_package.get('name', 'default') if self.current_game_package else 'default'}")
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
            self.run_command(cmd)

        def run_configure(self):
            """Run CMake configuration"""
            cmd = [sys.executable, __file__, "configure"]
            if self.preset != "default":
                cmd.extend(["--preset", self.preset])
            else:
                cmd.extend([
                    "--generator", self.generator,
                    "--build-type", self.build_type
                ])
            self.run_command(cmd)

        def run_build(self):
            """Run build command"""
            build_dir = self._find_build_dir()
            if not build_dir:
                # No configured build found — default to Conan nested layout for chosen generator
                root = self._project_root()
                build_dir = root / GENERATOR_DEFAULT_DIR.get(self.generator, DEFAULT_BUILD_DIR) / "build" / self.build_type
            cmd = [
                sys.executable, __file__, "build",
                "--build-dir", str(build_dir),
                "--target", self.target,
            ]
            self.run_command(cmd)

        def run_tests(self):
            """Build and run tests"""
            build_dir = self._find_build_dir()
            if not build_dir:
                root = self._project_root()
                build_dir = root / GENERATOR_DEFAULT_DIR.get(self.generator, DEFAULT_BUILD_DIR) / "build" / self.build_type
            cmd = [
                sys.executable, __file__, "tests",
                "--build-dir", str(build_dir),
                "--config", self.build_type,
                "--target", "all",
            ]
            self.run_command(cmd)

        def sync_assets(self):
            """Sync assets into the active build directory"""
            build_dir = self._find_build_dir()
            if not build_dir:
                self.log("⚠️  No configured build directory found. Run Configure CMake first.")
                return
            self.console.clear()
            self.log("=== Syncing Assets ===\n")
            _sync_assets(str(build_dir), dry_run=False)
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

    gen = subparsers.add_parser("generate", help="generate CMakeLists.txt from JSON config")
    gen.add_argument(
        "--config", default="cmake_config.json",
        help="path to cmake_config.json (default: cmake_config.json)",
    )
    gen.add_argument(
        "--template", default=None,
        help="path to Jinja2 template (default: CMakeLists.txt.jinja2)",
    )
    gen.add_argument(
        "--output", default="CMakeLists.txt",
        help="output CMakeLists.txt path (default: CMakeLists.txt)",
    )
    gen.add_argument(
        "--validate", action="store_true",
        help="validate config without generating",
    )
    gen.set_defaults(func=generate)

    allp = subparsers.add_parser(
        "all", help="full pipeline: dependencies + generate + configure + build [+ run]"
    )
    allp.add_argument(
        "--build-type", default="Release",
        help="build type (default: Release)",
    )
    allp.add_argument(
        "--target", default="sdl3_app",
        help="build target (default: sdl3_app)",
    )
    allp.add_argument(
        "--run", action="store_true",
        help="run the app after building",
    )
    allp.add_argument(
        "--bootstrap", default="bootstrap_windows" if IS_WINDOWS else "bootstrap_mac",
        help="bootstrap package (auto-detected from platform)",
    )
    allp.add_argument(
        "--game", default="seed",
        help="game package to run (default: seed)",
    )
    allp.set_defaults(func=full_build)

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
