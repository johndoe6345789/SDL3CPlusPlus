# Star Wars Episode I Racer (2026 update)

A modernisation of *Star Wars Episode I Racer* (LucasArts, 1999). The
package ships no game data. It reads an existing install, decodes what it
needs, and writes upscaled or rebuilt assets to `packages/racer/generated/`.

Status: **prototype.** The asset pipeline is C++ and runs end to end. No
game loop or rendering exists yet. Format survey below. Block tables, model chunk tags, and texture
format (4-bit indexed, RGB565 palettes) are mapped. The palette pairing
is unverified, and the vertex layout is not yet known.
UI images and audio have a first modernised pass.

## Install layout (`<game>/data`)

| path                    | contents                                              |
|-------------------------|-------------------------------------------------------|
| `lev01/out_modelblock.bin`   | 323 entries, tagged chunks (see below)           |
| `lev01/out_splineblock.bin`  | 91 entries, track splines (no tags)              |
| `lev01/out_spriteblock.bin`  | 179 entries, sprites (no tags)                   |
| `lev01/out_textureblock.bin` | 1648 entries, texture data (no tags)             |
| `images/*.TGA`          | 92 UI textures, uncompressed truecolour TGA           |
| `wavs/22K`, `wavs/11K`  | ~2400 WAV sound effects and voice lines               |
| `wavs/Music`            | music loops                                           |
| `anims/*.znm`           | zlib-wrapped cutscene/planet animation scripts        |
| `bundle*.fcr`           | Smush/FCR bundles (DirectShow filter graph, not assets) |

## Block tables

Each `out_*block.bin` starts with a big-endian `u32` count `N`, then `N`
big-endian `u32` offsets. Entry `i` runs from its offset to the next
larger offset (or end of file). Texture offsets are not ascending, so
ends are taken from the sorted offset set.

Model entries carry four-character ASCII tags at their start:

| tag    | count | guess                                   |
|--------|-------|-----------------------------------------|
| `Part` | 57    | model part (mesh piece)                 |
| `Pupp` | 29    | pod or puppet animation/rig             |
| `MAlt` | 23    | model alternate / LOD                   |
| `Podd` | 23    | racing pod                              |
| `Trak` | 21    | track geometry                          |
| `Scen` | 5     | scenery                                 |
| `Modl` | 3     | top-level model header                  |

The remaining 162 model entries have no tag; they are not yet understood.


## Model chunks (`Modl`, `Part`)

Model entries are a sequence of tagged chunks. Each starts with a four
character tag and a big-endian `u32` that looks like the chunk's header
length, then words that point at sub-chunks by offset from the start of
the entry. Sentinel values `0xffffffff` mark unused slots. Chunks seen:

| tag    | role (tentative)                                             |
|--------|--------------------------------------------------------------|
| `Modl` | model header: a table of offsets to its parts                |
| `Part` | one mesh part: vertex-like floats and an `HEnd` terminator   |
| `Anim` | animation keyframe block, inside `Modl` and `Part`           |
| `Cnfj` | config block, seen twice inside `Part`                       |
| `HEnd` | end-of-header marker, four bytes, no payload                 |

Confirmed: `HEnd` closes each header, and `Anim` offsets point inside
their own entry. Not confirmed: the exact field meaning of each word,
and the vertex layout. Floats such as `0x3f800000` (1.0) appear in
`Part` bodies, which points at position or matrix data.

## Texture entries

The texture block has no tags. Entry sizes give the format:

| entry size (bytes) | count | reading                                     |
|--------------------|-------|---------------------------------------------|
| 2048               | 351   | 64x64, 4 bits per pixel (palette index)     |
| 512                | 128   | 32x32, 4 bits per pixel                     |
| 1024               | 290   | 32x32, 8 bits per pixel                     |
| 256                | 20    | 16x16, 8 bits per pixel                     |
| 32                 | 759   | small 4-bit tile or palette                 |

A 4-bit greyscale render of the 2048-byte entries shows clear shapes
(hooks, rings, bands), so they are indexed pixels.

Palettes: the 32-byte entries are 16 big-endian RGB565 colours. Decoded,
they give plausible sand, grass, sky and metal tones. In file order each
2048-byte texture sits directly beside a 32-byte palette (345 and 332
adjacent pairs), so the decoder pairs each texture with the nearest
palette before it. The pairing is a heuristic and not yet verified
against a known image. the `racer.assets.build` step writes 350 PNGs; most
look coherent (spirals, stripes, sand), a few may use the wrong palette.

## Track splines (`out_splineblock.bin`)

Verified by plotting. Each entry is a 16-byte header plus `count` records
of 84 bytes. The header's second word is the record count; the first word
is a pointer, purpose unknown. Records are big-endian:

| word  | meaning                                                      |
|-------|--------------------------------------------------------------|
| 0-2   | `0xffffffff` sentinels (record 0 holds the header instead)   |
| 3     | flags (`0xffff0000` or a pointer in record 0)                |
| 4     | `0x00010001`                                                 |
| 5     | high 16 bits: this record's 1-based id, wrapping to 0        |
| 6     | high 16 bits: the previous record's id (a ring)              |
| 8-10  | point 0 as x, z, height                                      |
| 13    | `1.0`                                                        |
| 14-16 | point 1 as x, z, height                                      |
| 17-19 | point 2 as x, z, height                                      |

