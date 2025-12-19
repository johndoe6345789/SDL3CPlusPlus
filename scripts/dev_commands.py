"""Helper CLI that mirrors the commands documented in README.md."""

from __future__ import annotations

import argparse
import os
import platform
import shlex
import subprocess


def run_commands(commands: list[str], dry_run: bool) -> None:
    for cmd in commands:
        print(f"\n> {cmd}")
        if dry_run:
            continue
        subprocess.run(cmd, shell=True, check=True)


IS_WINDOWS = platform.system() == "Windows"
DEFAULT_GENERATOR = "ninja-msvc" if IS_WINDOWS else "ninja"
GENERATOR_DEFAULT_DIR = {
    "vs": "build",
    "ninja": "build-ninja",
    "ninja-msvc": "build-ninja-msvc",
}
DEFAULT_BUILD_DIR = GENERATOR_DEFAULT_DIR[DEFAULT_GENERATOR]
SPINNING_BINARY = os.path.join(
    DEFAULT_BUILD_DIR, "spinning_cube.exe" if IS_WINDOWS else "spinning_cube"
)

ALIASES = {
    "conan_detect": "conan profile detect",
    "conan_install": (
        "conan install . -of build -b missing "
        "-s compiler=msvc -s compiler.version=194 -s compiler.cppstd=17 "
        '-c tools.cmake.cmaketoolchain:generator="Visual Studio 17 2022"'
    ),
    "msvc_quick": (
        'cmd /c "call \\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\VC\\Auxiliary\\Build\\vcvarsall.bat\\" x64 '
        "&& cmake --build build-ninja-msvc --config Release\""
    ),
}


def dependencies(args: argparse.Namespace) -> None:
    commands = [ALIASES["conan_detect"], ALIASES["conan_install"]]
    run_commands(commands, args.dry_run)


def configure(args: argparse.Namespace) -> None:
    generator = args.generator or DEFAULT_GENERATOR
    build_dir = args.build_dir or GENERATOR_DEFAULT_DIR.get(generator, "build")
    if generator == "vs":
        cmd = f"cmake -B {build_dir} -S ."
    else:
        cmd = (
            f'cmake -G Ninja -B {build_dir} -S . -DCMAKE_BUILD_TYPE={args.build_type}'
        )
    run_commands([cmd], args.dry_run)


def build(args: argparse.Namespace) -> None:
    cmd = (
        f"cmake --build {args.build_dir} --config {args.config}"
        + (f" --target {args.target}" if args.target else "")
    )
    run_commands([cmd], args.dry_run)


def msvc_quick(args: argparse.Namespace) -> None:
    if not IS_WINDOWS:
        raise SystemExit("msvc-quick is only supported on Windows")
    if args.bat_path:
        alias = ALIASES["msvc_quick"].replace(
            r'"C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"',
            args.bat_path,
        )
    else:
        alias = ALIASES["msvc_quick"]
    if args.dry_run:
        print("\n> " + alias)
    else:
        subprocess.run(alias, shell=True, check=True)


def run_demo(args: argparse.Namespace) -> None:
    build_dir = args.build_dir or DEFAULT_BUILD_DIR
    exe_name = args.target or ("sdl3_app.exe" if IS_WINDOWS else "sdl3_app")
    binary = os.path.join(build_dir, exe_name)
    cmd = binary
    if args.args:
        extra = " ".join(shlex.quote(arg) for arg in args.args)
        cmd = f"{cmd} {extra}"
    if args.dry_run:
        print("\n> " + cmd)
    else:
        subprocess.run(cmd, shell=True, check=True)


def main() -> int:
    parser = argparse.ArgumentParser(description="Run build helper commands.")
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="print commands without executing them",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    deps = subparsers.add_parser("dependencies", help="run Conan setup from README")
    deps.set_defaults(func=dependencies)

    conf = subparsers.add_parser("configure", help="configure CMake projects")
    conf.add_argument(
        "--generator",
        choices=["vs", "ninja", "ninja-msvc"],
        help=(
            "which generator to invoke (default: Ninja+MSVC on Windows, Ninja elsewhere)"
        ),
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
    conf.set_defaults(func=configure)

    build_parser = subparsers.add_parser("build", help="run cmake --build")
    build_parser.add_argument(
        "--build-dir", default=DEFAULT_BUILD_DIR, help="which directory to build"
    )
    build_parser.add_argument(
        "--config", default="Release", help="configuration for multi-config builders"
    )
    build_parser.add_argument(
        "--target",
        default="sdl3_app",
        help="target to build (README mentions sdl3_app and spinning_cube)",
    )
    build_parser.set_defaults(func=build)

    msvc = subparsers.add_parser(
        "msvc-quick", help="run the documented VS Developer Prompt one-liner"
    )
    msvc.add_argument(
        "--bat-path",
        help="full path to vcvarsall.bat (defaults to the VS 2022 Professional path from README)",
    )
    msvc.set_defaults(func=msvc_quick)

    run_parser = subparsers.add_parser(
        "run", help="execute the spinning_cube binary from the build folder"
    )
    run_parser.add_argument(
        "--build-dir",
        help="where the binary lives (defaults to the Ninja folder from configure)",
    )
    run_parser.add_argument(
        "--target",
        help="executable name to run (defaults to `sdl3_app`)",
    )
    run_parser.add_argument(
        "--args",
        nargs=argparse.REMAINDER,
        help="arguments to forward to the executable (prefix with '--' before positional args when needed)",
    )
    run_parser.set_defaults(func=run_demo)

    args = parser.parse_args()
    try:
        args.func(args)
    except subprocess.CalledProcessError as exc:
        return exc.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
