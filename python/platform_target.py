"""Per-platform facts the build helper and its GUI both need.

Every function takes the system name so the behaviour for all three
platforms can be exercised from any one of them, rather than only the
branch the developer happens to be running on.
"""

from __future__ import annotations

import platform

WINDOWS = "Windows"
MACOS = "Darwin"
LINUX = "Linux"

# packages/bootstrap_* supply the platform's window and GPU setup.
BOOTSTRAP_PACKAGES = {
    WINDOWS: "bootstrap_windows",
    MACOS: "bootstrap_mac",
    LINUX: "bootstrap_linux",
}

# Ninja everywhere; on Windows the MSVC-flavoured variant.
GENERATORS = {WINDOWS: "ninja-msvc", MACOS: "ninja", LINUX: "ninja"}


def current_system(system: str | None = None) -> str:
    return system or platform.system()


def bootstrap_package(system: str | None = None) -> str:
    """Bootstrap package for the platform, falling back to Linux's."""
    return BOOTSTRAP_PACKAGES.get(current_system(system),
                                  BOOTSTRAP_PACKAGES[LINUX])


def app_executable(name: str = "sdl3_app", system: str | None = None) -> str:
    """Executable file name, with the extension Windows requires."""
    if current_system(system) == WINDOWS:
        return name if name.endswith(".exe") else name + ".exe"
    return name[:-4] if name.endswith(".exe") else name


def default_generator(system: str | None = None) -> str:
    return GENERATORS.get(current_system(system), GENERATORS[LINUX])


def is_windows(system: str | None = None) -> bool:
    return current_system(system) == WINDOWS