Plotted top-down, entry 0 is a closed circuit with a hairpin and a
chicane, drawn as three parallel rows (lane 0 red, lane 1 green, lane 2
blue). Which row is the left edge, centre line or right edge is not yet
confirmed. About 51 entries have 20 or more records and plot as circuits
(`racer.assets.build` writes them to `racer_generated/tracks/`). Some plots
have straight lines across the interior, which suggests the ring order
does not always follow array order, not yet checked.

## Texture pairing and palettes

Palettes are **ARGB1555**, big-endian, 16 entries per 32-byte palette:
red in bits 11-15, green 6-10, blue 1-5, alpha in bit 0. An earlier
version of this decoder read them as RGB565, which made most textures
look purple and yellow. With ARGB1555 the full texture sheet reads as
natural sand, stone, metal, grass and sky palettes.

The pairing rule (each texture takes the nearest palette before it in
block order) now looks right across the sheet. The palette-after rule has
not been tested under the corrected format.

Texture dimensions are not stored in the block. The community tools take
them from XML; the 2048-byte entries are 64x64 at 4 bits per pixel, which
matches the sizes seen here.

References for the formats (facts only, no code copied):

- [OpenSWE1R swe1r-tools](https://github.com/OpenSWE1R/swe1r-tools): texture
  and spline extractors, GPL-2.0-or-later. The source of the ARGB1555 layout
  and the 4-bit index packing.
- [OpenSWE1R swe1r-re](https://github.com/OpenSWE1R/swe1r-re): reverse
  engineering notes for the game.
- [tim-tim707 SW_RACER_RE](https://github.com/tim-tim707/SW_RACER_RE): a
  decompilation project. It reads the same four blocks.

## Models: N64 display lists

The community notes that some model data is N64 display lists in the
F3DEX2 (GBI) form, and that the blocks are shared with the Nintendo 64
version. That fits the `Part` chunks here, and gives a route to the vertex
layout: N64 `Vtx` records are 16 bytes (three s16 positions, a u16 flag,
two s16 texture coordinates, four colour or normal bytes). Not yet checked
against a `Part` chunk.

## Spline record: disagreement

[swe1r-tools](https://github.com/OpenSWE1R/swe1r-tools) places the position
at bytes 16-27 and a normal at 28-39. The plots in this package show
readable float triples at bytes 32-43 and 56-67 and 68-79, and a closed
track when plotted from there. The community layout may mislabel the
fields. The float reading is what the plots use.

## Confirmed against the reverse-engineering notes

The OpenSWE1R `swep1rcr.exe` notes (see
[the folder](https://github.com/OpenSWE1R/swe1r-re/tree/master/swep1rcr.exe))
name the chunk tags `Comp`, `Data`, `Anim`, `AltN`, `Modl`, `Trak`,
`Podd`, `Part`, `Scen`, `MAlt` and `Pupp`, and say the data is
byte-swapped on load. Counted in the model block here:

| tag    | entries containing it | note                                        |
|--------|-----------------------|---------------------------------------------|
| `HEnd` | 162                   | the 162 untagged entries start with this     |
| `Part` | 58                    |                                             |
| `Anim` | 64                    |                                             |
| `AltN` | 47                    | alternates                                  |
| `Pupp` | 30                    |                                             |
| `MAlt` | 24                    |                                             |
| `Podd` | 24                    |                                             |
| `Trak` | 22                    |                                             |
| `Data` | 13                    | see below                                   |
| `Scen` | 6                     |                                             |
| `Modl` | 4                     |                                             |
| `Comp` | 0                     | no compressed entries in this install       |

`Data` chunks hold `LStr` records: the tag, then three big-endian floats,
16 bytes each. Three checked chunks all keep that stride, with 28, 6 and
22 records. The points run along the track, so these look like polylines.
The axis order is not yet known (height or z).

The spline loader in those notes reads about 42 bytes per entry. That
matches 42 big-endian 16-bit fields, consistent with the 84-byte record
above.

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

## Generated assets

Written to `D:
acer_generated\` by the tools below, outside the repo:


- `textures/`: 350 decoded textures, scaled 4x with Scale2x, and
  `sheet.png`, all textures side by side.
- `tracks/`: top-down plots of the circuit splines.
- `images/`: the 92 UI TGAs scaled 4x with Scale2x.
- `wavs/`: 2479 WAVs resampled from 11.025/22.05 kHz to 44.1 kHz.

## Running the asset build (C++)

The asset pipeline is a workflow step, `racer.assets.build`, in
`src/services/impl/workflow/racer/`. It runs against the install and
writes to `racer_generated/` under the working directory:

    RACER_DIR="<game>/data/.." SDL3CPP_HEADLESS=1       ./sdl3_app --bootstrap bootstrap_windows --game racer

It decodes the textures, upscales the UI images, and resamples the
sounds to 44.1 kHz. About 11 seconds on a desktop, 588 MB of output.
Parameters are in `workflows/racer_assets.json` (`scale`, `audio_rate`).

Unit tests: `racer_block_table_test`, `racer_texture_test`,
`racer_upscale_test` and `racer_wav_test`, all gtest.

## Next steps

0. Build a game workflow that draws the track and textures in 3D.
1. Pin down the `Part` vertex layout and the `Modl` offset table.
2. Verify the texture-to-palette pairing, for example with the
   sprite block, which may reference textures by index.
3. Build the game workflow once models and textures render in the
   engine.

## Upgrade rules

- Keep original data read-only; write only to `generated/`.
- Trace logging when fixing a parser bug, per AGENTS.md.
- Keep files under 80 lines and lines under 80 columns.
