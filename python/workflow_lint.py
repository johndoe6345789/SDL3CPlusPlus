"""Static checks for workflow graphs.

Every rule here corresponds to a defect that shipped in this repo and was
only visible by watching the screen: geometry drawn outside the render
pass, and a frame loop whose condition nobody set.
"""

from __future__ import annotations

from typing import NamedTuple

from workflow_graph import WorkflowGraph

PASS_BEGIN = ("frame.gpu.begin", "frame.gpu.begin_offscreen", "frame.begin")
PASS_END = ("frame.gpu.end", "frame.gpu.end_scene", "frame.render")
DRAW_PREFIXES = ("draw.", "q3.md3.draw", "q3.bots.draw", "q3.pickups.draw")


class Finding(NamedTuple):
    rule: str
    node: str
    message: str


def _is_draw(step_type: str) -> bool:
    return any(step_type.startswith(p) for p in DRAW_PREFIXES)


def check_single_entry(graph: WorkflowGraph) -> list[Finding]:
    roots = graph.roots()
    if len(roots) <= 1:
        return []
    return [Finding("multiple-entry-points", r,
                    f"'{r}' has no inbound edge, so its chain is interleaved "
                    "into the frame at an arbitrary point")
            for r in roots[1:]]


def check_draws_inside_pass(graph: WorkflowGraph) -> list[Finding]:
    order = graph.order()
    begins = [i for i, n in enumerate(order) if graph.types[n] in PASS_BEGIN]
    ends = [i for i, n in enumerate(order) if graph.types[n] in PASS_END]
    if not begins or not ends:
        return []
    first, last = begins[0], ends[-1]
    return [Finding("draw-outside-render-pass", n,
                    f"'{n}' is ordered outside the render pass; its "
                    "geometry and per-frame uniforms are lost")
            for i, n in enumerate(order)
            if _is_draw(graph.types[n]) and not first < i < last]


def check_loop_conditions(graph: WorkflowGraph) -> list[Finding]:
    order = graph.order()
    findings = []
    for index, node in enumerate(order):
        if graph.types[node] != "control.loop.while":
            continue
        key = (graph.params.get(node) or {}).get("condition_key")
        if not isinstance(key, str):
            continue
        if any(key in graph.writes(n) for n in order[:index]):
            continue
        findings.append(Finding(
            "loop-condition-never-set", node,
            f"'{node}' loops while '{key}' is true, but no earlier step "
            "writes that key, so the body never runs"))
    return findings


CHECKS = (check_single_entry, check_draws_inside_pass, check_loop_conditions)


def lint(graph: WorkflowGraph) -> list[Finding]:
    return [f for check in CHECKS for f in check(graph)]
