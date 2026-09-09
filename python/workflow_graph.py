"""Graph model of a workflow document.

Two authoring schemas exist. The n8n-style one lists "nodes" with a
separate "connections" map and is ordered by topological sort; the
simpler one lists "steps" that run in sequence. Both normalise to the
same node/edge model here so invariants can be checked once.
"""

from __future__ import annotations

import collections
import json
from pathlib import Path


class WorkflowGraph:
    def __init__(self, document: dict):
        self.types: dict[str, str] = {}
        self.params: dict[str, dict] = {}
        self.edges: list[tuple[str, str]] = []
        self._by_name: dict[str, str] = {}
        if "steps" in document:
            self._load_steps(document["steps"])
        else:
            self._load_nodes(document)

    @classmethod
    def from_file(cls, path: str | Path) -> "WorkflowGraph":
        return cls(json.loads(Path(path).read_text()))

    def _record(self, entry: dict, kind: str) -> str:
        node_id = entry.get("id", "")
        self.types[node_id] = entry.get(kind, "")
        self.params[node_id] = entry.get("parameters", {}) or {}
        self._by_name[node_id] = node_id
        if entry.get("name"):
            self._by_name[entry["name"]] = node_id
        return node_id

    def _resolve(self, reference: str) -> str | None:
        """Connections may name a node by id or by display name."""
        return self._by_name.get(reference)

    def _load_steps(self, steps: list) -> None:
        ids = [self._record(s, "plugin") for s in steps]
        self.edges = list(zip(ids, ids[1:]))

    def _load_nodes(self, document: dict) -> None:
        for entry in document.get("nodes", []):
            self._record(entry, "type")
        for source, conn in (document.get("connections") or {}).items():
            src = self._resolve(source)
            for targets in (conn.get("main") or {}).values():
                for target in targets:
                    dst = self._resolve(target.get("node", ""))
                    if src and dst:
                        self.edges.append((src, dst))

    def indegree(self) -> collections.Counter:
        degree = collections.Counter({n: 0 for n in self.types})
        for _, dst in self.edges:
            degree[dst] += 1
        return degree

    def roots(self) -> list[str]:
        degree = self.indegree()
        return [n for n in self.types if degree[n] == 0]

    def order(self) -> list[str]:
        """Kahn's algorithm, keeping declaration order among ready nodes."""
        degree = self.indegree()
        adjacency: dict[str, list[str]] = collections.defaultdict(list)
        for src, dst in self.edges:
            adjacency[src].append(dst)
        ready = [n for n in self.types if degree[n] == 0]
        ordered: list[str] = []
        while ready:
            node = ready.pop(0)
            ordered.append(node)
            for nxt in adjacency[node]:
                degree[nxt] -= 1
                if degree[nxt] == 0:
                    ready.append(nxt)
        if len(ordered) != len(self.types):
            raise ValueError("workflow contains a cycle")
        return ordered

    def writes(self, node_id: str) -> set[str]:
        """Context keys this node publishes for later nodes to read."""
        params = self.params.get(node_id, {})
        keys = set((params.get("outputs") or {}).values())
        if isinstance(params.get("output_key"), str):
            keys.add(params["output_key"])
        return {k for k in keys if isinstance(k, str)}
