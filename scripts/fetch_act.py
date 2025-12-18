#!/usr/bin/env python3
"""Download and cache the latest release of the `act` CLI."""

from __future__ import annotations

import json
import logging
import os
import platform
import shutil
import stat
import tarfile
import zipfile
from pathlib import Path
from typing import Any
import urllib.error
import urllib.request

RELEASE_URL = "https://api.github.com/repos/nektos/act/releases/latest"
USER_AGENT = "gh-actions-local-docker"

logger = logging.getLogger(__name__)


class ActBinaryError(SystemExit):
    """Raised when the requested `act` binary cannot be provided."""


def _default_cache_root() -> Path:
    explicit = os.environ.get("ACT_CACHE_DIR")
    if explicit:
        return Path(explicit).expanduser()
    if os.name == "nt":
        base = Path(os.environ.get("LOCALAPPDATA", Path.home() / "AppData" / "Local"))
        return base / "act"
    env = os.environ.get("XDG_CACHE_HOME")
    if env:
        return Path(env).expanduser() / "act"
    return Path.home() / ".cache" / "act"


def _expected_asset_name(system: str, machine: str) -> str:
    system = system.lower()
    normalized = machine.lower()
    if system == "linux":
        if normalized in {"x86_64", "amd64"}:
            return "act_Linux_x86_64.tar.gz"
        if normalized in {"aarch64", "arm64"}:
            return "act_Linux_arm64.tar.gz"
        if normalized in {"armv6l", "armv6"}:
            return "act_Linux_armv6.tar.gz"
        if normalized in {"armv7l", "armv7"}:
            return "act_Linux_armv7.tar.gz"
        if normalized in {"i386", "i686", "x86"}:
            return "act_Linux_i386.tar.gz"
        if normalized == "riscv64":
            return "act_Linux_riscv64.tar.gz"
    if system == "darwin":
        if normalized in {"arm64"}:
            return "act_Darwin_arm64.tar.gz"
        if normalized in {"x86_64", "amd64"}:
            return "act_Darwin_x86_64.tar.gz"
    if system == "windows":
        if normalized in {"arm64"}:
            return "act_Windows_arm64.zip"
        if normalized in {"armv7", "armv7l"}:
            return "act_Windows_armv7.zip"
        if normalized in {"i386", "i686", "x86"}:
            return "act_Windows_i386.zip"
        if normalized in {"x86_64", "amd64"}:
            return "act_Windows_x86_64.zip"
    raise ActBinaryError(
        f"Unsupported platform for act binary: system={system} machine={machine}"
    )


def _select_asset(release: dict[str, Any]) -> dict[str, Any]:
    assets = release.get("assets")
    if not isinstance(assets, list):
        raise ActBinaryError("Release metadata missing assets list.")
    asset_name = _expected_asset_name(platform.system(), platform.machine())
    for asset in assets:
        if asset.get("name") == asset_name:
            return asset
    raise ActBinaryError(
        f"Could not find act release asset {asset_name} in {release.get('tag_name')}"
    )


def _fetch_latest_release() -> dict[str, Any]:
    req = urllib.request.Request(
        RELEASE_URL,
        headers={"User-Agent": USER_AGENT},
    )
    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            if resp.status != 200:
                raise ActBinaryError(
                    "Unexpected response while fetching act release metadata."
                )
            return json.load(resp)
    except urllib.error.URLError as exc:
        raise ActBinaryError(f"Failed to download act release metadata: {exc}") from exc


def _download_asset(url: str, dest: Path) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    try:
        with urllib.request.urlopen(req, timeout=60) as resp, dest.open("wb") as out:
            shutil.copyfileobj(resp, out)
    except urllib.error.URLError as exc:
        raise ActBinaryError(f"Failed to download act asset {url}: {exc}") from exc


def _extract_archive(archive: Path, dest_dir: Path) -> None:
    if archive.name.endswith(".zip"):
        with zipfile.ZipFile(archive, "r") as zf:
            zf.extractall(dest_dir)
        return
    with tarfile.open(archive, "r:gz") as tf:
        tf.extractall(dest_dir)


def _binary_name_for_asset(asset_name: str) -> str:
    if asset_name.lower().endswith(".zip"):
        return "act.exe"
    return "act"


def _set_executable(path: Path) -> None:
    mode = path.stat().st_mode
    path.chmod(mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)


def ensure_act_binary(act_path: Path | None = None) -> Path:
    """Resolve the act binary, downloading it into the cache if necessary."""

    if act_path:
        resolved = act_path.expanduser().resolve()
        if not resolved.exists():
            raise ActBinaryError(f"act binary not found at {resolved}")
        logger.info("Using act binary from %s", resolved)
        return resolved

    release = _fetch_latest_release()
    asset = _select_asset(release)
    cache_root = _default_cache_root()
    release_tag = release.get("tag_name", "latest")
    release_dir = cache_root / release_tag
    release_dir.mkdir(parents=True, exist_ok=True)

    asset_name = asset["name"]
    asset_path = release_dir / asset_name
    binary_name = _binary_name_for_asset(asset_name)
    binary_path = release_dir / binary_name

    if binary_path.exists():
        logger.info("Using cached act binary at %s", binary_path)
        return binary_path
    if not asset_path.exists():
        logger.info("Downloading act asset %s", asset["browser_download_url"])
        _download_asset(asset["browser_download_url"], asset_path)
        logger.info("Downloaded act asset %s", asset_path)
    else:
        logger.debug("Reusing previously downloaded asset %s", asset_path)
    logger.info("Extracting act asset %s", asset_path)
    _extract_archive(asset_path, release_dir)
    if not binary_path.exists():
        raise ActBinaryError("act binary missing after extracting release asset")
    _set_executable(binary_path)
    logger.info("Act binary ready at %s", binary_path)
    return binary_path


def main() -> None:
    logging.basicConfig(level=logging.INFO)
    try:
        path = ensure_act_binary()
    except ActBinaryError as exc:
        raise SystemExit(exc)
    print(path)


if __name__ == "__main__":
    main()
