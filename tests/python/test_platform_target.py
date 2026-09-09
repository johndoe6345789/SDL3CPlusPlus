"""Platform facts, checked for all three targets from any one of them."""

import pytest

import platform_target as target


@pytest.mark.parametrize("system,expected", [
    ("Windows", "bootstrap_windows"),
    ("Darwin", "bootstrap_mac"),
    ("Linux", "bootstrap_linux"),
])
def test_each_platform_has_its_bootstrap(system, expected):
    assert target.bootstrap_package(system) == expected


def test_unknown_platform_falls_back_to_linux():
    assert target.bootstrap_package("Haiku") == "bootstrap_linux"


@pytest.mark.parametrize("system,expected", [
    ("Windows", "sdl3_app.exe"),
    ("Darwin", "sdl3_app"),
    ("Linux", "sdl3_app"),
])
def test_executable_name_matches_the_platform(system, expected):
    assert target.app_executable(system=system) == expected


def test_executable_extension_is_not_doubled():
    assert target.app_executable("sdl3_app.exe", "Windows") == "sdl3_app.exe"


def test_executable_extension_is_stripped_off_windows():
    assert target.app_executable("sdl3_app.exe", "Linux") == "sdl3_app"


def test_a_named_target_keeps_its_name():
    assert target.app_executable("q3_slide_planes_test", "Darwin") == (
        "q3_slide_planes_test")


@pytest.mark.parametrize("system,expected", [
    ("Windows", "ninja-msvc"),
    ("Darwin", "ninja"),
    ("Linux", "ninja"),
])
def test_generator_per_platform(system, expected):
    assert target.default_generator(system) == expected


def test_is_windows_only_for_windows():
    assert target.is_windows("Windows")
    assert not target.is_windows("Darwin")
    assert not target.is_windows("Linux")


def test_defaults_to_the_running_system():
    assert target.bootstrap_package() in target.BOOTSTRAP_PACKAGES.values()
    assert target.current_system() == platform_system()


def platform_system():
    import platform
    return platform.system()
