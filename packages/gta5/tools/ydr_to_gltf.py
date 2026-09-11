#!/usr/bin/env python3
"""Convert extracted GTA V drawables (.ydr) to glTF.

Reads the RAGE resource format directly, so the mesh half of the pipeline
needs neither Blender nor CodeWalker. It only reads files a tool has
already extracted for you and never touches archive encryption: an
extracted .ydr is a plain RSC7 resource that stock zlib decompresses.

    python packages/gta5/tools/ydr_to_gltf.py --in <dir> --out <dir> \\
        --texture-dir <PNGs from ytd_to_png.py>

Emits one primitive per geometry, each with POSITION, NORMAL, TEXCOORD_0
and a material naming its diffuse texture. Pair it with ytd_to_png.py and
the join is by filename.

Format notes, verified against real downtown drawables rather than taken
on trust:

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

Texture assignment walks:

    DrawableModel +0x20 -> shader index per geometry
    ShaderGroup   +0x10 -> shader array
    shader        +0x10 -> parameter array
    parameter[0]  +0x28 -> texture name, a real string, not a hash

The vertex layout beyond position is not declared anywhere this reader
could find -- the table at the vertex buffer's +0x38 is identical across
strides 52, 64 and 68, so it is not one -- and it varies per archetype.
Normal and texcoord offsets are recovered from the data instead: normals
are the Float3s of unit length, and the texcoord is the last Float2 clear
of them holding plausible values. Across every stride in downtown that
yields a 0..1 range, which is what a texcoord should look like.

Vertices are mapped from GTA's Z-up space to the engine's Y-up by
(x, y, z) -> (x, z, -y), the same mapping import_codewalker_export.py
applies to placements. Both sides must agree: a placement transform is
M*T*inverse(M), so converting the transform without converting the mesh
lays every building on its side. That mapping has determinant +1, so
winding is preserved and indices are left alone.
"""

import argparse
import base64
import json
import math
import os
import struct
import sys
import xml.etree.ElementTree as ElementTree

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from rage_resource import Resource, jenkins, to_engine  # noqa: E402


def unit_float3_offsets(raw, data, count, stride):
    """Offsets holding a Float3 of unit length: the normal and tangent."""
    samples = min(count, 128)
    found = []
    for off in range(0, stride - 11, 4):
        for i in range(samples):
            vec = struct.unpack_from("<3f", raw, data + i * stride + off)
            if not 0.97 < math.sqrt(sum(c * c for c in vec)) < 1.03:
                break
        else:
            found.append(off)
    return found


def texcoord_offset(raw, data, count, stride, blocked):
    """The last plausible Float2 clear of position and the unit vectors.

    Colour sits between the tangent and the texcoord as a UByte4, and
    reads as either enormous or denormal when taken as floats, so the
    magnitude test rejects it.
    """
    samples = min(count, 128)
    best = None
    for off in range(0, stride - 7, 4):
        if any(off < start + size and start < off + 8
               for start, size in blocked):
            continue
        values = []
        for i in range(samples):
            u, v = struct.unpack_from("<2f", raw, data + i * stride + off)
            if not (math.isfinite(u) and math.isfinite(v)):
                break
            if max(abs(u), abs(v)) > 64.0:
                break
            values += [u, v]
        else:
            if values and (max(values) - min(values)) > 1e-4:
                best = off
    return best


def shader_textures(res, base=0):
    """Diffuse texture name per shader index, empty where none is found."""
    group = res.resolve(res.u64(base + 0x10))
    if group is None:
        return []
    names = []
    for shader_ptr in res.pointer_list(group + 0x10):
        shader = res.resolve(shader_ptr)
        name = ""
        params = res.resolve(res.u64(shader + 0x10)) if shader else None
        if params is not None:
            for i in range(8):
                entry = res.resolve(res.u64(params + 8 * i))
                if entry is None or entry >= res.sys_size:
                    continue
                name_at = res.resolve(res.u64(entry + 0x28))
                if name_at is None or name_at >= res.sys_size:
                    continue
                try:
                    candidate = res.string(name_at)
                except ValueError:
                    continue
                # The diffuse comes first; _n and _s are normal and spec.
                if candidate and not candidate.endswith(("_n", "_s")):
                    name = candidate
                    break
        names.append(name)
    return names


