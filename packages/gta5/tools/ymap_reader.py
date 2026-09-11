#!/usr/bin/env python3
"""Read the entities out of a binary .ymap, without GTAUtil.

A ymap is an RSC7 resource (version 2) holding a Meta block: a list of
typed data blocks, each a run of one structure, with one of them the
root. Inside the data, pointers are not page pointers but block
references -- a 1-based block id in the low 12 bits and a byte offset
in bits 12 to 31 -- so every hop goes through the block list.

    Meta   +0x1C root block id    +0x30 data blocks    +0x4C their count
    block  +0x00 structure hash   +0x04 length         +0x08 data pointer
    CMapData   +0x08 name hash    +0x60 entities (array of pointers)
    array  +0x00 meta pointer     +0x08 count (u16)
    CEntityDef (128 bytes)
           +0x08 archetype hash   +0x20 position       +0x30 rotation
           +0x40 scaleXY          +0x44 scaleZ         +0x48 parentIndex
           +0x4C lodDist          +0x54 lodLevel

Positions and rotations come back in GTA's own frame, exactly as the
XML writes them; the axis change and the rotation conjugation belong to
whoever places them, as for the XML.

Grass, LOD lights and occlusion ymaps have no entities to speak of, and
come back empty rather than as the 40 MB of XML GTAUtil makes of grass.
"""

import struct

from rage_resource import Resource

ENTITY_SIZE = 128


class Ymap(object):
    """One ymap's name hash and its entities."""

    def __init__(self, path):
        self.res = Resource(path)
        raw = self.res.raw
        self.blocks = []
        table = self.res.resolve(self.res.u64(0x30))
        for i in range(self.res.u16(0x4C) if table is not None else 0):
            _, length, pointer = struct.unpack_from("<IiQ", raw, table + 16 * i)
            self.blocks.append((self.res.resolve(pointer), length))
        root = self.at(self.res.u32(0x1C), 0)
        self.name_hash = self.res.u32(root + 0x08) if root is not None else 0
        self.entities = self._entities(root) if root is not None else []

    def at(self, block_id, offset):
        """Flat offset of a Meta block reference, or None."""
        if not 0 < block_id <= len(self.blocks):
            return None
        base, length = self.blocks[block_id - 1]
        if base is None or offset >= length:
            return None
        return base + offset

    def deref(self, pointer):
        return self.at(pointer & 0xFFF, (pointer >> 12) & 0xFFFFF)

    def _entities(self, root):
        array = self.deref(self.res.u64(root + 0x60))
        count = self.res.u16(root + 0x68)
        out = []
        for i in range(count if array is not None else 0):
            entity = self.deref(self.res.u64(array + 8 * i))
            if entity is not None:
                out.append(read_entity(self.res.raw, entity))
        return out


def read_entity(raw, off):
    """One CEntityDef as a plain dict, in GTA's frame."""
    hash_, = struct.unpack_from("<I", raw, off + 0x08)
    pos = struct.unpack_from("<3f", raw, off + 0x20)
    rot = struct.unpack_from("<4f", raw, off + 0x30)
    sxy, sz, parent, lod_dist = struct.unpack_from("<ffif", raw, off + 0x40)
    lod_level, = struct.unpack_from("<I", raw, off + 0x54)
    return {"archetype": hash_, "position": pos, "rotation": rot,
            "scaleXY": sxy, "scaleZ": sz, "parentIndex": parent,
            "lodDist": lod_dist, "lodLevel": lod_level}
