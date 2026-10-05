# Star Wars Episode I Racer (2026 update)

A modernisation of *Star Wars Episode I Racer* (LucasArts, 1999). The
package ships no game data. It reads an existing install, decodes what it
needs, and writes upscaled or rebuilt assets to `packages/racer/generated/`.

Status: **scaffold and format survey.** The block tables are decoded;
model, spline, sprite and texture payloads are not yet.

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

## Tools

    python packages/racer/tools/racer_extract.py \
        "<game>/data/lev01" <out_dir>

Dumps each block entry to `<out_dir>/<block>/<index>_<tag>.bin` and writes
`index.json` beside them. Stdlib only.

## Next steps

1. Identify the header of the tagged model chunks (`Modl`, `Part`).
2. Decode the texture entries (the header fields at the start of each
   entry, then palette or pixel layout).
3. Convert the `images/*.TGA` UI set and `wavs/` audio as a first
   modernisation pass (upscale and resample), which needs no reverse
   engineering.
4. Build the game workflow once models and textures render in the
   engine.

## Upgrade rules

- Keep original data read-only; write only to `generated/`.
- Trace logging when fixing a parser bug, per AGENTS.md.
- Keep files under 80 lines and lines under 80 columns.
