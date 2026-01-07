# Scene Framework

A reusable Lua framework for 3D scene construction in SDL3CPlusPlus.

## Overview

`scene_framework.lua` provides commonly-used utilities for building 3D scenes:
- **Mesh Generation**: Planes, cubes, and primitives
- **Object Builders**: Standardized scene object creation with metadata
- **Config Utilities**: Type-safe configuration resolution
- **Material Registry**: Centralized material/shader management

## Usage

```lua
local scene_framework = require("scene_framework")

-- Generate a tessellated floor
local vertices, indices = scene_framework.generate_plane_mesh(20, 20, 20, {1.0, 1.0, 1.0})

-- Create a static cube object
local cube = scene_framework.create_static_cube(
    {0, 5, 0},          -- position
    {2, 2, 2},          -- scale
    {1.0, 0.5, 0.2},    -- color
    "solid",            -- shader_key
    "my_cube"           -- object_type
)

-- Config resolution with fallbacks
local speed = scene_framework.resolve_number(config.move_speed, 5.0)
local position = scene_framework.resolve_vec3(config.spawn_point, {0, 0, 0})
```

## API Reference

### Mesh Generation

#### `generate_plane_mesh(width, depth, subdivisions, color)`
Generates a tessellated plane with normals pointing up (+Y).
- Returns: `vertices, indices` (0-based indices)
- Example: 20x20 subdivisions = 441 vertices, 800 triangles

#### `generate_cube_mesh(double_sided)`
Generates a unit cube (-0.5 to 0.5) with proper normals per face.
- Single-sided: 24 vertices, 36 indices
- Double-sided: 24 vertices, 72 indices

#### `apply_color_to_vertices(vertices, color)`
Copies vertex array and applies new color to all vertices.

#### `flip_normals(vertices)`
Inverts normals in-place (useful for ceilings, inside-out geometry).

### Object Builders

#### `create_static_object(vertices, indices, position, scale, shader_key, object_type)`
Creates a static scene object with no animation.

#### `create_static_cube(position, scale, color, shader_key, object_type, cube_mesh)`
Convenience builder for colored cube objects.

#### `create_dynamic_object(vertices, indices, compute_fn, shader_key, object_type)`
Creates animated object with `compute_fn(time) -> matrix` callback.

### Config Utilities

- `resolve_number(value, fallback)` - Safe number extraction
- `resolve_boolean(value, fallback)` - Safe boolean extraction
- `resolve_string(value, fallback)` - Safe string extraction
- `resolve_table(value, fallback)` - Safe table extraction
- `resolve_vec3(value, fallback)` - Safe {x,y,z} extraction

### Transform Utilities

#### `build_static_model_matrix(position, scale)`
Builds transformation matrix from position {x,y,z} and scale {sx,sy,sz}.

### Material Registry

```lua
local registry = scene_framework.MaterialRegistry.new(config)

if registry:has("floor") then
    local material = registry:get("floor")
    local shader_key = registry:get_key("floor")  -- Returns key or default
end
```

## Testing

Run the test suite:
```bash
cd scripts && lua test_scene_framework.lua
```

65 tests covering:
- Config resolution edge cases
- Mesh generation correctness
- Object builder metadata
- Material registry lookups

## Integration

See `cube_logic.lua` for real-world usage:
```lua
local scene_framework = require("scene_framework")

-- Delegate to framework
local function generate_plane_mesh(width, depth, subdivisions, color)
    local vertices, indices_zero = scene_framework.generate_plane_mesh(width, depth, subdivisions, color)
    -- Convert 0-based to 1-based indices for Lua
    local indices = {}
    for i = 1, #indices_zero do
        indices[i] = indices_zero[i] + 1
    end
    return vertices, indices
end
```

## Benefits

- **DRY**: Fix mesh generation bugs once, benefit everywhere
- **Consistency**: All scenes use same object structure with `object_type`
- **Testability**: Framework tested independently in Lua
- **Discoverability**: New scenes see available primitives via documentation
- **Type Safety**: Config resolution prevents nil dereferences

## Future Enhancements

Potential additions:
- `generate_sphere_mesh()` for skyboxes, particles
- `generate_cylinder_mesh()` for columns, barrels
- Physics object helpers with mass/friction
- Texture coordinate generation utilities
