#!/usr/bin/env python3
"""Export the seed demo room as a glTF/GLB file.

Reads the seed_game.json workflow and converts all physics bodies
(boxes) into a glTF scene with meshes. The output can be opened
in Blender, edited, and re-exported for the engine to load.

Usage:
    python export_room_gltf.py [--output packages/seed/map.glb]
"""

import argparse
import json
import struct
import math
from pathlib import Path


def create_box_mesh(sx, sy, sz):
    """Create a unit box mesh scaled by sx, sy, sz. Returns (vertices, indices).
    Vertex format: position(3) + normal(3) + uv(2) = 8 floats per vertex.
    """
    hx, hy, hz = sx / 2, sy / 2, sz / 2

    # 6 faces, 4 vertices each = 24 vertices
    # Each face has its own normal for flat shading
    faces = [
        # front (+Z)
        (( hx,  hy,  hz), (-hx,  hy,  hz), (-hx, -hy,  hz), ( hx, -hy,  hz), ( 0,  0,  1)),
        # back (-Z)
        ((-hx,  hy, -hz), ( hx,  hy, -hz), ( hx, -hy, -hz), (-hx, -hy, -hz), ( 0,  0, -1)),
        # right (+X)
        (( hx,  hy, -hz), ( hx,  hy,  hz), ( hx, -hy,  hz), ( hx, -hy, -hz), ( 1,  0,  0)),
        # left (-X)
        ((-hx,  hy,  hz), (-hx,  hy, -hz), (-hx, -hy, -hz), (-hx, -hy,  hz), (-1,  0,  0)),
        # top (+Y)
        (( hx,  hy, -hz), (-hx,  hy, -hz), (-hx,  hy,  hz), ( hx,  hy,  hz), ( 0,  1,  0)),
        # bottom (-Y)
        (( hx, -hy,  hz), (-hx, -hy,  hz), (-hx, -hy, -hz), ( hx, -hy, -hz), ( 0, -1,  0)),
    ]

    vertices = []
    indices = []
    uvs_face = [(1, 0), (0, 0), (0, 1), (1, 1)]

    for face_idx, (v0, v1, v2, v3, n) in enumerate(faces):
        base = len(vertices)
        for vi, (vx, vy, vz) in enumerate([v0, v1, v2, v3]):
            u, v = uvs_face[vi]
            # Scale UVs by face dimensions for tiling
            if abs(n[0]) > 0.5:  # X-facing
                u *= sz
                v *= sy
            elif abs(n[1]) > 0.5:  # Y-facing
                u *= sx
                v *= sz
            else:  # Z-facing
                u *= sx
                v *= sy
            vertices.append((vx, vy, vz, n[0], n[1], n[2], u, v))

        indices.extend([base, base + 1, base + 2, base, base + 2, base + 3])

    return vertices, indices


