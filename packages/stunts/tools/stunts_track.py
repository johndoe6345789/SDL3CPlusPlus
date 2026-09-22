"""Stunts .TRK track files.

A track is exactly 1802 bytes and carries no header:

    0    .. 899   30x30 road tile ids, row-major, north to south
    900  .. 1799  30x30 terrain ids over the same grid
    1800 .. 1801  two trailing bytes (horizon / scenery selector)

Tile ids are opaque, so `make_tile_table.py` derives what each id
connects to by looking at which neighbours are occupied across every
track of an install, rather than hard-coding a guessed list.
"""

GRID = 30
CELLS = GRID * GRID
TRACK_BYTES = CELLS * 2 + 2


def load(path):
    with open(path, 'rb') as handle:
        raw = handle.read()
    if len(raw) != TRACK_BYTES:
        raise ValueError('%s: expected %d bytes, got %d'
                         % (path, TRACK_BYTES, len(raw)))
    return {
        'road': raw[0:CELLS],
        'terrain': raw[CELLS:CELLS * 2],
        'horizon': raw[CELLS * 2],
        'flags': raw[CELLS * 2 + 1],
    }


def at(grid, x, y):
    return grid[y * GRID + x] if 0 <= x < GRID and 0 <= y < GRID else 0


def occupied(grid):
    """Yield (x, y, id) for every non-empty cell."""
    for y in range(GRID):
        for x in range(GRID):
            value = grid[y * GRID + x]
            if value:
                yield x, y, value
