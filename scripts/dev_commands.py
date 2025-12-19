#!/usr/bin/env python3
"""
Helper CLI that mirrors the commands documented in README.md.

Design goals:
- No shell=True for normal commands (reliable quoting, safer).
- Explicit argv execution throughout.
- Where practical, allow forwarding extra arguments to underlying tools.
- Keep Windows/MSVC "one-liner" behavior via cmd.exe only when required.

NOTE (Windows quoting):
cmd.exe has special/quirky parsing rules for /c when the command contains quotes.
We use: cmd.exe /d /s /c ""call "<bat>" <arch> && <then...>""
The doubled quotes at both ends are intentional and required for robust behavior.
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

DEFAULT_VCVARSALL = (
    "C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional"
    "\\VC\\Auxiliary\\Build\\vcvarsall.bat"
)


def _print_cmd(argv: Sequence[str]) -> None:
    if IS_WINDOWS:
        rendered = subprocess.list2cmdline(list(argv))
    else:
        rendered = " ".join(_sh_quote(a) for a in argv)
    print("\n> " + rendered)


def _sh_quote(s: str) -> str:
    if not s:
        return "''"
    safe = set(
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
        "._-/:@=+"
    )
    if all(c in safe for c in s):
        return s
    return "'" + s.replace("'", "'\"'\"'") + "'"


def run_argvs(argvs: Iterable[Sequence[str]], dry_run: bool) -> None:
    for argv in argvs:
        _print_cmd(argv)
        if dry_run:
            continue
        subprocess.run(list(argv), check=True)


def _as_build_dir(path_str: str | None, fallback: str) -> str:
    return path_str or fallback


def dependencies(args: argparse.Namespace) -> None:
    cmd_detect = ["conan", "profile", "detect", "-f"]
    cmd_install = ["conan", "install", ".", "-of", "build", "-b", "missing"]

    if args.conan_install_args:
        cmd_install.extend(args.conan_install_args)

    run_argvs([cmd_detect, cmd_install], args.dry_run)


def configure(args: argparse.Namespace) -> None:
    generator = args.generator or DEFAULT_GENERATOR
    build_dir = _as_build_dir(
        args.build_dir,
        GENERATOR_DEFAULT_DIR.get(generator, "build"),
    )

    cmake_args: list[str] = ["cmake", "-B", build_dir, "-S", "."]

    if generator == "vs":
        cmake_args.extend(["-G", CMAKE_GENERATOR["vs"]])
    else:
        cmake_args.extend(["-G", CMAKE_GENERATOR[generator]])
        cmake_args.append(f"-DCMAKE_BUILD_TYPE={args.build_type}")

    if args.cmake_args:
        cmake_args.extend(args.cmake_args)

    run_argvs([cmake_args], args.dry_run)


def build(args: argparse.Namespace) -> None:
    cmd: list[str] = ["cmake", "--build", args.build_dir]

    if args.config:
        cmd.extend(["--config", args.config])

    if args.target:
        cmd.extend(["--target", args.target])

    if args.build_tool_args:
        cmd.append("--")
        cmd.extend(args.build_tool_args)

    run_argvs([cmd], args.dry_run)


def _cmd_quote_arg_for_display(argv: Sequence[str]) -> str:
    # Display-only: how this argv might look as a single command line.
    return subprocess.list2cmdline(list(argv))


def _cmd_one_liner_vcvars_then(
    bat: str,
    arch: str,
    then_parts: Sequence[str],
) -> list[str]:
    """
    Robust cmd.exe invocation for:
      call "<bat>" <arch> && <then...>

    Uses cmd.exe quote rules:
      cmd.exe /d /s /c ""call "<bat>" <arch> && <then...>""
    """
    then_cmdline = _cmd_quote_arg_for_display(then_parts)
    inner = f'call "{bat}" {arch} && {then_cmdline}'
    wrapped = f'""{inner}""'
    return ["cmd.exe", "/d", "/s", "/c", wrapped]


def msvc_quick(args: argparse.Namespace) -> None:
    if not IS_WINDOWS:
        raise SystemExit("msvc-quick is only supported on Windows")

    bat = args.bat_path or DEFAULT_VCVARSALL
    arch = args.arch or "x64"

    if args.then_command:
        then_cmd = list(args.then_command)
    else:
        build_dir = _as_build_dir(args.build_dir, DEFAULT_BUILD_DIR)
        then_cmd = ["cmake", "--build", build_dir]
        if args.config:
            then_cmd.extend(["--config", args.config])
        if args.target:
            then_cmd.extend(["--target", args.target])
        if args.build_tool_args:
            then_cmd.append("--")
            then_cmd.extend(args.build_tool_args)

    cmd = _cmd_one_liner_vcvars_then(bat, arch, then_cmd)
    run_argvs([cmd], args.dry_run)


def run_demo(args: argparse.Namespace) -> None:
    build_dir = _as_build_dir(args.build_dir, DEFAULT_BUILD_DIR)

    exe_name = args.target or ("sdl3_app.exe" if IS_WINDOWS else "sdl3_app")
    binary = str(Path(build_dir) / exe_name)

    cmd: list[str] = [binary]
    if args.args:
        cmd.extend(args.args)

    run_argvs([cmd], args.dry_run)


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
        help="which generator to invoke (default: Ninja+MSVC on Windows, Ninja elsewhere)",
    )
    conf.add_argument(
        "--build-dir",
        help="override the directory where CMake writes build files",
    )
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
        "--build-dir",
        default=DEFAULT_BUILD_DIR,
        help="which directory to build",
    )
    bld.add_argument(
        "--config",
        default="Release",
        help="configuration for multi-config generators",
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
        "msvc-quick",
        help="run a VS env setup + follow-on command (README one-liner style)",
    )
    msvc.add_argument(
        "--bat-path",
        help="full path to vcvarsall.bat",
    )
    msvc.add_argument(
        "--arch",
        default="x64",
        help="architecture argument passed to vcvarsall.bat (default: x64)",
    )
    msvc.add_argument(
        "--build-dir",
        default=DEFAULT_BUILD_DIR,
        help="build directory (used by default follow-on build command)",
    )
    msvc.add_argument(
        "--config",
        default="Release",
        help="configuration for multi-config generators (used by default follow-on build)",
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
        "run",
        help="execute a built binary from the build folder",
    )
    runp.add_argument(
        "--build-dir",
        help="where the binary lives",
    )
    runp.add_argument(
        "--target",
        help="executable name to run (defaults to `sdl3_app[.exe]`)",
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

    args = parser.parse_args()

    try:
        args.func(args)
    except subprocess.CalledProcessError as exc:
        return int(exc.returncode)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
