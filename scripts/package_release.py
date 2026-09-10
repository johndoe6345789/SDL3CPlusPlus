#!/usr/bin/env python3
"""Bundle a built sdl3_app tree into a distributable ZIP.

Environment:
  BUILD_DIR  CMake binary directory holding the built executable (required)
  ZIP_NAME   Name of the archive to write into release/ (required)
"""
import os
import shutil
from pathlib import Path


def require_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise SystemExit(f"Environment variable {name} is not set")
    return value


def find_binary(build_dir: Path) -> Path:
    """Locate sdl3_app, allowing for multi-config generator subdirectories."""
    candidates = [
        build_dir / "sdl3_app",
        build_dir / "sdl3_app.exe",
        build_dir / "Release" / "sdl3_app",
        build_dir / "Release" / "sdl3_app.exe",
    ]
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    searched = "\n  ".join(str(c) for c in candidates)
    raise SystemExit(f"Missing sdl3_app binary. Looked in:\n  {searched}")


def main() -> None:
    root = Path.cwd()
    build_dir = root / require_env("BUILD_DIR")
    zip_name = require_env("ZIP_NAME")

    release_dir = root / "release"
    package_dir = release_dir / "package"
    if package_dir.exists():
        shutil.rmtree(package_dir)
    package_dir.mkdir(parents=True)

    binary = find_binary(build_dir)
    destination = package_dir / binary.name
    shutil.copy2(binary, destination)
    destination.chmod(destination.stat().st_mode | 0o111)

    # The app loads its workflows and assets from packages/, which CMake copies
    # next to the executable at configure time.
    packages = build_dir / "packages"
    if not packages.is_dir():
        raise SystemExit(f"Missing packages directory at {packages}")
    shutil.copytree(packages, package_dir / "packages")

    for doc in ("README.md", "LICENSE"):
        source = root / doc
        if source.is_file():
            shutil.copy2(source, package_dir / doc)

    archive_stem = release_dir / Path(zip_name).stem
    shutil.make_archive(str(archive_stem), "zip", root_dir=package_dir)
    print(f"Created {archive_stem.with_suffix('.zip')}")


if __name__ == "__main__":
    main()
