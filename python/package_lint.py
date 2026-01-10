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
WORKFLOW_TOP_LEVEL_KEYS = {"template", "nodes", "steps", "connections"}
WORKFLOW_NODE_KEYS = {"id", "name", "plugin", "type", "position", "inputs", "outputs", "parameters"}
PACKAGE_ALLOWED_KEYS = {
    "name",
    "version",
    "description",
    "defaultWorkflow",
    "workflows",
    "assets",
    "scene",
    "shaders",
    "dependencies",
    "bundled",
    "notes",
}


class WorkflowReferenceProfile:
    def __init__(self,
                 required_top_keys: set[str],
                 allowed_top_keys: set[str],
                 require_nodes: bool,
                 require_template: bool,
                 require_connections: bool,
                 require_id: bool,
                 require_plugin: bool,
                 require_position: bool):
        self.required_top_keys = required_top_keys
        self.allowed_top_keys = allowed_top_keys
        self.require_nodes = require_nodes
        self.require_template = require_template
        self.require_connections = require_connections
        self.require_id = require_id
        self.require_plugin = require_plugin
        self.require_position = require_position


def build_workflow_profile(reference: dict) -> WorkflowReferenceProfile:
    required_top_keys = set(reference.keys())
    allowed_top_keys = set(reference.keys())
    require_nodes = "nodes" in reference
    require_template = "template" in reference
    require_connections = "connections" in reference
    require_id = True
    require_plugin = True
    require_position = False
    if require_nodes:
        nodes = reference.get("nodes")
        if isinstance(nodes, list) and nodes:
            require_position = all(
                isinstance(node, dict) and "position" in node
                for node in nodes
            )
    return WorkflowReferenceProfile(
        required_top_keys,
        allowed_top_keys,
        require_nodes,
        require_template,
        require_connections,
        require_id,
        require_plugin,
        require_position,
    )

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


def _is_non_empty_string(value: object) -> bool:
    return isinstance(value, str) and value.strip() != ""


def _validate_string_map(value: object, context: str) -> list[str]:
    if not isinstance(value, dict):
        return [f"{context} must be an object"]
    issues: list[str] = []
    for key, item in value.items():
        if not _is_non_empty_string(key):
            issues.append(f"{context} keys must be non-empty strings")
            continue
        if not _is_non_empty_string(item):
            issues.append(f"{context}.{key} must map to a non-empty string")
    return issues


def _validate_parameter_value(value: object, context: str) -> list[str]:
    if isinstance(value, (str, bool, int, float)):
        if isinstance(value, str) and value.strip() == "":
            return [f"{context} must be a non-empty string"]
        return []
    if isinstance(value, list):
        if not value:
            return []
        has_strings = any(isinstance(item, str) for item in value)
        has_numbers = any(isinstance(item, (int, float)) for item in value)
        has_other = any(not isinstance(item, (str, int, float)) for item in value)
        if has_other:
            return [f"{context} must contain only strings or numbers"]
        if has_strings and has_numbers:
            return [f"{context} cannot mix strings and numbers"]
        if has_strings and any(item.strip() == "" for item in value if isinstance(item, str)):
            return [f"{context} cannot contain empty strings"]
        return []
    return [f"{context} must be a string, number, bool, or array"]


def _validate_parameters(value: object) -> list[str]:
    if not isinstance(value, dict):
        return ["parameters must be an object"]
    issues: list[str] = []
    for key, item in value.items():
        if not _is_non_empty_string(key):
            issues.append("parameters keys must be non-empty strings")
            continue
        issues.extend(_validate_parameter_value(item, f"parameters.{key}"))
    return issues


