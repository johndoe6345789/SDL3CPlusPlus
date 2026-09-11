#!/usr/bin/env python3
"""Reader for RAGE resource files (RSC7), shared by the gta5 tools.

An extracted .ydr, .ytd or .ymap is a plain RSC7 resource: a 16-byte
header followed by a raw-deflate payload. Nothing here touches archive
encryption; these are files a tool has already extracted for you.

The payload is two blocks of paged memory, system then graphics, and
pointers inside it carry a page tag in their top bits. The page sizes
encoded in the header's two flag words must sum exactly to the payload
length, which is asserted on load: a format reader that is subtly wrong
produces plausible rubbish rather than failing, so it is worth checking
the one thing that can be checked.
"""

import struct
import zlib


def page_size(flags):
    """Decode a RAGE page-size flag word into a byte count.

    Each bit group counts pages of one size class, and the low nibble
    selects the base class.
    """
    parts = [((flags >> 27) & 0x1) << 0, ((flags >> 26) & 0x1) << 1,
             ((flags >> 25) & 0x1) << 2, ((flags >> 24) & 0x1) << 3,
             ((flags >> 17) & 0x7F) << 4, ((flags >> 11) & 0x3F) << 5,
             ((flags >> 7) & 0xF) << 6, ((flags >> 5) & 0x3) << 7,
             ((flags >> 4) & 0x1) << 8]
    return (0x200 << (flags & 0xF)) * sum(parts)


def to_engine(vec):
    """GTA Z-up to engine Y-up.

    Every tool here must apply the same mapping. A placement transform is
    M*T*inverse(M), so converting transforms without converting meshes
    lays every building on its side.
    """
    return (vec[0], vec[2], -vec[1])


class Resource(object):
    """A decompressed RSC7 resource and its paged pointer space."""

    def __init__(self, path):
        blob = open(path, "rb").read()
        if blob[:4] != b"RSC7":
            raise ValueError("not an RSC7 resource")
        _, self.version, sys_flags, gfx_flags = struct.unpack("<IIII", blob[:16])
        self.raw = zlib.decompress(blob[16:], -15)
        self.sys_size = page_size(sys_flags)
        expected = self.sys_size + page_size(gfx_flags)
        if expected != len(self.raw):
            raise ValueError("page sizes %d != payload %d"
                             % (expected, len(self.raw)))

    def resolve(self, ptr):
        """RAGE pointer to flat offset. System pages first, graphics after."""
        if not ptr:
            return None
        low = ptr & 0xFFFFFFFF
        offset = low & 0x0FFFFFFF
        if low & 0x50000000 == 0x50000000:
            return offset
        if low & 0x60000000 == 0x60000000:
            return self.sys_size + offset
        return None

    def u8(self, off):
        return self.raw[off]

    def u16(self, off):
        return struct.unpack_from("<H", self.raw, off)[0]

    def u32(self, off):
        return struct.unpack_from("<I", self.raw, off)[0]

    def u64(self, off):
        return struct.unpack_from("<Q", self.raw, off)[0]

    def string(self, off):
        """NUL-terminated ASCII at a flat offset."""
        if off is None:
            return ""
        end = self.raw.index(b"\0", off)
        return self.raw[off:end].decode("ascii", "replace")

    def pointer_list(self, off):
        """A pointer-to-array plus count, as RAGE stores collections."""
        array = self.resolve(self.u64(off))
        count = self.u16(off + 8)
        if array is None:
            return []
        return [self.u64(array + 8 * i) for i in range(count)]
