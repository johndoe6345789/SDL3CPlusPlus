# Star Wars Episode I Racer (2026 update)

*Star Wars Episode I Racer* (LucasArts, 1999) rebuilt on this engine. Every
track and pod is decoded from the game's own data at run time, its textures
upscaled 4x, and raced with new pod physics. **No game data ships here**:
point `RACER_DIR` at your own install.

    RACER_DIR="D:/SteamLibrary/steamapps/common/Star Wars Episode I Racer"       ./sdl3_app --bootstrap bootstrap_windows --game racer

| variable            | meaning                                                   |
|---------------------|-----------------------------------------------------------|
| `RACER_DIR`         | the install (holds `SWEP1RCR.EXE` and `data/`)             |
| `RACER_TRACK`       | `0`-`24` or part of a name (`Inferno`): race it at once    |
| `RACER_POD`         | part of a racer's name (`Sebulba`): race as them at once   |
| `RACER_LAPS`        | `1`-`5`: the lap count for such a race                     |
| `RACER_AUTOPILOT`   | `1`: skip the menus and let the pod drive itself           |
| `RACER_UNLOCK_ALL`  | `1`: every circuit, track and racer open                   |
| `RACER_PROFILE`     | a file to keep the career in, instead of the user's        |
| `RACER_VIDEOS`      | `1`: play cutscenes even headless or on autopilot          |
| `RACER_SKIP_VIDEOS` | `1`: never play cutscenes                                  |
| `RACER_EXPORT`      | a folder: also export textures, OBJ models, UI art, audio  |
| `RACER_SHOT`        | a file: save a screenshot about twelve seconds in          |
| `RACER_MUTE`        | `1`: no sound                                              |

Runs with `SDL3CPP_HEADLESS=1` are silent and skip cutscenes, unless
`SDL_AUDIO_DRIVER=dummy` (sound that plays to nowhere) or `RACER_VIDEOS`
is set.

## Playing

The first run plays the install's opening cutscenes (the LucasArts logo,
the crawl and the Boonta Eve intro; Enter or Escape skips). The title
screen (the original splash art, Anakin's theme) offers:

- **Tournament**: the original's four circuits (Amateur, Semi-Pro,
  Galactic, Invitational) and their 25 tracks in order. Choose the
  track, the racer and how the purse is shared: Fair (top four paid),
  Skilled (top three, a bigger purse) or Winner Takes All. A podium
  opens the circuit's next track, podiums on every track open the next
  circuit, and a first win on a track opens another racer (seven to
  start). Points add up per circuit. The first race on each planet
  starts with its flyover cutscene.
- **Free race**: any open track and racer, 1-5 laps, 0-11 rivals. It
  runs the racer's stock pod and pays nothing, as in the original.
- **Watto's shop**: the game's own 42 parts, six per type (R-20 to R-600
  Repulsorgrip, Control Linkage to Control Stabilizer, Dual 20PcX to
  Mag-6 Injector, Plug2 to Block6 Thrust Coil, Mark II to Quadrijet Air
  Brake, Coolant Radiator to Turbo Coolant Pump, Single Power Cell to
  Cluster2 Power Plug) at the game's own prices. Watto stocks better
  parts as podiums mount up, allows a trade-in for the part replaced,
  and talks in his own voice.
- **Junkyard**: used parts, cheaper and worn, a new pile after each
  tournament race.
- **Pit droids**: a crew of up to four (2,000 truguts each). Tournament
  races wear every part (more for a battered pod, and the coolers and
  injectors when run hot); after each race every droid mends 6% of each
  part. A worn part gives only part of its improvement.

Truguts, parts, droids and progress are saved in `profile.json` in the
user's SDL pref folder (`SDL3CPlusPlus/EpisodeIRacer`).

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

Each race has a loading screen, a countdown, and banners for GO!, FINAL
LAP and FINISHED; pause offers resume, restart and quit. Every racer
flies the game's own handling figures (Sebulba tops out at 600 km/h,
Anakin at 490 before parts). Boost heats the engines; at full heat they
catch fire, cut out and take damage. Each engine is damaged on its own,
and a hurt engine lowers top speed and pulls the pod toward its side
until repaired. Hazards wait on some planets: Tusken Raiders fire from
Tatooine's canyon sides, lava and methane vents erupt on Baroonda and
Malastare, rocks fall on Ando Prime, Ord Ibanna and Mon Gazza. The
player's racer reacts out loud in their own voice: contact, walls, big
jumps, fire, repairs, passing someone, winning and losing.

## Scale

