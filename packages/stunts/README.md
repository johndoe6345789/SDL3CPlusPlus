# Stunts

Drives the tracks and cars of a Stunts (4D Sports Driving) installation,
read from the game's own files at run time. **No game data ships here** --
point `STUNTS_DIR` at your own copy.

    STUNTS_DIR=/path/to/stunts STUNTS_TRACK=DEFAULT.TRK \
      SDL3CPP_HEADLESS=1 ./sdl3_app --bootstrap bootstrap_windows --game stunts

Arrow keys or WASD drive; Escape quits. A gamepad's left stick works too.

## What the engine reads

Everything below was worked out from a retail install and is implemented
in `src/services/impl/workflow/stunts/data/`. The Python in `tools/`
mirrors it, for looking at files without building.

### Containers

A compressed file opens with one or more four-byte headers:

| bytes | meaning                                                      |
|-------|--------------------------------------------------------------|
| 0     | bits 0-6 compression type, bit 7 "another header follows"     |
| 1-3   | little-endian 24-bit size of that pass's output               |

Type 2 is a canonical Huffman code: a width count, one count byte per
code length starting at length 1, the symbols in code order, then an
MSB-first bit stream. Two independent checks confirm the reading -- the
counts' Kraft sum is exactly 1.0 for every file in an install, and
decoding the declared number of symbols consumes the bit stream to
within its final padding byte.

`.TRK` and `.RES` have no container and are read as they are.

### Tracks (`.TRK`, always 1802 bytes)

| bytes     | meaning                                        |
|-----------|------------------------------------------------|
| 0-899     | 30x30 road tile ids, row-major from the north  |
| 900-1799  | 30x30 terrain ids over the same cells          |
| 1800-1801 | horizon and scenery flags                      |

Tile ids are opaque. Rather than guess them, `tools/make_tile_table.py`
derives what each id connects to statistically: it walks every track in
an install and records how often each of an id's four neighbours is
occupied. An id whose road runs north-south has a northern and southern
neighbour in essentially every placement it ever appears in. Two
opposite directions make a straight, two adjacent ones a corner, three
or more a junction. Across 41 tracks that identifies 77 road tiles, and
it agrees with what the grids look like by eye: `0x04` north-south,
`0x05` east-west, `0xfc`-`0xff` the four corners.

The result is committed as `assets/stunts_tiles.json`; regenerate it for
an install with different tracks.

### Cars (`CAR*.RES`)

An uncompressed archive: a 32-bit size, a 16-bit entry count, one
four-character tag per entry, then one 32-bit offset per entry measured
from the end of that header. Entries are not in tag order, so each blob
runs to the next offset.

`simd` opens with seven little-endian 16-bit fields -- gear count, a
performance index, a constant, idle rpm, peak-power rpm, red line and
rev limit. `edes` is the showroom text, whose quoted power, 0-60 and top
speed cross-check the numeric fields, which is how the field order was
settled. Both are read per car: nothing about any car is hard-coded.

## The second pass

The Huffman output is not the finished file: it begins with a header of
its own, so the passes nest rather than stack.

| bytes | meaning                                                    |
|-------|--------------------------------------------------------------|
| 0     | compression type -- 1, run-length                           |
| 1-3   | uncompressed size, which equals the outer header's size     |
| 4-7   | length of the compressed body that follows                  |
| 8     | escape count, 10, with a "no sequences" flag in bit 7        |
| 9-18  | the ten escape byte values                                  |
| 19..  | the body                                                    |

Escape 0 is `[count][value]`, copied `count` times. Escape 2 is
`[count-lo][count-hi][value]`, a 16-bit count for long runs. Every
other escape at index `i` is `[value]` alone, copied `i` times -- `i`
itself never appears as a byte value in the escape table, which is
what makes this unambiguous. Escape 1 doubles as a sequence delimiter
unless the "no sequences" flag is set: a first pass expands each
`escape1 ... escape1 [count]` run into `count` copies of the bytes
between the two markers, before the escape codes above run over what
remains. All 62 compressed files in a retail install decode to their
declared size exactly under this scheme, and the results check out
semantically too -- GAME1.P3S's decoded archive lists 58 real track
piece names (`road`, `turn`, `loop`, `pipe`, `tunn`, ...) with a
rising offset table that lands exactly inside the file.

## The shape format

What a decoded `.P3S`/`.PVS` holds is a small archive -- a size, an
entry count, that many four-character tags, then that many offsets --
of one or more 3D shapes. A shape is:

| bytes           | meaning                                          |
|------------------|--------------------------------------------------|
| 0-3              | vertex count, primitive count, paint job count, reserved (always 0) |
| 4..               | that many `int16 x,y,z` vertices                 |
| ..                | 8 bytes per primitive of still-undecoded culling data |
| ..                | the primitives                                   |

A primitive is `[type][flags][materials * paintJobCount][payload]`.
Type 1-10 is a flat polygon with that many sides, whose payload is
that many vertex indices. Type 11-13 is a wheel, whose payload is a
fixed six bytes regardless of the type value: two vertex indices per
face (inner and outer), each a disc centre, a point exactly `radius`
above it, and a second point exactly `radius` further round the rim
-- both always offset purely along the shape's Y and Z axes, never X,
in every wheel checked, which is what a wheel spinning on an axle
along X should look like. `materials` holds one colour id per paint
job the shape offers, so a part whose colour never changes between
schemes (tyres, glass) simply repeats the same id in every slot --
that repetition, seen in real data before it was understood, is what
gave the paint-job structure away. This model decodes every shape
checked exactly: 58/58 track pieces and 28/28 car bodies (all four
stock cars' high, medium and low detail levels) land on a valid parse
with every vertex index in range and nothing but a few trailing bytes
left over.

Colour ids map to RGB via `assets/stunts_materials.json`, read off
wiki.stunts.hu's materials table; only the ids this data actually
uses have been looked up so far, with a neutral grey default for the
rest.

Two things are still open. The flags byte's two documented bits
(two-sided, to disable back-face culling; z-bias, to force draw order
for coplanar details like door handles) are not wired into the
renderer yet. And the 8-byte-per-primitive culling block between the
vertices and the primitives -- angular visibility windows, per the
wiki -- is read past, not decoded; every shape still draws in full
regardless of view angle.

## Scale

The files carry no unit. A cell is 24 m and the road 8 m wide, which
puts a stock lap at about the length the game's own lap times imply.
Both are `stunts.world.load` parameters.

## Steps

| plugin               | what it does                                  |
|----------------------|-----------------------------------------------|
| `stunts.world.load`  | opens the install, meshes/uploads the track and the driven car's own real body |
| `stunts.car.drive`   | drives it on the car's own engine figures      |
| `stunts.lap.timer`   | times laps against the starting cell           |
| `stunts.camera.chase`| follows the car                                |
| `stunts.track.draw`  | draws ground then road                         |
| `stunts.car.draw`    | draws the car's real body panels and wheels, placed and turned by its live position |