def build_gltf(bodies, output_path):
    """Build a GLB file from a list of physics bodies."""
    import io

    nodes = []
    meshes = []
    accessors = []
    buffer_views = []
    bin_data = io.BytesIO()

    for body in bodies:
        name = body["name"]
        px = body.get("pos_x", 0)
        py = body.get("pos_y", 0)
        pz = body.get("pos_z", 0)
        sx = body.get("size_x", 1)
        sy = body.get("size_y", 1)
        sz = body.get("size_z", 1)
        shape = body.get("shape", "box")

        if shape == "capsule":
            continue  # Skip player

        verts, idxs = create_box_mesh(sx, sy, sz)

        # Write index data (uint16)
        idx_offset = bin_data.tell()
        for i in idxs:
            bin_data.write(struct.pack("<H", i))
        idx_size = bin_data.tell() - idx_offset
        # Pad to 4-byte boundary
        while bin_data.tell() % 4 != 0:
            bin_data.write(b'\x00')

        # Write vertex data (float32 x 8 per vertex)
        vtx_offset = bin_data.tell()
        pos_min = [1e9, 1e9, 1e9]
        pos_max = [-1e9, -1e9, -1e9]
        for v in verts:
            bin_data.write(struct.pack("<8f", *v))
            for j in range(3):
                pos_min[j] = min(pos_min[j], v[j])
                pos_max[j] = max(pos_max[j], v[j])
        vtx_size = bin_data.tell() - vtx_offset

        # Buffer views
        idx_bv = len(buffer_views)
        buffer_views.append({
            "buffer": 0, "byteOffset": idx_offset,
            "byteLength": idx_size, "target": 34963  # ELEMENT_ARRAY_BUFFER
        })
        vtx_bv = len(buffer_views)
        buffer_views.append({
            "buffer": 0, "byteOffset": vtx_offset,
            "byteLength": vtx_size, "byteStride": 32,
            "target": 34962  # ARRAY_BUFFER
        })

        # Accessors
        idx_acc = len(accessors)
        accessors.append({
            "bufferView": idx_bv, "componentType": 5123,  # UNSIGNED_SHORT
            "count": len(idxs), "type": "SCALAR"
        })
        pos_acc = len(accessors)
        accessors.append({
            "bufferView": vtx_bv, "byteOffset": 0,
            "componentType": 5126, "count": len(verts),  # FLOAT
            "type": "VEC3", "min": pos_min, "max": pos_max
        })
        nrm_acc = len(accessors)
        accessors.append({
            "bufferView": vtx_bv, "byteOffset": 12,
            "componentType": 5126, "count": len(verts),
            "type": "VEC3"
        })
        uv_acc = len(accessors)
        accessors.append({
            "bufferView": vtx_bv, "byteOffset": 24,
            "componentType": 5126, "count": len(verts),
            "type": "VEC2"
        })

        # Mesh
        mesh_idx = len(meshes)
        meshes.append({
            "name": name,
            "primitives": [{
                "attributes": {
                    "POSITION": pos_acc,
                    "NORMAL": nrm_acc,
                    "TEXCOORD_0": uv_acc
                },
                "indices": idx_acc
            }]
        })

        # Node with translation
        nodes.append({
            "name": name,
            "mesh": mesh_idx,
            "translation": [px, py, pz]
        })

    # Build glTF JSON
    gltf = {
        "asset": {"version": "2.0", "generator": "metabuilder-export"},
        "scene": 0,
        "scenes": [{"name": "SeedDemo", "nodes": list(range(len(nodes)))}],
        "nodes": nodes,
        "meshes": meshes,
        "accessors": accessors,
        "bufferViews": buffer_views,
        "buffers": [{"byteLength": bin_data.tell()}]
    }

    # Write GLB
    gltf_json = json.dumps(gltf, separators=(',', ':'))
    # Pad JSON to 4-byte boundary
    while len(gltf_json) % 4 != 0:
        gltf_json += ' '

    bin_bytes = bin_data.getvalue()
    while len(bin_bytes) % 4 != 0:
        bin_bytes += b'\x00'

    total_size = 12 + 8 + len(gltf_json) + 8 + len(bin_bytes)

    with open(output_path, 'wb') as f:
        # GLB header
        f.write(struct.pack("<III", 0x46546C67, 2, total_size))
        # JSON chunk
        f.write(struct.pack("<II", len(gltf_json), 0x4E4F534A))
        f.write(gltf_json.encode('ascii'))
        # BIN chunk
        f.write(struct.pack("<II", len(bin_bytes), 0x004E4942))
        f.write(bin_bytes)

    print(f"Exported {len(nodes)} nodes to {output_path} ({total_size} bytes)")


def main():
    parser = argparse.ArgumentParser(description="Export seed room as glTF/GLB")
    parser.add_argument("--workflow", default="packages/seed/workflows/seed_game.json")
    parser.add_argument("--output", default="packages/seed/map.glb")
    args = parser.parse_args()

    with open(args.workflow) as f:
        workflow = json.load(f)

    # Extract static level geometry only (mass=0, not player/dynamic)
    skip_keywords = {"player", "cube", "crate"}
    bodies = []
    for node in workflow.get("nodes", []):
        if node.get("type") == "physics.body.add":
            params = node.get("parameters", {})
            name = params.get("name", "")
            mass = params.get("mass", 0)
            # Only static bodies (mass=0), skip capsules and dynamic objects
            if mass != 0 or params.get("shape") == "capsule":
                continue
            if any(kw in name for kw in skip_keywords):
                continue
            if params.get("spinning", 0):
                continue
            bodies.append(params)

    build_gltf(bodies, args.output)


if __name__ == "__main__":
    main()
