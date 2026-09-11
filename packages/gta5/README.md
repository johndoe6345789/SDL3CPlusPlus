# gta5

Streams the GTA V map -- Los Santos and Blaine County -- straight from
your extracted game files, and moves the player around it with Quake 3's
kinematics. There is no conversion step: nothing is exported, converted
or written to disk. The engine reads each ymap, drawable and texture
dictionary the first time a tile needs it and keeps it in memory.

## The one thing you have to do yourself

The game's archives are RPF7 with the NG encryption flag set. Reading
them means pulling key tables out of the game executable and decrypting
the archives, which is circumventing the game's copy protection rather
than parsing a file format, so none of that code is in here.

You extract the files yourself, with your own tool and your own copy --
the package was built against an extract made with GTAUtil. Everything
past that line is here. An extracted `.ymap`, `.ydr`, `.ydd`, `.yft` or
`.ytd` is a plain RSC7 resource: a 16-byte header, then raw deflate.

Point `map_dir` at the extracted `levels/gta5` folder, on the
`assets_index` node in `workflows/gta5_game.json`, and run:

```bash
python python/dev_commands.py run --game gta5
```

The extract is G9 -- resource versions 159 for `.ydr`/`.ydd`, 171 for
`.yft`, 5 for `.ytd`, which are CodeWalker's Gen9 values -- and the
readers use the G9 layouts.

## Reading the map on demand

`gta5.assets.index` runs once at startup, on another thread, and builds
an index in memory: ymaps by tile, drawables by archetype hash, textures
by name hash. On the full map that is 90,358 files, 3,456 ymaps over 381
tiles, 131,819 drawables and 39,033 textures -- 1.1 s with the files in
the OS cache, 11-18 s cold.

A `.ydr` or `.yft` is named after its archetype, so its file name is the
key. A `.ydd` or `.ytd` stores names only as Jenkins hashes, so each is
opened to read its table -- inflating only the system pages, which is
where that table lives. The 14.8 GB of `.ytd` pixel data is not touched.

When a tile is wanted, `gta5.tiles.load` reads its ymaps and hands every
archetype it will draw to a pool of worker threads, one per core but one.
Workers inflate the drawable, decode its mesh, build its collision BVH
and read its textures; the main thread only uploads finished work, for
`upload_budget_ms` a frame, so the frame rate -- and the physics, which
is capped at 1/30 s a step -- holds up while a district streams in.
Opened dictionaries sit in an LRU bounded by inflated bytes, 8 GB by
default (`resource_cache_mb`), shared by the workers under a lock.
Drawing tests each instance's bounding sphere against the view and skips
any smaller than `cull_size_ratio` of its distance; physics steps up to
0.15 s of wall time a frame (`max_delta_time`), so a slow frame no
longer means slow motion.

Drawing is instanced. `gta5.tiles.cull` groups the visible instances by
archetype and uploads their matrices to one storage buffer, and every map
mesh lives in a few large vertex and index blocks (`Gta5GeometryArena`),
so a frame binds one buffer pair rather than two per draw: SDL's Vulkan
backend tracks each buffer a command buffer uses with a linear scan,
which at 6,000 buffers a frame cost more than everything else together.
`gta5.frame.stats` logs the frame rate and per-step CPU time every 2 s,
and `SDL3CPP_PRESENT_MODE=immediate` lifts vsync to measure headroom.

The scene renders at twice the window's size (`render_scale` on
`frame.gpu.begin_offscreen`), and the composite resolves each pixel from
the texels under it, then applies a light contrast-adaptive sharpen, in
place of FXAA's edge blur. Textures use 16x anisotropic filtering with no
mip bias, and entities switch LOD at their own `lodDist` and
`childLodDist`, so each place is drawn by one level of detail at a time.

Skipped on purpose: grass ymaps (315 files of instance data, and GTAUtil
turned each into ~40 MB of XML), LOD lights, occlusion, and placed
interiors (`CMloInstanceDef`).