def read_geometry(res, geom_ptr):
    """One geometry's vertex arrays and indices, or None if unusable."""
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

    raw = res.raw
    positions = [to_engine(struct.unpack_from("<3f", raw, data + i * stride))
                 for i in range(count)]

    units = unit_float3_offsets(raw, data, count, stride)
    if units:
        noff = units[0]
        normals = [to_engine(struct.unpack_from(
            "<3f", raw, data + i * stride + noff)) for i in range(count)]
    else:
        normals = [(0.0, 1.0, 0.0)] * count

    blocked = [(0, 12)] + [(off, 12) for off in units]
    uoff = texcoord_offset(raw, data, count, stride, blocked)
    if uoff is None:
        uvs = [(0.0, 0.0)] * count
    else:
        uvs = [struct.unpack_from("<2f", raw, data + i * stride + uoff)
               for i in range(count)]

    indices = list(struct.unpack_from("<%dH" % index_count, raw, index_data))
    if max(indices) >= count:
        return None
    return positions, normals, uvs, indices


def read_drawable_at(res, base):
    """Every high-LOD geometry of one drawable, with its texture name.

    `base` is 0 for a .ydr, or the entry offset inside a .ydd: the two
    hold the same structure, so a dictionary is just several of them.
    """
    model_list = res.resolve(res.u64(base + 0x50))
    if model_list is None:
        return []

    textures = shader_textures(res, base)
    parts = []
    for model_ptr in res.pointer_list(model_list):
        model = res.resolve(model_ptr)
        if model is None:
            continue
        geoms = res.pointer_list(model + 0x08)
        mapping = res.resolve(res.u64(model + 0x20))
        for index, geom_ptr in enumerate(geoms):
            part = read_geometry(res, geom_ptr)
            if part is None:
                continue
            texture = ""
            if mapping is not None and textures:
                shader_index = res.u16(mapping + 2 * index)
                if shader_index < len(textures):
                    texture = textures[shader_index]
            parts.append(part + (texture,))
    return parts


def read_drawable(path):
    """A .ydr holds its drawable at the start of the resource.

    A .yft is a fragment -- vehicles and breakables -- which wraps one at
    +0x30, so the same walk serves both.
    """
    res = Resource(path)
    if path.lower().endswith(".yft"):
        base = res.resolve(res.u64(0x30))
        return read_drawable_at(res, base) if base is not None else []
    return read_drawable_at(res, 0)


def read_dictionary(path, by_hash):
    """A .ydd: several drawables, named by Jenkins hash.

    Entries whose hash matches no known archetype are skipped -- they are
    usually LOD children nothing places directly.
    """
    res = Resource(path)
    hashes_at = res.resolve(res.u64(0x20))
    entries = res.pointer_list(0x30)
    if hashes_at is None:
        return []
    out = []
    for index, entry in enumerate(entries):
        name = by_hash.get(res.u32(hashes_at + 4 * index))
        base = res.resolve(entry)
        if not name or base is None:
            continue
        parts = read_drawable_at(res, base)
        if parts:
            out.append((name, parts))
    return out


