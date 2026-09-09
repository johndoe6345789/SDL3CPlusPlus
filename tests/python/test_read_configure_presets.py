"""Reading presets off disk, including Conan's indirection via include."""

import pytest

from cmake_presets import (
    read_configure_presets,
    resolve_binary_dir,
    resolve_configure_preset,
)


def test_reads_presets_from_included_file(tmp_path, write_presets):
    write_presets(tmp_path, [{"name": "vita-release"}],
                  nested=[{"name": "conan-release"}])
    names = [p["name"] for p in read_configure_presets(tmp_path)]
    assert names == ["vita-release", "conan-release"]


def test_resolves_preset_through_include(tmp_path, write_presets):
    write_presets(tmp_path, [{"name": "vita-release"}],
                  nested=[{"name": "conan-release"}])
    assert resolve_configure_preset("Release", tmp_path) == "conan-release"


def test_skips_entries_without_a_name(tmp_path, write_presets):
    write_presets(tmp_path, [{"binaryDir": "/tmp/x"}, {"name": "conan-debug"}])
    names = [p["name"] for p in read_configure_presets(tmp_path)]
    assert names == ["conan-debug"]


def test_resolves_binary_dir_of_nested_preset(tmp_path, write_presets):
    write_presets(tmp_path, [], nested=[
        {"name": "conan-release", "binaryDir": "/tmp/build/Release"},
    ])
    found = resolve_binary_dir("conan-release", tmp_path)
    assert found == "/tmp/build/Release"


def test_binary_dir_is_none_when_undeclared(tmp_path, write_presets):
    write_presets(tmp_path, [{"name": "vita-release"}])
    assert resolve_binary_dir("vita-release", tmp_path) is None


def test_missing_include_target_is_skipped(tmp_path, write_presets):
    write_presets(tmp_path, [{"name": "conan-release"}],
                  nested=[{"name": "conan-debug"}])
    (tmp_path / "build" / "generators" / "CMakePresets.json").unlink()
    names = [p["name"] for p in read_configure_presets(tmp_path)]
    assert names == ["conan-release"]


@pytest.mark.parametrize("content", ["", "{not json"])
def test_malformed_presets_do_not_raise(tmp_path, content):
    (tmp_path / "CMakeUserPresets.json").write_text(content)
    assert read_configure_presets(tmp_path) == []


def test_missing_presets_file_is_empty(tmp_path):
    assert read_configure_presets(tmp_path) == []
    assert resolve_configure_preset("Release", tmp_path) is None
