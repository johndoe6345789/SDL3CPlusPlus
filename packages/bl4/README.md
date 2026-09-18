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
- **This package** only reads bl4x's *output*: OBJ meshes (+ `.mtl`),
  TGA base-colour maps and JSON placement lists. It has no BL4-specific
  parsing at all.

## Finding a region worth baking

World_P cells have opaque hashed filenames -- nothing in the name says
"this one has buildings" or "this one is empty desert". `bl4x`'s
`find-cells` command walks every cell looking for placements whose mesh
path contains any of a list of substrings, and prints each match's
instance count and centroid, so you can pick a region by *content*
rather than guessing:

```
bl4x.exe find-cells <max_hits> <pattern1> [pattern2 ...]
# e.g. a settlement/outpost area (roads + modular architecture kit):
bl4x.exe find-cells 60 SM_Building SM_Road SM_Global_Catwalk SM_Concrete_Block SM_Trim_Concrete
```

Two adjacent cells found this way (near World-space (245, 125, 435))
turned out to share a modular outpost: paved roads, walls with doors and
windows, catwalks, and a roof -- baked as the package's current default
test region. Worth knowing before picking a spawn point: some pieces of
this kit are long, thin meshes (a `SM_Road_Paved_Shoulder_2000_A`
embankment segment, 20 m long) whose *pivot* can be many metres from
where the mesh actually reaches -- checking "is anything within N metres
of my spawn point's *pivot*" isn't enough; the first two spawn points
this session that passed exactly that check still ended up with the
camera embedded in that shoulder's slope. Once actually clear (checked
against the full instance list, not just nearby pivots) and re-oriented
to look down the road rather than face-on into a wall, the view reads
correctly as a road between buildings.

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
<out_dir>/models/<name>.obj                one mesh, position+normal+uv+faces,
                                           one `usemtl` group per material slot
<out_dir>/models/<name>.mtl                map_Kd ../textures/<tex>.tga per slot
<out_dir>/textures/<tex>.tga               base-colour map, RGBA8, <= 1024 px
```

The `.mtl` is only written for slots whose material instance chain
(`TextureParameterValues`, then `Parent`, up to 8 hops) binds a texture
that looks like a colour map -- a `BaseColor`/`Albedo`/`Diffuse`
parameter, else a `_D`/`_BC` texture name. On the default test region
that's 102 of 114 materials; the rest are emissive/light-fixture
materials or foliage that only binds a packed "Composite" map, and draw
with `assets/bl4_placeholder.png`.

A single richly-populated World_P cell bakes to roughly a 2x2 grid of
64 m tiles (~128x128 m). Baking more cells (or a lower `tile_size`) grows
the streamed area; nothing else needs to change -- tiles are keyed by
world position, so a spawn point stays put whichever set of cells was
baked.

`bake-all` does the whole map (all 16,863 World_P cells, ~2.25M
placements) into the same layout. Both commands are resumable: a mesh or
texture whose file already exists is not decoded again, so an
interrupted run continues where it stopped.

## Running it

The map root isn't checked into this repo (BL4's assets aren't either --
see the boundary note below). Point `BL4_MAP_DIR` at wherever you baked
to -- it defaults to `D:/BL4Export/bake_town` (`launch_options` in
`package.json`), and the launcher has a folder picker for it:

```
python python/dev_commands.py all --run --game bl4 --env BL4_MAP_DIR=D:/bl4x/out
```

`workflows/bl4_game.json`'s `bl4.tiles.resolve` init node reads
`${env:BL4_MAP_DIR}` (see `ExpandEnvPlaceholders` in
`workflow_parameter_value_parser.cpp`) as its `map_root` parameter.

## Falling through the world

The baked ground has holes, and not all of them are bugs. The city in
the middle of the map is built around a designed bottomless shaft,
which the game guards with kill volumes rather than a floor.
`bl4.player.respawn` does what Borderlands does: it remembers the last
walkable ground you stood on and puts you back there after a fall of
more than `fall_distance` (60 m), or below `kill_y` (-1000 m). Only a
real fall (downward velocity) triggers it, so free flight, the spawn
hold and the orbit camera never do.

What bl4x now fills rather than leaves:

- **Water.** The terrain is cut away wherever water sits. Custom water
  bodies bake their `WaterMeshOverride` swim plane; lakes, whose surface
  the game builds at runtime, are triangulated from their spline
  outline. Both are drawn with one flat water colour.
- **Spline meshes.** Roads, pipes and river pieces are short meshes UE
  bends along a spline; bl4x bends each one itself (the Hermite and
  slice frame `USplineMeshComponent::CalcSliceTransform` uses). Placed
  unbent they came out tens of kilometres across.

`D:\BL4Export\coverage.py` maps what is left: every upward-facing
triangle rasterized onto a grid, flood-filled from the border, so any
enclosed empty cell is a place you would fall through.

## Engine-space conversion

bl4x's placement walker (`world/walker.cpp`) already converts every
placement to glTF convention -- Y up, metres, xyzw quaternions -- the
same convention this engine's other packages use. **Nothing downstream
of bl4x re-maps axes or units.** See `package.json`'s
`world_specifications.axis_convention`.

## Scope of this first slice

- **Base colour only.** BL4's materials are layered graphs
  (`MaterialLayers`/`Blends`: colourisation tint, grime, wear, detail
  and normal maps); only the base-colour map survives, lit by one sun
  and a sky term (`shaders/spirv/bl4_model.frag`). So surfaces BL4 tints
  at runtime (e.g. the paved road) come out paler than in game. Each
  texture is loaded once and ref-counted across submeshes
  (`bl4_texture_cache.cpp`), so evicting a region frees its textures too.
- **Culled and instanced, but not batched further.** `bl4.models.draw`
  culls every resident instance against the view frustum and against
  `size_ratio` (drop anything whose bounding sphere is under that
  fraction of its distance -- BL4's kit is full of bolts and cables that
  cover no pixel at 200 m), then groups what survives by archetype into
  one storage buffer of model matrices and issues one instanced draw per
  archetype submesh (`bl4_instance_batch_*.cpp`, the same shape as
  `packages/gta5`'s cull/batch steps). Drawing one call per placement
  instead ran the full map at 4 FPS with the GPU at 14% -- entirely CPU
  submission cost. Still missing: merging submeshes that share a texture
  across archetypes, and any LOD scheme (BL4's own HLOD proxies are
  skipped, see below).
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
