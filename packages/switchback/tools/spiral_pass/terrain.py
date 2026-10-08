"""Heightfield pieces for Spiral Pass: mountain, noise and the carved road."""
import math
import random

NOISE_OCTAVES = 3
NOISE_AMPLITUDE_M = 60.0
BANK_M = 30.0


def _smooth(t):
    return t * t * (3.0 - 2.0 * t)


def make_noise(seed):
    rng = random.Random(seed)
    layers = []
    for octave in range(NOISE_OCTAVES):
        cells = 4 << octave
        lattice = [[rng.uniform(-1.0, 1.0) for _ in range(cells + 1)]
                   for _ in range(cells + 1)]
        layers.append((cells, lattice))

    def sample(u, v):
        total = 0.0
        weight = 0.0
        amp = 1.0
        for cells, lat in layers:
            x = u * cells
            y = v * cells
            x0 = min(int(x), cells - 1)
            y0 = min(int(y), cells - 1)
            fx = _smooth(x - x0)
            fy = _smooth(y - y0)
            top = lat[y0][x0] + (lat[y0][x0 + 1] - lat[y0][x0]) * fx
            low = lat[y0 + 1][x0] + (lat[y0 + 1][x0 + 1]
                                     - lat[y0 + 1][x0]) * fx
            total += amp * (top + (low - top) * fy)
            weight += amp
            amp *= 0.5
        return total / weight
    return sample


def mountain_height(r, base_radius, peak):
    t = min(r / base_radius, 1.0)
    return peak * (1.0 - t * t) ** 1.5


def road_centreline(road, outer_m, inner_m, samples):
    start = math.radians(road["startAngleDegrees"])
    sweep = road["turns"] * 2.0 * math.pi
    flat = []
    for i in range(samples + 1):
        t = i / samples
        theta = start + sweep * t
        r = outer_m + (inner_m - outer_m) * t
        flat.append((r * math.cos(theta), r * math.sin(theta)))
    along = [0.0]
    for (x0, z0), (x1, z1) in zip(flat, flat[1:]):
        along.append(along[-1] + math.hypot(x1 - x0, z1 - z0))
    total = along[-1]
    span = road["endHeight"] - road["startHeight"]
    return [(x, road["startHeight"] + span * s / total, z, s)
            for (x, z), s in zip(flat, along)]


def carve_road(heights, step_m, extent_m, points, road_half_width):
    n = len(heights)
    for x, y, z, _ in points:
        lo_i = max(0, int((x - BANK_M + extent_m) / step_m))
        hi_i = min(n - 1, int((x + BANK_M + extent_m) / step_m) + 1)
        lo_j = max(0, int((z - BANK_M + extent_m) / step_m))
        hi_j = min(n - 1, int((z + BANK_M + extent_m) / step_m) + 1)
        for j in range(lo_j, hi_j + 1):
            pz = -extent_m + j * step_m
            row = heights[j]
            for i in range(lo_i, hi_i + 1):
                px = -extent_m + i * step_m
                d = math.hypot(px - x, pz - z)
                if d >= BANK_M:
                    continue
                t = (d - road_half_width) / (BANK_M - road_half_width)
                w = 1.0 - _smooth(min(1.0, max(0.0, t)))
                row[i] += (y - row[i]) * w
