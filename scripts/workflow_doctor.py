#!/usr/bin/env python3
"""
workflow_doctor.py

A lightweight diagnostic tool to help ChatGPT/Codex agents reason about GitHub
Actions workflow health without needing to reach out to the GitHub API or run
CI. It performs static checks on the workflow YAML files and surfaces common
security and correctness concerns.
"""
from __future__ import annotations

import argparse
import pathlib
import re
import sys
from typing import Dict, Iterable, List, Tuple, Union

try:
    import yaml  # type: ignore
except ImportError:  # pragma: no cover - defensive
    sys.stderr.write(
        "PyYAML is required to parse workflow files. Install it with `pip install pyyaml`\n"
    )
    sys.exit(2)

PathLike = Union[str, pathlib.Path]


class WorkflowReport:
    def __init__(self, path: pathlib.Path) -> None:
        self.path = path
        self.triggers: List[str] = []
        self.warnings: List[str] = []
        self.infos: List[str] = []

    def add_warning(self, message: str) -> None:
        self.warnings.append(message)

    def add_info(self, message: str) -> None:
        self.infos.append(message)

    def render(self) -> str:
        lines = [f"== {self.path} =="]
        if self.triggers:
            lines.append("Triggers: " + ", ".join(sorted(self.triggers)))
        if self.infos:
            lines.append("Info:")
            for info in self.infos:
                lines.append(f"  • {info}")
        if self.warnings:
            lines.append("Warnings:")
            for warning in self.warnings:
                lines.append(f"  • {warning}")
        if not self.warnings:
            lines.append("No warnings detected.")
        return "\n".join(lines)


def find_workflows(directory: PathLike) -> List[pathlib.Path]:
    base = pathlib.Path(directory)
    if not base.exists():
        raise FileNotFoundError(f"Workflow directory not found: {base}")
    return sorted(base.glob("*.yml")) + sorted(base.glob("*.yaml"))


def load_yaml_file(path: pathlib.Path) -> Dict:
    with path.open("r", encoding="utf-8") as handle:
        return yaml.safe_load(handle) or {}


def collect_triggers(raw_on: Union[Dict, List, str, None]) -> List[str]:
    if raw_on is None:
        return []
    if isinstance(raw_on, str):
        return [raw_on]
    if isinstance(raw_on, list):
        return [str(item) for item in raw_on]
    return list(raw_on.keys())


def classify_action_reference(ref: str) -> str:
    if "@" not in ref:
        return "unpinned"
    _, version = ref.split("@", 1)
    if version.lower() in {"main", "master", "latest", "edge"}:
        return "floating-branch"
    if re.fullmatch(r"v?\d+(\.\d+)*", version):
        # Major/minor tags are better than branches but still float.
        return "floating-tag"
    if re.fullmatch(r"[0-9a-fA-F]{7,40}", version):
        return "pinned-sha"
    return "tagged"


def iter_uses_statements(obj: Dict) -> Iterable[Tuple[str, str]]:
    """Yield (location, uses value) pairs for jobs and steps."""
    jobs = obj.get("jobs", {})
    for job_name, job in jobs.items():
        if isinstance(job, dict) and "uses" in job:
            yield (f"job `{job_name}`", str(job["uses"]))
        for step in job.get("steps", []) or []:
            if isinstance(step, dict) and "uses" in step:
                name = step.get("name") or step.get("id") or "unnamed step"
                yield (f"step `{name}` in job `{job_name}`", str(step["uses"]))


def check_permissions(report: WorkflowReport, workflow: Dict) -> None:
    if "permissions" not in workflow:
        report.add_warning(
            "Workflow does not declare top-level `permissions`. Define minimal permissions to avoid unexpected token scope."
        )
    jobs = workflow.get("jobs", {}) or {}
    for job_name, job in jobs.items():
        if not isinstance(job, dict):
            continue
        if "permissions" not in job:
            report.add_info(
                f"Job `{job_name}` inherits workflow permissions. If it needs fewer privileges, set job-specific `permissions`."
            )


def check_uses(report: WorkflowReport, workflow: Dict) -> None:
    for location, ref in iter_uses_statements(workflow):
        classification = classify_action_reference(ref)
        if classification == "unpinned":
            report.add_warning(
                f"{location} references `{ref}` without a version. Pin to a tag or commit to avoid unexpected updates."
            )
        elif classification == "floating-branch":
            report.add_warning(
                f"{location} uses floating branch `{ref}`. Prefer a stable release or commit SHA."
            )
        elif classification == "floating-tag":
            report.add_warning(
                f"{location} uses floating tag `{ref}`. Pin to a specific version or commit for reproducibility."
            )
        elif classification == "pinned-sha":
            report.add_info(f"{location} pins `{ref}` to a specific commit, which is reproducible.")


def check_pr_target(report: WorkflowReport, workflow: Dict) -> None:
    triggers = set(report.triggers)
    if "pull_request_target" in triggers:
        report.add_warning(
            "`pull_request_target` runs with elevated permissions. Ensure all referenced actions are pinned and inputs validated."
        )


def analyze_workflow(path: pathlib.Path) -> WorkflowReport:
    workflow = load_yaml_file(path)
    report = WorkflowReport(path)
    report.triggers = collect_triggers(workflow.get("on"))
    check_permissions(report, workflow)
    check_uses(report, workflow)
    check_pr_target(report, workflow)
    return report


def parse_args(argv: List[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Static analyzer for GitHub Actions workflow hygiene",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "--workflows-dir",
        default=pathlib.Path(".github/workflows"),
        type=pathlib.Path,
        help="Directory containing workflow YAML files",
    )
    return parser.parse_args(argv)


def main(argv: List[str] | None = None) -> int:
    args = parse_args(argv or sys.argv[1:])
    try:
        workflow_paths = find_workflows(args.workflows_dir)
    except FileNotFoundError as exc:  # pragma: no cover - CLI guard
        sys.stderr.write(str(exc) + "\n")
        return 1

    if not workflow_paths:
        print(f"No workflows found in {args.workflows_dir}")
        return 0

    reports = [analyze_workflow(path) for path in workflow_paths]
    for report in reports:
        print(report.render())
        print()
    return 0


if __name__ == "__main__":  # pragma: no cover - CLI entrypoint
    sys.exit(main())
