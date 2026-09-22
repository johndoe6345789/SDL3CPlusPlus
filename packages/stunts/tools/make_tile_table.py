"""Derive the road-tile connection table from an install's tracks.

Stunts stores only an opaque id per cell, so this walks every .TRK in
a game directory and records, for each id, how often each of its four
neighbours is occupied.  An id whose road runs north-south has a
northern and a southern neighbour in essentially every placement,
which is what identifies it.  Ids that reach the threshold in two
opposite directions are straights, in two adjacent directions corners,
and in three or more junctions; anything else is scenery.

88% first looked like a safe margin, but it turned out to exclude
real, heavily-used junction pieces: a busy crossroads does not carry
traffic through every one of its four exits in every placement, so
its own occupancy sits in the 70-90% band rather than near 100% the
way a plain straight's does. 88% dropped those tiles as "scenery",
which showed up in-game as literal gaps in an otherwise continuous
road. 70%, checked against every track in an install, recovers 96% of
all road cells (up from 91%) without pulling in tiles whose occupancy
is genuinely uniform-but-low across all four sides, which is what
actual roadside scenery looks like in this data.

    python make_tile_table.py <stunts dir> <out.json>
"""

import collections
import glob
import json
import os
import sys

import stunts_track as trk

DIRS = (('N', 0, -1), ('E', 1, 0), ('S', 0, 1), ('W', -1, 0))
THRESHOLD = 70
MIN_SAMPLES = 8


def tally(paths):
    seen = collections.Counter()
    near = collections.defaultdict(collections.Counter)
    for path in paths:
        road = trk.load(path)['road']
        for x, y, value in trk.occupied(road):
            seen[value] += 1
            for name, dx, dy in DIRS:
                if trk.at(road, x + dx, y + dy):
                    near[value][name] += 1
    return seen, near


def classify(seen, near):
    tiles = {}
    for value, count in seen.items():
        if count < MIN_SAMPLES:
            continue
        links = [name for name, _, _ in DIRS
                 if 100 * near[value][name] // count >= THRESHOLD]
        if len(links) >= 3:
            kind = 'junction'
        elif len(links) == 2:
            kind = ('straight' if set(links) in ({'N', 'S'}, {'E', 'W'})
                    else 'corner')
        else:
            continue
        tiles['%d' % value] = {'kind': kind, 'links': ''.join(links),
                               'samples': count}
    return tiles


def main(argv):
    paths = sorted(glob.glob(os.path.join(argv[1], '*.[Tt][Rr][Kk]')))
    if not paths:
        raise SystemExit('no .TRK files in %s' % argv[1])
    seen, near = tally(paths)
    tiles = classify(seen, near)
    table = {'comment': 'Derived from %d tracks by make_tile_table.py'
                        % len(paths),
             'grid': trk.GRID, 'tiles': tiles}
    with open(argv[2], 'w') as handle:
        json.dump(table, handle, indent=2, sort_keys=True)
    print('%d road tiles from %d tracks -> %s'
          % (len(tiles), len(paths), argv[2]))


if __name__ == '__main__':
    main(sys.argv)
