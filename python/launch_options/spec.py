"""Per-game launch options, read from each package's package.json.

A package lists what it needs under "launch_options"; each entry is one
environment variable its workflows read through ${env:NAME}. A variable
the JSON names without declaring still gets a plain text field, so no
option is ever unreachable from the launcher.
"""

from __future__ import annotations

import json
import re
from dataclasses import dataclass
from pathlib import Path

KINDS = ("text", "number", "directory", "file", "choice")
_ENV_REF = re.compile(r"\$\{env:([A-Za-z_][A-Za-z0-9_]*)\}")


@dataclass(frozen=True)
class LaunchOption:
    env: str
    label: str
    kind: str = "text"
    required: bool = False
    help: str = ""
    default: str = ""
    file_filter: str = ""
    choices_from: str = ""


def referenced_env(package_dir: Path) -> list[str]:
    """Every ${env:NAME} any JSON file under the package refers to."""
    found: set[str] = set()
    for json_file in package_dir.rglob("*.json"):
        try:
            text = json_file.read_text(encoding="utf-8")
        except OSError:
            continue
        found.update(_ENV_REF.findall(text))
    return sorted(found)


def _from_entry(entry: dict) -> LaunchOption:
    kind = entry.get("kind", "text")
    if kind not in KINDS:
        raise ValueError(f"{entry.get('env')}: unknown kind {kind!r}")
    return LaunchOption(
        env=entry["env"],
        label=entry.get("label", entry["env"]),
        kind=kind,
        required=bool(entry.get("required", False)),
        help=entry.get("help", ""),
        default=str(entry.get("default", "")),
        file_filter=entry.get("file_filter", ""),
        choices_from=entry.get("choices_from", ""),
    )


def _manifest(package_dir: Path) -> dict:
    try:
        text = (package_dir / "package.json").read_text(encoding="utf-8")
        return json.loads(text)
    except (OSError, json.JSONDecodeError):
        return {}


def load_launch_options(package_dir: Path) -> list[LaunchOption]:
    """Declared options in order, then one text field per undeclared."""
    entries = _manifest(package_dir).get("launch_options", [])
    options = [_from_entry(entry) for entry in entries]
    declared = {option.env for option in options}
    for name in referenced_env(package_dir):
        if name not in declared:
            options.append(LaunchOption(env=name, label=name))
    return options
