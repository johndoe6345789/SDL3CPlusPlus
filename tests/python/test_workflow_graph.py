"""Graph model for workflow JSON, covering both authoring schemas."""

import pytest

from workflow_graph import WorkflowGraph

NODES = {
    "nodes": [
        {"id": "a", "type": "input.poll"},
        {"id": "b", "type": "frame.gpu.begin_offscreen"},
        {"id": "c", "type": "draw.textured"},
    ],
    "connections": {
        "a": {"main": {"0": [{"node": "b"}]}},
        "b": {"main": {"0": [{"node": "c"}]}},
    },
}

STEPS = {
    "steps": [
        {"id": "one", "plugin": "frame.begin"},
        {"id": "two", "plugin": "frame.render"},
    ]
}


def test_reads_node_schema():
    g = WorkflowGraph(NODES)
    assert g.types == {"a": "input.poll",
                       "b": "frame.gpu.begin_offscreen",
                       "c": "draw.textured"}


def test_reads_step_schema_as_a_sequential_chain():
    g = WorkflowGraph(STEPS)
    assert g.order() == ["one", "two"]
    assert g.roots() == ["one"]


def test_single_root_when_fully_connected():
    assert WorkflowGraph(NODES).roots() == ["a"]


def test_detects_a_second_orphan_root():
    doc = {"nodes": NODES["nodes"] + [{"id": "z", "type": "draw.textured"}],
           "connections": NODES["connections"]}
    assert WorkflowGraph(doc).roots() == ["a", "z"]


def test_order_is_topological():
    order = WorkflowGraph(NODES).order()
    assert order.index("b") < order.index("c")


def test_order_covers_every_node():
    doc = {"nodes": NODES["nodes"] + [{"id": "z", "type": "draw.textured"}],
           "connections": NODES["connections"]}
    assert sorted(WorkflowGraph(doc).order()) == ["a", "b", "c", "z"]


def test_cycle_is_reported_rather_than_hanging():
    doc = {"nodes": [{"id": "a", "type": "t"}, {"id": "b", "type": "t"}],
           "connections": {"a": {"main": {"0": [{"node": "b"}]}},
                           "b": {"main": {"0": [{"node": "a"}]}}}}
    with pytest.raises(ValueError, match="cycle"):
        WorkflowGraph(doc).order()


def test_collects_context_keys_a_node_writes():
    doc = {"nodes": [{"id": "lit", "type": "value.literal",
                      "parameters": {"value": True,
                                     "outputs": {"value": "game_running"}}},
                     {"id": "bsp", "type": "bsp.load",
                      "parameters": {"output_key": "q3_map"}}],
           "connections": {}}
    g = WorkflowGraph(doc)
    assert g.writes("lit") == {"game_running"}
    assert g.writes("bsp") == {"q3_map"}


def test_connections_may_reference_nodes_by_name():
    """The C++ resolver maps display names to ids; so must we."""
    doc = {"nodes": [{"id": "a", "name": "Begin Frame", "type": "t"},
                     {"id": "b", "name": "Step Physics", "type": "t"}],
           "connections": {"Begin Frame":
                           {"main": {"0": [{"node": "Step Physics"}]}}}}
    g = WorkflowGraph(doc)
    assert g.roots() == ["a"]
    assert g.order() == ["a", "b"]


def test_unknown_connection_endpoints_are_ignored():
    doc = {"nodes": [{"id": "a", "type": "t"}],
           "connections": {"a": {"main": {"0": [{"node": "ghost"}]}}}}
    assert WorkflowGraph(doc).order() == ["a"]
