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

## Map data is never committed

`packages/gta5/assets/` is gitignored. Tile files carry archetype names
and coordinates lifted out of a game install, which is game-derived data
and has no place in a public repository. Regenerate what you need:

```bash
python packages/gta5/tools/make_test_tiles.py   # stand-in city, seconds
packages/gta5/tools/export_map.bat              # the real thing
```

Both write to `assets/tiles`, so the second replaces the first. If the
map goes empty after an export, that is why: real placements without
exported meshes have nothing to draw.

## Exporting, in one script

`tools/export_map.bat` drives the placement half end to end: GTAUtil
builds its cache of the install, `exportmeta` turns ymaps into XML, then
`import_codewalker_export.py` pairs them with meshes and writes tiles.
Edit the paths at the top and run it.

It reads the **Legacy** install directly rather than a pile of
previously extracted files, so the whole chain stays on one edition.
Feeding a tool that predates the Enhanced edition a set of
Enhanced-extracted ymaps may parse cleanly or may quietly misread them,
and a map that loads but is subtly wrong is the worst outcome to debug.

The cache build is slow and happens once; the script drops a
`.cache_built` marker in the work folder to skip it afterwards. Delete
that marker to force a rebuild after patching the game.

It does the meshes too. `tools/ydr_to_gltf.py` reads the RAGE resource
format directly, so CodeWalker is not needed anywhere in the pipeline.
On downtown it converts 1,879 drawables -- 6.4 million vertices -- with
nothing skipped.

That reader never touches archive encryption. An extracted `.ydr` is a
plain RSC7 resource: stock `zlib.decompress(data[16:], -15)` opens it.
Everything after that is structure walking, and it is checked rather
than assumed -- the two page-size flag words must sum exactly to the
decompressed length or the file is refused, and positions were
confirmed by checking they fall inside the drawable's own bounding box.

It emits POSITION and NORMAL only. UVs are skipped deliberately: the
vertex layout varies per archetype and textures live in `.ytd`, a format
this does not read, so a UV would have nothing to sample. Buildings come
out untextured but correctly shaped, placed and lit.

Archetypes still missing are the ones in `.ydd` dictionaries and the
SLOD meshes that live in other folders; they are written with
`model: null`, reported, and skipped at load.

On its first run GTAUtil prompts `GTAV folder :` and waits for you to
type the path to your install. **It has to be a Legacy install.**
GTAUtil predates the Enhanced edition and identifies a game folder by
`GTA5.exe`, which Enhanced does not ship -- it has `GTA5_Enhanced.exe`
instead -- so an Enhanced path is rejected and it simply asks again.
Answer it once with a Legacy folder and it remembers. Do not
pipe anything into the script, or that prompt cannot be answered -- this
is also why `GTAUtil.exe --help` appears to hang, since it is sitting at
the same prompt.

The script still counts the XML files afterwards and fails if none
appeared, so a run that produces nothing stops loudly rather than
writing an empty map.

## Textures

`tools/ytd_to_png.py` extracts textures from `.ytd` dictionaries as PNG.
It runs on the standard library alone -- the BC decoder and PNG writer
are both in the file -- and uses numpy and Pillow when installed, which
is about 14x faster and makes full-resolution extraction practical:
2,413 downtown textures in under two minutes.

Both paths are kept honest by `--self-test`, which decodes with each and
compares byte for byte. That is not decoration; it caught a real bug the
eye had already missed. The stdlib path decoded BC3 alpha and then let
the colour block overwrite all four channels with opaque 255, so every
cutout was silently lost. A birch billboard still *looked* plausible,
because the discarded texels keep their RGB. Now BC3 comes out with
alpha 0-255 and 24% of that billboard fully transparent, while a BC1
building atlas is opaque throughout -- exactly as each format should
behave.

Dictionary layout, probed rather than assumed -- `+0x20` is a list of
name hashes, `+0x30` a list of texture pointers, and per texture:

