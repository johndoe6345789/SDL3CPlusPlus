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
from dataclasses import dataclass
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

logger = logging.getLogger("package_lint")

try:
    from jsonschema import Draft202012Validator
except ImportError:
    Draft202012Validator = None


@dataclass(frozen=True)
class WorkflowSchemaDefinition:
    raw_schema: dict
    top_level_keys: set[str]
    required_top_keys: set[str]
    node_keys: set[str]
    node_required: set[str]
    tag_keys: set[str]
    tag_required: set[str]
    settings_keys: set[str]
    credential_ref_keys: set[str]
    credential_ref_required: set[str]
    credential_binding_keys: set[str]
    credential_binding_required: set[str]
    connection_types: set[str]


def load_json(path: Path) -> dict:
    logger.debug("Reading JSON from %s", path)
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def load_schema_from_roadmap(roadmap_path: Path) -> dict:
    if not roadmap_path.exists():
        raise FileNotFoundError(f"ROADMAP not found at {roadmap_path}")
    lines = roadmap_path.read_text(encoding="utf-8").splitlines()
    in_schema_section = False
    in_block = False
    schema_lines: list[str] = []
    for line in lines:
        if not in_schema_section:
            if line.strip().lower() == "n8n style schema:":
                in_schema_section = True
            continue
        if not in_block:
            if line.strip().startswith("```json"):
                in_block = True
            continue
        if line.strip().startswith("```"):
            break
        schema_lines.append(line)
    if not schema_lines:
        raise ValueError("Failed to locate n8n schema block in ROADMAP.md")
    return json.loads("\n".join(schema_lines))


def build_schema_definition(schema: dict) -> WorkflowSchemaDefinition:
    properties = schema.get("properties") or {}
    required = schema.get("required") or []
    defs = schema.get("$defs") or {}
    node_def = defs.get("node") or {}
    tag_def = defs.get("tag") or {}
    settings_def = defs.get("workflowSettings") or {}
    credential_ref_def = defs.get("credentialRef") or {}
    credential_binding_def = defs.get("credentialBinding") or {}
    connections_def = defs.get("nodeConnectionsByType") or {}
    connection_types = set((connections_def.get("properties") or {}).keys())
    return WorkflowSchemaDefinition(
        raw_schema=schema,
        top_level_keys=set(properties.keys()),
        required_top_keys=set(required),
        node_keys=set((node_def.get("properties") or {}).keys()),
        node_required=set(node_def.get("required") or []),
        tag_keys=set((tag_def.get("properties") or {}).keys()),
        tag_required=set(tag_def.get("required") or []),
        settings_keys=set((settings_def.get("properties") or {}).keys()),
        credential_ref_keys=set((credential_ref_def.get("properties") or {}).keys()),
        credential_ref_required=set(credential_ref_def.get("required") or []),
        credential_binding_keys=set((credential_binding_def.get("properties") or {}).keys()),
        credential_binding_required=set(credential_binding_def.get("required") or []),
        connection_types=connection_types or {"main", "error"},
    )


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


def _is_non_empty_string(value: object) -> bool:
    return isinstance(value, str) and value.strip() != ""


def _is_number(value: object) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool)


def _is_int(value: object) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


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
        if key in {"inputs", "outputs"}:
            issues.extend(_validate_string_map(item, f"parameters.{key}"))
            continue
        issues.extend(_validate_parameter_value(item, f"parameters.{key}"))
    return issues


def _validate_tags(tags: object, schema_def: WorkflowSchemaDefinition) -> list[str]:
    if not isinstance(tags, list):
        return ["tags must be an array"]
    issues: list[str] = []
    for index, tag in enumerate(tags):
        if not isinstance(tag, dict):
            issues.append(f"tags[{index}] must be an object")
            continue
        extra_keys = set(tag.keys()) - schema_def.tag_keys
        if extra_keys:
            issues.append(f"tags[{index}] has unsupported keys: {sorted(extra_keys)}")
        missing_keys = schema_def.tag_required - set(tag.keys())
        if missing_keys:
            issues.append(f"tags[{index}] missing required keys: {sorted(missing_keys)}")
        name = tag.get("name")
        if not _is_non_empty_string(name):
            issues.append(f"tags[{index}].name must be a non-empty string")
        if "id" in tag and not isinstance(tag["id"], (str, int)):
            issues.append(f"tags[{index}].id must be a string or integer")
    return issues