def write_gltf(path, parts, texture_uri):
    """One primitive and material per part."""
    blob = bytearray()
    views, accessors, primitives = [], [], []
    images, gl_textures, materials = [], [], []
    material_of = {}

    for positions, normals, uvs, indices, texture in parts:
        base = len(accessors)
        for values, fmt, kind in ((positions, "<3f", "VEC3"),
                                  (normals, "<3f", "VEC3"),
                                  (uvs, "<2f", "VEC2")):
            offset = len(blob)
            for value in values:
                blob += struct.pack(fmt, *value)
            views.append({"buffer": 0, "byteOffset": offset,
                          "byteLength": len(blob) - offset, "target": 34962})
            accessor = {"bufferView": len(views) - 1, "componentType": 5126,
                        "count": len(values), "type": kind}
            if values is positions:
                accessor["min"] = [min(p[i] for p in values) for i in range(3)]
                accessor["max"] = [max(p[i] for p in values) for i in range(3)]
            accessors.append(accessor)

        offset = len(blob)
        for index in indices:
            blob += struct.pack("<I", index)
        views.append({"buffer": 0, "byteOffset": offset,
                      "byteLength": len(blob) - offset, "target": 34963})
        accessors.append({"bufferView": len(views) - 1, "componentType": 5125,
                          "count": len(indices), "type": "SCALAR"})

        primitive = {"attributes": {"POSITION": base, "NORMAL": base + 1,
                                    "TEXCOORD_0": base + 2},
                     "indices": base + 3}
        if texture:
            if texture not in material_of:
                images.append({"uri": texture_uri(texture)})
                gl_textures.append({"source": len(images) - 1, "sampler": 0})
                materials.append({
                    "name": texture,
                    "pbrMetallicRoughness": {
                        "baseColorTexture": {"index": len(gl_textures) - 1},
                        "metallicFactor": 0.0,
                        "roughnessFactor": 0.9,
                    },
                })
                material_of[texture] = len(materials) - 1
            primitive["material"] = material_of[texture]
        primitives.append(primitive)

    if not primitives:
        return False

    doc = {
        "asset": {"version": "2.0", "generator": "ydr_to_gltf.py"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0}],
        "meshes": [{"primitives": primitives}],
        "buffers": [{
            "byteLength": len(blob),
            "uri": "data:application/octet-stream;base64," +
                   base64.b64encode(bytes(blob)).decode("ascii"),
        }],
        "bufferViews": views,
        "accessors": accessors,
    }
    if images:
        doc["images"] = images
        doc["samplers"] = [{"wrapS": 10497, "wrapT": 10497}]
        doc["textures"] = gl_textures
        doc["materials"] = materials

    with open(path, "w", encoding="utf-8") as handle:
        json.dump(doc, handle)
    return True


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--in", dest="src", required=True,
                        help="directory of extracted .ydr files")
    parser.add_argument("--out", dest="dst", required=True)
    parser.add_argument("--texture-dir", dest="textures", default=None,
                        help="PNGs from ytd_to_png.py, referenced by the "
                             "materials; without it meshes are untextured")
    parser.add_argument("--ymap-dir", dest="ymaps", default=None,
                        help="ymap XML, to name .ydd entries by hash; "
                             "without it only loose .ydr are converted")
    parser.add_argument("--limit", type=int, default=0,
                        help="convert at most this many, for a quick look")
    args = parser.parse_args()

    files, dictionaries = [], []
    for root, _, names in os.walk(args.src):
        for name in names:
            lowered = name.lower()
            if lowered.endswith(".ydr") or lowered.endswith(".yft"):
                files.append(os.path.join(root, name))
            elif lowered.endswith(".ydd"):
                dictionaries.append(os.path.join(root, name))
    files.sort()
    dictionaries.sort()
    if args.limit:
        files = files[:args.limit]
        dictionaries = dictionaries[:args.limit]
    if not files and not dictionaries:
        sys.exit("no .ydr, .yft or .ydd files under %s" % args.src)

    # Dictionary entries are keyed by hash, so a pool of real names from
    # the ymaps is what turns them back into archetypes.
    by_hash = {}
    if args.ymaps:
        for root, _, names in os.walk(args.ymaps):
            for name in names:
                if not name.lower().endswith(".xml"):
                    continue
                try:
                    tree = ElementTree.parse(os.path.join(root, name))
                except ElementTree.ParseError:
                    continue
                for node in tree.getroot().iter("archetypeName"):
                    text = (node.get("value") or node.text or "").strip()
                    if text:
                        by_hash[jenkins(text)] = text

    os.makedirs(args.dst, exist_ok=True)
    # Shaders name textures in mixed case ("IM_DT1_02_Metal_01") while
    # the dictionaries store them lowercase, so match case-insensitively
    # and keep the real filename for the URI.
    available = {}
    if args.textures and os.path.isdir(args.textures):
        for entry in os.listdir(args.textures):
            if entry.endswith(".png"):
                stem = os.path.splitext(entry)[0]
                available[stem.lower()] = stem

    def texture_uri(name):
        target = os.path.join(os.path.abspath(args.textures), name + ".png")
        try:
            uri = os.path.relpath(target, os.path.abspath(args.dst))
        except ValueError:
            # Different drives on Windows have no relative path between
            # them; an absolute one still resolves.
            uri = target
        return uri.replace("\\", "/")

    done = skipped = 0
    textured = untextured = 0
    missing = set()

    jobs = [(os.path.splitext(os.path.basename(p))[0], p, None) for p in files]

    def emit(name, parts):
        """Resolve textures for one drawable and write it out."""
        nonlocal done, skipped, textured, untextured

        # Only reference textures that exist, so no material points at a
        # missing file.
        resolved = []
        for part in parts:
            wanted = available.get(part[4].lower(), "") if part[4] else ""
            if part[4] and not wanted:
                missing.add(part[4])
            resolved.append(part[:4] + (wanted,))
        textured += sum(1 for p in resolved if p[4])
        untextured += sum(1 for p in resolved if not p[4])

        if write_gltf(os.path.join(args.dst, name + ".gltf"), resolved,
                      texture_uri):
            done += 1
        else:
            skipped += 1

    for name, path, _ in jobs:
        try:
            parts = read_drawable(path)
        except Exception:
            skipped += 1
            continue
        if parts:
            emit(name, parts)
        else:
            skipped += 1

    from_dicts = 0
    for path in dictionaries:
        try:
            entries = read_dictionary(path, by_hash)
        except Exception:
            continue
        for name, parts in entries:
            emit(name, parts)
            from_dicts += 1

    print("converted %d drawables (%d from .ydd), skipped %d -> %s"
          % (done, from_dicts, skipped, args.dst))
    print("  primitives: %d textured, %d untextured" % (textured, untextured))
    if missing:
        print("  %d texture names had no PNG (run ytd_to_png.py over more "
              "dictionaries)" % len(missing))


if __name__ == "__main__":
    main()
