"""Airport pavement as engine-space polygons.

Runways become rectangles (plus their painted markings), aprons the
triangles FS2024 stores them as. Everything is (x, z) in metres.
"""

from __future__ import annotations

import math

from .bgl_airport import Airport, Runway
from .geo import LocalFrame

ASPHALT = (58, 58, 60)
CONCRETE = (146, 145, 139)


def _axes(runway: Runway):
    """Unit vectors along and across the runway, in engine (x, z)."""
    h = math.radians(runway.heading)
    along = (math.sin(h), -math.cos(h))
    across = (-along[1], along[0])
    return along, across


def _box(centre, along, across, start, end, left, right):
    """Rectangle spanning [start, end] along and [left, right] across."""
    cx, cz = centre
    return [(cx + along[0] * a + across[0] * s,
             cz + along[1] * a + across[1] * s)
            for a, s in ((start, left), (end, left),
                         (end, right), (start, right))]


def runway_polygons(runway: Runway, frame: LocalFrame):
    """(colour, polygon) pairs for the runway slab.

    Only the asphalt: the markings are far finer than the ground map's
    2 m texel, so fs2024_terrain.frag draws them from runway_info().
    """
    cx, cz = frame.to_engine(runway.lon, runway.lat)
    along, across = _axes(runway)
    half_l, half_w = runway.length / 2, runway.width / 2
    return [(ASPHALT, _box((float(cx), float(cz)), along, across,
                           -half_l, half_l, -half_w, half_w))]


def runway_info(runway: Runway, frame: LocalFrame):
    """What fs2024.terrain.draw's runway_* parameters need."""
    cx, cz = frame.to_engine(runway.lon, runway.lat)
    return {"x": float(cx), "z": float(cz), "heading": runway.heading,
            "length": runway.length, "width": runway.width,
            "number": f"{runway.number:02d}"}


def apron_polygons(airport: Airport, frame: LocalFrame):
    shapes = []
    for apron in airport.aprons:
        xs, zs = frame.to_engine([p[0] for p in apron.points],
                                 [p[1] for p in apron.points])
        for tri in apron.triangles:
            if max(tri) >= len(apron.points):
                continue
            shapes.append((CONCRETE,
                           [(float(xs[i]), float(zs[i])) for i in tri]))
    return shapes


def pavement_polygons(airport: Airport, frame: LocalFrame):
    """Aprons under the runway, so runway paint wins where they overlap.

    Only the longest runway record is painted. LOWI carries two more
    0xCE records offset north of its single real runway, whose meaning
    is not yet known; drawn, they paint phantom runways over the grass.
    """
    shapes = apron_polygons(airport, frame)
    runway = max(airport.runways, key=lambda r: r.length)
    shapes.extend(runway_polygons(runway, frame))
    return shapes


def spawn_point(airport: Airport, frame: LocalFrame):
    """Just inside the threshold of the longest runway, facing along it."""
    runway = max(airport.runways, key=lambda r: r.length)
    cx, cz = frame.to_engine(runway.lon, runway.lat)
    along, _ = _axes(runway)
    back = runway.length / 2 - 40.0
    return {"x": float(cx - along[0] * back),
            "z": float(cz - along[1] * back),
            "y": runway.altitude - frame.altitude,
            "heading": runway.heading,
            "runway": f"{runway.number:02d}"}
