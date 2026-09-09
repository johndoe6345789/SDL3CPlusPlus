"""Preset selection across Conan's multi- and single-config layouts."""

from cmake_presets import choose_configure_preset


def test_prefers_conan_default_for_multi_config():
    names = ["conan-default", "conan-release"]
    assert choose_configure_preset(names, "Release") == "conan-default"


def test_uses_typed_preset_for_single_config():
    chosen = choose_configure_preset(["conan-release"], "Release")
    assert chosen == "conan-release"


def test_matches_requested_build_type():
    names = ["conan-debug", "conan-release"]
    assert choose_configure_preset(names, "Debug") == "conan-debug"


def test_build_type_match_is_case_insensitive():
    assert choose_configure_preset(["conan-release"], "RELEASE") == (
        "conan-release"
    )


def test_falls_back_to_any_conan_preset():
    chosen = choose_configure_preset(["conan-release"], "Debug")
    assert chosen == "conan-release"


def test_ignores_non_conan_presets():
    assert choose_configure_preset(["vita-release"], "Release") is None


def test_returns_none_when_nothing_generated():
    assert choose_configure_preset([], "Release") is None
