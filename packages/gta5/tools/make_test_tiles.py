#!/usr/bin/env python3
"""Generate a stand-in city so the streamer can be exercised without game data.

Writes valid glTF 2.0 box meshes plus the tile files that place them, into
the same assets/tiles layout that import_codewalker_export.py produces. The
point is to test streaming, LOD banding and eviction across tile
boundaries, so the layout deliberately spans several 512 m tiles.

    python packages/gta5/tools/make_test_tiles.py

Delete packages/gta5/assets once you have a real export; nothing here is
meant to survive it.
"""

import argparse
import base64
import json
import os
import struct

TILE_SIZE = 512.0
GRID_ORIGIN = (-4000.0, -8000.0)
# Legion Square, matching scene/gta5_map.json's spawn.
SPAWN = (195.0, 30.7, 934.0)


def box_mesh(width, height, depth):
    """A box as 24 vertices (4 per face, so each face gets clean UVs)."""
    hx, hz = width / 2.0, depth / 2.0
    # (origin at the base, so a placement's Y is ground level)
    y0, y1 = 0.0, height
    faces = [
        ([(-hx, y0, hz), (hx, y0, hz), (hx, y1, hz), (-hx, y1, hz)]),   # +Z
        ([(hx, y0, -hz), (-hx, y0, -hz), (-hx, y1, -hz), (hx, y1, -hz)]),  # -Z
        ([(hx, y0, hz), (hx, y0, -hz), (hx, y1, -hz), (hx, y1, hz)]),   # +X
        ([(-hx, y0, -hz), (-hx, y0, hz), (-hx, y1, hz), (-hx, y1, -hz)]),  # -X
        ([(-hx, y1, hz), (hx, y1, hz), (hx, y1, -hz), (-hx, y1, -hz)]),  # +Y
        ([(-hx, y0, -hz), (hx, y0, -hz), (hx, y0, hz), (-hx, y0, hz)]),  # -Y
    ]
    positions, uvs, indices = [], [], []
    for face in faces:
        base = len(positions)
        positions.extend(face)
        # Tile the texture by size so big walls do not get a stretched brick.
        us = max(1.0, width / 4.0)
        vs = max(1.0, height / 4.0)
        uvs.extend([(0.0, 0.0), (us, 0.0), (us, vs), (0.0, vs)])
        indices.extend([base, base + 1, base + 2, base, base + 2, base + 3])
    return positions, uvs, indices


def write_gltf(path, positions, uvs, indices):
    pos_blob = b"".join(struct.pack("<3f", *p) for p in positions)
    uv_blob = b"".join(struct.pack("<2f", *u) for u in uvs)
    idx_blob = b"".join(struct.pack("<H", i) for i in indices)
    # Each bufferView must start on a 4-byte boundary.
    pad = (4 - len(idx_blob) % 4) % 4
    blob = pos_blob + uv_blob + idx_blob + b"\x00" * pad

    mins = [min(p[i] for p in positions) for i in range(3)]
    maxs = [max(p[i] for p in positions) for i in range(3)]
    doc = {
        "asset": {"version": "2.0", "generator": "make_test_tiles.py"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0}],
        "meshes": [{"primitives": [{
            "attributes": {"POSITION": 0, "TEXCOORD_0": 1},
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
             "byteLength": len(uv_blob), "target": 34962},
            {"buffer": 0, "byteOffset": len(pos_blob) + len(uv_blob),
             "byteLength": len(idx_blob), "target": 34963},
        ],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": len(positions),
             "type": "VEC3", "min": mins, "max": maxs},
            {"bufferView": 1, "componentType": 5126, "count": len(uvs),
             "type": "VEC2"},
            {"bufferView": 2, "componentType": 5123, "count": len(indices),
             "type": "SCALAR"},
        ],
    }
    with open(path, "w", encoding="utf-8") as fh:
        json.dump(doc, fh)


# Block pitch. The road slab is exactly this wide so neighbouring slabs
# meet edge to edge: anything smaller leaves strips of missing floor.
SPACING = 128.0

# name -> (width, height, depth, lod band)
ARCHETYPES = {
    "test_road":     (SPACING, 0.4, SPACING, "slod2"),
    "test_block":    (28.0, 14.0, 28.0, "hd"),
    "test_tower_a":  (22.0, 55.0, 22.0, "lod"),
    "test_tower_b":  (18.0, 90.0, 18.0, "slod1"),
}


def tile_for(x, z):
    return (int((x - GRID_ORIGIN[0]) // TILE_SIZE),
            int((z - GRID_ORIGIN[1]) // TILE_SIZE))


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", default="packages/gta5/assets")
    ap.add_argument("--blocks", type=int, default=11,
                    help="blocks per side (default 11, about 1.4 km)")
    args = ap.parse_args()

    model_dir = os.path.join(args.out, "models")
    tile_dir = os.path.join(args.out, "tiles")
    os.makedirs(model_dir, exist_ok=True)
    os.makedirs(tile_dir, exist_ok=True)

    models = {}
    for name, (w, h, d, _) in ARCHETYPES.items():
        path = os.path.join(model_dir, name + ".gltf").replace("\\", "/")
        write_gltf(path, *box_mesh(w, h, d))
        models[name] = path
    print("wrote %d models -> %s" % (len(models), model_dir))

    half = args.blocks // 2
    tiles = {}
    for bx in range(-half, half + 1):
        for bz in range(-half, half + 1):
            x = SPAWN[0] + bx * SPACING
            z = SPAWN[2] + bz * SPACING
            # A road slab under every block, then something on top of it.
            entries = [("test_road", x, SPAWN[1], z)]
            ring = max(abs(bx), abs(bz))
            if ring == 0:
                pass  # leave the spawn block clear so you land outdoors
            elif ring <= 2:
                entries.append(("test_tower_b", x, SPAWN[1], z))
            elif ring <= 4:
                entries.append(("test_tower_a", x, SPAWN[1], z))
            else:
                entries.append(("test_block", x, SPAWN[1], z))

            for name, px, py, pz in entries:
                tiles.setdefault(tile_for(px, pz), []).append({
                    "archetype": name,
                    "model": models[name],
                    "position": [round(px, 3), round(py, 3), round(pz, 3)],
                    "rotation": [0.0, 0.0, 0.0, 1.0],
                    "scale": [1.0, 1.0, 1.0],
                    "lod": ARCHETYPES[name][3],
                })

    for (tx, tz), placements in sorted(tiles.items()):
        with open(os.path.join(tile_dir, "%d_%d.json" % (tx, tz)), "w",
                  encoding="utf-8") as fh:
            json.dump({
                "tile": [tx, tz],
                "bounds_min": [GRID_ORIGIN[0] + tx * TILE_SIZE,
                               GRID_ORIGIN[1] + tz * TILE_SIZE],
                "tile_size": TILE_SIZE,
                "placements": placements,
            }, fh, indent=2)

    total = sum(len(p) for p in tiles.values())
    print("wrote %d tiles, %d placements -> %s" % (len(tiles), total, tile_dir))


if __name__ == "__main__":
    main()