def _validate_settings(settings: object, schema_def: WorkflowSchemaDefinition) -> list[str]:
    if not isinstance(settings, dict):
        return ["settings must be an object"]
    issues: list[str] = []
    extra_keys = set(settings.keys()) - schema_def.settings_keys
    if extra_keys:
        issues.append(f"settings has unsupported keys: {sorted(extra_keys)}")
    if "timezone" in settings and not _is_non_empty_string(settings["timezone"]):
        issues.append("settings.timezone must be a non-empty string")
    if "executionTimeout" in settings:
        value = settings["executionTimeout"]
        if not _is_int(value) or value < 0:
            issues.append("settings.executionTimeout must be an integer >= 0")
    for key in ("saveExecutionProgress", "saveManualExecutions"):
        if key in settings and not isinstance(settings[key], bool):
            issues.append(f"settings.{key} must be a boolean")
    for key in ("saveDataErrorExecution", "saveDataSuccessExecution", "saveDataManualExecution"):
        if key in settings:
            value = settings[key]
            if not _is_non_empty_string(value):
                issues.append(f"settings.{key} must be a non-empty string")
            elif value not in {"all", "none"}:
                issues.append(f"settings.{key} must be 'all' or 'none'")
    if "errorWorkflowId" in settings and not isinstance(settings["errorWorkflowId"], (str, int)):
        issues.append("settings.errorWorkflowId must be a string or integer")
    if "callerPolicy" in settings and not _is_non_empty_string(settings["callerPolicy"]):
        issues.append("settings.callerPolicy must be a non-empty string")
    return issues


def _validate_credential_ref(value: object, context: str, schema_def: WorkflowSchemaDefinition) -> list[str]:
    if not isinstance(value, dict):
        return [f"{context} must be an object"]
    issues: list[str] = []
    extra_keys = set(value.keys()) - schema_def.credential_ref_keys
    if extra_keys:
        issues.append(f"{context} has unsupported keys: {sorted(extra_keys)}")
    missing = schema_def.credential_ref_required - set(value.keys())
    if missing:
        issues.append(f"{context} missing required keys: {sorted(missing)}")
    if "id" in value and not isinstance(value["id"], (str, int)):
        issues.append(f"{context}.id must be a string or integer")
    if "name" in value and not _is_non_empty_string(value["name"]):
        issues.append(f"{context}.name must be a non-empty string")
    return issues


def _validate_credential_binding(value: object, index: int, schema_def: WorkflowSchemaDefinition) -> list[str]:
    context = f"credentials[{index}]"
    if not isinstance(value, dict):
        return [f"{context} must be an object"]
    issues: list[str] = []
    extra_keys = set(value.keys()) - schema_def.credential_binding_keys
    if extra_keys:
        issues.append(f"{context} has unsupported keys: {sorted(extra_keys)}")
    missing = schema_def.credential_binding_required - set(value.keys())
    if missing:
        issues.append(f"{context} missing required keys: {sorted(missing)}")
    if "nodeId" in value and not _is_non_empty_string(value["nodeId"]):
        issues.append(f"{context}.nodeId must be a non-empty string")
    if "credentialType" in value and not _is_non_empty_string(value["credentialType"]):
        issues.append(f"{context}.credentialType must be a non-empty string")
    if "credentialId" in value and not isinstance(value["credentialId"], (str, int)):
        issues.append(f"{context}.credentialId must be a string or integer")
    return issues


