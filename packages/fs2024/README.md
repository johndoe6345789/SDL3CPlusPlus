# fs2024

Walk the ground around a Microsoft Flight Simulator 2024 airport —
Innsbruck (LOWI) — with Quake 3's player kinematics. This is the first
step of bringing FS2024 into the engine: ground you can stand on and
walk across, lit, with the real airport laid out on it. There are no
buildings or aircraft yet.

## What FS2024 does and does not give you

FS2024 (Steam) installs its packages under
`%APPDATA%\Microsoft Flight Simulator 2024\Packages\Official2024\Steam`.
What is on disk, and what this package uses:

| File | What it is | Used |
|---|---|---|
| `scenery/**/<icao>.bgl` | classic BGL: the airport record `0x113`, runways `0xCE`, aprons `0xD0` | yes: position, elevation, runway, aprons |
| `*.gltf.fsc`, `*.bin.fsc` | zlib-compressed glTF, with `EXT_meshopt_compression` and `ASOBO_*` extensions | not yet |
| `*.ktx2` | plain KTX2, BC-compressed, no supercompression | not yet |
| `*.ktx2p` | "KTX2PACKED" | no; not decoded |
| `*.fsarchive` | about half say `"scheme":"notEncrypted"`; the rest are encrypted | no |
| `cgl/**/sai*.cgl` | airport-local data, format unknown | no |

**The world's elevation and land cover are not on disk.** FS2024
streams them. This package does not read the stream or its cache.
Instead, the ground comes from a public DEM (Copernicus GLO-30), and
the land cover is guessed from height and slope.

## Baking the ground

The bake needs Python with `numpy`, `Pillow`, `tifffile` and
`imagecodecs`. Everything it writes is large, so it goes to `D:`, not
the repo.

```bash
pip install --user tifffile imagecodecs
curl -LO --output-dir D:/fs2024/dem https://copernicus-dem-30m.s3.amazonaws.com/Copernicus_DSM_COG_10_N47_00_E011_00_DEM/Copernicus_DSM_COG_10_N47_00_E011_00_DEM.tif
python -m python.fs2024.bake_terrain --icao LOWI \
    --dem D:/fs2024/dem/Copernicus_DSM_COG_10_N47_00_E011_00_DEM.tif \
    --out D:/fs2024/lowi
```

It finds `lowi.bgl` in the FS2024 install and writes three files:

- **`terrain.fst`**: a 1001 × 1001 heightfield at 16 m, covering 16 km.
  The format is the magic `FST1`, then `u32` columns and rows, then
  `f32` spacing, origin x and origin z, then the heights as `f32`, one
  row after another. Heights are levelled to the runway under and
  around the pavement, because a DSM has hangars and trees in it.
- **`ground.jpg`**: an 8192² colour map, about 2 m per texel, with the
  apron triangles and the longest runway's asphalt drawn over it. The
  markings are too fine for a 2 m texel, so they are not baked (see
  below).
- **`airport.json`**: the local frame, the height range, the runway and
  the spawn point.

Engine space is a tangent plane on the airport reference point: x is
east, y is height above the airport elevation (576.8 m), and z is
south. Everything is in metres.

LOWI has three `0xCE` runway records: its real 2 km runway and two
shorter records offset to the north. What the extra two are for is not
known yet, so only the longest runway is painted.

A different airport needs its own DEM tile and a new bake. It also
needs the `terrain_load` path and `spawn` values in
`workflows/fs2024_game.json`, and the `terrain_draw` runway values in
`workflows/fs2024_frame.json`, changed to match.

## Running

```bash
python python/dev_commands.py run --game fs2024
```

You start on the threshold of runway 26, facing down the runway towards
the city. The controls are the same as in the quake3 package: WASD to
move, mouse to look, Space to jump, Left Shift to sprint, Esc for the
menu. From the gta5 package you also get an Xbox pad, and Y toggles free
flight: hold W to speed up, Space to rise and Left Ctrl to sink.

## How it is put together

- `fs2024.terrain.load` reads `terrain.fst`, uploads it as 64 × 64-cell
  blocks, and adds a static `btHeightfieldTerrainShape`. A heightfield
  costs nothing to build, and its cells are split on the same diagonal
  as the drawn mesh and `Fs2024HeightAt`. `fs2024_heightfield_test`
  checks that all three agree.
- `fs2024.terrain.draw` skips blocks outside the view and draws the
  rest with `fs2024_terrain.{vert,frag}`. The shading is sun plus sky,
  world-space detail noise up close, and fog towards the sky's horizon
  colour.
- The runway markings are drawn in the fragment shader
  (`include/fs2024_runway.glsl`), measured in runway metres, with edges
  smoothed over one pixel, so they stay sharp underfoot and still read
  from a kilometre away. They are ICAO markings for a 45 m runway: side
  stripes, a dashed centreline, threshold stripes and aiming points.
  Baked into the 2 m ground map, they were blurred blobs up close and a
  checkerboard at a distance. The draw step takes `runway_x`,
  `runway_z`, `runway_heading`, `runway_length` and `runway_width` from
  `airport.json`. There are no designation numbers yet.
- Borrowed from gta5: `gta5.sky.draw` and its shaders, the composite,
  `gta5.gamepad`, `gta5.player.fly`, and the `q3.pm.*` walking tune.
  None of these need gta5's streaming data. The input group is
  quake3's.
- `fs2024.player.spawn` stands the player on the real ground at init.
  `fs2024.player.ground_guard` runs after the slide move and free
  flight, and keeps the player on the field and above the ground,
  because the field ends in a cliff to nothing.
- `fs2024_terrain_walk_test` runs the real pmove chain over a Bullet
  heightfield, on the flat and up slopes.

Only SPIR-V is built:

```bash
glslc -fshader-stage=frag shaders/spirv/fs2024_terrain.frag.glsl -o shaders/spirv/fs2024_terrain.frag.spv
glslc -fshader-stage=vert shaders/spirv/fs2024_terrain.vert.glsl -o shaders/spirv/fs2024_terrain.vert.spv
```

## Next

1. Airport models. Inflate the `.fsc` files, decode the meshopt
   buffers (meshoptimizer), and load the `.ktx2` textures. The model
   placements are in the BGL, and the models themselves are in
   `<icao>_modellib.bgl`.
2. A coarser outer ring of terrain, so the mountains do not stop at the
   16 km edge.
3. Real imagery or land cover from an open source, in place of the
   height and slope guess.
