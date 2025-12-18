#!/usr/bin/env python3
import os
import shutil
from pathlib import Path

root = Path.cwd()
release_dir = root / "release"
package_dir = release_dir / "package"
if package_dir.exists():
    shutil.rmtree(package_dir)
package_dir.mkdir(parents=True)

platform_name = os.environ["PLATFORM_NAME"]
build_dir = root / os.environ["BUILD_DIR"]
binary_name = "sdl3_app.exe" if platform_name == "windows" else "sdl3_app"
if platform_name == "windows":
    binary_source = build_dir / "Release" / binary_name
else:
    binary_source = build_dir / binary_name
if not binary_source.exists():
    raise SystemExit(f"Missing binary at {binary_source}")
shutil.copy2(binary_source, package_dir / binary_name)

for subdir in ("shaders", "scripts"):
    src = build_dir / subdir
    target = package_dir / subdir
    if not src.exists():
        raise SystemExit(f"Missing {subdir} directory at {src}")
    shutil.copytree(src, target)

shutil.copy2(root / "README.md", package_dir / "README.md")

zip_name = os.environ["ZIP_NAME"]
archive_path = release_dir / zip_name
shutil.make_archive(str(archive_path.with_suffix("")), "zip", root_dir=package_dir)
print(f"Created {archive_path}")
