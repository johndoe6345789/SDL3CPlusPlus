"""Decode Episode I Racer indexed textures to PNG.

    python racer_textures.py <game>/data/lev01 <out_dir>

Each 2048-byte texture (64x64, 4 bits per pixel) sits next to a 32-byte
palette (16 big-endian RGB565 colours) in the texture block. The palette
is taken from the nearest 32-byte entry before the texture in file order.
Needs Pillow.
"""

import os
import sys

from PIL import Image

from racer_blocks import BLOCK_FILES, read_table

TEXTURE_BYTES = 2048
PALETTE_BYTES = 32


def rgb565_palette(raw):
    colours = []
    for i in range(0, PALETTE_BYTES, 2):
        value = (raw[i] << 8) | raw[i + 1]
        colours.append(((value >> 11) * 255 // 31,
                        ((value >> 5) & 63) * 255 // 63,
                        (value & 31) * 255 // 31))
    return colours


def decode_texture(raw, palette):
    pixels = []
    for byte in raw:
        pixels.append(palette[byte >> 4])
        pixels.append(palette[byte & 15])
    image = Image.new("RGB", (64, 64))
    image.putdata(pixels)
    return image


def decode_block(lev_dir, out_dir):
    path = os.path.join(lev_dir, BLOCK_FILES["texture"])
    with open(path, "rb") as handle:
        data = handle.read()
    spans = sorted({(s, e) for _, s, e in read_table(data)})
    palette = None
    done = 0
    os.makedirs(out_dir, exist_ok=True)
    for index, (start, end) in enumerate(spans):
        size = end - start
        if size == PALETTE_BYTES:
            palette = rgb565_palette(data[start:end])
        elif size == TEXTURE_BYTES and palette is not None:
            image = decode_texture(data[start:end], palette)
            image.save(os.path.join(out_dir, f"tex_{index:04d}.png"))
            done += 1
    return done


def main(argv):
    if len(argv) != 3:
        print(__doc__)
        return 2
    count = decode_block(argv[1], argv[2])
    print(f"decoded {count} textures")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
