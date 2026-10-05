"""Run the Episode I Racer asset pipeline in one go.

    python racer_prototype.py <game>/data <out_dir>

Writes, under <out_dir>:
    blocks/       every lev01 block entry, with index.json per block
    textures/     decoded 64x64 textures as PNG
    images_x4/    UI TGAs upscaled 4x as PNG
    wavs_44k/     WAVs resampled to 44.1 kHz
Needs Pillow. Read-only on the game install.
"""

import os
import sys

from racer_extract import main as extract_main
from racer_resample import main as resample_main
from racer_textures import decode_block
from racer_upscale import upscale_folder


def run(data_dir, out_dir):
    lev_dir = os.path.join(data_dir, "lev01")
    extract_main(["racer_extract", lev_dir, os.path.join(out_dir, "blocks")])
    count = decode_block(lev_dir, os.path.join(out_dir, "textures"))
    print(f"decoded {count} textures")
    images = upscale_folder(os.path.join(data_dir, "images"),
                            os.path.join(out_dir, "images_x4"), 4)
    print(f"upscaled {images} images")
    resample_main(["racer_resample", os.path.join(data_dir, "wavs"),
                   os.path.join(out_dir, "wavs_44k")])
    return 0


def main(argv):
    if len(argv) != 3:
        print(__doc__)
        return 2
    return run(argv[1], argv[2])


if __name__ == "__main__":
    sys.exit(main(sys.argv))