def _validate_node_entry(node: dict,
                         index: int,
                         reference_profile: Optional[WorkflowReferenceProfile]) -> tuple[str, list[str]]:
    issues: list[str] = []
    if not isinstance(node, dict):
        return "", [f"nodes[{index}] must be an object"]
    extra_keys = set(node.keys()) - WORKFLOW_NODE_KEYS
    if extra_keys:
        issues.append(
            f"nodes[{index}] has unsupported keys: {sorted(extra_keys)}"
        )
    node_id = node.get("id") if reference_profile and reference_profile.require_id else node.get("id") or node.get("name")
    if not _is_non_empty_string(node_id):
        issues.append(f"nodes[{index}] requires non-empty id")
    plugin = node.get("plugin") if reference_profile and reference_profile.require_plugin else node.get("plugin") or node.get("type")
    if not _is_non_empty_string(plugin):
        issues.append(f"nodes[{index}] requires non-empty plugin")
    if "inputs" in node:
        issues.extend(_validate_string_map(node["inputs"], f"nodes[{index}].inputs"))
    if "outputs" in node:
        issues.extend(_validate_string_map(node["outputs"], f"nodes[{index}].outputs"))
    if "parameters" in node:
        issues.extend(_validate_parameters(node["parameters"]))
    if reference_profile and reference_profile.require_position and "position" not in node:
        issues.append(f"nodes[{index}] requires position")
    if "position" in node:
        position = node["position"]
        if (not isinstance(position, list) or len(position) != 2 or
                not all(isinstance(item, (int, float)) for item in position)):
            issues.append(f"nodes[{index}].position must be [x, y] numbers")
    return (node_id if isinstance(node_id, str) else ""), issues


def _validate_connections(connections: object, node_ids: set[str]) -> list[str]:
    if not isinstance(connections, dict):
        return ["connections must be an object"]
    issues: list[str] = []
    for from_node, link in connections.items():
        if not _is_non_empty_string(from_node):
            issues.append("connections keys must be non-empty strings")
            continue
        if from_node not in node_ids:
            issues.append(f"connections references unknown node '{from_node}'")
        if not isinstance(link, dict):
            issues.append(f"connections.{from_node} must be an object")
            continue
        extra_keys = set(link.keys()) - {"main"}
        if extra_keys:
            issues.append(f"connections.{from_node} has unsupported keys: {sorted(extra_keys)}")
        if "main" not in link:
            continue
        main_value = link["main"]
        if not isinstance(main_value, list):
            issues.append(f"connections.{from_node}.main must be an array")
            continue
        for branch_index, branch in enumerate(main_value):
            if not isinstance(branch, list):
                issues.append(f"connections.{from_node}.main[{branch_index}] must be an array")
                continue
            for entry_index, entry in enumerate(branch):
                if not isinstance(entry, dict):
                    issues.append(
                        f"connections.{from_node}.main[{branch_index}][{entry_index}] must be an object"
                    )
                    continue
                node_name = entry.get("node")
                if not _is_non_empty_string(node_name):
                    issues.append(
                        f"connections.{from_node}.main[{branch_index}][{entry_index}] missing node"
                    )
                    continue
                if node_name not in node_ids:
                    issues.append(
                        f"connections.{from_node}.main[{branch_index}][{entry_index}] "
                        f"references unknown node '{node_name}'"
                    )
                if "type" in entry and not _is_non_empty_string(entry["type"]):
                    issues.append(
                        f"connections.{from_node}.main[{branch_index}][{entry_index}].type "
                        "must be a non-empty string"
                    )
                if "index" in entry and not isinstance(entry["index"], int):
                    issues.append(
                        f"connections.{from_node}.main[{branch_index}][{entry_index}].index "
                        "must be an integer"
                    )
    return issues


