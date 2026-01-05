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

DEFAULT_VCVARSALL = (
    "C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional"
    "\\VC\\Auxiliary\\Build\\vcvarsall.bat"
)


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


def _strip_leading_double_dash(args: Sequence[str] | None) -> list[str]:
    """Drop a leading `--` that argparse keeps with REMAINDER arguments."""
    if not args:
        return []
    args_list = list(args)
    if args_list and args_list[0] == "--":
        return args_list[1:]
    return args_list


def run_argvs(argvs: Iterable[Sequence[str]], dry_run: bool) -> None:
    """
    Run a sequence of commands represented as lists of arguments. Each command
    is printed before execution. If `dry_run` is True, commands are printed
    but not executed.
    """
    for argv in argvs:
        _print_cmd(argv)
        if dry_run:
            continue
        subprocess.run(list(argv), check=True)


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

    The default `conan install -of build` + `cmake_layout()` layout produces
    `build/build/<build_type>/generators/conan_toolchain.cmake`.
    """
    candidates = [
        Path("build") / "build" / build_type / "generators" / "conan_toolchain.cmake",
        Path("build") / build_type / "generators" / "conan_toolchain.cmake",
        Path("build") / "generators" / "conan_toolchain.cmake",
        Path("build") / "conan_toolchain.cmake",
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
    cmd_install = ["conan", "install", ".", "-of", "build", "-b", "missing"]
    conan_install_args = _strip_leading_double_dash(args.conan_install_args)
    if conan_install_args:
        cmd_install.extend(conan_install_args)
    run_argvs([cmd_detect, cmd_install], args.dry_run)


def configure(args: argparse.Namespace) -> None:
    """Configure a CMake project based on the chosen generator and options."""
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


def _compile_shaders(dry_run: bool) -> None:
    """
    Compile GLSL shaders to SPIR-V format using glslangValidator.
    Compiles all .vert and .frag files in the shaders directory.
    """
    shaders_dir = Path("shaders")
    if not shaders_dir.exists():
        return

    # Find shader compiler
    compiler = None
    for cmd in ["glslangValidator", "glslc"]:
        try:
            result = subprocess.run([cmd, "--version"],
                                   capture_output=True,
                                   timeout=5)
            if result.returncode == 0:
                compiler = cmd
                break
        except (FileNotFoundError, subprocess.TimeoutExpired):
            continue

    if not compiler:
        print("⚠️  No shader compiler found (glslangValidator or glslc)")
        print("   Skipping shader compilation")
        return

    print("\n=== Compiling Shaders ===")
    shader_files = list(shaders_dir.glob("*.vert")) + list(shaders_dir.glob("*.frag"))

    for shader_file in shader_files:
        output_file = shader_file.with_suffix(shader_file.suffix + ".spv")

        # Check if compilation is needed
        if output_file.exists():
            if output_file.stat().st_mtime >= shader_file.stat().st_mtime:
                continue  # Skip if .spv is newer than source

        print(f"  Compiling {shader_file.name} -> {output_file.name}")

        if not dry_run:
            if compiler == "glslangValidator":
                cmd = [compiler, "-V", str(shader_file), "-o", str(output_file)]
            else:  # glslc
                cmd = [compiler, str(shader_file), "-o", str(output_file)]

            result = subprocess.run(cmd, capture_output=True, text=True)
            if result.returncode != 0:
                print(f"    ❌ Failed: {result.stderr}")
            else:
                print(f"    ✓ Success")

    print("=== Shaders Compiled ===\n")


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
        ("shaders", ["*.spv"]),
        ("scripts/models", ["*.stl", "*.obj", "*.fbx"]),
        ("config", ["*.json"]),
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
                if src_file.is_file():
                    dst_file = dst_path / src_file.name
                    print(f"  {src_file} -> {dst_file}")
                    if not dry_run:
                        shutil.copy2(src_file, dst_file)

    print("=== Assets Synced ===\n")


def run_demo(args: argparse.Namespace) -> None:
    """
    Run a compiled demo application from the build directory. The default
    executable is `sdl3_app` (or `sdl3_app.exe` on Windows). Additional
    arguments can be passed to the executable after `--`.

    By default, compiles shaders and syncs asset files before running.
    Use --no-sync to skip shader compilation and asset synchronization.
    """
    build_dir = _as_build_dir(args.build_dir, DEFAULT_BUILD_DIR)

    # Compile shaders and sync assets unless --no-sync is specified
    if not args.no_sync:
        _compile_shaders(args.dry_run)
        _sync_assets(build_dir, args.dry_run)

    exe_name = args.target or ("sdl3_app.exe" if IS_WINDOWS else "sdl3_app")
    binary = str(Path(build_dir) / exe_name)
    cmd: list[str] = [binary, "-j", "config/seed_runtime.json"]
    run_args = _strip_leading_double_dash(args.args)
    if run_args:
        cmd.extend(run_args)
    run_argvs([cmd], args.dry_run)


def gui(args: argparse.Namespace) -> None:
    """
    Launch a PyQt6 GUI launcher similar to Steam's interface for managing builds
    and running demos.
    """
    try:
        from PyQt6.QtWidgets import (
            QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
            QPushButton, QLabel, QTextEdit, QComboBox, QCheckBox, QSplitter
        )
        from PyQt6.QtCore import Qt, QProcess, pyqtSignal
        from PyQt6.QtGui import QFont, QPalette, QColor
    except ImportError:
        raise SystemExit(
            "PyQt6 is not installed. Install it with:\n"
            "  pip install PyQt6"
        )

    import sys

    class BuildLauncherGUI(QMainWindow):
        def __init__(self):
            super().__init__()
            self.process = None
            self.init_ui()

        def init_ui(self):
            self.setWindowTitle("SDL3 C++ Build Launcher")
            self.setMinimumSize(900, 600)

            # Set dark theme similar to Steam
            self.set_dark_theme()

            # Central widget
            central_widget = QWidget()
            self.setCentralWidget(central_widget)
            main_layout = QVBoxLayout(central_widget)
            main_layout.setContentsMargins(10, 10, 10, 10)

            # Title
            title = QLabel("SDL3 C++ Project Manager")
            title_font = QFont()
            title_font.setPointSize(18)
            title_font.setBold(True)
            title.setFont(title_font)
            title.setAlignment(Qt.AlignmentFlag.AlignCenter)
            main_layout.addWidget(title)

            # Create splitter for controls and output
            splitter = QSplitter(Qt.Orientation.Vertical)

            # Control panel
            control_widget = QWidget()
            control_layout = QVBoxLayout(control_widget)

            # Build options
            build_group = QWidget()
            build_layout = QVBoxLayout(build_group)

            # Generator selection
            gen_layout = QHBoxLayout()
            gen_layout.addWidget(QLabel("Generator:"))
            self.generator_combo = QComboBox()
            self.generator_combo.addItems(["ninja", "ninja-msvc", "vs"])
            self.generator_combo.setCurrentText(DEFAULT_GENERATOR)
            gen_layout.addWidget(self.generator_combo)
            gen_layout.addStretch()
            build_layout.addLayout(gen_layout)

            # Build type selection
            type_layout = QHBoxLayout()
            type_layout.addWidget(QLabel("Build Type:"))
            self.build_type_combo = QComboBox()
            self.build_type_combo.addItems(["Release", "Debug", "RelWithDebInfo"])
            type_layout.addWidget(self.build_type_combo)
            type_layout.addStretch()
            build_layout.addLayout(type_layout)

            # Target selection
            target_layout = QHBoxLayout()
            target_layout.addWidget(QLabel("Target:"))
            self.target_combo = QComboBox()
            self.target_combo.addItems(["sdl3_app", "all"])
            target_layout.addWidget(self.target_combo)
            target_layout.addStretch()
            build_layout.addLayout(target_layout)

            control_layout.addWidget(build_group)

            # Action buttons in a grid layout
            buttons_widget = QWidget()
            buttons_layout = QVBoxLayout(buttons_widget)

            # Row 1: Dependencies and Configure
            row1 = QHBoxLayout()
            self.deps_btn = self.create_styled_button("Install Dependencies", "#2a475e")
            self.deps_btn.clicked.connect(self.run_dependencies)
            row1.addWidget(self.deps_btn)

            self.config_btn = self.create_styled_button("Configure", "#2a475e")
            self.config_btn.clicked.connect(self.run_configure)
            row1.addWidget(self.config_btn)
            buttons_layout.addLayout(row1)

            # Row 2: Build and Run
            row2 = QHBoxLayout()
            self.build_btn = self.create_styled_button("Build", "#3e6b3e")
            self.build_btn.clicked.connect(self.run_build)
            row2.addWidget(self.build_btn)

            self.run_btn = self.create_styled_button("▶ Run Demo", "#5b7c99")
            self.run_btn.clicked.connect(self.run_demo)
            row2.addWidget(self.run_btn)
            buttons_layout.addLayout(row2)

            # Row 3: Compile Shaders and Stop
            row3 = QHBoxLayout()
            self.shader_btn = self.create_styled_button("Compile Shaders", "#5e4a2a")
            self.shader_btn.clicked.connect(self.compile_shaders)
            row3.addWidget(self.shader_btn)

            self.stop_btn = self.create_styled_button("Stop Process", "#7e2a2a")
            self.stop_btn.clicked.connect(self.stop_process)
            self.stop_btn.setEnabled(False)
            row3.addWidget(self.stop_btn)
            buttons_layout.addLayout(row3)

            control_layout.addWidget(buttons_widget)
            control_layout.addStretch()

            splitter.addWidget(control_widget)

            # Output console
            self.console = QTextEdit()
            self.console.setReadOnly(True)
            self.console.setMinimumHeight(200)
            console_font = QFont("Courier New")
            console_font.setPointSize(9)
            self.console.setFont(console_font)
            splitter.addWidget(self.console)

            splitter.setStretchFactor(0, 1)
            splitter.setStretchFactor(1, 2)

            main_layout.addWidget(splitter)

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

        def create_styled_button(self, text: str, base_color: str) -> QPushButton:
            """Create a button with Steam-like styling"""
            btn = QPushButton(text)
            btn.setMinimumHeight(45)
            btn.setStyleSheet(f"""
                QPushButton {{
                    background-color: {base_color};
                    color: #c6d1db;
                    border: 1px solid #1b2838;
                    border-radius: 3px;
                    padding: 8px;
                    font-size: 12pt;
                    font-weight: bold;
                }}
                QPushButton:hover {{
                    background-color: {self.lighten_color(base_color)};
                }}
                QPushButton:pressed {{
                    background-color: {self.darken_color(base_color)};
                }}
                QPushButton:disabled {{
                    background-color: #1b2838;
                    color: #4e5a66;
                }}
            """)
            return btn

        @staticmethod
        def lighten_color(hex_color: str) -> str:
            """Lighten a hex color by 20%"""
            hex_color = hex_color.lstrip('#')
            r, g, b = tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))
            r, g, b = min(255, int(r * 1.2)), min(255, int(g * 1.2)), min(255, int(b * 1.2))
            return f"#{r:02x}{g:02x}{b:02x}"

        @staticmethod
        def darken_color(hex_color: str) -> str:
            """Darken a hex color by 20%"""
            hex_color = hex_color.lstrip('#')
            r, g, b = tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))
            r, g, b = int(r * 0.8), int(g * 0.8), int(b * 0.8)
            return f"#{r:02x}{g:02x}{b:02x}"

        def log(self, message: str):
            """Add a message to the console"""
            self.console.append(message)
            # Auto-scroll to bottom
            self.console.verticalScrollBar().setValue(
                self.console.verticalScrollBar().maximum()
            )

        def run_command(self, args: list[str]):
            """Execute a command using QProcess"""
            if self.process and self.process.state() == QProcess.ProcessState.Running:
                self.log("⚠️  A process is already running. Stop it first.")
                return

            self.console.clear()
            self.log(f"▶ Running: {' '.join(args)}\n")

            self.process = QProcess(self)
            self.process.readyReadStandardOutput.connect(self.handle_stdout)
            self.process.readyReadStandardError.connect(self.handle_stderr)
            self.process.finished.connect(self.process_finished)
            self.process.start(args[0], args[1:])

            self.stop_btn.setEnabled(True)
            self.disable_action_buttons()

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
            self.enable_action_buttons()

        def stop_process(self):
            """Stop the running process"""
            if self.process and self.process.state() == QProcess.ProcessState.Running:
                self.log("\n⏸ Stopping process...")
                self.process.kill()
                self.process.waitForFinished(3000)

        def disable_action_buttons(self):
            """Disable action buttons while a process is running"""
            self.deps_btn.setEnabled(False)
            self.config_btn.setEnabled(False)
            self.build_btn.setEnabled(False)
            self.run_btn.setEnabled(False)
            self.shader_btn.setEnabled(False)

        def enable_action_buttons(self):
            """Enable action buttons after process completes"""
            self.deps_btn.setEnabled(True)
            self.config_btn.setEnabled(True)
            self.build_btn.setEnabled(True)
            self.run_btn.setEnabled(True)
            self.shader_btn.setEnabled(True)

        def run_dependencies(self):
            """Run conan dependencies installation"""
            cmd = [sys.executable, __file__, "dependencies"]
            self.run_command(cmd)

        def run_configure(self):
            """Run CMake configuration"""
            generator = self.generator_combo.currentText()
            build_type = self.build_type_combo.currentText()
            cmd = [
                sys.executable, __file__, "configure",
                "--generator", generator,
                "--build-type", build_type
            ]
            self.run_command(cmd)

        def run_build(self):
            """Run build command"""
            target = self.target_combo.currentText()
            build_dir = GENERATOR_DEFAULT_DIR.get(
                self.generator_combo.currentText(),
                DEFAULT_BUILD_DIR
            )
            cmd = [
                sys.executable, __file__, "build",
                "--build-dir", build_dir,
                "--target", target
            ]
            self.run_command(cmd)

        def compile_shaders(self):
            """Compile shaders manually"""
            self.console.clear()
            self.log("=== Compiling Shaders ===\n")
            _compile_shaders(dry_run=False)
            self.log("\n✓ Shader compilation completed")

        def run_demo(self):
            """Run the demo application"""
            build_dir = GENERATOR_DEFAULT_DIR.get(
                self.generator_combo.currentText(),
                DEFAULT_BUILD_DIR
            )
            cmd = [sys.executable, __file__, "run", "--build-dir", build_dir]
            self.run_command(cmd)

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
        help="skip shader compilation and asset syncing before running",
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
    try:
        args.func(args)
    except subprocess.CalledProcessError as exc:
        return int(exc.returncode)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
