#!/usr/bin/env python3
"""
Lightweight package validator that walks the `packages/` tree for all `package.json` files,
checks their npm-style schema, validates referenced assets/workflows/shaders/scenes, and logs
missing folders and schema violations.
"""

from __future__ import annotations

import argparse
import json
import logging
import sys
from pathlib import Path
from typing import Callable, Iterable, Optional, Sequence

COMMON_FOLDERS = ("assets", "scene", "shaders", "workflows")
REQUIRED_FIELDS = ("name", "version", "description", "workflows", "defaultWorkflow")
FIELD_TO_FOLDER = {
    "assets": "assets",
    "scene": "scene",
    "shaders": "shaders",
    "workflows": "workflows",
}

logger = logging.getLogger("package_lint")

try:
    from jsonschema import Draft7Validator
except ImportError:
    Draft7Validator = None


def load_json(path: Path) -> dict:
    logger.debug("Reading JSON from %s", path)
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def check_paths(
    root: Path,
    entries: Iterable[str],
    key: str,
    on_exist: Optional[Callable[[Path, str], None]] = None,
) -> Sequence[str]:
    """Return list of missing files for the given key list and optionally call `on_exist` for existing items."""
    missing = []
    for rel in entries:
        if not isinstance(rel, str):
            missing.append(f"{rel!r} (not a string)")
            continue
        candidate = root / rel
        logger.debug("Checking %s entry %s", key, candidate)
        if not candidate.exists():
            missing.append(str(rel))
            continue
        if on_exist:
            on_exist(candidate, rel)
    return missing


def validate_workflow_schema(workflow_path: Path, validator) -> list[str]:
    """Validate a workflow JSON file against the provided schema validator."""
    try:
        content = load_json(workflow_path)
    except json.JSONDecodeError as exc:
        return [f"invalid JSON: {exc}"]

    issues: list[str] = []
    for err in sorted(
        validator.iter_errors(content),
        key=lambda x: tuple(x.absolute_path),
    ):
        pointer = "/".join(str(part) for part in err.absolute_path) or "<root>"
        issues.append(f"schema violation at {pointer}: {err.message}")
    return issues


def validate_package(
    pkg_root: Path,
    pkg_data: dict,
    registry_names: Sequence[str],
    available_dirs: Sequence[str],
    workflow_schema_validator: Optional["Draft7Validator"] = None,
) -> tuple[list[str], list[str]]:
    errors: list[str] = []
    warnings: list[str] = []

    logger.debug("Validating %s", pkg_root)

    for field in REQUIRED_FIELDS:
        if field not in pkg_data:
            errors.append(f"missing required field `{field}`")
    workflows = pkg_data.get("workflows")
    default_workflow = pkg_data.get("defaultWorkflow")
    if workflows and isinstance(workflows, list):
        if default_workflow and default_workflow not in workflows:
            errors.append("`defaultWorkflow` is not present in `workflows` array")
    # schema-like validations
    for key in ("workflows", "assets", "scene", "shaders"):
        value = pkg_data.get(key)
        if value is None:
            continue
        if not isinstance(value, list):
            errors.append(f"`{key}` must be an array if present")
            continue
        on_exist: Optional[Callable[[Path, str], None]] = None
        if key == "workflows" and workflow_schema_validator:
            def on_exist(candidate: Path, rel: str) -> None:
                schema_issues = validate_workflow_schema(candidate, workflow_schema_validator)
                for issue in schema_issues:
                    errors.append(f"workflow `{rel}`: {issue}")

        missing = check_paths(pkg_root, value, key, on_exist=on_exist)
        if missing:
            warnings.append(f"{key} entries not found: {missing}")
    # dependencies validation
    deps = pkg_data.get("dependencies", [])
    if deps and not isinstance(deps, list):
        errors.append("`dependencies` must be an array")
    else:
        known_names = set(registry_names)
        known_names.update(available_dirs)
        for dep in deps:
            if dep not in known_names:
                warnings.append(f"dependency `{dep}` is not known in registry")
    # common folder existence
    for field, folder in FIELD_TO_FOLDER.items():
        entries = pkg_data.get(field) or []
        if entries and not (pkg_root / folder).exists():
            warnings.append(f"common folder `{folder}` referenced but missing")
    return errors, warnings


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate package metadata and assets.")
    parser.add_argument(
        "--packages-root",
        type=Path,
        default=Path("packages"),
        help="Root folder containing package directories",
    )
    parser.add_argument(
        "--workflow-schema",
        type=Path,
        help="Optional workflow JSON schema (default: config/schema/workflow_v1.schema.json when available)",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Enable debug logging for tracing validation steps",
    )
    args = parser.parse_args()

    logging.basicConfig(
        format="%(levelname)s: %(message)s",
        level=logging.DEBUG if args.verbose else logging.INFO,
    )

    if not args.packages_root.exists():
        logger.error("packages root %s does not exist", args.packages_root)
        return 2

    schema_candidate = args.workflow_schema
    default_schema = Path("config/schema/workflow_v1.schema.json")
    if schema_candidate is None and default_schema.exists():
        schema_candidate = default_schema

    workflow_validator: Optional["Draft7Validator"] = None
    if schema_candidate:
        if not schema_candidate.exists():
            logger.error("specified workflow schema %s not found", schema_candidate)
            return 5
        try:
            workflow_schema = load_json(schema_candidate)
        except json.JSONDecodeError as exc:
            logger.error("invalid JSON schema %s: %s", schema_candidate, exc)
            return 6
        if Draft7Validator is None:
            logger.warning("jsonschema dependency not installed; skipping workflow schema validation")
        else:
            try:
                workflow_validator = Draft7Validator(workflow_schema)
            except Exception as exc:
                logger.error("failed to compile workflow schema %s: %s", schema_candidate, exc)
                return 7

    package_dirs = [
        child
        for child in sorted(args.packages_root.iterdir())
        if child.is_dir() and (child / "package.json").exists()
    ]

    if not package_dirs:
        logger.warning("no package directories with package.json found under %s", args.packages_root)

    loaded_packages: list[tuple[Path, dict]] = []
    summary_errors = 0
    summary_warnings = 0

    for pkg_root in package_dirs:
        pkg_json_file = pkg_root / "package.json"
        try:
            pkg_data = load_json(pkg_json_file)
        except json.JSONDecodeError as exc:
            logger.error("invalid JSON in %s: %s", pkg_json_file, exc)
            summary_errors += 1
            continue
        loaded_packages.append((pkg_root, pkg_data))

    registry_names = [
        pkg_data.get("name")
        for _, pkg_data in loaded_packages
        if isinstance(pkg_data.get("name"), str)
    ]

    available_dirs = [entry.name for entry in args.packages_root.iterdir() if entry.is_dir()]
    for pkg_root, pkg_data in loaded_packages:
        pkg_json_file = pkg_root / "package.json"
        errors, warnings = validate_package(
            pkg_root,
            pkg_data,
            registry_names,
            available_dirs,
            workflow_validator,
        )
        for err in errors:
            logger.error("%s: %s", pkg_json_file, err)
        for warn in warnings:
            logger.warning("%s: %s", pkg_json_file, warn)
        summary_errors += len(errors)
        summary_warnings += len(warnings)

    logger.info("lint complete: %d errors, %d warnings", summary_errors, summary_warnings)
    return 1 if summary_errors else 0


if __name__ == "__main__":
    sys.exit(main())