def _validate_nodes(nodes: object, schema_def: WorkflowSchemaDefinition) -> tuple[list[str], list[str], list[str]]:
    if not isinstance(nodes, list):
        return ["nodes must be an array"], [], []
    if not nodes:
        return ["nodes must contain at least one node"], [], []
    issues: list[str] = []
    node_names: list[str] = []
    node_ids: list[str] = []
    seen_names: set[str] = set()
    seen_ids: set[str] = set()
    for index, node in enumerate(nodes):
        if not isinstance(node, dict):
            issues.append(f"nodes[{index}] must be an object")
            continue
        extra_keys = set(node.keys()) - schema_def.node_keys
        if extra_keys:
            issues.append(f"nodes[{index}] has unsupported keys: {sorted(extra_keys)}")
        missing_keys = schema_def.node_required - set(node.keys())
        if missing_keys:
            issues.append(f"nodes[{index}] missing required keys: {sorted(missing_keys)}")
        node_id = node.get("id")
        if not _is_non_empty_string(node_id):
            issues.append(f"nodes[{index}].id must be a non-empty string")
        else:
            if node_id in seen_ids:
                issues.append(f"duplicate node id '{node_id}'")
            seen_ids.add(node_id)
            node_ids.append(node_id)
        node_name = node.get("name")
        if not _is_non_empty_string(node_name):
            issues.append(f"nodes[{index}].name must be a non-empty string")
        else:
            if node_name in seen_names:
                issues.append(f"duplicate node name '{node_name}'")
            seen_names.add(node_name)
            node_names.append(node_name)
        node_type = node.get("type")
        if not _is_non_empty_string(node_type):
            issues.append(f"nodes[{index}].type must be a non-empty string")
        version = node.get("typeVersion")
        if version is not None:
            if not _is_number(version) or version < 1:
                issues.append(f"nodes[{index}].typeVersion must be a number >= 1")
        position = node.get("position")
        if position is not None:
            if (not isinstance(position, list) or len(position) != 2 or
                    not all(_is_number(item) for item in position)):
                issues.append(f"nodes[{index}].position must be [x, y] numbers")
        for key in ("disabled", "notesInFlow", "retryOnFail", "continueOnFail",
                    "alwaysOutputData", "executeOnce"):
            if key in node and not isinstance(node[key], bool):
                issues.append(f"nodes[{index}].{key} must be a boolean")
        if "notes" in node and not isinstance(node["notes"], str):
            issues.append(f"nodes[{index}].notes must be a string")
        if "maxTries" in node:
            value = node["maxTries"]
            if not _is_int(value) or value < 1:
                issues.append(f"nodes[{index}].maxTries must be an integer >= 1")
        if "waitBetweenTries" in node:
            value = node["waitBetweenTries"]
            if not _is_int(value) or value < 0:
                issues.append(f"nodes[{index}].waitBetweenTries must be an integer >= 0")
        if "parameters" in node:
            issues.extend(_validate_parameters(node["parameters"]))
        if "credentials" in node:
            credentials = node["credentials"]
            if not isinstance(credentials, dict):
                issues.append(f"nodes[{index}].credentials must be an object")
            else:
                for cred_key, cred_value in credentials.items():
                    if not _is_non_empty_string(cred_key):
                        issues.append(f"nodes[{index}].credentials keys must be non-empty strings")
                        continue
                    issues.extend(
                        _validate_credential_ref(
                            cred_value,
                            f"nodes[{index}].credentials.{cred_key}",
                            schema_def,
                        )
                    )
        if "webhookId" in node and not _is_non_empty_string(node["webhookId"]):
            issues.append(f"nodes[{index}].webhookId must be a non-empty string")
        if "onError" in node:
            value = node["onError"]
            allowed = {"stopWorkflow", "continueRegularOutput", "continueErrorOutput"}
            if not _is_non_empty_string(value) or value not in allowed:
                issues.append(f"nodes[{index}].onError must be one of {sorted(allowed)}")
    return issues, node_names, node_ids


