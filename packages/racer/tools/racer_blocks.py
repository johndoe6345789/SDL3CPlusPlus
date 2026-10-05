"""Star Wars Episode I Racer block tables (lev01/out_*block.bin).

Each block opens with a big-endian u32 entry count N, followed by N
big-endian u32 offsets into the same file. Entry i spans from its
offset to the next larger offset in the file (or the end of the file).
Texture offsets are not in ascending order, so ends come from the
sorted set of offsets, not from the table order.
"""

import struct

BLOCK_FILES = {
    "model": "out_modelblock.bin",
    "spline": "out_splineblock.bin",
    "sprite": "out_spriteblock.bin",
    "texture": "out_textureblock.bin",
}


def read_table(data):
    """Return a list of (index, start, end) for every entry in a block."""
    count = struct.unpack(">I", data[:4])[0]
    offsets = [struct.unpack(">I", data[4 * i:4 * i + 4])[0]
               for i in range(1, count + 1)]
    ends = sorted(set(offsets + [len(data)]))
    spans = []
    for index, start in enumerate(offsets):
        end = next(e for e in ends if e > start)
        spans.append((index, start, end))
    return spans


def entry_tag(data, start, end):
    """Return the four-character ASCII tag at an entry, or None."""
    if end - start < 4:
        return None
    raw = data[start:start + 4]
    if all(32 <= c < 127 for c in raw):
        return raw.decode("ascii")
    return None
