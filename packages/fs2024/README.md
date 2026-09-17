# fs2024

Walk a real place with Quake 3's player kinematics, its ground and
buildings streamed in around you as you move. Defaults to a road at
Westminster, London (Bridge Street, by Big Ben); an airport such as
Innsbruck (LOWI) works too. There are no aircraft, and buildings are
flat-shaded massing, not textured models.

## What FS2024 does and does not give you

FS2024 (Steam) installs its packages under
`%APPDATA%\Microsoft Flight Simulator 2024\Packages\Official2024\Steam`.
What is on disk, and what this package uses:

| File | What it is | Used |
|---|---|---|
| `scenery/**/<icao>.bgl` | classic BGL: the airport record `0x113`, runways `0xCE`, aprons `0xD0` | yes, for an airport: position, elevation, runway, aprons |
| `*.gltf.fsc`, `*.bin.fsc` | zlib-compressed glTF, with `EXT_meshopt_compression` and `ASOBO_*` extensions | not yet (real building models) |
| `*.ktx2` | plain KTX2, BC-compressed, no supercompression | not yet (real building models) |
| `*.ktx2p` | "KTX2PACKED" | no; not decoded |
| `*.fsarchive` | about half say `"scheme":"notEncrypted"`; the rest are encrypted | no |
| `cgl/**/sai*.cgl` | airport-local data, format unknown | no |

**The world's elevation, land cover and buildings are not on disk.**
FS2024 streams them, and this package does not read that stream or its
cache. Instead, the ground comes from a public DEM (Copernicus
GLO-30), and the land cover is guessed from height and slope. Roads
and building footprints (with a height, where OpenStreetMap has one)
come from OpenStreetMap instead: whichever real, drivable road is
nearest the requested point becomes the spawn, and every building
within the baked area gets a simple extruded box.

## Streaming, not one fixed bake

Earlier this baked one fixed square around a single airport. It now
bakes a grid of tiles, keyed by `floor(x / tile_size), floor(z /
tile_size)` the same way a player could in principle walk anywhere in
the world: `fs2024.tiles.resolve/load/evict` stream tiles in around
the player and drop the ones left behind, mirroring how the gta5
package streams its map tiles. Nothing here is actually
worldwide-scale by itself yet -- only tiles baked to disk exist, and a
player who reaches an unbaked one simply has no ground there (remembered
as "missing" so it is not retried every frame) -- but the runtime
machinery does not know or care how big the baked area is, which is
the part that matters for scaling this up later.

## Baking a place: `fs2024_prepare`

The bake is a C++ tool (`fs2024_prepare`, built alongside `sdl3_app`),
not a script -- it reads BGL, GeoTIFF and OpenStreetMap's own JSON
formats directly, the same way gta5 reads RSC7 directly rather than
through a converter. The one thing it does not do itself is talk to
the network: this engine makes no HTTP calls anywhere, so DEM and
OpenStreetMap data are fetched once with `curl`, the same boundary
gta5 draws around RPF7 extraction (you get the raw files yourself; see
`packages/gta5/README.md`).

```bash
# The DEM tile is whichever 1x1 degree Copernicus GLO-30 cell covers
# the point, named by its south-west corner (N51_00_W001_00 for London).
curl -o D:/fs2024/dem/N51_00_W001_00.tif ^
  https://copernicus-dem-30m.s3.amazonaws.com/Copernicus_DSM_COG_10_N51_00_W001_00_DEM/Copernicus_DSM_COG_10_N51_00_W001_00_DEM.tif

# Roads and buildings within about 3.5 km of the point, saved once.
# The User-Agent matters -- Overpass returns 406 without one.
curl -A "sdl3cplusplus-fs2024/1.0" --data-urlencode ^
  "data=[out:json][timeout:30];(way(around:3500,51.5007,-0.1246)[highway~\"^(motorway|trunk|primary|secondary|tertiary|unclassified|residential|living_street|service)$\"];way(around:3500,51.5007,-0.1246)[building];);out geom;" ^
  https://overpass-api.de/api/interpreter -o D:/fs2024/probe/westminster_osm.json

# A road, anywhere: give a point near it and it snaps to the nearest
# real, drivable way.
fs2024_prepare --lat 51.5007 --lon -0.1246 \
    --osm-json D:/fs2024/probe/westminster_osm.json \
    --dem D:/fs2024/dem/N51_00_W001_00.tif --out D:/fs2024/westminster

# An airport: needs its own scenery BGL, copied out of the FS2024
# install (packages/<id>/scenery/**/<icao>.bgl).
fs2024_prepare --icao LOWI --bgl D:/fs2024/probe/lowi.bgl \
    --dem D:/fs2024/dem/N47_00_E011_00.tif --out D:/fs2024/lowi
```

