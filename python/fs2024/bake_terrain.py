"""Bake a walkable terrain around an FS2024 airport.

    python -m python.fs2024.bake_terrain --icao LOWI \
        --dem D:/fs2024/dem/Copernicus_DSM_COG_10_N47_00_E011_00_DEM.tif \
        --out D:/fs2024/lowi

FS2024 streams its world elevation, so heights come from a public DEM
(Copernicus GLO-30); the airport itself - where it is, its elevation,
its runways and aprons - is read from the airport BGL FS2024 installs.
Outputs, all large, go to --out:

    terrain.fst   heightfield the fs2024.terrain.load step reads
    ground.jpg    colour map covering exactly the heightfield
    airport.json  the frame, runways and a spawn point, for workflows
"""

from __future__ import annotations

import argparse
import glob
import json
import os
import struct

import numpy as np
from PIL import ImageFilter

from .bgl_airport import read_airports
from .geo import DemTile, LocalFrame
from .ground_paint import cover_colours, paint_ground, pavement_mask
from .pavement import pavement_polygons, runway_info, spawn_point

MAGIC = b"FST1"


def find_bgl(icao: str) -> str:
    root = os.path.join(os.environ["APPDATA"], "Microsoft Flight Simulator"
                        " 2024", "Packages", "Official2024", "Steam")
    hits = glob.glob(os.path.join(root, "*", "scenery", "**",
                                  f"{icao.lower()}.bgl"), recursive=True)
    if not hits:
        raise SystemExit(f"no {icao.lower()}.bgl under {root}")
    return hits[0]


def flatten(heights, mask, target, cells_per_blur):
    """Level the ground under and around pavement to the runway."""
    grown = mask.filter(ImageFilter.MaxFilter(5))
    soft = grown.filter(ImageFilter.GaussianBlur(cells_per_blur))
    weight = np.clip(np.asarray(soft, np.float32) / 255.0 * 1.6, 0.0, 1.0)
    return heights + (target - heights) * weight


def write_heightfield(path, heights, spacing, origin):
    rows, cols = heights.shape
    with open(path, "wb") as out:
        out.write(MAGIC)
        out.write(struct.pack("<IIfff", cols, rows, spacing, *origin))
        out.write(heights.astype("<f4").tobytes())


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--icao", default="LOWI")
    parser.add_argument("--bgl", help="airport BGL (found if omitted)")
    parser.add_argument("--dem", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--extent", type=float, default=16000.0)
    parser.add_argument("--spacing", type=float, default=16.0)
    parser.add_argument("--texture", type=int, default=8192)
    args = parser.parse_args()

    bgl = args.bgl or find_bgl(args.icao)
    airport = read_airports(bgl, args.icao)[0]
    frame = LocalFrame(airport.lon, airport.lat, airport.altitude)
    print(f"{args.icao}: {airport.lat:.5f} {airport.lon:.5f} "
          f"{airport.altitude:.1f} m, {len(airport.runways)} runways, "
          f"{len(airport.aprons)} aprons")

    cells = int(round(args.extent / args.spacing)) + 1
    half = args.extent / 2
    axis = np.linspace(-half, half, cells, dtype=np.float64)
    xs, zs = np.meshgrid(axis, axis)
    lon, lat = frame.to_geo(xs, zs)
    absolute = DemTile(args.dem).sample(lon, lat)

    origin = (-half, -half)
    shapes = pavement_polygons(airport, frame)
    runway = max(airport.runways, key=lambda r: r.length)
    mask = pavement_mask(shapes, cells, args.extent, origin)
    absolute = flatten(absolute, mask, runway.altitude, 3.0)

    os.makedirs(args.out, exist_ok=True)
    heights = absolute - airport.altitude
    write_heightfield(os.path.join(args.out, "terrain.fst"), heights,
                      args.spacing, origin)
    colours = cover_colours(absolute, args.spacing)
    image = paint_ground(colours, args.texture, args.extent, origin, shapes)
    image.save(os.path.join(args.out, "ground.jpg"), quality=92)

    info = {"icao": args.icao, "lon": airport.lon, "lat": airport.lat,
            "altitude": airport.altitude, "extent": args.extent,
            "spacing": args.spacing, "spawn": spawn_point(airport, frame),
            "runway": runway_info(runway, frame),
            "height_range": [float(heights.min()), float(heights.max())]}
    with open(os.path.join(args.out, "airport.json"), "w") as out:
        json.dump(info, out, indent=2)
    print(json.dumps(info["spawn"]), info["height_range"])


if __name__ == "__main__":
    main()
