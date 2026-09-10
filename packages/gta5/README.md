# gta5

Streams the GTA V map — Los Santos and Blaine County — and moves the player
around it with Quake 3's kinematics.

## The one thing you have to do yourself

This package does not read anything out of your game install, and it cannot.
The map archives in

    D:\SteamLibrary\steamapps\common\Grand Theft Auto V Enhanced

are RPF7 archives with the NG encryption flag set (`0x0FEFFFFF` at offset
0x0C — you can see it in the first 16 bytes of `x64a.rpf`). Every road and
every building on the map lives behind that. Reading them means pulling the
AES key and the NG key tables out of `GTA5_Enhanced.exe` and decrypting the
archives with them, which is circumventing the game's copy protection rather
than parsing a file format, so none of that code is in here.

What is here is everything on the other side of that line. You run
[CodeWalker](https://github.com/dexyfex/CodeWalker) yourself, against your own
installed copy, and export the map. Then this package consumes the export.
That is also the sane engineering path regardless: you do not want to be
decrypting 100 GB of archives at engine startup.

### Exporting

In CodeWalker:

1. Open the RPF explorer and let it build its cache.
2. Project Explorer → select the ymaps you want → **Export XML**.
   Start with a few downtown ones (`dt1_*.ymap`) rather than the whole map.
3. Select the drawables those ymaps reference → export as **glTF**.

Then:

```bash
python packages/gta5/tools/import_codewalker_export.py \
  --ymap-dir  /path/to/exported/ymaps \
  --model-dir /path/to/exported/models \
  --out       packages/gta5/assets/tiles
```

That writes one JSON file per 512 m tile into `assets/tiles/`, which is what
the streamer reads. Archetypes with no matching exported model are written
with `"model": null` and reported, so a partial export still loads.

## Why the Quake 3 movement drops in unchanged

The kinematics are not reimplemented here. `workflows/gta5_physics.json` runs
the same `q3.pm.*` steps the `quake3` package runs — friction, acceleration,
air control, jump, step-slide — which are already in this engine under
`src/services/impl/workflow/quake3/`.

They transplant cleanly because both sides are in metres.
`q3_pm_constants.hpp` already divides the raw Quake units by 32 so that one
engine unit is one metre, and GTA V authors its world in metres to begin
with. Quake's 18-unit step-up becomes 0.5625 m, which clears a GTA V kerb
(~0.15 m) and its stairs (~0.18 m) without adjustment.

So you get Quake 3 movement — the acceleration curve, the air control, the
strafe-jumping — over Los Santos. That is the interesting part of the idea and
it needs no retuning at all.

## Coordinate conventions

GTA V is Z-up right-handed; this engine is Y-up. The importer applies
`(x, y, z) -> (x, z, -y)` once, at import time, and the runtime never
re-maps anything.

It also conjugates entity rotations. A `CEntityDef` stores the *inverse* of
the entity's orientation — place buildings with the quaternion as written and
every rotated prop on the map comes out mirrored.

## Exporting, in one script

`tools/export_map.bat` drives the placement half end to end: GTAUtil
`exportmeta` turns ymaps into XML, then `import_codewalker_export.py`
pairs them with meshes and writes tiles. Edit the four paths at the top
and run it.

It cannot do the meshes. GTAUtil has no drawable export command, so
`.ydr`/`.ydd` still have to come out of CodeWalker as glTF, OBJ, FBX,
DAE or PLY, named after their archetype. Point `MODEL_SRC` at them and
re-run; until then placements are written with `model: null`, reported,
and skipped at load, so a placements-only run still works.

On its first run GTAUtil prompts `GTAV folder :` and waits for you to
type the path to your install. Answer it once and it remembers. Do not
pipe anything into the script, or that prompt cannot be answered -- this
is also why `GTAUtil.exe --help` appears to hang, since it is sitting at
the same prompt.

The script still counts the XML files afterwards and fails if none
appeared, so a run that produces nothing stops loudly rather than
writing an empty map.

## Shaders

The package has its own pair, `shaders/spirv/gta5_model.{vert,frag}`,
because neither existing shader fits:

- seed's `textured.vert` feeds the fragment stage one constant normal
  from a uniform, so every building wall shaded as though it faced the
  sky. The gta5 vertex shader passes the real per-vertex normal.
- seed's `textured.frag` is a room-scale demo: its fog is hardcoded at
  `1 - exp(-dist * 0.06)`, which is 99.95% opaque by 128 m, so every
  building past the near kerb collapsed to a black silhouette. It also
  ray-marches a 48-step volumetric beam per fragment, far too expensive
  across a skyline. The gta5 fragment shader is a plain textured Lambert
  with kilometre-scale haze and a Reinhard tone map.
- the BSP shader carries normals but lights purely from a baked
  lightmap, which streamed GTA geometry does not have.

Rebuild them with the Vulkan SDK's `glslc` after editing:

```bash
glslc -fshader-stage=vert shaders/spirv/gta5_model.vert.glsl   -o shaders/spirv/gta5_model.vert.spv
```

**Only SPIR-V is built.** The workflow names the `shaders/msl/` path
because the engine rewrites it to `shaders/spirv/` off Metal, so Windows
and Linux work; a Metal build needs someone to write the two `.metal`
variants.

## What still needs writing

Everything the package references now exists: `gta5.tiles.resolve`,
`.load`, `.evict`, `.draw` and `gta5.lod.select` alongside the `q3.pm.*`
movement chain, physics and camera steps.

Known gaps:

- **Metal shaders.** See above; SPIR-V only.
- **Normals under non-uniform scale.** The vertex shader transforms the
  normal by `mat3(u_model)`, matching `bsp.vert`. A placement with
  `scaleXY != scaleZ` -- which real ymaps do use -- would want the
  inverse-transpose to stay exactly perpendicular.
- **No frustum culling.** Every instance in every resident tile is
  drawn. Fine for a few hundred; a real district will want culling
  before it is fine for tens of thousands.
