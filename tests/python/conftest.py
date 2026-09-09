"""Make the repo's ``python/`` helpers importable from these tests."""

import json
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "python"))


@pytest.fixture
def write_presets():
    """Return a helper writing CMakeUserPresets.json into a directory.

    ``nested`` mirrors how Conan puts the real presets in a separate file
    under the build folder and references it from the top-level include.
    """

    def _write(root: Path, presets: list[dict], nested=None) -> None:
        top: dict = {"version": 4, "configurePresets": presets}
        if nested is not None:
            include = root / "build" / "generators" / "CMakePresets.json"
            include.parent.mkdir(parents=True, exist_ok=True)
            include.write_text(json.dumps({"configurePresets": nested}))
            top["include"] = [str(include.relative_to(root))]
        (root / "CMakeUserPresets.json").write_text(json.dumps(top))

    return _write
