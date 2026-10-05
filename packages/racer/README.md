# Star Wars Episode I Racer (2026 update)

A modernisation of *Star Wars Episode I Racer* (LucasArts, 1999). The
package ships no game data. It reads an existing install, decodes what it
needs, and writes upscaled or rebuilt assets to `packages/racer/generated/`.

Status: **format survey.** Block tables, model chunk tags, and texture
entry sizes are mapped; the texture palette and vertex layout are not.
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
(hooks, rings, bands), while RGB565 renders are noise. So they are
indexed, but the palette is not located yet. The palette is the
open question for texture decoding.

## Generated assets

Written to `D:acer_generated\` by the tools below, outside the repo:

- `images_x4b/`: the 92 UI TGAs upscaled 4x (Lanczos) to PNG.
- `wavs_44k/`: 2479 WAVs resampled from 11.025/22.05 kHz to 44.1 kHz.

## Tools

    python packages/racer/tools/racer_extract.py \
        "<game>/data/lev01" <out_dir>

Dumps each block entry to `<out_dir>/<block>/<index>_<tag>.bin` and writes
`index.json` beside them. Stdlib only.

Also:

    python packages/racer/tools/racer_upscale.py "<game>/data/images" <out> 4
    python packages/racer/tools/racer_resample.py "<game>/data/wavs" <out>

## Next steps

1. Pin down the `Part` vertex layout and the `Modl` offset table.
2. Find the texture palette (search the block for 16- or 32-bit colour
   runs near each texture entry).
3. Build the game workflow once models and textures render in the
   engine.

## Upgrade rules

- Keep original data read-only; write only to `generated/`.
- Trace logging when fixing a parser bug, per AGENTS.md.
- Keep files under 80 lines and lines under 80 columns.
