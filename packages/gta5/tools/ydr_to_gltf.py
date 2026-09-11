#!/usr/bin/env python3
"""Convert extracted GTA V drawables (.ydr) to glTF.

Reads the RAGE resource format directly, so the mesh half of the pipeline
needs neither Blender nor CodeWalker. It only reads files a tool has
already extracted for you and never touches archive encryption: an
extracted .ydr is a plain RSC7 resource that stock zlib decompresses.

    python packages/gta5/tools/ydr_to_gltf.py --in <dir> --out <dir>

Emits POSITION and NORMAL. UVs are skipped on purpose: the vertex layout
varies per archetype, and textures live in .ytd, a separate format this
does not read, so a UV would have nothing to sample.

Format notes, all verified against real downtown drawables rather than
taken on trust:

  * header is magic / version / sysFlags / gfxFlags, then raw deflate.
  * the two flag words decode to page sizes summing exactly to the
    decompressed length. That equality is asserted, so a wrong
    assumption fails loudly on the first file instead of producing
    plausible rubbish.
  * pointers are 64-bit: 0x50000000 marks a system page, 0x60000000 a
    graphics page, and the offset is the low 28 bits.
  * Drawable +0x50 is the high-LOD model list. Models hold geometries. A
    geometry has its vertex buffer at +0x18, index buffer at +0x38, and
    index count at +0x58.
  * Position is Float3 at vertex offset 0 -- confirmed by checking the
    values land inside the drawable's own bounding box.

Vertices are mapped from GTA's Z-up space to the engine's Y-up by
(x, y, z) -> (x, z, -y), the same mapping import_codewalker_export.py
applies to placements. Both sides must agree: a placement transform is
really M*T*inverse(M), so converting the transform without converting
the mesh lays every building on its side.
"""

import argparse
import base64
import json
import math
import os
import struct
import sys
import zlib


def to_engine(vec):
    """GTA Z-up to engine Y-up. Must match the placement importer."""
    return (vec[0], vec[2], -vec[1])


def page_size(flags):
    """Decode a RAGE page-size flag word into a byte count."""
    parts = [((flags >> 27) & 0x1) << 0, ((flags >> 26) & 0x1) << 1,
             ((flags >> 25) & 0x1) << 2, ((flags >> 24) & 0x1) << 3,
             ((flags >> 17) & 0x7F) << 4, ((flags >> 11) & 0x3F) << 5,
             ((flags >> 7) & 0xF) << 6, ((flags >> 5) & 0x3) << 7,
             ((flags >> 4) & 0x1) << 8]
    return (0x200 << (flags & 0xF)) * sum(parts)


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
            raise ValueError("page sizes %d != payload %d" % (expected, len(self.raw)))

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

    def u16(self, off):
        return struct.unpack_from("<H", self.raw, off)[0]

    def u32(self, off):
        return struct.unpack_from("<I", self.raw, off)[0]

    def u64(self, off):
        return struct.unpack_from("<Q", self.raw, off)[0]

    def pointer_list(self, off):
        """A pointer-to-array plus count, as RAGE stores collections."""
        array = self.resolve(self.u64(off))
        count = self.u16(off + 8)
        if array is None:
            return []
        return [self.u64(array + 8 * i) for i in range(count)]


def normal_offset(res, data, count, stride):
    """Locate the normal by finding a Float3 of unit length.

    The vertex declaration is not decoded, so the layout is recovered
    from the data. Position is always first; the normal is the next
    Float3 whose length is 1 across every sample. Returns None when
    nothing qualifies, which is normal for a position-only buffer.
    """
    samples = min(count, 256)
    for off in range(4, stride - 11, 4):
        for i in range(samples):
            vec = struct.unpack_from("<3f", res.raw, data + i * stride + off)
            if not 0.97 < math.sqrt(sum(c * c for c in vec)) < 1.03:
                break
        else:
            return off
    return None


def read_geometry(res, geom_ptr):
    """One geometry's positions, normals and indices, or None if unusable."""
    geom = res.resolve(geom_ptr)
    vbuf = res.resolve(res.u64(geom + 0x18))
    ibuf = res.resolve(res.u64(geom + 0x38))
    if vbuf is None or ibuf is None:
        return None

    count = res.u32(vbuf + 0x08)
    stride = res.u32(vbuf + 0x0C)
    data = res.resolve(res.u64(vbuf + 0x18))
    index_count = res.u32(geom + 0x58)
    index_data = res.resolve(res.u64(ibuf + 0x18))
    if not count or not stride or data is None or index_data is None:
        return None
    if not index_count:
        return None

    positions = [to_engine(struct.unpack_from("<3f", res.raw, data + i * stride))
                 for i in range(count)]
    noff = normal_offset(res, data, count, stride)
    if noff is None:
        normals = [(0.0, 1.0, 0.0)] * count
    else:
        normals = [to_engine(struct.unpack_from(
            "<3f", res.raw, data + i * stride + noff)) for i in range(count)]

    indices = list(struct.unpack_from("<%dH" % index_count, res.raw, index_data))
    if max(indices) >= count:
        return None
    return positions, normals, indices


