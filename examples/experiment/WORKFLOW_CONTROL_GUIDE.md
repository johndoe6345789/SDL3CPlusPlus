# Workflow Control Structures - JSON Primitives Guide

## Overview

The workflow system now supports C++-like control structures directly in JSON, enabling complex logic without writing code:

- **for loops** - Iterate over ranges
- **while loops** - Conditional iteration
- **if/else** - Branching logic
- **scopes** - Local variable isolation
- **switch statements** - Multi-way branching
- **Variable operations** - Increment, decrement, assignment
- **Parallel execution** - Mark nodes for concurrent execution

---

## 1. FOR LOOP

Iterate a specified number of times with a counter variable.

```json
{
  "type": "for",
  "variable": "i",           // Loop variable name
  "start": 0,               // Starting value
  "end": 10,                // End value (exclusive)
  "step": 1,                // Increment per iteration (optional, default 1)
  "nodes": [
    // Nodes to execute each iteration
    // Can reference ${i} to get loop variable
  ]
}
```

### Example: Render 11×11 grid with nested loops

```json
{
  "type": "for",
  "variable": "row",
  "start": 0,
  "end": 11,
  "nodes": [
    {
      "type": "for",
      "variable": "col",
      "start": 0,
      "end": 11,
      "nodes": [
        {
          "type": "render.cube",
          "parameters": {
            "x": "${variables.grid_start_x} + ${col} * 3.0",
            "y": "${variables.grid_start_y} + ${row} * 3.0",
            "rotation_x": "${row} * 0.21",
            "rotation_y": "${col} * 0.37"
          }
        }
      ]
    }
  ]
}
```

---

## 2. WHILE LOOP

Repeat until a condition becomes false.

```json
{
  "type": "while",
  "condition": {
    "op": "<",              // Comparison operator
    "left": "${frame_count}",
    "right": "${max_frames}"
  },
  "nodes": [
    // Nodes to repeat
  ]
}
```

### Supported Condition Operators

- **Comparison**: `==`, `!=`, `<`, `<=`, `>`, `>=`
- **Logical**: `&&`, `||`, `!`
- **Arithmetic**: `+`, `-`, `*`, `/`, `%`

### Example: Render frames until max reached

```json
{
  "type": "while",
  "condition": {
    "op": "<",
    "left": "${variables.frame_count}",
    "right": 120
  },
  "nodes": [
    {
      "type": "render.frame",
      "parameters": {
        "time": "${variables.frame_count} * 0.016"  // ~60 FPS
      }
    },
    {
      "type": "var",
      "op": "increment",
      "name": "frame_count"
    }
  ]
}
```

---

## 3. IF / ELSE

Conditional branching.

```json
{
  "type": "if",
  "condition": {
    "op": "==",
    "left": "${variables.quality_level}",
    "right": "high"
  },
  "then": [
    // Nodes if condition is true
    {
      "type": "load_shaders",
      "parameters": {
        "shader_set": "advanced"
      }
    }
  ],
  "else": [
    // Nodes if condition is false (optional)
    {
      "type": "load_shaders",
      "parameters": {
        "shader_set": "basic"
      }
    }
  ]
}
```

### Example: Conditional feature loading

```json
{
  "type": "if",
  "condition": {
    "op": "==",
    "left": "${variables.enable_ssgi}",
    "right": true
  },
  "then": [
    {
      "type": "enable_feature",
      "parameters": {
        "feature": "screen_space_global_illumination"
      }
    }
  ]
}
```

---

## 4. SCOPE

Create local variable isolation.

```json
{
  "type": "scope",
  "locals": {
    "temp_value": 42,
    "temp_array": [1, 2, 3, 4, 5],
    "temp_string": "hello"
  },
  "nodes": [
    // Nodes can use temp_value, temp_array, etc.
    // Variables are scoped to this block
  ]
}
```

### Example: Calculate intermediate values

```json
{
  "type": "scope",
  "locals": {
    "grid_size": 11,
    "spacing": 3.0,
    "total_width": 33,           // 11 * 3.0
    "center_offset": -15.0       // -(33 / 2)
  },
  "nodes": [
    {
      "type": "render.cube_grid",
      "parameters": {
        "grid_width": "${grid_size}",
        "grid_height": "${grid_size}",
        "grid_spacing": "${spacing}",
        "grid_start_x": "${center_offset}",
        "grid_start_y": "${center_offset}"
      }
    }
  ]
}
```

---

## 5. SWITCH STATEMENT

Multi-way branching based on value.