| offset | field |
|--------|-------|
| `+0x18` / `+0x1A` | width / height |
| `+0x1F` | DXGI format code (71 = BC1, 77 = BC3) |
| `+0x22` | mip count |
| `+0x28` | pointer to the name, as a real string |
| `+0x38` | pixel data, in the graphics block |

Width and height were found as the only offset where both values are
powers of two across every texture in a dictionary. The format field
reads as valid DXGI codes throughout and lands where it should
semantically: foliage is BC3 because it needs alpha, building faces are
BC1. That agreement is the evidence the decode is right, and the decoded
images confirm it.

### Which texture goes on which triangle

The whole chain is reachable from the drawable, and unlike shader names
the textures are stored as **real strings**, not hashes:

```
DrawableModel +0x20  ->  shader index per geometry   e.g. [0,1,2,3,4,4]
ShaderGroup   +0x10  ->  shader array
shader        +0x10  ->  parameter array
parameter[0]  +0x28  ->  texture name, e.g. "im_wall_concrete32"
```

Entry `[0]` is the diffuse; `_n` and `_s` siblings are normal and spec
maps. The name matches a PNG that `ytd_to_png.py` wrote, so the join is
by filename.

This is wired up end to end. The converter emits `TEXCOORD_0` and one
primitive and material per geometry; the engine uploads one submesh per
material, caches textures by path, and binds per submesh.

The texcoord offset is recovered the same way the normal is, because
nothing in the file declares it -- the table at the vertex buffer's
`+0x38` is identical across strides 52, 64 and 68, so it is not a
layout. The texcoord is the last Float2 clear of position and the
unit-length vectors, and colour sits between them as a `UByte4` that
reads as enormous or denormal taken as floats, so a magnitude test
rejects it. Across every stride in downtown that lands on a 0..1 range.

Textures are cached per path and shared across archetypes, and are
deliberately *not* freed by the geometry sweep: one texture is typically
used across a whole district, so tying its lifetime to a single
archetype would thrash it.

Texture names are matched **case-insensitively**. Shaders name textures
in mixed case (`IM_DT1_02_Metal_01`) while the dictionaries store them
lowercase, so a literal match silently fails on most of them: downtown
went from 1,490 textured primitives to 8,375 of 9,320 on that one change
alone, having barely moved when thousands more textures were extracted.
Worth remembering as a shape of bug -- it looked exactly like missing
data, and extracting more data was the wrong fix.

Shared `im_*` textures do live in `levels/gta5/generic` rather than the
district folder, so both are still worth extracting. A submesh whose
texture is genuinely missing still draws on the package default rather
than vanishing.

## Driving

`gta5.vehicle.spawn` puts a car on the road as a **btRaycastVehicle**:
a chassis body plus four wheels that cast a ray each step and apply
suspension and tyre forces. It rides on springs and grips through
corners, rather than sliding as a box.

| key | action |
|-----|--------|
| `F` | get in or out of the nearest car within 6 m |
| `W` / `S` | throttle / reverse (rear-wheel drive) |
| `A` / `D` | steer (front wheels) |
| `Space` | brake |

Notes on how it is wired, and why:

- The collider is a **box sized to the mesh bounds**, not the render
  mesh. A concave car shell is a poor dynamic collider and Bullet will
  not solve it against the ground.
- The chassis gets `DISABLE_DEACTIVATION`. A sleeping raycast vehicle
  stops casting its wheels and sinks.
- While seated the player's physics body is **pinned** to the chassis
  each frame rather than constrained. The camera already follows the
  player body, so that is all it takes to ride along, and a constraint
  would feed the player's mass back into the suspension.
- Spawning happens inside the frame loop and waits for a resident tile.
  A car dropped into a world that has not streamed yet falls through it.
- Input lands a frame after it is pressed, because the physics step for
  the frame has already run by the time the control step sees the key.

**The wheels do not turn or steer visually.** The taxi's wheels are part
of the chassis drawable rather than separate ones, so the suspension
moves the body but nothing spins. Drawing them properly means reading
the fragment's wheel drawables and placing them from
`getWheelTransformWS`.

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
