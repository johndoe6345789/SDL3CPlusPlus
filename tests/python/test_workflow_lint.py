"""Invariants that catch workflow wiring bugs before they reach a GPU.

Each rule here corresponds to a defect found in this repo's own packages.
"""

from workflow_graph import WorkflowGraph
from workflow_lint import lint

BEGIN = "frame.gpu.begin_offscreen"


def rules(doc):
    return {f.rule for f in lint(WorkflowGraph(doc))}


def chain(*ids):
    """Connections wiring the given node ids into one linear chain."""
    return {a: {"main": {"0": [{"node": b}]}} for a, b in zip(ids, ids[1:])}


def test_clean_graph_reports_nothing():
    doc = {"nodes": [{"id": "a", "type": "input.poll"},
                     {"id": "b", "type": BEGIN},
                     {"id": "c", "type": "draw.textured"},
                     {"id": "d", "type": "frame.gpu.end_scene"}],
           "connections": chain("a", "b", "c", "d")}
    assert rules(doc) == set()


def test_flags_an_orphan_second_root():
    """seed frame_tick.json: the room geometry chain hung off nothing."""
    doc = {"nodes": [{"id": "a", "type": "input.poll"},
                     {"id": "b", "type": BEGIN},
                     {"id": "d", "type": "frame.gpu.end_scene"},
                     {"id": "orphan", "type": "draw.textured"}],
           "connections": chain("a", "b", "d")}
    assert "multiple-entry-points" in rules(doc)


def test_flags_a_draw_issued_before_the_pass_begins():
    """The visible symptom of the orphan root: geometry outside the pass."""
    doc = {"nodes": [{"id": "early", "type": "draw.textured"},
                     {"id": "b", "type": BEGIN},
                     {"id": "d", "type": "frame.gpu.end_scene"}],
           "connections": chain("early", "b", "d")}
    assert "draw-outside-render-pass" in rules(doc)


def test_flags_a_draw_issued_after_the_pass_ends():
    doc = {"nodes": [{"id": "b", "type": BEGIN},
                     {"id": "d", "type": "frame.gpu.end_scene"},
                     {"id": "late", "type": "draw.textured"}],
           "connections": chain("b", "d", "late")}
    assert "draw-outside-render-pass" in rules(doc)


def test_flags_a_loop_whose_condition_is_never_set():
    """seed_game.json: game_loop waited on a key nothing wrote."""
    doc = {"nodes": [{"id": "loop", "type": "control.loop.while",
                      "parameters": {"condition_key": "game_running"}}],
           "connections": {}}
    assert "loop-condition-never-set" in rules(doc)


def test_accepts_a_loop_whose_condition_is_set_upstream():
    doc = {"nodes": [{"id": "init", "type": "value.literal",
                      "parameters": {"outputs": {"value": "game_running"}}},
                     {"id": "loop", "type": "control.loop.while",
                      "parameters": {"condition_key": "game_running"}}],
           "connections": chain("init", "loop")}
    assert "loop-condition-never-set" not in rules(doc)


def test_findings_name_the_offending_node():
    doc = {"nodes": [{"id": "loop", "type": "control.loop.while",
                      "parameters": {"condition_key": "nope"}}],
           "connections": {}}
    assert [f.node for f in lint(WorkflowGraph(doc))] == ["loop"]


def test_loop_without_a_condition_key_is_left_to_the_engine():
    """control.loop.while itself throws on a missing condition_key."""
    doc = {"nodes": [{"id": "loop", "type": "control.loop.while"}],
           "connections": {}}
    assert "loop-condition-never-set" not in rules(doc)