Each bake writes `<out>/world.json` (the frame, the true tile size,
and the spawn point) and a grid of tiles under `<out>/tiles/<tx>_<tz>/`:

- **`terrain.fst`**: that tile's block of the heightfield, at 16 m
  spacing. Magic `FST1`, `u32` columns and rows, `f32` spacing, origin
  x, origin z, then the heights.
- **`ground.png`**: that tile's slice of the colour map, about 2 m a
  texel. Road or apron polygons are drawn onto it as flat asphalt;
  fine markings are not baked (see below). PNG, not the old JPEG --
  lossless, so flat pavement colour survives it perfectly.
- **`roads.json`**: an airport tile's share of the runway, when one
  crosses it; `{}` otherwise. A worldwide tile's roads are baked
  straight into its ground texture, so nothing here describes them.
- **`buildings.fsb`**: that tile's buildings, as raw footprint-plus-
  height records -- magic `FSB1`, `u32` count, then per building a
  point count, a height, and that many (x, z) pairs. The actual wall
  and roof mesh is built at load time (`AppendBuildingMesh`), the same
  way `terrain.fst`'s heights become a mesh at load time rather than
  being baked as one.

Engine space is a tangent plane on the spawn point itself: x is east,
y is height above the spawn's elevation, z is south. A tile's own key
is `floor(engine_x / tile_size), floor(engine_z / tile_size)`.

