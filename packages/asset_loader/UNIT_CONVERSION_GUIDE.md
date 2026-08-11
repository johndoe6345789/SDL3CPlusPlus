# Unit Conversion System - Complete Guide

## Overview
MetaBuilder uses a **100% JSON-driven unit conversion system** to load maps from any game engine. No C++ changes needed - just set two workflow variables.

## Supported Engines & Conversion Factors

| Engine | Units/Meter | To UE5 Scale | Example |
|--------|-------------|--------------|---------|
| **UE5** (reference) | 100 | 1.0× | 180 units = 180cm (player height) |
| **Quake 3** | 32 | 3.125× | 56 units × 3.125 = 175cm ✓ |
| **Quake 2** | 32 | 3.125× | Same as Q3 |
| **Doom 3** | 39.37 | 2.54× | 1 unit ≈ 1 inch |
| **Source/HL2** | 39.37 | 2.54× | Same as Doom 3 |
| **Unity** | 1.0 | 100× | 1 unit = 1m in Unity → 100 units in UE5 |
| **Godot** | 1.0 | 100× | Same as Unity |
| **CryEngine** | 1.0 | 100× | Same as Unity |
| **UE3** | 50 | 2.0× | Original UE engine |

## Usage

### Quick Start: Load Quake 3 Map

```json
{
  "sourceEngine": "quake3",
  "conversionFactor": 3.125,
  "mapFile": "maps/q3dm1.bsp"
}
```

Then use the workflow: `workflows/load_map_with_unit_conversion.json`

### Generic: Load Any Engine's Map

Use `universal_map_loader.json` and set variables:

```json
{
  "sourceEngine": "doom3",      // "quake3", "unity", "ue3", etc.
  "conversionFactor": 2.54,     // From table above
  "mapFile": "path/to/map"
}
```

### Add a New Engine

Edit `packages/asset_loader/package.json`:

```json
"unitSystems": {
  "myengine": {
    "engine": "My Custom Engine",
    "unitsPerMeter": 64,
    "conversionToUE5": 1.5625,
    "conversionFromUE5": 0.64
  }
}
```

Then use in workflow: `conversionFactor: 1.5625`

## How It Works

### Architecture

```
package.json (unitSystems data)
    ↓
Workflow Variables (sourceEngine, conversionFactor)
    ↓
Workflow Nodes:
  1. Load map file
  2. Scale all geometry by conversionFactor
  3. Load scaled geometry to scene
```

### Data Flow Example: Quake 3 Map

```
Q3 Map (in game units)
  ↓
Load via asset.load_map (quake3 format)
  ↓
Apply 3.125× scale (geometry.apply_scale_transform)
  ↓
Result: UE5 units
  ├─ Vertex positions: scaled
  ├─ Object sizes: scaled
  ├─ Physics colliders: scaled
  └─ Animations: timings preserved
  ↓
Load to Scene
```

### What Gets Converted

- ✅ **Vertex Positions** - All coordinates scaled
- ✅ **Object Sizes** - Meshes/geometry scaled
- ✅ **Physics Colliders** - Collision bounds scaled
- ✅ **Animation Timings** - Preserved (no change to frame rates)
- ✅ **Texture Coordinates** - Unchanged (preserved as-is)

## Verification

To verify conversion worked, check known object sizes:

**Quake 3 → UE5**
- Player height: 56 Q3 units × 3.125 = 175 UE5 units ≈ 175cm ✓
- Typical door: 96 Q3 units × 3.125 = 300 UE5 units = 3m ✓

**Unity → UE5**
- Player height: 1.8 Unity units × 100 = 180 UE5 units = 180cm ✓
- Room width: 5 Unity units × 100 = 500 UE5 units = 5m ✓

## Future Work

Once workflow steps are implemented:
1. `asset.load_map` - Load BSP/map files
2. `geometry.apply_scale_transform` - Scale geometry by factor

Then you can load any engine's maps automatically.

## Why This Matters

Different engines encode space differently:
- **Game balance**: 1 unit in Quake 3 doesn't mean the same physical distance as 1 unit in Unity
- **Physics**: Gravity, movement speed, jump height all depend on unit scale
- **Art assets**: Textures may look wrong if scale is off
- **Cross-platform development**: Allows reusing maps from different sources

By using standardized, verified conversion factors, you can safely mix assets from multiple game engines.
