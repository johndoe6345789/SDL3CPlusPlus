"""
Steam install detection for legitimate access to game data the user owns.

Two-phase discovery:
  1. Find Steam's install root (Windows registry / standard *nix paths).
  2. Parse libraryfolders.vdf to enumerate all Steam libraries and which
     app IDs each contains.

Then resolve_game_files() walks a known-games registry and exports paths
for any owned games that are present.

This module is platform-aware but keeps Windows-specific code optional —
Linux/macOS Steam users have ~/.steam/steam and ~/Library/Application Support/Steam.
"""

from __future__ import annotations

import os
import re
import sys
from pathlib import Path


# ─── Phase 1: Find Steam ─────────────────────────────────────────────────────

def find_steam_root() -> Path | None:
    """Locate Steam's install directory across platforms."""
    if sys.platform == "win32":
        try:
            import winreg
            for hive, key in [
                (winreg.HKEY_CURRENT_USER, r"Software\Valve\Steam"),
                (winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\WOW6432Node\Valve\Steam"),
                (winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\Valve\Steam"),
            ]:
                try:
                    with winreg.OpenKey(hive, key) as k:
                        value_name = "SteamPath" if hive == winreg.HKEY_CURRENT_USER else "InstallPath"
                        path, _ = winreg.QueryValueEx(k, value_name)
                        p = Path(path)
                        if p.exists():
                            return p
                except OSError:
                    continue
        except ImportError:
            pass
        # Fallback: standard install location
        for candidate in [r"C:\Program Files (x86)\Steam", r"C:\Program Files\Steam"]:
            p = Path(candidate)
            if p.exists():
                return p
    elif sys.platform == "darwin":
        p = Path.home() / "Library/Application Support/Steam"
        if p.exists():
            return p
    else:  # linux
        for candidate in [Path.home() / ".steam/steam", Path.home() / ".local/share/Steam"]:
            if candidate.exists():
                return candidate
    return None


# ─── Phase 2: Parse libraryfolders.vdf ───────────────────────────────────────

def parse_library_folders(steam_root: Path) -> list[dict]:
    """
    Parse libraryfolders.vdf into a list of {path, apps: {appid: size}} dicts.

    The VDF format is a recursive KeyValues structure; we only need the
    flat list of "path" + "apps" per library, so a regex pass is sufficient
    rather than pulling in a full vdf parser.
    """
    vdf_path = steam_root / "steamapps" / "libraryfolders.vdf"
    if not vdf_path.exists():
        return []

    text = vdf_path.read_text(encoding="utf-8", errors="replace")
    libraries = []

    # Each library block: "0" { "path" "..." ... "apps" { "appid" "size" ... } }
    # Split on top-level numeric keys. Robust enough for our purposes.
    lib_pattern = re.compile(r'"\d+"\s*\{(.*?)\n\t\}', re.DOTALL)
    for block in lib_pattern.finditer(text):
        body = block.group(1)
        path_match = re.search(r'"path"\s*"([^"]+)"', body)
        if not path_match:
            continue
        # Steam writes Windows paths with doubled backslashes in vdf
        lib_path = Path(path_match.group(1).replace("\\\\", "\\"))

        apps = {}
        apps_match = re.search(r'"apps"\s*\{(.*?)\n\t\t\}', body, re.DOTALL)
        if apps_match:
            for app in re.finditer(r'"(\d+)"\s*"(\d+)"', apps_match.group(1)):
                apps[app.group(1)] = int(app.group(2))

        libraries.append({"path": lib_path, "apps": apps})

    return libraries


# ─── Known-games registry ────────────────────────────────────────────────────
#
# Maps Steam app ID → metadata for resolving the game's data files.
#   - install_dir: folder under steamapps/common/
#   - files: {env_var_name: relative_path_to_required_file}
#
# Adding a new game here is the integration point — no code changes needed.

KNOWN_GAMES: dict[str, dict] = {
    "2200": {  # Quake 3 Arena
        "name": "Quake 3 Arena",
        "install_dir": "Quake 3 Arena",
        "files": {
            "QUAKE3_PAK0": "baseq3/pak0.pk3",
        },
    },
    # Future: Quake (id 2310), Doom 3 (208200), etc.
}


# ─── Phase 3: Resolve game files (THE PIECE FOR YOU TO WRITE) ────────────────

def resolve_game_files(libraries: list[dict]) -> dict[str, str]:
    """
    Walk the libraries and KNOWN_GAMES registry; return a dict of
    {env_var_name: absolute_path_string} for every required file we found.

    TODO (your turn — see the prompt below).

    Args:
        libraries: list of {"path": Path, "apps": {appid: size}} dicts
                   from parse_library_folders()

    Returns:
        dict mapping env var name (e.g., "QUAKE3_PAK0") to absolute path string.
        Skip games whose required files don't actually exist on disk.
    """
    resolved: dict[str, str] = {}
    for appid, meta in KNOWN_GAMES.items():
        for lib in libraries:
            install_root = lib["path"] / "steamapps" / "common" / meta["install_dir"]
            if not install_root.exists():
                continue
            for env_var, rel_path in meta["files"].items():
                if env_var in resolved:
                    continue  # first-match-wins across libraries
                full_path = install_root / rel_path
                if full_path.is_file():
                    resolved[env_var] = full_path.as_posix()
            break  # found this game in this library; don't keep scanning
    return resolved


# ─── Public entry point ──────────────────────────────────────────────────────

def detect_and_export() -> dict[str, str]:
    """
    Top-level: find Steam, enumerate libraries, resolve known-game files.
    Returns a dict of env vars ready to be merged into os.environ.
    Empty dict if Steam isn't installed (silent — not an error).
    """
    root = find_steam_root()
    if not root:
        return {}
    libs = parse_library_folders(root)
    if not libs:
        return {}
    return resolve_game_files(libs)


if __name__ == "__main__":
    # Run as a script to see what got detected.
    root = find_steam_root()
    print(f"Steam root: {root}")
    if not root:
        sys.exit(0)
    libs = parse_library_folders(root)
    print(f"Libraries found: {len(libs)}")
    for lib in libs:
        print(f"  {lib['path']} ({len(lib['apps'])} apps)")
    try:
        resolved = resolve_game_files(libs)
        print(f"\nResolved env vars:")
        for k, v in resolved.items():
            print(f"  {k}={v}")
    except NotImplementedError as e:
        print(f"\n[!] {e}")
