"""Every shipped workflow must satisfy the graph invariants.

This is the regression net: the flickering seed demo and its dead
flashlight were both a wiring defect this catches statically.
"""

import glob
from pathlib import Path

import pytest

from workflow_graph import WorkflowGraph
from workflow_lint import lint

ROOT = Path(__file__).resolve().parents[2]
WORKFLOWS = sorted(glob.glob(str(ROOT / "packages/*/workflows/*.json")))


def test_packages_are_present():
    assert WORKFLOWS, "no package workflows found to lint"


@pytest.mark.parametrize("path", WORKFLOWS, ids=lambda p: Path(p).stem)
def test_workflow_graph_is_well_formed(path):
    findings = lint(WorkflowGraph.from_file(path))
    assert not findings, "\n".join(
        f"[{f.rule}] {f.node}: {f.message}" for f in findings)


@pytest.mark.parametrize("path", WORKFLOWS, ids=lambda p: Path(p).stem)
def test_workflow_is_acyclic(path):
    WorkflowGraph.from_file(path).order()
