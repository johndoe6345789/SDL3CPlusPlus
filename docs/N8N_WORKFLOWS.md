N8N-Style Workflows

Overview
- Workflows can be declared with `nodes` and `connections`, modeled after n8n.
- Each node maps to a C++ workflow plugin (via `plugin` or `type`).
- Connections define execution order. When connections are omitted, the node array order is used.

Node Shape
- `id` or `name`: required. Used to identify the node in `connections`.
- `plugin` or `type`: required. The plugin string must match a registered step.
- `inputs` / `outputs`: optional string maps for workflow context keys.
- `position`: optional `[x, y]` array for layout parity with n8n.

Connections Shape
```json
{
  "connections": {
    "load_config": {
      "main": [
        [
          { "node": "validate_schema", "type": "main", "index": 0 }
        ]
      ]
    }
  }
}
```

Templates
- `config/workflows/templates/boot_default.json` and `config/workflows/templates/frame_default.json` are n8n-style now.
- `config/workflows/templates/n8n_skeleton.json` is a minimal starting point.

Generic Steps
- Implementations live in `src/services/impl/workflow/workflow_generic_steps/`.
- Full catalog lives in `ROADMAP.md` under "Generic Step Catalog (Implemented)".