```json
{
  "type": "switch",
  "expr": "${variables.render_mode}",
  "cases": [
    {
      "value": "deferred",
      "nodes": [
        {
          "type": "set_render_technique",
          "parameters": {"technique": "deferred_rendering"}
        }
      ]
    },
    {
      "value": "forward",
      "nodes": [
        {
          "type": "set_render_technique",
          "parameters": {"technique": "forward_rendering"}
        }
      ]
    },
    {
      "value": "pbr",
      "nodes": [
        {
          "type": "set_render_technique",
          "parameters": {"technique": "pbr_rendering"}
        }
      ]
    }
  ]
}
```

---

## 6. VARIABLE OPERATIONS

Manipulate variables without code.

```json
{
  "type": "var",
  "op": "let",          // or: set, increment, decrement, push
  "name": "variable_name",
  "value": 42
}
```

### Operations

| Operation | Description | Syntax |
|-----------|-------------|--------|
| `let` | Declare new variable | `{"op": "let", "name": "x", "value": 10}` |
| `set` | Assign variable | `{"op": "set", "name": "x", "value": 20}` |
| `increment` | `x++` | `{"op": "increment", "name": "x"}` |
| `decrement` | `x--` | `{"op": "decrement", "name": "x"}` |
| `push` | Array append | `{"op": "push", "array": "items", "value": 42}` |
| `log` | Print to console | `{"op": "log", "message": "Progress..."}` |

### Examples

```json
[
  {
    "type": "var",
    "op": "let",
    "name": "frame_count",
    "value": 0
  },
  {
    "type": "var",
    "op": "increment",
    "name": "frame_count"
  },
  {
    "type": "var",
    "op": "push",
    "array": "rendered_frames",
    "value": "${variables.frame_count}"
  }
]
```

---

## 7. SEQUENCE

Explicitly mark nodes to execute sequentially (default behavior).

```json
{
  "type": "sequence",
  "nodes": [
    {"type": "init"},
    {"type": "load"},
    {"type": "execute"},
    {"type": "cleanup"}
  ]
}
```

---

## 8. PARALLEL

Mark nodes for potential parallel execution (implementation-specific).

```json
{
  "type": "parallel",
  "nodes": [
    {"type": "load_texture_1"},
    {"type": "load_texture_2"},
    {"type": "load_texture_3"},
    {"type": "load_shader"}
  ]
}
```

---

## Complete Advanced Example

Render an 11×11 grid of animated cubes using loops and control flow:

```json
{
  "type": "scope",
  "locals": {
    "grid_width": 11,
    "grid_height": 11,
    "cube_count": 0
  },
  "nodes": [
    {
      "type": "graphics.init",
      "parameters": {"window_width": 1280, "window_height": 720}
    },
    {
      "type": "for",
      "variable": "row",
      "start": 0,
      "end": 11,
      "nodes": [
        {
          "type": "for",
          "variable": "col",
          "start": 0,
          "end": 11,
          "nodes": [
            {
              "type": "render.cube",
              "parameters": {
                "x": "-15.0 + ${col} * 3.0",
                "y": "-15.0 + ${row} * 3.0",
                "rotation_x": "${row} * 0.21",
                "rotation_y": "${col} * 0.37"
              }
            },
            {
              "type": "var",
              "op": "increment",
              "name": "cube_count"
            }
          ]
        }
      ]
    },
    {
      "type": "if",
      "condition": {
        "op": "==",
        "left": "${cube_count}",
        "right": 121
      },
      "then": [
        {
          "type": "var",
          "op": "log",
          "message": "✓ Successfully rendered 121 cubes!"
        }
      ]
    }
  ]
}
```

---

## Key Benefits

1. **No C++ needed** for logic - everything in JSON
2. **Type-safe conditions** - conditions are evaluated safely
3. **Scoped variables** - avoid naming conflicts
4. **Composable** - nest loops, conditionals, scopes freely
5. **Readable** - JSON structure mirrors C-like syntax
6. **Debuggable** - each operation logs its execution

---

## Performance Considerations

- **Loops**: No hard limits, but practical limit ~10,000 iterations
- **Variables**: Scoped locally, freed when scope exits
- **Parallel nodes**: Currently sequential; future GPU acceleration
- **Conditions**: Evaluated once per iteration; no lazy evaluation

---

## See Also

- `workflow_cubes_advanced.json` - Full advanced example
- `workflow_cubes.json` - Simple workflow (no control structures)
- Main rendering engine: `standalone_workflow_cubes/main.cpp`
