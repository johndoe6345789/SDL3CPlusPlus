# fs2024

Walk a real place with Quake 3's player kinematics, its ground and
buildings streamed in around you as you move. Defaults to a road at
Westminster, London (Bridge Street, by Big Ben); an airport such as
Innsbruck (LOWI) works too. There are no aircraft; every OSM
building's footprint is real (OpenStreetMap) and its walls and roof
are real FS2024 material textures (see `--wall-texture`/
`--roof-texture` below), though the shape is still a plain extruded
box, not a modelled facade -- a handful of named landmarks (see
`--landmark-catalog` below) are real, complete meshes instead.

## What FS2024 does and does not give you

FS2024 (Steam) installs its packages under
`%APPDATA%\Microsoft Flight Simulator 2024\Packages\Official2024\Steam`.
What is on disk, and what this package uses:

| File | What it is | Used |
|---|---|---|
| `scenery/**/<icao>.bgl` | classic BGL: the airport record `0x113`, runways `0xCE`, aprons `0xD0` | yes, for an airport: position, elevation, runway, aprons |
| `scenery/**/*.bgl` `ModelData` (`0x2B`) section | a GUID-keyed directory of `RIFF ... GLTF` blobs, each a named model's `GXML` (name, LOD list) plus one standard binary glTF per LOD | yes, for a landmark named in `--landmark-catalog` (`fs-base/scenery/Global/Asobo_POI/Asobo_POI.BGL` holds hundreds, worldwide) |
| `*.dds` (referenced by a model's own glTF via `MSFT_texture_dds`) | classic DDS, BC1/BC3 seen so far (BC5 normal maps are not decoded -- this engine's fs2024 shader has no normal mapping) | yes, for a landmark's own textures |
| `*.gltf.fsc`, `*.bin.fsc` | zlib-compressed glTF, with `EXT_meshopt_compression` and `ASOBO_*` extensions | not found in this install; may not exist in this game version |
| `*.ktx2` | plain KTX2, BC-compressed, no supercompression | not found in this install |
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
  buildings' walls and roof (if it baked any) as two more draw calls
  through the *same* pipeline and shader -- FS2024's own generic brick
  and roof-tile textures (`--wall-texture`/`--roof-texture`), shared
  citywide, and no runway overlay, but otherwise the identical sun
  plus sky plus fog shading the ground gets. Extruding on load rather
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
  polygon triangulation, building extrusion (a footprint's walls and
  roof come out counter-clockwise seen from outside, matching this
  engine's own terrain-winding convention, and tile by real-world
  distance at the wall/roof texture's own fixed metres-per-tile), the
  landmark catalog and its OSM name matching. `fs2024_gltf_model_test`
  covers the landmark glTF reader itself, against a small hand-built
  RIFF/GLB fixture rather than a real (and uncommittable) FS2024 file.

Only SPIR-V is built:

```bash
glslc -fshader-stage=frag shaders/spirv/fs2024_terrain.frag.glsl -o shaders/spirv/fs2024_terrain.frag.spv
glslc -fshader-stage=vert shaders/spirv/fs2024_terrain.vert.glsl -o shaders/spirv/fs2024_terrain.vert.spv
```

## Real landmarks: `--landmark-catalog`

A handful of real-world landmarks -- Big Ben, Tower Bridge, and
hundreds more -- turn out to already be ordinary meshes and textures
in FS2024's own data, not the `.fsc`/meshopt/ktx2 format earlier notes
here expected: `fs-base/scenery/Global/Asobo_POI/Asobo_POI.BGL` is a
classic BGL `ModelData` (`0x2B`) section -- the same section type as
`asobo-modellib-buildings`' generic prop kit -- holding hundreds of
named, RIFF-wrapped, standard binary glTF models with plain DXT1/DXT5
(BC1/BC3) textures. `fs2024_prepare` can substitute one of these for
an OSM building's flat massing:

```bash
fs2024_prepare ... --landmark-catalog D:/fs2024/landmark_catalog.json
```

```json
{
  "landmarks": [
    { "match": "Elizabeth Tower",
      "bgl": "<FS2024 install>/fs-base/scenery/Global/Asobo_POI/Asobo_POI.BGL",
      "texturesDir": "<FS2024 install>/fs-base/scenery/Global/Asobo_POI/TEXTURE",
      "model": "WestminsterPalace",
      "headingDegrees": 0.0 }
  ]
}
```

Two real gotchas in FS2024's own glTF, found only by looking at a
screenshot of the result -- neither is flagged by any
`extensionsUsed`/`extensionsRequired` entry, so nothing in the file
itself says to expect them: NORMAL/TANGENT/TEXCOORD accessors are
signed-normalised BYTE/SHORT (`max(v / 127.0, -1.0)` etc, exactly
[`KHR_mesh_quantization`](https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_mesh_quantization/README.md)'s
formula) without ever declaring `"normalized": true`, which the core
spec requires for that to be valid glTF; and a mesh's primitives share
one `"indices"` accessor rather than each having its own, with a
`StartIndex`/`PrimitiveCount` (index elements/triangles) in each
primitive's own `extras.ASOBO_primitive` naming its slice. Reading
that shared accessor whole, as ordinary glTF would, drew every
primitive with the entire mesh's geometry -- every material
overlapping the same full shape, one indistinguishable dark mass.
`fs2024_gltf_model_test`'s `SlicesEachPrimitivesOwnRangeOfASharedIndexAccessor`
is the regression test.

`match` is a case-insensitive substring of an OSM building's own
`name` tag; `model` is the name the model's own `GXML` chunk gives it
(not always the same as the OSM name -- "Elizabeth Tower" is the
whole `WestminsterPalace` model, since FS2024 modelled the tower and
the rest of the Palace of Westminster as one landmark). Placement is
the matched building's OSM centroid; `headingDegrees` is a plain
manual value; a footprint this irregular has no reliable automatic
orientation, so, like the runway markings, this is tuned by eye
in-game rather than computed. The extracted model is real-world
scale already -- no resizing to fit the OSM footprint.

A matched landmark is extracted once into `<out>/landmarks/<model>.lmk`
(that LOD's mesh, grouped by material -- FS2024's own node hierarchy,
translation/rotation/scale and all, composed in at extraction time)
plus `<out>/landmarks/textures/*.png` (its DDS textures, decoded once),
and referenced by each tile it falls in via `landmarks.json`. Both are
shared, global data: baking the same place again, or a second tile
that happened to reference the same landmark, does not repeat the
extraction. `fs2024.tiles.load` uploads a referenced model's kit once,
the first tile that needs it; `fs2024.terrain.draw` draws it through
the same pipeline as everything else, one draw call per material.

## Real walls and roofs everywhere: `--wall-texture`/`--roof-texture`

Every ordinary OSM building -- not just a named landmark -- is now
textured with FS2024's own generic wall and roof materials, not the
old flat placeholder colour:

```bash
fs2024_prepare ... \
    --wall-texture "<FS2024 install>/asobo-modellib-texture/Asobo_Buildings/Texture/TILE_BRICKS_BROWN_01_ALBD.PNG.DDS" \
    --roof-texture "<FS2024 install>/asobo-modellib-texture/Asobo_Buildings/Texture/TILE_ROOFTILE_BROWNDIRTY_ALBEDO.PNG.DDS"
```

Unlike a landmark, these are plain DDS files FS2024 ships directly --
no BGL or glTF involved, just `DecodeDds` and a PNG write, once into
shared `<out>/building_kit/wall.png`/`roof.png` (skipped if already
there). `AppendBuildingMesh` (`fs2024_building_mesh.cpp`) now emits
walls and roofs as two separate vertex/index lists with real UVs --
a wall tiles its texture every 4 m of its own length/height, a roof
every 8 m of its own (x, z), a flat planar projection from above --
so `fs2024.tiles.load` uploads two GPU chunks per tile instead of
one, and `fs2024.terrain.draw` draws them with their own real
textures rather than one shared flat colour. No `buildings.fsb`
format change: the split happens at mesh-build time, not bake time.

The result is real, directional shading, not a flat colour: a box's
sunlit face reads as bright brick, its shadowed face much darker,
exactly like a real photograph of a street would -- confirmed by
comparing two faces of the same building side by side, not assumed.
A distant view where every visible wall happens to face away from the
sun will look darker than the old flat placeholder ever did, simply
because the placeholder had no real material darkness to reveal.

## Next

1. A coarser outer ring of tiles at a lower level of detail, the way
   gta5's LOD bands work, so a baked area does not just stop.
2. Real imagery or land cover from an open source, in place of the
   height and slope guess.
3. More than one road class drawn differently (a residential street
   need not look like a primary road), and drawing a worldwide tile's
   roads with any sharpness, the way an airport's runway already is.
4. More than one instance of the same landmark model: right now a
   model's extracted kit bakes in whichever instance's placement
   loads it first, since placement is applied to the mesh once at
   load time rather than through a per-instance model matrix (this
   engine's fs2024 shader takes none -- ground and OSM buildings are
   baked in world space too). Fine while every catalog only ever
   places each landmark once.
5. More than one wall/roof material citywide -- every OSM building
   currently shares the same one of each, picked once for the whole
   bake, not varied per building the way a real street never repeats
   the same brick on every house.
