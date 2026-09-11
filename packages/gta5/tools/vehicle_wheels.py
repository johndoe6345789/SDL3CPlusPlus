#!/usr/bin/env python3
"""Wheels for a GTA V vehicle.

A vehicle's .yft has no wheels in it. Where they belong it carries only
a small brake-disc hub -- for the taxi a filled disc of radius 0.17m,
one per corner, drawn with the tyre-wall material. The game instances a
wheel model chosen by the vehicle's wheelType and places one at each
axle, and those models live in their own pack, wheels_mods.rpf, as
wheel_<family>_<nn>.ydr.

So the mesh is taken from the pack and the axles come from the
fragment's physics LOD, which is where the game takes them from too.
Carving the body mesh near an axle, which is what this module used to
do, only ever cut holes in the wheel arches.
"""

import math
import struct

from rage_resource import Resource, to_engine

# Bone tags, in the order AttachGta5Wheels expects: front-right,
# front-left, rear-right, rear-left, steered pair first.
AXLE_BONES = [26418, 27922, 26398, 27902]


def face_forward(parts):
    """Turn a vehicle to face +z, which is the engine's forward.

    GTA models face +y, which the axis conversion maps to -z, so a car
    imported as-is drives backwards. A half turn about the up axis is
    (x, y, z) -> (-x, y, -z); its determinant is +1, so winding and
    normals come along unchanged.
    """
    out = []
    for positions, normals, uvs, indices, texture, trait in parts:
        out.append(([(-x, y, -z) for x, y, z in positions],
                    [(-x, y, -z) for x, y, z in normals],
                    uvs, indices, texture, trait))
    return out


def wheel_axles(path):
    """The four axle centres of a vehicle, forward-facing.

    A fragment's physics LOD holds one transform per child and one
    child per bone, so the wheel bones give the axles outright. This is
    where the game takes them from, and it is not where the mesh is:
    the brake-disc hubs sit about 0.14 m behind their own axle, which
    is enough to see.

    FragType +0xF0 is the LOD group and +0x10 of that is LOD 1. In the
    LOD, +0x30 is a position offset added to every transform, +0xD0 the
    children, +0x11D their count, and +0x100 the transform block, whose
    matrices start at +0x20 and are 64 bytes each, translation last. A
    child's bone tag is at +0x12.

    Returns [] when the file is not a vehicle fragment.
    """
    res = Resource(path)
    group = res.resolve(res.u64(0xF0))
    lod = res.resolve(res.u64(group + 0x10)) if group is not None else None
    if lod is None:
        return []
    block = res.resolve(res.u64(lod + 0x100))
    children = res.resolve(res.u64(lod + 0xD0))
    if block is None or children is None:
        return []
    offset = struct.unpack_from("<3f", res.raw, lod + 0x30)
    found = {}
    for i in range(res.u8(lod + 0x11D)):
        child = res.resolve(res.u64(children + 8 * i))
        if child is None:
            continue
        row = struct.unpack_from("<3f", res.raw, block + 0x20 + 64 * i + 48)
        found[res.u16(child + 0x12)] = to_engine(
            tuple(row[k] + offset[k] for k in range(3)))
    if not all(tag in found for tag in AXLE_BONES):
        return []
    # The vehicle itself is turned to face +z, so its axles turn too.
    return [(-found[tag][0], found[tag][1], -found[tag][2])
            for tag in AXLE_BONES]


def shape_wheel(parts, radius, width, mirror):
    """A pack wheel resized to one vehicle, and mirrored for its side.

    Pack wheels are modelled about the origin with the axle along x,
    which is what the physics wheel transform expects, and with the rim
    face towards -x, so they suit the left of the car as they stand.
    Mirroring negates x, which reverses the winding, so each triangle is
    flipped back.
    """
    radial, axial = 0.0, 0.0
    for positions, *_ in parts:
        for x, y, z in positions:
            radial = max(radial, math.hypot(y, z))
            axial = max(axial, abs(x))
    if radial <= 0.0 or axial <= 0.0:
        return []
    across = (width * 0.5 / axial) * (-1.0 if mirror else 1.0)
    around = radius / radial
    out = []
    for positions, normals, uvs, indices, texture, trait in parts:
        moved = [(x * across, y * around, z * around)
                 for x, y, z in positions]
        turned = [(-x, y, z) if mirror else (x, y, z) for x, y, z in normals]
        order = list(indices)
        if mirror:
            for t in range(0, len(order) - 2, 3):
                order[t + 1], order[t + 2] = order[t + 2], order[t + 1]
        out.append((moved, turned, uvs, order, texture, trait))
    return out
