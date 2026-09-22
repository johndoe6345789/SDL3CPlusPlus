"""Stunts .RES resource archives (uncompressed).

    uint32 total size of the file
    uint16 entry count
    char   tag[4] per entry
    uint32 offset per entry, relative to the end of this header

Car files hold `simd` (simulation parameters) and `edes` (the spec
text the showroom screen prints).  `simd` opens with, as little-endian
16-bit fields: gear count, a performance index, a fixed 0x0100, idle
rpm, peak-power rpm, red line and the rev limit.
"""

import struct


def parse(raw):
    """Return {tag: bytes} for one .RES image."""
    size, count = struct.unpack('<IH', raw[0:6])
    pos = 6
    tags = [raw[pos + i * 4:pos + i * 4 + 4].decode('latin1')
            for i in range(count)]
    pos += 4 * count
    offsets = list(struct.unpack('<%dI' % count, raw[pos:pos + 4 * count]))
    base = pos + 4 * count
    order = sorted(range(count), key=lambda i: offsets[i])
    out = {}
    for rank, index in enumerate(order):
        start = base + offsets[index]
        end = (base + offsets[order[rank + 1]]
               if rank + 1 < count else size)
        out[tags[index]] = raw[start:end]
    return out


ENGINE_FIELDS = ('gears', 'performance', 'reserved',
                 'idle_rpm', 'power_rpm', 'red_line', 'rev_limit')


def engine(simd):
    """Decode the engine block at the head of `simd`."""
    values = struct.unpack('<7H', simd[0:14])
    return dict(zip(ENGINE_FIELDS, values))


def spec_lines(edes):
    """Split the showroom text into its ']'-separated lines."""
    text = edes.split(b'\x00')[0].decode('latin1')
    return [line for line in text.split(']') if line]
