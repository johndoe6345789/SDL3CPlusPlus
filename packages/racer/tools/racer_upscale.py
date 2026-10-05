"""Upscale Episode I Racer TGA images with Lanczos resampling.

    python racer_upscale.py <game>/data/images <out_dir> [scale]

Writes one PNG per TGA, scaled by [scale] (default 4). Needs Pillow.
"""

import os
import sys

from PIL import Image


def upscale_folder(src_dir, out_dir, scale):
    os.makedirs(out_dir, exist_ok=True)
    done = 0
    for name in sorted(os.listdir(src_dir)):
        if not name.lower().endswith(".tga"):
            continue
        image = Image.open(os.path.join(src_dir, name)).convert("RGBA")
        size = (image.width * scale, image.height * scale)
        stem = os.path.splitext(name)[0]
        image.resize(size, Image.LANCZOS).save(
            os.path.join(out_dir, stem + ".png"))
        done += 1
    return done


def main(argv):
    if len(argv) not in (3, 4):
        print(__doc__)
        return 2
    scale = int(argv[3]) if len(argv) == 4 else 4
    done = upscale_folder(argv[1], argv[2], scale)
    print(f"upscaled {done} images by {scale}x")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