def validate_workflow_structure(workflow_path: Path,
                                content: dict,
                                reference_profile: Optional[WorkflowReferenceProfile]) -> list[str]:
    issues: list[str] = []
    logger.debug("Validating workflow structure: %s", workflow_path)
    allowed_top_keys = WORKFLOW_TOP_LEVEL_KEYS
    required_top_keys = set()
    if reference_profile:
        allowed_top_keys = reference_profile.allowed_top_keys
        required_top_keys = reference_profile.required_top_keys
    extra_keys = set(content.keys()) - allowed_top_keys
    if extra_keys:
        issues.append(f"unsupported workflow keys: {sorted(extra_keys)}")
    missing_keys = required_top_keys - set(content.keys())
    if missing_keys:
        issues.append(f"workflow missing required keys: {sorted(missing_keys)}")
    has_nodes = "nodes" in content
    has_steps = "steps" in content
    if has_nodes and has_steps:
        issues.append("workflow cannot define both 'nodes' and 'steps'")
    if reference_profile and reference_profile.require_nodes and has_steps:
        issues.append("workflow must not define 'steps' when using reference schema")
    if not has_nodes and not has_steps:
        issues.append("workflow must define 'nodes' or 'steps'")
        return issues
    if reference_profile and reference_profile.require_template and "template" not in content:
        issues.append("workflow missing required template")
    if "template" in content and not _is_non_empty_string(content["template"]):
        issues.append("workflow template must be a non-empty string")
    if reference_profile and reference_profile.require_connections and "connections" not in content:
        issues.append("workflow missing required connections")
    node_ids: list[str] = []
    if has_nodes:
        nodes = content.get("nodes")
        if not isinstance(nodes, list) or not nodes:
            issues.append("workflow nodes must be a non-empty array")
        else:
            seen = set()
            for index, node in enumerate(nodes):
                node_id, node_issues = _validate_node_entry(node, index, reference_profile)
                issues.extend(node_issues)
                if node_id:
                    if node_id in seen:
                        issues.append(f"duplicate node id '{node_id}'")
                    else:
                        seen.add(node_id)
                        node_ids.append(node_id)
    if has_steps:
        steps = content.get("steps")
        if not isinstance(steps, list) or not steps:
            issues.append("workflow steps must be a non-empty array")
        else:
            seen = set()
            for index, step in enumerate(steps):
                node_id, node_issues = _validate_node_entry(step, index, reference_profile)
                issues.extend(node_issues)
                if node_id:
                    if node_id in seen:
                        issues.append(f"duplicate step id '{node_id}'")
                    else:
                        seen.add(node_id)
                        node_ids.append(node_id)
    if "connections" in content:
        issues.extend(_validate_connections(content["connections"], set(node_ids)))
    return issues


def validate_workflow(workflow_path: Path,
                      validator: Optional["Draft7Validator"],
                      reference_profile: Optional[WorkflowReferenceProfile]) -> list[str]:
    try:
        content = load_json(workflow_path)
    except json.JSONDecodeError as exc:
        return [f"invalid JSON: {exc}"]
    issues: list[str] = []
    if validator:
        for err in sorted(
            validator.iter_errors(content),
            key=lambda x: tuple(x.absolute_path),
        ):
            pointer = "/".join(str(part) for part in err.absolute_path) or "<root>"
            issues.append(f"schema violation at {pointer}: {err.message}")
    issues.extend(validate_workflow_structure(workflow_path, content, reference_profile))
    return issues


