"""Builds Spiral Pass's heightfield and map data from assets/spiral_pass.json.

Run from anywhere: python packages/switchback/tools/spiral_pass/generate.py
Writes assets/spiral_pass_heightmap.r16 and assets/spiral_pass_map.json.
Output is generated from the spec only; it contains no GTA V data.
"""
import bisect
import json
import math
import sys
from array import array
from pathlib import Path

import terrain

ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "assets"
GRID = 1025
HEIGHT_MAX_M = 512.0
ROAD_OUTER_M = 765.0
ROAD_INNER_M = 150.0
ROAD_SAMPLES = 2000


def build_heights(mountain, step, road_points, road):
    base = mountain["baseRadius"]
    noise = terrain.make_noise(mountain["seed"])
    heights = []
    for j in range(GRID):
        z = -base + j * step
        row = []
        for i in range(GRID):
            x = -base + i * step
            h = terrain.mountain_height(math.hypot(x, z), base,
                                        mountain["peakHeight"])
            h += (terrain.NOISE_AMPLITUDE_M * mountain["roughness"]
                  * noise(i / (GRID - 1), j / (GRID - 1)))
            row.append(max(0.0, h))
        heights.append(row)
    terrain.carve_road(heights, step, base, road_points,
                       road["width"] / 2.0)
    return heights


def max_grade_percent(points):
    worst = 0.0
    for (_, y0, _, s0), (_, y1, _, s1) in zip(points, points[1:]):
        if s1 > s0:
            worst = max(worst, abs(y1 - y0) / (s1 - s0) * 100.0)
    return worst


def checkpoints_on(points, count):
    along = [p[3] for p in points]
    total = along[-1]
    result = []
    for k in range(count):
        target = total * k / (count - 1)
        i = min(bisect.bisect_left(along, target), len(points) - 1)
        i = max(i, 1)
        (x0, y0, z0, s0), (x1, y1, z1, s1) = points[i - 1], points[i]
        f = 0.0 if s1 == s0 else (target - s0) / (s1 - s0)
        result.append({"index": k,
                       "x": x0 + (x1 - x0) * f,
                       "y": y0 + (y1 - y0) * f,
                       "z": z0 + (z1 - z0) * f,
                       "distance_m": target})
    return result


def write_heightmap(heights, path):
    values = array("H")
    for row in heights:
        for h in row:
            clamped = min(HEIGHT_MAX_M, max(0.0, h))
            values.append(round(clamped / HEIGHT_MAX_M * 65535))
    if sys.byteorder == "big":
        values.byteswap()
    path.write_bytes(values.tobytes())


def main():
    spec = json.loads((ASSETS / "spiral_pass.json").read_text("utf-8"))
    mountain = spec["mountain"]
    road = spec["road"]
    if mountain["centre"] != [0.0, 0.0]:
        raise ValueError("Spiral Pass assumes the mountain is centred at 0,0")
    step = 2.0 * mountain["baseRadius"] / (GRID - 1)
    points = terrain.road_centreline(road, ROAD_OUTER_M, ROAD_INNER_M,
                                     ROAD_SAMPLES)
    grade = max_grade_percent(points)
    if grade > road["maxGradePercent"]:
        raise ValueError(f"road grade {grade:.1f}% exceeds "
                         f"{road['maxGradePercent']}%")
    heights = build_heights(mountain, step, points, road)
    write_heightmap(heights, ASSETS / "spiral_pass_heightmap.r16")
    map_data = {
        "id": spec["id"],
        "grid": {"size": GRID, "step_m": step,
                 "extent_m": mountain["baseRadius"],
                 "height_max_m": HEIGHT_MAX_M,
                 "format": "uint16 little-endian, row-major from z=-extent,"
                           " height = value / 65535 * height_max_m"},
        "road": {"length_m": points[-1][3],
                 "max_grade_percent": grade,
                 "centreline": [[x, y, z] for x, y, z, _ in points[::20]]},
        "checkpoints": checkpoints_on(points, spec["checkpoints"]["count"]),
        "hazards": spec["hazards"],
        "spawn": spec["spawn"],
    }
    (ASSETS / "spiral_pass_map.json").write_text(
        json.dumps(map_data, indent=2), "utf-8")
    print(f"road {points[-1][3]:.0f} m, max grade {grade:.1f}%, "
          f"end height {points[-1][1]:.1f} m")


if __name__ == "__main__":
    main()
