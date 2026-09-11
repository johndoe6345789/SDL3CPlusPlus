#!/usr/bin/env python3
"""Extract textures from GTA V texture dictionaries (.ytd) as PNG.

    python packages/gta5/tools/ytd_to_png.py --in <dir> --out <dir>

Like the drawable reader, this only opens files a tool has already
extracted; a .ytd is a plain RSC7 resource. Everything is stdlib -- the
block decoder and the PNG writer are here rather than pulled from
Pillow, to match the other tools in this folder.

Texture dictionary layout, established by probing real downtown .ytd
files rather than assumed:

    +0x20   list of name hashes (count at +0x28)
    +0x30   list of texture pointers (count at +0x38)

and per texture:

    +0x18   width      (u16)
    +0x1A   height     (u16)
    +0x1F   DXGI format code (71 = BC1, 77 = BC3, ...)
    +0x22   mip count
    +0x28   pointer to the name, as a real string
    +0x38   pointer to the pixel data, in the graphics block

Width and height were located by finding the only offset where both
values are powers of two across every texture in a dictionary. The
format field reads as valid DXGI codes throughout, and lands where you
would expect semantically: foliage comes out BC3 (it needs alpha) and
building faces BC1. That agreement is the evidence the decode is right.
"""

import argparse
import os
import struct
import sys
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from rage_resource import Resource  # noqa: E402

# DXGI format code -> (name, bytes per 4x4 block)
FORMATS = {
    71: ("BC1", 8), 72: ("BC1", 8),      # BC1_UNORM / _SRGB
    74: ("BC2", 16), 75: ("BC2", 16),    # BC2_UNORM / _SRGB
    77: ("BC3", 16), 78: ("BC3", 16),    # BC3_UNORM / _SRGB
    80: ("BC4", 8), 83: ("BC5", 16),
    98: ("BC7", 16), 99: ("BC7", 16),
}


def rgb565(value):
    r = (value >> 11) & 0x1F
    g = (value >> 5) & 0x3F
    b = value & 0x1F
    return (r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2)