def _validate_connections(connections: object,
                          node_names: set[str],
                          schema_def: WorkflowSchemaDefinition) -> list[str]:
    if not isinstance(connections, dict):
        return ["connections must be an object"]
    issues: list[str] = []
    for from_node, link in connections.items():
        if not _is_non_empty_string(from_node):
            issues.append("connections keys must be non-empty strings")
            continue
        if from_node not in node_names:
            issues.append(f"connections references unknown node '{from_node}'")
        if not isinstance(link, dict):
            issues.append(f"connections.{from_node} must be an object")
            continue
        extra_keys = set(link.keys()) - schema_def.connection_types
        if extra_keys:
            issues.append(f"connections.{from_node} has unsupported keys: {sorted(extra_keys)}")
        if not any(key in link for key in schema_def.connection_types):
            issues.append(f"connections.{from_node} must define at least one connection type")
        for conn_type in schema_def.connection_types:
            if conn_type not in link:
                continue
            index_map = link[conn_type]
            if not isinstance(index_map, dict):
                issues.append(f"connections.{from_node}.{conn_type} must be an object")
                continue
            for index_key, targets in index_map.items():
                if not _is_non_empty_string(index_key) or not index_key.isdigit():
                    issues.append(
                        f"connections.{from_node}.{conn_type} index keys must be numeric strings"
                    )
                    continue
                if not isinstance(targets, list):
                    issues.append(
                        f"connections.{from_node}.{conn_type}.{index_key} must be an array"
                    )
                    continue
                for target_index, target in enumerate(targets):
                    context = f"connections.{from_node}.{conn_type}.{index_key}[{target_index}]"
                    if not isinstance(target, dict):
                        issues.append(f"{context} must be an object")
                        continue
                    extra_keys = set(target.keys()) - {"node", "type", "index"}
                    if extra_keys:
                        issues.append(f"{context} has unsupported keys: {sorted(extra_keys)}")
                    node_name = target.get("node")
                    if not _is_non_empty_string(node_name):
                        issues.append(f"{context}.node must be a non-empty string")
                    elif node_name not in node_names:
                        issues.append(f"{context} references unknown node '{node_name}'")
                    if "type" in target and not _is_non_empty_string(target["type"]):
                        issues.append(f"{context}.type must be a non-empty string")
                    if "index" in target:
                        index_value = target["index"]
                        if not _is_int(index_value) or index_value < 0:
                            issues.append(f"{context}.index must be an integer >= 0")
    return issues


