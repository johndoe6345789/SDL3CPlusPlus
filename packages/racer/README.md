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

## Texture pairing: unresolved

Two pairing rules were tested by eye on the full texture sheet:

- palette before the texture (current code): first textures look coherent,
  many later ones are dominated by one blue-and-yellow palette;
- palette after the texture: more varied in places, but the same
  blue-and-yellow tone recurs.

Neither is confirmed. The tones may be real art, which would mean the
rule is right. A reference is needed to decide. The contact sheet is at
`racer_generated/textures/sheet.png`.

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