**The requested `--tile-size` (default 1000 m) is a target, not the
real pitch.** The true tile size is whatever whole number of 16 m
cells comes closest, and every tile's origin is placed at an exact
multiple of that. Baking round `--extent` metres into round
`--tile-size` pieces independently was a real bug this replaced: a
tile written at, say, x = 1984 m (62 cells) named itself by
`floor(1984 / 1000)`, one off from where the runtime's `floor(x /
1000)` would look for it once it had walked a few tiles out, so a ring
of tiles at each bake's edge silently had no ground.
`ComputeGridLayout` (`fs2024_grid_layout.hpp`) is the fix, and
`fs2024_prepare_test`'s `Fs2024GridLayout` cases are the regression
test.

A different place just needs its own DEM tile and OSM fetch, baked
into a new `--out`; then update `D:/fs2024/probe/gen_workflows.py`'s
`DATA` path (or wherever `tiles_root` is set in
`workflows/fs2024_game.json`) and regenerate. That generator is a
plain script, not part of the engine's own C++ -- it only writes the
package's own workflow JSON, the same role `generate_cmake.py` plays
for `CMakeLists.txt`, not a reader of any game data format.

## Running

```bash
python python/dev_commands.py run --game fs2024
```

You start on Bridge Street, facing along it towards Big Ben. The
controls are the same as in the quake3 package: WASD to move, mouse to
look, Space to jump, Left Shift to sprint, Esc for the menu. From the
gta5 package you also get an Xbox pad, and Y toggles free flight: hold
W to speed up, Space to rise and Left Ctrl to sink.

## How it is put together

- `fs2024.tiles.resolve` reads the player's position from `q3.ps` and
  works out which tiles are wanted: every tile within
  `load_radius_tiles` (default 2) goes on to load, and every resident
  tile beyond `evict_radius_tiles` (default 3, deliberately wider than
  the load radius so a player standing on a tile boundary does not
  load and evict the same tile every frame) goes to evict. Runs after
  movement in the frame, so it centres on the origin this frame
  produced.
- `fs2024.tiles.load` loads a handful of the nearest pending tiles
  (`max_loads_per_call`, or all of them at once with `force: 1`, used
  once at init so the spawn point's ground exists before the first
  frame): each tile's own heightfield, its own `btHeightfieldTerrainShape`,
  its own ground texture, its buildings meshed from `buildings.fsb`,
  and, for an airport tile a runway crosses, that runway. A tile whose
  files are not on disk is remembered as missing rather than retried
  every call.
- `fs2024.tiles.evict` releases a tile's GPU texture, mesh buffers and
  collision body, and drops it. `fs2024.tiles.free` does the same for
  every resident tile, once, before `system.exit`.
- `fs2024.terrain.draw` draws every resident tile's ground blocks that
  reach into the view, each with its own texture bound, then its
  building mesh (if it baked any) through the *same* pipeline and
  shader -- buildings are drawn with the shared flat
  `packages/fs2024/assets/building_flat.png` in place of a real ground
  texture, and no runway overlay, but otherwise get the identical sun
  plus sky plus fog shading the ground does. Extruding on load rather
  than baking a mesh meant no second pipeline or vertex format was
  needed.
- An airport's runway markings are drawn in the fragment shader
  (`include/fs2024_runway.glsl`), measured in runway metres, with
  edges smoothed over one pixel, so they stay sharp underfoot and
  still read from a kilometre away. They are ICAO markings for a 45 m
  runway: side stripes, a dashed centreline, threshold stripes and
  aiming points. Baked into the 2 m ground map, they were blurred
  blobs up close and a checkerboard at a distance. A road, by
  contrast, is baked flat: it has no fine markings to blur.
- Borrowed from gta5: `gta5.sky.draw` and its shaders, the composite,
  `gta5.gamepad`, `gta5.player.fly`, and the `q3.pm.*` walking tune.
  None of these need gta5's streaming data. The input group is
  quake3's.
- `fs2024.player.spawn` stands the player on the ground of whichever
  tile is under the spawn point, once it has been force-loaded at
  init. `fs2024.player.ground_guard` runs after the slide move and
  free flight; it looks up the tile under the player and, when there
  is one, keeps them from sinking through it. It does nothing while
  that tile has not streamed in yet -- normal for a moment, not a fall
  to correct.
- `fs2024_tile_streaming_test` covers the resolve pass (the load/evict
  radius hysteresis, never re-queuing a resident or already-pending
  tile, never retrying a tile marked missing) and the tile lookup that
  finds the right tile's field for a given position.
  `fs2024_terrain_walk_test` runs the real pmove chain over a Bullet
  heightfield, on the flat and up slopes. `fs2024_prepare_test` covers
  the bake tool's own pure logic: argument parsing, grid alignment,
  polygon triangulation, and building extrusion (a footprint's walls
  and roof come out counter-clockwise seen from outside, matching this
  engine's own terrain-winding convention).

Only SPIR-V is built:

```bash
glslc -fshader-stage=frag shaders/spirv/fs2024_terrain.frag.glsl -o shaders/spirv/fs2024_terrain.frag.spv
glslc -fshader-stage=vert shaders/spirv/fs2024_terrain.vert.glsl -o shaders/spirv/fs2024_terrain.vert.spv
```

## Next

1. Real building models. Inflate the `.fsc` files, decode the meshopt
   buffers (meshoptimizer), and load the `.ktx2` textures, in place of
   flat-shaded massing. Placements are in the BGL model library
   (`<icao>_modellib.bgl` for an airport; a worldwide equivalent is
   not yet identified).
2. A coarser outer ring of tiles at a lower level of detail, the way
   gta5's LOD bands work, so a baked area does not just stop.
3. Real imagery or land cover from an open source, in place of the
   height and slope guess.
4. More than one road class drawn differently (a residential street
   need not look like a primary road), and drawing a worldwide tile's
   roads with any sharpness, the way an airport's runway already is.