### The readers

**ymap.** A Meta block: a list of typed data blocks (`+0x30`, count at
`+0x4C`), the root `CMapData` named by a 1-based id at `+0x1C`. Pointers
inside are block references -- block id in the low 12 bits, offset in
bits 12-31. Entities are an array at `CMapData +0x60`; only blocks whose
structure hash is `CEntityDef`'s are read. Checked against GTAUtil's XML
on every ymap that places anything: 725,777 entities agree in name,
position, rotation and scale. Where they differ, GTAUtil is the one that
is wrong -- its reverse dictionary is case-sensitive and labelled one
archetype `MaxTimeForAmbientReaction`.

**Drawable.** `+0x10` shader group, `+0x50` high-detail models; each
model lists geometries at `+0x08` and their shader indices at `+0x20`. A
geometry has its vertex buffer at `+0x18`, index buffer at `+0x38`,
index count at `+0x58`. The vertex layout is read, not guessed: the G9
declaration at the buffer's `+0x38` is 52 slots in fixed semantic order
-- 0 position, 4 normal, 28 texcoord 0 -- with a u32 offset (`+0`), a
stride byte (`+208`) and a DXGI-style format byte (`+260`) each.

**Shaders.** The name hash is at `+0x00`, the render bucket at `+0x39`,
checked against `(1 << bucket) | 0xFF00` at `+0x3C`. Bucket 3 is cutout
-- a birch's trunk is bucket 0 and every leaf shader bucket 3 -- so
foliage discards below alpha 0.5. `vehicle_paint*` takes the vehicle's
paint colour, which the game keeps in `carcols.ymt`.

**Textures** go to the GPU as stored: BC1/2/3/4/5/7 stay compressed,
mips included. A `.ytd` holds name hashes at `+0x20` (count at `+0x28`)
and textures at `+0x30`; a texture its size at `+0x18`/`+0x1A`, format at
`+0x1F`, mip count at `+0x22`, name at `+0x28` and pixels at `+0x38`.
Mip levels follow mip 0 back to back -- checked by decoding mip 1 where
mip 0 ends and comparing it with mip 0 halved. Names match
case-insensitively: shaders say `IM_DT1_02_Metal_01`, dictionaries
store lowercase, and a literal match silently lost most of downtown.

## Loading

While the map loads, the player is held at the spawn point and a label
in the corner says what it is waiting on: `Indexing map...`, then
`Loading: 43%`. The ground counts as in once the spawn tile *and its
eight neighbours* have spawned -- entities are tiled by origin, and a
terrain piece whose origin is in the next tile can be the ground here.
Released, the player is put down on the first surface below.

Leaf cards do not collide: a leaf is a rectangle whose shape is only in
its alpha, and as collision a canopy held up a car.

## Why the Quake 3 movement drops in unchanged

The kinematics are not reimplemented here. `workflows/gta5_physics.json`
runs the same `q3.pm.*` steps the `quake3` package runs -- friction,
acceleration, air control, jump, step-slide.

They transplant cleanly because both sides are in metres.
`q3_pm_constants.hpp` already divides the raw Quake units by 32, and GTA
V authors its world in metres. Quake's 18-unit step-up becomes 0.5625 m,
which clears a GTA V kerb (~0.15 m) and its stairs (~0.18 m) as it is.

## Coordinate conventions

GTA V is Z-up right-handed; this engine is Y-up. The readers apply
`(x, y, z) -> (x, z, -y)` as they read, and nothing downstream re-maps.
Entity rotations are also conjugated: a `CEntityDef` stores the
*inverse* of the entity's orientation, and placed as written every
rotated prop on the map comes out mirrored.

## Driving

`gta5.vehicle.spawn` reads a car from the map and puts it on the road as
a **btRaycastVehicle** -- a chassis body plus four wheels that cast a ray
each step and apply suspension and tyre forces.