def bc1_block(data, off, out, ox, oy, width, height, opaque):
    """Decode one BC1 colour block into the RGBA buffer."""
    c0, c1 = struct.unpack_from("<HH", data, off)
    bits = struct.unpack_from("<I", data, off + 4)[0]
    p0, p1 = rgb565(c0), rgb565(c1)
    if c0 > c1 or opaque:
        palette = [p0, p1,
                   tuple((2 * p0[i] + p1[i]) // 3 for i in range(3)),
                   tuple((p0[i] + 2 * p1[i]) // 3 for i in range(3))]
        alpha = [255, 255, 255, 255]
    else:
        palette = [p0, p1,
                   tuple((p0[i] + p1[i]) // 2 for i in range(3)),
                   (0, 0, 0)]
        alpha = [255, 255, 255, 0]
    for py in range(4):
        for px in range(4):
            x, y = ox + px, oy + py
            if x >= width or y >= height:
                continue
            idx = (bits >> (2 * (4 * py + px))) & 3
            r, g, b = palette[idx]
            base = (y * width + x) * 4
            out[base:base + 4] = bytes((r, g, b, alpha[idx]))


def bc3_alpha(data, off, out, ox, oy, width, height):
    """Decode one BC3 alpha block into the RGBA buffer's alpha channel."""
    a0, a1 = data[off], data[off + 1]
    # Interpolation weights must sum to the divisor: 6:1 .. 1:6 over 7,
    # and 4:1 .. 1:4 over 5. Getting that wrong overflows the byte.
    if a0 > a1:
        table = [a0, a1] + [((6 - i) * a0 + (1 + i) * a1) // 7 for i in range(6)]
    else:
        table = ([a0, a1] + [((4 - i) * a0 + (1 + i) * a1) // 5 for i in range(4)]
                 + [0, 255])
    bits = int.from_bytes(data[off + 2:off + 8], "little")
    for py in range(4):
        for px in range(4):
            x, y = ox + px, oy + py
            if x >= width or y >= height:
                continue
            idx = (bits >> (3 * (4 * py + px))) & 7
            out[(y * width + x) * 4 + 3] = table[idx]


def decode(data, width, height, kind):
    """Decode a compressed mip level into an RGBA byte buffer."""
    out = bytearray(width * height * 4)
    blocks_x = max(1, (width + 3) // 4)
    blocks_y = max(1, (height + 3) // 4)
    off = 0
    for by in range(blocks_y):
        for bx in range(blocks_x):
            if kind == "BC1":
                bc1_block(data, off, out, bx * 4, by * 4, width, height, False)
                off += 8
            elif kind in ("BC2", "BC3"):
                if kind == "BC3":
                    bc3_alpha(data, off, out, bx * 4, by * 4, width, height)
                bc1_block(data, off + 8, out, bx * 4, by * 4, width, height, True)
                off += 16
            else:
                return None
    return out


def write_png(path, width, height, rgba):
    """Minimal RGBA PNG writer: zlib is all this needs."""
    rows = bytearray()
    for y in range(height):
        rows.append(0)  # no per-row filter
        rows += rgba[y * width * 4:(y + 1) * width * 4]

    def chunk(tag, payload):
        return (struct.pack(">I", len(payload)) + tag + payload +
                struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF))

    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    with open(path, "wb") as handle:
        handle.write(b"\x89PNG\r\n\x1a\n")
        handle.write(chunk(b"IHDR", header))
        handle.write(chunk(b"IDAT", zlib.compress(bytes(rows), 6)))
        handle.write(chunk(b"IEND", b""))


def read_dictionary(path):
    """Yield (name, width, height, format name, pixel bytes) per texture."""
    res = Resource(path)
    for ptr in res.pointer_list(0x30):
        tex = res.resolve(ptr)
        if tex is None:
            continue
        width, height = res.u16(tex + 0x18), res.u16(tex + 0x1A)
        code = res.u8(tex + 0x1F)
        name = res.string(res.resolve(res.u64(tex + 0x28)))
        data_off = res.resolve(res.u64(tex + 0x38))
        if not width or not height or data_off is None or code not in FORMATS:
            continue
        kind, block = FORMATS[code]
        size = max(1, (width + 3) // 4) * max(1, (height + 3) // 4) * block
        if data_off + size > len(res.raw):
            continue
        yield name, width, height, kind, res.raw[data_off:data_off + size]


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--in", dest="src", required=True)
    parser.add_argument("--out", dest="dst", required=True)
    parser.add_argument("--max-size", type=int, default=1024,
                        help="skip textures larger than this (pure-Python "
                             "decoding is slow; 0 for no limit)")
    args = parser.parse_args()

    files = []
    for root, _, names in os.walk(args.src):
        files += [os.path.join(root, n) for n in names
                  if n.lower().endswith(".ytd")]
    files.sort()
    if not files:
        sys.exit("no .ytd files under %s" % args.src)

    os.makedirs(args.dst, exist_ok=True)
    written = skipped = 0
    seen = set()
    for path in files:
        try:
            entries = list(read_dictionary(path))
        except Exception as exc:
            print("  skip %s: %s" % (os.path.basename(path), exc))
            continue
        for name, width, height, kind, data in entries:
            if not name or name in seen:
                continue
            seen.add(name)
            if args.max_size and max(width, height) > args.max_size:
                skipped += 1
                continue
            rgba = decode(data, width, height, kind)
            if rgba is None:
                skipped += 1
                continue
            write_png(os.path.join(args.dst, name + ".png"),
                      width, height, rgba)
            written += 1

    print("wrote %d textures, skipped %d, from %d dictionaries -> %s"
          % (written, skipped, len(files), args.dst))


if __name__ == "__main__":
    main()
