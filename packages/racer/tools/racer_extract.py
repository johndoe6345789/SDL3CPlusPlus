"""Dump every entry of the Episode I Racer lev01 blocks to disk.

    python racer_extract.py <game>/data/lev01 <out_dir>

Writes <out_dir>/<block>/<index>_<tag>.bin per entry, plus an index.json
listing each entry's offset, size and four-character tag.
"""

import json
import os
import sys

from racer_blocks import BLOCK_FILES, entry_tag, read_table


def extract_block(lev_dir, out_dir, block):
    path = os.path.join(lev_dir, BLOCK_FILES[block])
    with open(path, "rb") as handle:
        data = handle.read()
    block_dir = os.path.join(out_dir, block)
    os.makedirs(block_dir, exist_ok=True)
    index = []
    for entry, start, end in read_table(data):
        tag = entry_tag(data, start, end) or "none"
        name = f"{entry:04d}_{tag}.bin"
        with open(os.path.join(block_dir, name), "wb") as out:
            out.write(data[start:end])
        index.append({"index": entry, "offset": start,
                      "size": end - start, "tag": tag})
    with open(os.path.join(block_dir, "index.json"), "w") as out:
        json.dump(index, out, indent=1)
    return len(index)


def main(argv):
    if len(argv) != 3:
        print(__doc__)
        return 2
    lev_dir, out_dir = argv[1], argv[2]
    for block in BLOCK_FILES:
        count = extract_block(lev_dir, out_dir, block)
        print(f"{block}: {count} entries")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
