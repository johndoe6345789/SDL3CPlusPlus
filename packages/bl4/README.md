# bl4: Borderlands 4's Kairos, walked with Quake 3 kinematics

A first, geometry-and-movement slice of Borderlands 4's open world
(Kairos), streamed in as fixed-size tiles the way `packages/fs2024`
streams its terrain and `packages/gta5` streams its ymaps. This package
never touches BL4's game files itself -- it only reads whatever
[bl4x](https://github.com/johndoe6345789/bl4x) (a standalone, from-scratch
C++ reader for BL4's IoStore archives, outside this repo) has already
baked to disk.

## What's here vs. what bl4x does

- **bl4x** (its own repo) parses BL4's `.utoc`/`.ucas` containers, Zen
  packages, unversioned properties, Nanite mesh clusters and Virtual
  Textures, and walks a World_P cell's actor tree into world-space mesh
  placements -- see its own README for the (considerable) format detail.
- **This package** only reads bl4x's *output*: OBJ meshes and JSON
  placement lists. It has no BL4-specific parsing at all.

## Baking a region

bl4x's `bake` command walks one or more World_P cell `.umap` packages,
exports every mesh they reference once (as `models/<name>.obj`), and
re-buckets every placement into a fixed-size XZ grid (independent of
BL4's own opaque per-cell naming, which carries no coordinate this tool
can read back out):

```
bl4x.exe bake <out_dir> <tile_size_metres> <cell1.umap> [cell2.umap ...]
```

Output layout (mirrors `packages/gta5`'s `assets/tiles/<x>_<z>.json`
placement format, which this package's tile streaming was built to read
directly):

```
<out_dir>/world.json                       { tile_size, spawn: {x,y,z,heading} }
<out_dir>/tiles/<tx>_<tz>/placements.json  { tile:[x,z], tile_size, placements:[
                                              {archetype, model, position[3],
                                               rotation[4] (xyzw), scale[3]}, ...] }
<out_dir>/models/<name>.obj                one mesh, position+normal+uv+faces
```

A single richly-populated World_P cell bakes to roughly a 2x2 grid of
64 m tiles (~128x128 m), which is what the default `map_root` in
`workflows/bl4_game.json` points at. Baking more cells (or a lower
`tile_size`) grows the streamed area; nothing else needs to change.

## Running it

The map root isn't checked into this repo (BL4's assets aren't either --
see the boundary note below). Point `BL4_MAP_DIR` at wherever you baked
to and launch with the `bl4` package:

```
python python/dev_commands.py all --run --game bl4 --env BL4_MAP_DIR=D:/bl4x/out
```

`workflows/bl4_game.json`'s `bl4.tiles.resolve` init node reads
`${env:BL4_MAP_DIR}` (see `ExpandEnvPlaceholders` in
`workflow_parameter_value_parser.cpp`) as its `map_root` parameter.

## Engine-space conversion

bl4x's placement walker (`world/walker.cpp`) already converts every
placement to glTF convention -- Y up, metres, xyzw quaternions -- the
same convention this engine's other packages use. **Nothing downstream
of bl4x re-maps axes or units.** See `package.json`'s
`world_specifications.axis_convention`.

## Scope of this first slice

- **No textures yet.** bl4x doesn't resolve `UMaterialInstance` ->
  texture-parameter references (a whole separate subsystem: material
  graphs, not just meshes/textures in isolation), so every mesh here
  draws with one placeholder texture (`assets/bl4_placeholder.png`) and
  flat sun/ambient shading (`shaders/spirv/bl4_model.frag`). Geometry,
  collision and streaming are otherwise the real thing -- see bl4x's own
  README for exactly how much of BL4's Nanite/Virtual Texture pipeline
  is decoded already.
- **No per-instance frustum culling or GPU instancing.** Every resident
  tile's every instance draws with its own draw call
  (`bl4_model_draw_step.cpp`). Fine at a few hundred instances (one
  test region); worth revisiting before streaming a much larger area.
- **No ground_guard.** Unlike fs2024's heightfield, BL4 mesh collision
  is arbitrary triangle geometry (`btBvhTriangleMeshShape`, same as
  `packages/gta5`), which has no cheap analytic height query to sanity-
  check against -- the Q3 traces are the only ground contact, exactly as
  in `packages/gta5`.
- **No landscape or spline meshes.** bl4x's walker records their
  transforms but doesn't yet bake `ULandscapeComponent` terrain or
  `USplineMeshComponent` bends into geometry, so `bl4.tiles.load` never
  sees a placement for them.

## Boundary

This engine never talks to Borderlands 4's servers, decrypts anything,
or ships BL4 assets. It reads a directory of OBJ/JSON files you produced
yourself, with your own copy of the game and your own bl4x build --
exactly the same posture `packages/gta5`'s README describes for its
GTAUtil extract.