def read_drawable(path):
    """Merge every high-LOD geometry in a drawable into one mesh."""
    res = Resource(path)
    model_list = res.resolve(res.u64(0x50))
    if model_list is None:
        return [], [], []

    positions, normals, indices = [], [], []
    for model_ptr in res.pointer_list(model_list):
        model = res.resolve(model_ptr)
        if model is None:
            continue
        for geom_ptr in res.pointer_list(model + 0x08):
            part = read_geometry(res, geom_ptr)
            if part is None:
                continue
            base = len(positions)
            positions += part[0]
            normals += part[1]
            indices += [base + i for i in part[2]]
    return positions, normals, indices


def write_gltf(path, positions, normals, indices):
    pos_blob = b"".join(struct.pack("<3f", *p) for p in positions)
    nrm_blob = b"".join(struct.pack("<3f", *n) for n in normals)
    idx_blob = b"".join(struct.pack("<I", i) for i in indices)
    blob = pos_blob + nrm_blob + idx_blob

    mins = [min(p[i] for p in positions) for i in range(3)]
    maxs = [max(p[i] for p in positions) for i in range(3)]
    doc = {
        "asset": {"version": "2.0", "generator": "ydr_to_gltf.py"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0}],
        "meshes": [{"primitives": [{
            "attributes": {"POSITION": 0, "NORMAL": 1},
            "indices": 2,
        }]}],
        "buffers": [{
            "byteLength": len(blob),
            "uri": "data:application/octet-stream;base64," +
                   base64.b64encode(blob).decode("ascii"),
        }],
        "bufferViews": [
            {"buffer": 0, "byteOffset": 0, "byteLength": len(pos_blob),
             "target": 34962},
            {"buffer": 0, "byteOffset": len(pos_blob),
             "byteLength": len(nrm_blob), "target": 34962},
            {"buffer": 0, "byteOffset": len(pos_blob) + len(nrm_blob),
             "byteLength": len(idx_blob), "target": 34963},
        ],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": len(positions),
             "type": "VEC3", "min": mins, "max": maxs},
            {"bufferView": 1, "componentType": 5126, "count": len(normals),
             "type": "VEC3"},
            {"bufferView": 2, "componentType": 5125, "count": len(indices),
             "type": "SCALAR"},
        ],
    }
    with open(path, "w", encoding="utf-8") as handle:
        json.dump(doc, handle)


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--in", dest="src", required=True,
                        help="directory of extracted .ydr files")
    parser.add_argument("--out", dest="dst", required=True)
    parser.add_argument("--limit", type=int, default=0,
                        help="convert at most this many, for a quick look")
    args = parser.parse_args()

    files = []
    for root, _, names in os.walk(args.src):
        files += [os.path.join(root, n) for n in names
                  if n.lower().endswith(".ydr")]
    files.sort()
    if args.limit:
        files = files[:args.limit]
    if not files:
        sys.exit("no .ydr files under %s" % args.src)

    os.makedirs(args.dst, exist_ok=True)
    done = 0
    skipped = 0
    vertices = 0
    reasons = {}
    for path in files:
        name = os.path.splitext(os.path.basename(path))[0]
        try:
            positions, normals, indices = read_drawable(path)
        except Exception as exc:
            reasons[str(exc)] = reasons.get(str(exc), 0) + 1
            skipped += 1
            continue
        if not positions or not indices:
            reasons["no geometry"] = reasons.get("no geometry", 0) + 1
            skipped += 1
            continue
        write_gltf(os.path.join(args.dst, name + ".gltf"),
                   positions, normals, indices)
        vertices += len(positions)
        done += 1

    print("converted %d drawables (%d vertices), skipped %d -> %s"
          % (done, vertices, skipped, args.dst))
    for reason, n in sorted(reasons.items(), key=lambda kv: -kv[1])[:5]:
        print("  %5d  %s" % (n, reason))


if __name__ == "__main__":
    main()
