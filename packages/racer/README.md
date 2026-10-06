# Star Wars Episode I Racer (2026 update)

*Star Wars Episode I Racer* (LucasArts, 1999) rebuilt on this engine. Every
track and pod is decoded from the game's own data at run time, its textures
upscaled 4x, and raced with new pod physics. **No game data ships here**:
point `RACER_DIR` at your own install.

    RACER_DIR="D:/SteamLibrary/steamapps/common/Star Wars Episode I Racer"       ./sdl3_app --bootstrap bootstrap_windows --game racer

| variable          | meaning                                                     |
|-------------------|-------------------------------------------------------------|
| `RACER_DIR`       | the install (holds `SWEP1RCR.EXE` and `data/`)               |
| `RACER_TRACK`     | `0`-`24` or part of a name (`Inferno`): preselects the track |
| `RACER_POD`       | part of a racer's name (`Sebulba`): preselects the racer     |
| `RACER_LAPS`      | `1`-`5`: preselects the lap count                            |
| `RACER_AUTOPILOT` | `1`: skip the menu and let the pod drive itself (demo, tests)|
| `RACER_PROFILE`   | a file to keep truguts and upgrades in, instead of the user's |
| `RACER_EXPORT`    | a folder: also export textures, OBJ models, UI art, audio    |
| `RACER_SHOT`      | a file: save a screenshot about twelve seconds in            |
| `RACER_MUTE`      | `1`: no sound                                                |

Runs with `SDL3CPP_HEADLESS=1` are silent too, unless
`SDL_AUDIO_DRIVER=dummy` is set (which plays to nowhere, to test audio).

## Playing

The game opens on the title screen (the original splash art) with
Anakin's theme. Choose the track (all 25), the racer (all 23 pods), laps
(1-5) and rivals (0-11), visit the pod shop, then start. Each race has a
loading screen, a countdown, and big banners for GO!, FINAL LAP and
FINISHED; the results table pays truguts by place (1,500 for a win down
to 50 for eighth). In the pod shop (the hangar backdrop) truguts buy five
levels each of traction, turning, acceleration, top speed, air brake,
cooling and repair, at 250 truguts times the next level. Truguts and
upgrades are saved in `profile.json` in the user's SDL pref folder
(`SDL3CPlusPlus/EpisodeIRacer`).

| action         | keyboard                | pad                    |
|----------------|-------------------------|------------------------|
| throttle       | Up / W                  | right trigger, stick up |
| brake          | Down / S                | left trigger, stick down |
| steer          | Left/Right, A/D         | left stick             |
| boost          | Space, Left Shift       | (A)                    |
| repair         | R                       | (X)                    |
| menu move      | arrows, W/A/S/D         | left stick             |
| select         | Enter                   | (A)                    |
| back / pause   | Escape, P               | (B), Start             |

Pause offers resume, restart and quit to the title. Boost needs
near-top speed and heats the engines; at full heat they catch fire, cut
out for three seconds and take damage. Each engine is damaged on its own
(walls hurt the side that hits them), and a hurt engine both lowers top
speed and pulls the pod toward its side until repaired; repairing costs
speed.

## Status

Working: all 25 tracks and 23 racers' pods decode; the track draws with
textures upscaled 4x (Scale2x) and mipmapped, baked N64 vertex colours,
the planet's skybox and per-planet distance fog at 1920x1080. Pods are
lit, draw with engine flames that grow under boost, the energy binder and
control cables between their parts, and a ground shadow. Pods hover on
the game's own collision surface, ride banks up to ~75 degrees, scrape
along walls and fall into gaps (and are put back on the lap). The track's
surface flags act: boost strips, slow ground, rough ground, slippery ice
and water, swamp, lava (heats the engines) and death drops.

A field of up to twelve races: rivals with their own pods fly the lap on
the autopilot at varied pace, steer round slower pods, jostle the player,
and are ranked. The HUD shows position, lap, time, best lap, speed,
engine heat and each engine's health. Audio comes from the install:
Anakin's theme and select sounds on the menus, planet music in races
(frozen while paused), an engine note pitched by speed and boost,
countdown beeps, the start, wall scrapes and engine fires.

Approximations, flagged in the code: pod parts (engines and cockpit) are
laid out by a rule, not by the per-racer spacing and cable length the
game uses; pods are lit once at load rather than per frame; walls are
tested along the pod's centre line, not its whole width; pods collide as
three circles along their length; the physics reads the track as a height
field, so near-vertical banks act as walls; the shadow is cut off beyond
25 m rather than faded. In a full field the autopilot is still pushed off
course more often than alone (about 15-18 recoveries in 200 s on Boonta
Classic, against 8 racing alone). The upgrade economy (prices, prizes,
5% per level) is new, not the original's parts dealers and junkyard.

