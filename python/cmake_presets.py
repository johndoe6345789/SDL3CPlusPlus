"""Resolve the Conan-generated CMake configure preset and its build dir.

Conan 2 names presets after the generator's config model: multi-config
generators (Visual Studio) get a single ``conan-default`` writing its
cache to ``<folder>/build``; single-config generators (Ninja, Makefiles)
get ``conan-<buildtype>`` and nest the cache in a per-config directory.
Hardcoding either shape breaks the other platform, so callers ask here.
"""

from __future__ import annotations

import json
from pathlib import Path

PRESETS_FILE = "CMakeUserPresets.json"


def _load(path: str | Path) -> dict | None:
    """Return parsed JSON, or None when unreadable or malformed."""
    try:
        return json.loads(Path(path).read_text())
    except (OSError, json.JSONDecodeError):
        return None


def read_configure_presets(root: str | Path = ".") -> list[dict]:
    """Return configure presets from CMakeUserPresets.json and includes.

    Conan writes the real presets into an included file under the build
    folder, so the top-level document is followed one level deep.
    """
    root = Path(root)
    top = _load(root / PRESETS_FILE)
    if top is None:
        return []
    presets = list(top.get("configurePresets", []))
    for include in top.get("include", []):
        included = _load(root / include)
        if included:
            presets.extend(included.get("configurePresets", []))
    return [p for p in presets if isinstance(p, dict) and "name" in p]


def choose_configure_preset(
    names: list[str], build_type: str = "Release"
) -> str | None:
    """Pick the best Conan preset from the ones actually generated.

    Prefers multi-config ``conan-default``, then the single-config preset
    matching ``build_type``, then any remaining Conan preset.
    """
    if "conan-default" in names:
        return "conan-default"
    typed = f"conan-{build_type.lower()}"
    if typed in names:
        return typed
    for name in names:
        if name.startswith("conan-"):
            return name
    return None


def resolve_configure_preset(
    build_type: str = "Release", root: str | Path = "."
) -> str | None:
    """Choose a configure preset for ``build_type`` under ``root``."""
    names = [p["name"] for p in read_configure_presets(root)]
    return choose_configure_preset(names, build_type)


def resolve_binary_dir(preset: str, root: str | Path = ".") -> str | None:
    """Return the binaryDir CMake uses for ``preset``, if it declares one."""
    for entry in read_configure_presets(root):
        if entry["name"] == preset and entry.get("binaryDir"):
            return str(entry["binaryDir"])
    return None
