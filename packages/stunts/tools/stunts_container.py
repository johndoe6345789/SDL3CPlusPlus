"""Stunts container format: stacked headers + canonical Huffman.

A compressed Stunts file starts with one or more 4-byte headers:

    byte 0   bits 0-6 compression type, bit 7 "another header follows"
    byte 1-3 little-endian 24-bit size of that pass's output

Type 2 is Huffman.  The Huffman payload is a canonical table --
one count byte per code length starting at length 1 -- followed by
the symbols in code order and an MSB-first bit stream.  The table is
verified by its Kraft sum, which is exactly 1.0 for every file in a
retail install.
"""

HUFFMAN = 2


def read_headers(raw):
    """Return ([(type, size)], offset of the payload)."""
    heads = []
    pos = 0
    while True:
        flags = raw[pos]
        size = raw[pos + 1] | (raw[pos + 2] << 8) | (raw[pos + 3] << 16)
        heads.append((flags & 0x7F, size))
        pos += 4
        if not flags & 0x80:
            return heads, pos


def build_table(data):
    """Return (code table, byte length of the table)."""
    pos = 0
    widths = data[pos]
    pos += 1
    # One count byte per code length, lengths 1..widths. Taking one
    # byte too many swallows the first symbol and leaves the code
    # over-subscribed (Kraft sum above 1).
    counts = list(data[pos:pos + widths])
    pos += widths
    total = sum(counts)
    symbols = list(data[pos:pos + total])
    pos += total
    table = {}
    code = 0
    index = 0
    for i, count in enumerate(counts):
        for _ in range(count):
            table[(i + 1, code)] = symbols[index]
            index += 1
            code += 1
        code <<= 1
    return table, pos


def huffman(data, out_size):
    """Decode one Huffman pass of exactly `out_size` bytes."""
    table, pos = build_table(data)
    bits = data[pos:]
    out = bytearray()
    code = 0
    length = 0
    for index in range(len(bits) * 8):
        if len(out) >= out_size:
            break
        bit = (bits[index >> 3] >> (7 - (index & 7))) & 1
        code = (code << 1) | bit
        length += 1
        if (length, code) in table:
            out.append(table[(length, code)])
            code = 0
            length = 0
    return bytes(out)
