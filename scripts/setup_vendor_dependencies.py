#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
VENDOR_DIR = ROOT / "vendor"
INSTALL_DIR = VENDOR_DIR / "install"


def run(command, cwd=None):
    print(f"[vendor-deps] running: {' '.join(command)} (cwd={cwd or ROOT})")
    subprocess.run(command, cwd=cwd or ROOT, check=True)


def fetch_repo(name, url, ref):
    target = VENDOR_DIR / name
    if (target / ".git").exists():
        print(f"[vendor-deps] updating {name}")
        run(["git", "-C", str(target), "fetch", "--tags", "origin"])
        run(["git", "-C", str(target), "checkout", ref])
        run(["git", "-C", str(target), "reset", "--hard", f"origin/{ref}"])
    else:
        print(f"[vendor-deps] cloning {name}")
        run(["git", "clone", "--depth", "1", "--branch", ref, url, str(target)])


def build_and_install(source_dir, build_dir, cmake_options=None):
    cmake_options = cmake_options or []
    build_dir.mkdir(parents=True, exist_ok=True)
    run(
        [
            "cmake",
            "-S",
            str(source_dir),
            "-B",
            str(build_dir),
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_INSTALL_PREFIX={INSTALL_DIR}",
            *cmake_options,
        ]
    )
    run(
        [
            "cmake",
            "--build",
            str(build_dir),
            "--config",
            "Release",
            "--",
            f"-j{max(1, (subprocess.os.cpu_count() or 1))}",
        ]
    )
    run(["cmake", "--install", str(build_dir), "--config", "Release"])


def main():
    VENDOR_DIR.mkdir(exist_ok=True)
    INSTALL_DIR.mkdir(exist_ok=True)

    fetch_repo("SDL", "https://github.com/libsdl-org/SDL.git", "release-3.0.0")
    build_and_install(
        VENDOR_DIR / "SDL",
        VENDOR_DIR / "build-sdl",
        ["-DSDL_SHARED=OFF", "-DSDL_STATIC=ON", "-DSDL_TEST=OFF", "-DBUILD_SHARED_LIBS=OFF"],
    )

    fetch_repo("Vulkan-Loader", "https://github.com/KhronosGroup/Vulkan-Loader.git", "sdk-1.3.275")
    build_and_install(
        VENDOR_DIR / "Vulkan-Loader",
        VENDOR_DIR / "build-vulkan-loader",
        ["-DBUILD_WSI=OFF", "-DBUILD_SAMPLES=OFF"],
    )

    print(f"[vendor-deps] vendor dependencies built into {INSTALL_DIR}")


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as exc:
        sys.exit(exc.returncode)
