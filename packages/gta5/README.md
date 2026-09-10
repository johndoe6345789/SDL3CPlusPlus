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

## What still needs writing

The streaming workflow references four plugins that do not exist yet:

    gta5.tiles.resolve   gta5.tiles.load   gta5.tiles.evict   gta5.lod.select

They are listed in `workflows/gta5_streaming.json` under
`unimplemented_plugins`. Everything else this package references
(`q3.pm.*`, `model.spawn`, `model.set_transform`, `model.despawn`,
`physics.*`, `camera.fps.update`) already exists.
