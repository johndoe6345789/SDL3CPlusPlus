#!/usr/bin/env python3
"""Extract textures from GTA V texture dictionaries (.ytd) as PNG.

    python packages/gta5/tools/ytd_to_png.py --in <dir> --out <dir>

Like the drawable reader, this only opens files a tool has already
extracted; a .ytd is a plain RSC7 resource.

It runs on the standard library alone -- the block decoder and the PNG
writer are both in this file -- but will use numpy and Pillow when they
are installed, which is roughly two orders of magnitude faster and makes
extracting full-resolution textures practical. Both paths are checked
against each other by --self-test.

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

try:
    import numpy as _np
except ImportError:
    _np = None
try:
    from PIL import Image as _Image
except ImportError:
    _Image = None

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


def _unpack_565(values):
    """Vectorised 565 -> 8-bit RGB."""
    r = (values >> 11) & 0x1F
    g = (values >> 5) & 0x3F
    b = values & 0x1F
    return (_np.stack([(r << 3) | (r >> 2),
                       (g << 2) | (g >> 4),
                       (b << 3) | (b >> 2)], axis=-1).astype(_np.uint8))


def _decode_colour_np(blocks, opaque):
    """BC1 colour blocks -> (n, 4, 4, 4) RGBA."""
    c0 = blocks[:, 0].astype(_np.uint16) | (blocks[:, 1].astype(_np.uint16) << 8)
    c1 = blocks[:, 2].astype(_np.uint16) | (blocks[:, 3].astype(_np.uint16) << 8)
    bits = (blocks[:, 4].astype(_np.uint32) |
            (blocks[:, 5].astype(_np.uint32) << 8) |
            (blocks[:, 6].astype(_np.uint32) << 16) |
            (blocks[:, 7].astype(_np.uint32) << 24))

    p0, p1 = _unpack_565(c0).astype(_np.uint16), _unpack_565(c1).astype(_np.uint16)
    four = (c0 > c1) | opaque
    mid2 = _np.where(four[:, None], (2 * p0 + p1) // 3, (p0 + p1) // 2)
    mid3 = _np.where(four[:, None], (p0 + 2 * p1) // 3, _np.zeros_like(p0))
    palette = _np.stack([p0, p1, mid2, mid3], axis=1).astype(_np.uint8)

    alpha = _np.full((len(blocks), 4), 255, dtype=_np.uint8)
    alpha[~four, 3] = 0

    shifts = (2 * _np.arange(16)).astype(_np.uint32)
    idx = ((bits[:, None] >> shifts[None, :]) & 3).astype(_np.intp)
    rows = _np.arange(len(blocks))[:, None]
    rgb = palette[rows, idx]
    a = alpha[rows, idx][..., None]
    return _np.concatenate([rgb, a], axis=-1).reshape(-1, 4, 4, 4)


def _decode_alpha_np(blocks):
    """BC3 alpha blocks -> (n, 4, 4) alpha."""
    a0 = blocks[:, 0].astype(_np.int32)
    a1 = blocks[:, 1].astype(_np.int32)
    table = _np.empty((len(blocks), 8), dtype=_np.int32)
    table[:, 0], table[:, 1] = a0, a1
    wide = a0 > a1
    for i in range(6):
        table[:, 2 + i] = _np.where(
            wide, ((6 - i) * a0 + (1 + i) * a1) // 7, 0)
    for i in range(4):
        table[:, 2 + i] = _np.where(
            wide, table[:, 2 + i], ((4 - i) * a0 + (1 + i) * a1) // 5)
    table[:, 6] = _np.where(wide, table[:, 6], 0)
    table[:, 7] = _np.where(wide, table[:, 7], 255)

    bits = _np.zeros(len(blocks), dtype=_np.uint64)
    for i in range(6):
        bits |= blocks[:, 2 + i].astype(_np.uint64) << _np.uint64(8 * i)
    shifts = (3 * _np.arange(16)).astype(_np.uint64)
    idx = ((bits[:, None] >> shifts[None, :]) & _np.uint64(7)).astype(_np.intp)
    rows = _np.arange(len(blocks))[:, None]
    return table[rows, idx].astype(_np.uint8).reshape(-1, 4, 4)


def decode_np(data, width, height, kind):
    """Vectorised decode. Same output as decode(), ~100x faster."""
    stride = 8 if kind in ("BC1", "BC4") else 16
    raw = _np.frombuffer(data, dtype=_np.uint8)
    n = len(raw) // stride
    blocks = raw[:n * stride].reshape(n, stride)

    if kind == "BC1":
        texels = _decode_colour_np(blocks, False)
    elif kind in ("BC2", "BC3"):
        texels = _decode_colour_np(blocks[:, 8:], True)
        if kind == "BC3":
            texels[..., 3] = _decode_alpha_np(blocks[:, :8])
    else:
        return None

    bx, by = max(1, (width + 3) // 4), max(1, (height + 3) // 4)
    if len(texels) < bx * by:
        return None
    image = (texels[:bx * by].reshape(by, bx, 4, 4, 4)
             .transpose(0, 2, 1, 3, 4).reshape(by * 4, bx * 4, 4))
    return bytearray(image[:height, :width].tobytes())


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
                # Colour first: bc1_block writes all four channels, so
                # decoding alpha before it would have it overwritten with
                # the opaque 255 and silently discard every cutout.
                bc1_block(data, off + 8, out, bx * 4, by * 4, width, height, True)
                if kind == "BC3":
                    bc3_alpha(data, off, out, bx * 4, by * 4, width, height)
                off += 16
            else:
                return None
    return out


def write_png(path, width, height, rgba):
    """Write RGBA as PNG, via Pillow when present."""
    if _Image is not None:
        _Image.frombytes("RGBA", (width, height), bytes(rgba)).save(path)
        return
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
    parser.add_argument("--self-test", action="store_true",
                        help="decode with both paths and compare, then exit")
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

    if args.self_test:
        if _np is None:
            sys.exit("--self-test needs numpy; nothing to compare against")
        checked = mismatch = 0
        kinds = {}
        for path in files:
            for name, w, h, kind, data in read_dictionary(path):
                if w * h > 256 * 256:      # keep the pure-Python side quick
                    continue
                slow = decode(data, w, h, kind)
                fast = decode_np(data, w, h, kind)
                if slow is None or fast is None:
                    continue
                checked += 1
                kinds[kind] = kinds.get(kind, 0) + 1
                if bytes(slow) != bytes(fast):
                    mismatch += 1
                    print("  MISMATCH %s %dx%d %s" % (name, w, h, kind))
        print("compared %d textures %s" % (checked, kinds))
        print("identical: %s" % ("yes" if not mismatch else "NO (%d)" % mismatch))
        sys.exit(0 if not mismatch else 1)

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
            rgba = (decode_np(data, width, height, kind) if _np is not None
                    else decode(data, width, height, kind))
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