def validate_package(
    pkg_root: Path,
    pkg_data: dict,
    registry_names: Sequence[str],
    available_dirs: Sequence[str],
    workflow_schema_validator: Optional["Draft7Validator"] = None,
    workflow_reference_profile: Optional[WorkflowReferenceProfile] = None,
) -> tuple[list[str], list[str]]:
    errors: list[str] = []
    warnings: list[str] = []

    logger.debug("Validating %s", pkg_root)
    extra_package_keys = set(pkg_data.keys()) - PACKAGE_ALLOWED_KEYS
    if extra_package_keys:
        warnings.append(f"unknown package keys: {sorted(extra_package_keys)}")

    for field in REQUIRED_FIELDS:
        if field not in pkg_data:
            errors.append(f"missing required field `{field}`")
    workflows = pkg_data.get("workflows")
    default_workflow = pkg_data.get("defaultWorkflow")
    if workflows is not None:
        if not isinstance(workflows, list):
            errors.append("`workflows` must be an array")
        elif not workflows:
            errors.append("`workflows` must include at least one entry")
        elif default_workflow and default_workflow not in workflows:
            errors.append("`defaultWorkflow` is not present in `workflows` array")
    if "name" in pkg_data and not _is_non_empty_string(pkg_data["name"]):
        errors.append("`name` must be a non-empty string")
    if "version" in pkg_data and not _is_non_empty_string(pkg_data["version"]):
        errors.append("`version` must be a non-empty string")
    if "description" in pkg_data and not _is_non_empty_string(pkg_data["description"]):
        errors.append("`description` must be a non-empty string")
    if default_workflow is not None and not _is_non_empty_string(default_workflow):
        errors.append("`defaultWorkflow` must be a non-empty string")
    if _is_non_empty_string(default_workflow):
        candidate = pkg_root / default_workflow
        if not candidate.exists():
            errors.append(f"`defaultWorkflow` does not exist: {default_workflow}")
    if "bundled" in pkg_data and not isinstance(pkg_data["bundled"], bool):
        errors.append("`bundled` must be a boolean")
    if "notes" in pkg_data and not _is_non_empty_string(pkg_data["notes"]):
        warnings.append("`notes` should be a non-empty string when present")
    # schema-like validations
    for key in ("workflows", "assets", "scene", "shaders"):
        value = pkg_data.get(key)
        if value is None:
            continue
        if not isinstance(value, list):
            errors.append(f"`{key}` must be an array if present")
            continue
        if not value and key == "workflows":
            errors.append("`workflows` must include at least one entry")
        for entry in value:
            if not isinstance(entry, str):
                errors.append(f"`{key}` entries must be strings")
        on_exist: Optional[Callable[[Path, str], None]] = None
        if key == "workflows":
            def on_exist(candidate: Path, rel: str) -> None:
                schema_issues = validate_workflow(candidate,
                                                 workflow_schema_validator,
                                                 workflow_reference_profile)
                for issue in schema_issues:
                    errors.append(f"workflow `{rel}`: {issue}")
        def validate_entry(entry: str) -> None:
            if ".." in Path(entry).parts:
                errors.append(f"`{key}` entry '{entry}' must not contain '..'")
            if entry.strip() == "":
                errors.append(f"`{key}` entries must be non-empty strings")
            if key == "workflows" and not entry.endswith(".json"):
                errors.append(f"`workflows` entry '{entry}' must be a .json file")
            if entry.endswith(".json"):
                try:
                    load_json(pkg_root / entry)
                except json.JSONDecodeError as exc:
                    errors.append(f"`{key}` entry '{entry}' invalid JSON: {exc}")

        for entry in value:
            if isinstance(entry, str):
                validate_entry(entry)
        missing = check_paths(pkg_root, value, key, on_exist=on_exist)
        if missing:
            warnings.append(f"{key} entries not found: {missing}")
        string_entries = [entry for entry in value if isinstance(entry, str)]
        if len(set(string_entries)) != len(string_entries):
            warnings.append(f"`{key}` entries contain duplicates")
    # dependencies validation
    deps = pkg_data.get("dependencies", [])
    if deps is None:
        deps = []
    if not isinstance(deps, list):
        errors.append("`dependencies` must be an array")
    else:
        known_names = set(registry_names)
        known_names.update(available_dirs)
        for dep in deps:
            if not _is_non_empty_string(dep):
                errors.append("`dependencies` entries must be non-empty strings")
                continue
            if dep == pkg_data.get("name"):
                errors.append("`dependencies` cannot include the package itself")
            if dep not in known_names:
                warnings.append(f"dependency `{dep}` is not known in registry")
        dep_strings = [dep for dep in deps if isinstance(dep, str)]
        if len(set(dep_strings)) != len(dep_strings):
            warnings.append("`dependencies` contains duplicates")
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
        "--workflow-reference",
        type=Path,
        help="Reference n8n-style workflow JSON used to validate workflow structure "
             "(default: packages/seed/workflows/demo_gameplay.json when available)",
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

    reference_path = args.workflow_reference
    default_reference = Path("packages/seed/workflows/demo_gameplay.json")
    if reference_path is None and default_reference.exists():
        reference_path = default_reference

    workflow_reference_profile: Optional[WorkflowReferenceProfile] = None
    if reference_path:
        if not reference_path.exists():
            logger.error("specified workflow reference %s not found", reference_path)
            return 8
        try:
            reference_workflow = load_json(reference_path)
        except json.JSONDecodeError as exc:
            logger.error("invalid workflow reference %s: %s", reference_path, exc)
            return 9
        workflow_reference_profile = build_workflow_profile(reference_workflow)
        logger.info("workflow reference loaded: %s", reference_path)

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
            workflow_reference_profile,
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
