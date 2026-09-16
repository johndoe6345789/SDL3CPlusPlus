"""The ground colour map: land cover guessed from height and slope.

There is no local land-cover data (FS2024 streams it), so this is a
stand-in that reads right for an Alpine valley: meadow on the floor,
forest on the flanks, rock on steep ground and snow up high. Pavement
from the airport BGL is painted over it at full resolution.
"""

from __future__ import annotations

import numpy as np
from PIL import Image, ImageDraw

MEADOW = np.array([92, 110, 56], np.float32)
FOREST = np.array([44, 62, 36], np.float32)
ALPINE = np.array([112, 112, 74], np.float32)
ROCK = np.array([118, 114, 106], np.float32)
SNOW = np.array([232, 236, 240], np.float32)


def _ramp(value, low, high):
    t = np.clip((value - low) / (high - low), 0.0, 1.0)
    return (t * t * (3 - 2 * t))[..., None]


def _box_blur(values, radius, passes=2):
    """Separable box blur; two passes approximate a Gaussian."""
    kernel = np.ones(2 * radius + 1, np.float32) / (2 * radius + 1)
    for _ in range(passes):
        for axis in (0, 1):
            padded = np.pad(values, [(radius, radius) if a == axis
                                     else (0, 0) for a in (0, 1)],
                            mode="edge")
            values = np.apply_along_axis(
                np.convolve, axis, padded, kernel, mode="valid")
    return values


def _slope_degrees(heights, spacing):
    # Smoothed first: the DSM's metre-scale noise (roofs, tree crowns)
    # otherwise speckles the cover classes into contour-like stripes.
    heights = _box_blur(heights, 2)
    dz, dx = np.gradient(heights, spacing)
    return np.degrees(np.arctan(np.hypot(dx, dz)))


def cover_colours(altitude, spacing):
    """RGB per grid sample from absolute altitude (metres)."""
    slope = _slope_degrees(altitude, spacing)
    colour = MEADOW + (FOREST - MEADOW) * _ramp(slope, 6.0, 14.0)
    colour += (ALPINE - colour) * _ramp(altitude, 1700.0, 2000.0)
    colour += (ROCK - colour) * _ramp(slope, 34.0, 44.0)
    colour += (ROCK - colour) * _ramp(altitude, 2250.0, 2450.0)
    snow = _ramp(altitude, 2500.0, 2700.0) * (1 - _ramp(slope, 38.0, 50.0))
    colour += (SNOW - colour) * snow
    return colour


def _noise(size, rng):
    """Multi-octave value noise in [-1, 1], one channel."""
    total = np.zeros((size, size), np.float32)
    weight = 0.0
    for cells, amp in ((16, 1.0), (64, 0.6), (256, 0.35), (1024, 0.25)):
        grid = rng.standard_normal((cells, cells)).astype(np.float32)
        img = Image.fromarray(grid, mode="F").resize(
            (size, size), Image.BICUBIC)
        total += np.asarray(img) * amp
        weight += amp
    return total / weight


def paint_ground(colours, size, extent, origin, shapes, seed=7):
    """Upsample the cover colours, break them up, draw the pavement."""
    rng = np.random.default_rng(seed)
    noise = _noise(size, rng)
    bands = []
    for band in range(3):
        channel = Image.fromarray(colours[..., band], mode="F").resize(
            (size, size), Image.BICUBIC)
        value = np.asarray(channel) * (1.0 + 0.22 * noise)
        bands.append(Image.fromarray(
            np.clip(value, 0, 255).astype(np.uint8), mode="L"))
    image = Image.merge("RGB", bands)
    draw = ImageDraw.Draw(image)
    scale = size / extent
    for colour, polygon in shapes:
        points = [((x - origin[0]) * scale, (z - origin[1]) * scale)
                  for x, z in polygon]
        draw.polygon(points, fill=colour)
    return image


def pavement_mask(shapes, cells, extent, origin):
    """1 where any pavement covers a grid cell, at grid resolution."""
    mask = Image.new("L", (cells, cells), 0)
    draw = ImageDraw.Draw(mask)
    scale = (cells - 1) / extent
    for _, polygon in shapes:
        draw.polygon([((x - origin[0]) * scale, (z - origin[1]) * scale)
                      for x, z in polygon], fill=255)
    return mask
