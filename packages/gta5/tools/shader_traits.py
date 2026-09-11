#!/usr/bin/env python3
"""What a RAGE shader says about how to draw the geometry under it.

The texture alone carries neither of the two facts that matter here. A
leaf billboard's shape lives in its alpha, so drawn opaque it is a
rectangle; and a painted car panel's texture is a few white pixels,
with the colour supplied per vehicle, so drawn as-is every car is
white.

Both facts are in the shader. In a G9 resource the shader's name hash
is at +0x00 and its render bucket at +0x39 -- and the bucket is worth
trusting over any table of names, because it is what the game itself
sorts by. Bucket 3 is the cutout bucket: a birch's trunk shader sits in
0 and every one of its leaf shaders in 3.

+0x3C holds (1 << bucket) | 0xFF00, which is a free check that the
byte was read from the right place.
"""

from rage_resource import jenkins

CUTOUT_BUCKET = 3

# Paint has no bucket of its own -- it is ordinary opaque geometry --
# so this one does need names. They are a short, closed list.
PAINT = ["vehicle_paint" + str(n) for n in range(1, 10)]
PAINT += ["vehicle_paint1_enveff", "vehicle_paint2_enveff",
          "vehicle_paint3_enveff", "vehicle_paint4_enveff",
          "vehicle_paint5_enveff", "vehicle_paint6_enveff",
          "vehicle_paint7_enveff", "vehicle_paint8_enveff",
          "vehicle_paint9_enveff", "vehicle_mesh", "vehicle_mesh_enveff"]
PAINT_HASHES = set()
for _name in PAINT:
    PAINT_HASHES.add(jenkins(_name))
    PAINT_HASHES.add(jenkins(_name + ".sps"))


def shader_trait(name_hash, bucket, mask):
    """"paint", "cutout" or "" for one shader.

    A mask that does not agree with the bucket means the read landed
    somewhere unexpected, so nothing is claimed about it.
    """
    if mask != ((1 << bucket) | 0xFF00):
        return ""
    if name_hash in PAINT_HASHES:
        return "paint"
    return "cutout" if bucket == CUTOUT_BUCKET else ""