def validate_workflow_structure(workflow_path: Path,
                                content: dict,
                                schema_def: WorkflowSchemaDefinition) -> list[str]:
    issues: list[str] = []
    logger.debug("Validating workflow structure: %s", workflow_path)
    extra_keys = set(content.keys()) - schema_def.top_level_keys
    if extra_keys:
        issues.append(f"unsupported workflow keys: {sorted(extra_keys)}")
    missing_keys = schema_def.required_top_keys - set(content.keys())
    if missing_keys:
        issues.append(f"workflow missing required keys: {sorted(missing_keys)}")
    if "name" in content and not _is_non_empty_string(content["name"]):
        issues.append("workflow name must be a non-empty string")
    if "id" in content and not isinstance(content["id"], (str, int)):
        issues.append("workflow id must be a string or integer")
    if "active" in content and not isinstance(content["active"], bool):
        issues.append("workflow active must be a boolean")
    for key in ("versionId", "createdAt", "updatedAt"):
        if key in content and not isinstance(content[key], str):
            issues.append(f"workflow {key} must be a string")
    if "tags" in content:
        issues.extend(_validate_tags(content["tags"], schema_def))
    if "meta" in content and not isinstance(content["meta"], dict):
        issues.append("workflow meta must be an object")
    if "settings" in content:
        issues.extend(_validate_settings(content["settings"], schema_def))
    if "pinData" in content:
        pin_data = content["pinData"]
        if not isinstance(pin_data, dict):
            issues.append("workflow pinData must be an object")
        else:
            for pin_key, pin_value in pin_data.items():
                if not _is_non_empty_string(pin_key):
                    issues.append("workflow pinData keys must be non-empty strings")
                    continue
                if not isinstance(pin_value, list):
                    issues.append(f"workflow pinData.{pin_key} must be an array")
                    continue
                for entry_index, entry in enumerate(pin_value):
                    if not isinstance(entry, dict):
                        issues.append(f"workflow pinData.{pin_key}[{entry_index}] must be an object")
    if "staticData" in content and not isinstance(content["staticData"], dict):
        issues.append("workflow staticData must be an object")
    node_issues: list[str] = []
    node_names: list[str] = []
    node_ids: list[str] = []
    if "nodes" in content:
        node_issues, node_names, node_ids = _validate_nodes(content["nodes"], schema_def)
        issues.extend(node_issues)
    if "connections" in content:
        issues.extend(_validate_connections(content["connections"], set(node_names), schema_def))
    if "credentials" in content:
        credentials = content["credentials"]
        if not isinstance(credentials, list):
            issues.append("workflow credentials must be an array")
        else:
            for index, entry in enumerate(credentials):
                issues.extend(_validate_credential_binding(entry, index, schema_def))
    if node_ids and "credentials" in content and isinstance(content.get("credentials"), list):
        known_ids = set(node_ids)
        for index, entry in enumerate(content.get("credentials", [])):
            if isinstance(entry, dict) and "nodeId" in entry:
                node_id = entry["nodeId"]
                if isinstance(node_id, str) and node_id not in known_ids:
                    issues.append(
                        f"credentials[{index}].nodeId references unknown node id '{node_id}'"
                    )
    return issues


def validate_workflow(workflow_path: Path,
                      validator: Optional["Draft202012Validator"],
                      schema_def: WorkflowSchemaDefinition) -> list[str]:
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
    issues.extend(validate_workflow_structure(workflow_path, content, schema_def))
    return issues


def validate_package(
    pkg_root: Path,
    pkg_data: dict,
    registry_names: Sequence[str],
    available_dirs: Sequence[str],
    workflow_schema_validator: Optional["Draft202012Validator"],
    workflow_schema_def: WorkflowSchemaDefinition,
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
                                                 workflow_schema_def)
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
        "--roadmap",
        type=Path,
        default=Path("ROADMAP.md"),
        help="Path to ROADMAP containing the n8n workflow schema",
    )
    parser.add_argument(
        "--workflow-schema",
        type=Path,
        help="Optional workflow JSON schema override",
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
    if schema_candidate is None:
        schema_candidate = args.roadmap

    workflow_schema: Optional[dict] = None
    if schema_candidate:
        if not schema_candidate.exists():
            logger.error("specified workflow schema source %s not found", schema_candidate)
            return 5
        try:
            workflow_schema = (
                load_json(schema_candidate)
                if schema_candidate.suffix == ".json"
                else load_schema_from_roadmap(schema_candidate)
            )
        except (json.JSONDecodeError, ValueError, FileNotFoundError) as exc:
            logger.error("invalid workflow schema source %s: %s", schema_candidate, exc)
            return 6

    if not workflow_schema:
        logger.error("workflow schema could not be loaded")
        return 7

    workflow_schema_def = build_schema_definition(workflow_schema)
    workflow_validator: Optional["Draft202012Validator"] = None
    if Draft202012Validator is None:
        logger.warning("jsonschema dependency not installed; skipping JSON Schema validation")
    else:
        try:
            workflow_validator = Draft202012Validator(workflow_schema)
        except Exception as exc:
            logger.error("failed to compile workflow schema: %s", exc)
            return 8
    logger.info("workflow schema loaded from %s", schema_candidate)

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
            workflow_schema_def,
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