Everything is sized from the game's own data. Each pod is laid out from
its shadow quad (its engines' spread and cable length) at an eighth of
the parts' raw size: the game's handling table gives every pod a contact
radius of 5-10 units and a hover height of 5, which only fit that size.
Real-world sizes then put a game unit at about 10 cm: tracks are about
25 m wide, Anakin's engines about 7 m long and his whole pod 26 m with
its cables, Sebulba's 13 m wide, and The Boonta Classic about 8.7 km a
lap. Speeds read the game's figures as km/h.

## Status

Working: all 25 tracks and 25 racers decode and race; the track draws
with textures upscaled 4x (Scale2x) and mipmapped, baked N64 vertex
colours, and the track's skybox fading into a fog taken from its own
horizon, at 1920x1080. Pods are lit and draw with engine flames,
binders, cables and a shadow. Pods hover on the game's own collision
surface and its surface flags act (boost strips, slow and rough ground,
ice, swamp, lava, death drops). Up to eleven rivals fly their own pods'
handling round the line on the autopilot (speed planned from the turns
ahead, steering by a cross-track controller, within a lane of the line
as the original's AI pods are), jostle by mass and are ranked.
Cutscenes (`data/anims/*.znm`: gzip around LucasArts SMUSH, Blocky16
video and VIMA audio) are decoded with FFmpeg and played letterboxed
across the screen with sound. Audio is the install's: menu and planet
music, engine, countdown, hazards, and the racers' and Watto's voices
(the numbered lines were transcribed to learn which line is which).

New rather than decoded, flagged in the code: the hazards (the
original's triggers are not decoded), purse sizes, points, the pit
droids' price and repair rate, how much a part improves (5% a level),
the wear rule, and the autopilot. Approximations: pods are lit once at
load; walls are tested along the pod's centre line; pods collide as
three circles; the physics reads the track as a height field, so
near-vertical banks act as walls. In a one-lap autopilot run of every
track, six need no recovery, ten need one or two, and the rest three or
four (falls from narrow ledges, such as Grabvine Gateway's 8 m paths).

Not done: the original's animated track scenery, the race announcer,
split screen and multiplayer.

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
by side ahead of the cockpit. Parts face game -y (the engines' intakes
point that way), so the engines go on the -y side. The flat four-corner
quads are the pod's shadows, in the parts' raw units: the longest spans
the whole pod (its width is the engines' spread, its length engines to
cockpit; a mirrored twin leans the other way), the other the cockpit
alone. The layout spreads the parts to fill the longest.

### Tables in the executable

Read from the unpacked image (see below) and kept as facts, not code:
the pod handling table (0x4C2BB0, 24 records of 15 floats in character
order: anti-skid, turn response, max turn rate, acceleration, max
speed, air brake, deceleration, boost thrust, heat rate, cool rate,
hover height, repair rate, bump mass, damage immunity, contact radius;
copied into `racer_tracks.json`), and the parts table (0x4C1CB8, 42
records of 16 bytes: id, level, podiums needed, type, price, sprite,
name pointer).

### Cutscenes and voices

`data/anims/*.znm` are gzip files holding LucasArts SMUSH (`SANM`)
streams: Blocky16 video at 640 x 272, 15 fps, and VIMA ADPCM audio at
22.05 kHz; FFmpeg's `smush` demuxer reads them once unzipped. `Goldie` is
the LucasArts logo, `TextCrawl` the crawl, `IntroScene` the Boonta Eve
intro; `PlanetTAT`, `A`, `B`, `C`, `D`, `E`, `F` and `J` fly over
Tatooine, Ando Prime, Aquilaris, Ord Ibanna, Baroonda, Mon Gazza, Oovo
IV and Malastare (`G` repeats `D`).

`data/wavs/22K/Voice` holds 956 numbered lines: 25 race lines (`sp`)
and 13 selection lines (`ui`) per racer, under a two-letter prefix (`as`
Anakin, `sb` Sebulba, ...), and Watto's 53 (`wtui`). Transcribing them
showed one scheme for every racer: 001 contact, 002 a whoop, 004
damage, 005-006 hits, 007 airborne, 009 a scream, 010 fixed, 014 the
win, 015 the loss, 016-025 taunts; Anakin's later lines sit one on.

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

Units: a game unit is about 10 cm (`kRacerWorldScale` = 0.1; see Scale);
the engine is y-up, so a game point (x, y, z) maps to (x, z, -y) x 0.1.

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
- `flow/`: the title menu, tournament, free race, Watto's shop, the
  junkyard, pit droids, loading, pause and results (`racer.flow`), the
  parts table, circuits and purses, and the saved profile.
- `render/`: `racer.camera.chase`, `racer.scene.draw`, `racer.hud.text`
  (HUD and banners) and `racer.screen.draw` (menu screens).
- `audio/`: `racer.audio.update`, its mixer, and the racers' and
  Watto's voices.
- `video/`: `racer.video.play`, the cutscene player (FFmpeg).

Shaders are in `shaders/spirv/` (GLSL source next to the SPIR-V; rebuild
with `glslc`). Unit tests: `racer_*_test`.