| key | action |
|-----|--------|
| `F` | get in or out of the nearest car within 6 m |
| `W` / `S` | throttle / reverse (rear-wheel drive) |
| `A` / `D` | steer (front wheels) |
| `Space` | brake |

The spawn parameters name the body (`taxi_hi`, the detailed model), the
wheel, the paint colour, the wheel size, the heading and the position.
The car waits for the ground under it, then is dropped `drop_height`
onto the first surface a ray finds, starting 5 m above the given height
-- so on a stacked road the height picks the level. It is currently on
the Grand Senora Desert road; downtown kept finding kerbs and
underpasses.

- The collider is a **box sized to the mesh**, with its floor at axle
  height. Reaching down to the brake discs, it left 8 cm of clearance
  and beached the car on the first kerb, front wheels in the air.
- A car with nothing under it is **frozen** until the ground arrives,
  so one driven faster than the map streams does not fall through it.
- While seated the player is **pinned** to the chassis, with no contact
  response -- the pin is inside the chassis box, and colliding it fired
  the car into the air.
- `gta5.vehicle.camera` replaces the first-person view with an eased
  chase camera while seated.

### Wheels

A vehicle's `.yft` has no wheels in it. Where each belongs it carries
only a brake-disc hub -- on the taxi a filled disc of radius 0.17 m --
because the game instances a wheel model at each wheel bone. Those are
their own pack, `vehiclemods/wheels_mods.rpf`: a tyre and a rim,
modelled about the origin with the axle along x.

The axles come from the fragment, as CodeWalker takes them: FragType
`+0xF0` is the physics LOD group, `+0x10` of that LOD 1; in the LOD,
`+0x30` is an offset added to every transform, `+0xD0` the children,
`+0x11D` their count, `+0x100` the transform block (64-byte matrices from
`+0x20`, translation last). A child's bone tag is at `+0x12`; the wheel
bones are 26418, 27922, 26398 and 27902. The hubs sit 0.14 m behind
their own axle, which is enough to see in the arch.

The pack wheel is read once and shaped twice, to `wheel_radius` and
`wheel_width` (the real sizes are in `vehicles.meta`): as modelled for
the right-hand pair, mirrored for the left. Vehicles are turned to face
+z as they are read; GTA models face +y, which the axis change maps to
-z, so a car read as it stands drives backwards.

## Shaders and the frame

The frame renders into an HDR target. `gta5.sky.draw` fills it with a
procedural sky lit by the same sun as the city, and publishes its
horizon colour, which the model shader fogs to -- where the two differ,
the horizon shows a band. The package's composite does FXAA, then ACES
and gamma: not MSAA, which does nothing for an edge a discard made, and
not the engine's TAA, which has no motion vectors and smears a moving
car.

Rebuild the shaders with the Vulkan SDK's `glslc` after editing, for
example:

```bash
glslc -fshader-stage=frag shaders/spirv/gta5_model.frag.glsl -o shaders/spirv/gta5_model.frag.spv
```

**Only SPIR-V is built.** The workflow names the `shaders/msl/` path
because the engine rewrites it to `shaders/spirv/` off Metal; a Metal
build needs the `.metal` variants written.

## Known gaps

- **Load speed.** Drawables and textures are read, inflated and
  uploaded on the main thread; a dense downtown tile takes about 20 s.
  Decoding on worker threads, with only the upload on the main one, is
  the fix.
- **The tile-file fallback.** Without `map_dir`, `gta5.tiles.load` still
  reads `assets/tiles/*.json` and glTF models through assimp. Nothing
  produces those any more; the path is legacy.
- **Metal shaders.** SPIR-V only.
- **Normals under non-uniform scale.** The vertex shader transforms
  normals by `mat3(u_model)`; a placement with `scaleXY != scaleZ` would
  want the inverse-transpose.
- **No frustum culling.** Every instance in every resident tile is
  drawn.
- **Not read:** interiors, `vehicles.meta` and `carcols.ymt` -- which is
  why the paint and wheel size are parameters.
