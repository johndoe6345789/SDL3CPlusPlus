#!/usr/bin/env python3
"""Turn a CodeWalker map export into the tile files this package streams.

CodeWalker is run by you, against your own installed copy of the game, to
export the map. This script does not read anything from the game itself: it
consumes what CodeWalker wrote out.

Inputs
  --ymap-dir     directory of .ymap.xml files (CodeWalker: right-click a ymap
                 in Project Explorer, "Export XML")
  --model-dir    directory of exported .gltf/.glb drawables, named after the
                 archetype they came from

Output
  --out          assets/tiles/<x>_<z>.json, one file per 512 m tile

Two conversions happen here and nowhere else at runtime:

  Axis   GTA V is Z-up right-handed, this engine is Y-up, so a position
         (x, y, z) becomes (x, z, -y).

  Rotation  A CEntityDef stores the *inverse* of the entity's orientation.
         Placing a building with the quaternion as-written mirrors every
         rotated prop on the map, which reads as "the map loaded but the
         alleyways are inside out". We conjugate it before the axis swap.
"""

import argparse
import json
import math
import os
import sys
import xml.etree.ElementTree as ET
from collections import defaultdict

TILE_SIZE = 512.0
GRID_ORIGIN = (-4000.0, -8000.0)  # engine-space (x, z) of tile (0, 0)

# lodDist thresholds, matching the rings in config/gta5_world.json.
LOD_RINGS = (("hd", 300.0), ("lod", 1000.0), ("slod1", 3000.0), ("slod2", math.inf))


def _f(node, attr="value", default=0.0):
    """CodeWalker writes scalars as <tag value="1.0" />."""
    if node is None:
        return default
    raw = node.get(attr)
    if raw is None:
        raw = (node.text or "").strip()
    try:
        return float(raw)
    except (TypeError, ValueError):
        return default


def _vec(node):
    if node is None:
        return (0.0, 0.0, 0.0)
    return (_f(node, "x"), _f(node, "y"), _f(node, "z"))


def _quat(node):
    if node is None:
        return (0.0, 0.0, 0.0, 1.0)
    return (_f(node, "x"), _f(node, "y"), _f(node, "z"), _f(node, "w"))


def to_engine_position(pos):
    """GTA Z-up -> engine Y-up."""
    x, y, z = pos
    return (x, z, -y)


def to_engine_rotation(quat):
    """Undo the stored inverse, then apply the same axis permutation.

    The axis map is a proper rotation (determinant +1), so the quaternion's
    vector part permutes exactly like a position and w is untouched.
    """
    qx, qy, qz, qw = quat
    # Conjugate: CEntityDef stores the inverse rotation.
    qx, qy, qz = -qx, -qy, -qz
    return (qx, qz, -qy, qw)


def lod_ring(lod_dist):
    for name, limit in LOD_RINGS:
        if lod_dist <= limit:
            return name
    return "slod2"


def tile_for(engine_pos):
    ex, _, ez = engine_pos
    return (
        int(math.floor((ex - GRID_ORIGIN[0]) / TILE_SIZE)),
        int(math.floor((ez - GRID_ORIGIN[1]) / TILE_SIZE)),
    )


def index_models(model_dir):
    """archetype name (lowercased) -> model path."""
    models = {}
    if not model_dir or not os.path.isdir(model_dir):
        return models
    for root, _, files in os.walk(model_dir):
        for name in files:
            stem, ext = os.path.splitext(name)
            if ext.lower() in (".gltf", ".glb"):
                models[stem.lower()] = os.path.join(root, name).replace("\\", "/")
    return models


def parse_ymap(path):
    """Yield one placement dict per CEntityDef in a CodeWalker ymap XML."""
    try:
        root = ET.parse(path).getroot()
    except ET.ParseError as exc:
        print(f"  skipped {os.path.basename(path)}: malformed XML ({exc})", file=sys.stderr)
        return

    for ent in root.iter("Item"):
        archetype = ent.find("archetypeName")
        if archetype is None:
            continue  # not an entity Item (ymaps also carry car generators etc.)
        name = (archetype.get("value") or archetype.text or "").strip()
        if not name:
            continue

        pos = to_engine_position(_vec(ent.find("position")))
        rot = to_engine_rotation(_quat(ent.find("rotation")))
        scale_xy = _f(ent.find("scaleXY"), default=1.0) or 1.0
        scale_z = _f(ent.find("scaleZ"), default=1.0) or 1.0
        lod_dist = _f(ent.find("lodDist"), default=100.0)

        yield {
            "archetype": name,
            "position": [round(v, 4) for v in pos],
            "rotation": [round(v, 6) for v in rot],
            # engine Y is GTA Z, engine X and Z are both GTA's XY plane.
            "scale": [scale_xy, scale_z, scale_xy],
            "lod": lod_ring(lod_dist),
            "lod_dist": lod_dist,
        }


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--ymap-dir", required=True)
    ap.add_argument("--model-dir", default=None)
    ap.add_argument("--out", default="packages/gta5/assets/tiles")
    args = ap.parse_args()

    if not os.path.isdir(args.ymap_dir):
        sys.exit(f"not a directory: {args.ymap_dir}")

    models = index_models(args.model_dir)
    print(f"indexed {len(models)} exported models")

    ymaps = []
    for root, _, files in os.walk(args.ymap_dir):
        ymaps += [os.path.join(root, f) for f in files if f.lower().endswith(".xml")]
    if not ymaps:
        sys.exit(f"no .xml ymaps found under {args.ymap_dir}")
    print(f"reading {len(ymaps)} ymaps")

    tiles = defaultdict(list)
    total = 0
    missing = set()
    for path in sorted(ymaps):
        for placement in parse_ymap(path):
            model = models.get(placement["archetype"].lower())
            if model is None:
                missing.add(placement["archetype"])
            placement["model"] = model
            tiles[tile_for(placement["position"])].append(placement)
            total += 1

    os.makedirs(args.out, exist_ok=True)
    for (tx, tz), placements in sorted(tiles.items()):
        out_path = os.path.join(args.out, f"{tx}_{tz}.json")
        with open(out_path, "w", encoding="utf-8") as fh:
            json.dump(
                {
                    "tile": [tx, tz],
                    "bounds_min": [GRID_ORIGIN[0] + tx * TILE_SIZE,
                                   GRID_ORIGIN[1] + tz * TILE_SIZE],
                    "tile_size": TILE_SIZE,
                    "placements": placements,
                },
                fh,
                indent=2,
            )

    print(f"wrote {len(tiles)} tiles, {total} placements -> {args.out}")
    if missing:
        print(f"{len(missing)} archetypes had no exported model; "
              f"they are written with model=null and will be skipped at load.")
        for name in sorted(missing)[:10]:
            print(f"  {name}")


if __name__ == "__main__":
    main()