Not done: the original's tournament (circuits and unlocking), Watto's
shop and the junkyard, track animations and hazards beyond the surface
flags, the pit droids, voice lines, and the cutscene videos.

## Data formats

All four `data/lev01/out_*block.bin` files are big-endian (the PC port
byte-swaps on load). Layouts below were checked against a retail install;
the model, material and spline layouts follow
[swe1r-assets](https://github.com/akopetsch/swe1r-assets) (MIT), whose
metadata also names the track and pod model ids in `assets/racer_tracks.json`.

### Block tables

`u32 count`, then `count x parts` u32 offsets, then a u32 total size. A
part runs to the next non-zero offset; offset 0 means the part is absent.

| block    | parts per item            | items |
|----------|---------------------------|-------|
| model    | relocation mask, data     | 323   |
| texture  | pixels, palette           | 1648  |
| spline   | one                       | 91    |
| sprite   | one                       | 179   |

The mask has one bit per data word marking pointers the game relocates;
pointers in the data are offsets from the data part's start.

### Models

A data part starts with a tag (`Trak`, `Podd`, `Part`, `Scen`, `MAlt`,
`Pupp`, `Modl`), then node pointers until `0xFFFFFFFF` (negative words are
placeholders), then optional `Data`, `Anim` and `AltN` sections, then
`HEnd`.

Every node starts with a 0x1C-byte header: kind (`u32`), two flag words,
two `s16`, a flag word, child count (+0x14) and a pointer to an array of
child pointers (+0x18). Kinds:

| kind     | node                 | own fields after the header           |
|----------|----------------------|---------------------------------------|
| `0x3064` | mesh group           | bounding box; children are meshes      |
| `0x5064` | group                | -                                     |
| `0x5065` | selector             | `s32` choice: -1 all, -2 none, else one |
| `0x5066` | LOD selector         | 8 distances; child 0 is most detailed  |
| `0xD064` | transform            | 3x4 matrix: right, forward, up, offset |
| `0xD065` | transform with pivot | matrix, then a pivot                   |
| `0xD066` | computed transform   | set by the game at run time            |

A mesh (0x40 bytes) points to its material (+0x00), display list (+0x30)
and vertices (+0x34), with the vertex count at +0x3A. Vertices are N64
`Vtx`: `s16 x, y, z`, flags, `s16 s, t`, then RGBA (baked lighting).
Display lists are F3DEX2: `gSPVertex` (0x01) loads vertices into a 64-slot
cache, `gSP1Triangle` (0x05) and `gSP2Triangles` (0x06) index the cache,
`0xDF` ends the list. Texture coordinates span the image at 4096 units.

A material points at a 0x40-byte texture descriptor: format at +0x0C, width
and height at +0x10/+0x12, six child pointers at +0x1C (byte 3 of a child:
`0x10` double width, `0x01` double height, meaning the image is mirrored),
and the texture-block index in the low 24 bits of +0x38 (top byte `0x0A`).

A `Podd` model's root selector holds, besides shadow quads and afterburner
cones in other units, a list of parts: two engines and a cockpit, each
under a uniform 0.02 scale the game replaces when it places the part. The
parts are taken from that scale node down, using the most detailed LOD
child present (child 0 is empty on some pods), and laid out engines side
by side ahead of the cockpit.

### Textures

The texture block stores no format or size; only a material can decode a
texture. Formats: `0x0003` RGBA32, `0x0200` 4-bit and `0x0201` 8-bit
indices into an ARGB1555 palette (red bits 11-15, green 6-10, blue 1-5,
alpha bit 0), `0x0400` 4-bit and `0x0401` 8-bit intensity (alpha = grey).
4-bit texels are high nibble first. Every model's materials together
decode 1,510 distinct textures.

### Splines

A 16-byte header (segment count at +4), then 84-byte segments: predecessor
and successor counts (`u16` at +0, +2), successor ids at +4 and +10,
predecessor ids at +8 and +6, a knot at +0x10, and Bezier control points
before (+0x28) and after (+0x34) the knot; positions are z-up. Two
successors mark a fork; alternate routes are stored running back toward
the fork. Following first successors from segment 0 gives the lap, and it
closes on all 25 tracks (Boonta Classic: 135 of 180 segments, 86,612
units round).

Units: a game unit is about 5 cm (`kRacerWorldScale` = 0.05); the engine
is y-up, so a game point (x, y, z) maps to (x, z, -y) x 0.05.

### Other data

`images/*.TGA` are plain truecolour TGA; `wavs/` holds 16-bit PCM at
11.025 and 22.05 kHz. The asset export upscales the former 4x and
resamples the latter to 44.1 kHz.

## Executable analysis: packed on disk

`SWEP1RCR.EXE` cannot be disassembled from the file as shipped:

- `.text` is 698 KB with Shannon entropy 8.00, which is effectively random.
  It has no standard function prologues (`55 8b ec` or `55 89 e5` never
  appear), and a linear sweep decodes junk from the first byte.
- The entry point lies in a `.bind` section (entropy 7.96). That is a
  protector stub, which decrypts the code at run time.
- The OpenSWE1R function addresses (`sub_448780` and others) are only valid
  for the unpacked image in memory.

The route is to read the unpacked image from a running process. Launching
the game from an unelevated shell fails with `WinError 740` (requires
elevation). The embedded manifest does not request elevation, so the cause
is probably the protector or a compatibility setting. The process must be
started from an elevated (administrator) shell.

Reading the image is a small script that starts the game, waits for the
unpack, reads 0x400000 to the end of `.data`, and terminates only the process
it started. It is not in the repo. Run it from an elevated terminal, after
the game has reached its menu:

    python D:\racer_asm\dump_running.py 25

The result is `D:\racer_asm\swep1rcr_memory.bin`. Disassemble from its
`.text` section, which is at 0x401000 in memory.

## Verified functions in the unpacked image

Checked against a memory dump taken from the running game. The dump is
about 99.7% valid code. Listings stay outside the repo, because they are
the game's code.

| address  | role                                                            |
|----------|-----------------------------------------------------------------|
| 0x42D600 | `block_slot(index)`: the cache slot for each block, 0-3 only    |
| 0x42D680 | `get_block(index)`: opens a block file once, with `rb`          |
| 0x49F1E0 | `fsopen_rb(path, mode)`: a wrapper passing `_SH_DENYNO` (0x40)  |
| 0x4475F0 | membership test in global table A (count at 0x50C628)           |
| 0x447630 | membership test in global table B (count at 0x50C62C)           |
| 0x446FC0 | `load_spline(index)`: header, offset table, read, byte-swap     |
| 0x42D640 | `read_block(index, offset, buffer, size)`: seek then fread      |
| 0x448780 | `load_model(index)`: count, 12-byte entry, size limit, swap     |
| 0x4485D0 | `parse_model(buffer)`: swaps the header, walks offset lists     |

Block indices, verified from the jump table: 0 is `out_modelblock.bin`, 1
is `out_spriteblock.bin`, 2 is `out_splineblock.bin` and 3 is
`out_textureblock.bin`. Any other index uses a path the caller supplies.
If the open fails, the game hangs in a loop at 0x42D6DA.

The spline loader reads the 32-bit count, allocates, reads the entry, and
byte-swaps its header and records in place from big-endian to
little-endian. Its buffer layout confirms the 16-byte header and the
84-byte record used by the decoder above.

## Code map

`src/services/{interfaces,impl}/workflow/racer/`:

- `data/`: block reader, model parser (`racer_model_*`), textures,
  palettes, mirroring, Scale2x upscaler, splines, WAV, track table, and the
  export passes (PNG, OBJ, plots, audio).
- `world/`: `racer.world.load` (loads a race when the menu asks), GPU
  upload with the upscaled texture cache, the ground grid used for
  hovering, walls and surface flags, the starting grid, and the pod rig
  (flames, binders, cables, shadow meshes).
- `player/`: pod physics (pure, unit-tested), surfaces, `racer.pod.drive`,
  the autopilot and traffic avoidance, rivals (`racer.opponents.fly`),
  field rules (contact, ranking), lap progress and `racer.lap.timer`.
- `flow/`: the title menu, pod shop, loading, pause and results
  (`racer.flow`), upgrades and the saved profile.
- `render/`: `racer.camera.chase`, `racer.scene.draw`, `racer.hud.text`
  (HUD and banners) and `racer.screen.draw` (menu screens).
- `audio/`: `racer.audio.update` and its mixer.

Shaders are in `shaders/spirv/` (GLSL source next to the SPIR-V; rebuild
with `glslc`). Unit tests: `racer_*_test`.
