"""The FS2024 airport BGL reader, against a hand-built file.

The layout mirrors what LOWI's lowi.bgl holds: one airport section, an
airport record 0x113 with a 0x5C header, then runway (0xCE), apron
(0xD0) and unrelated sub-records.
"""

import struct

import pytest

from fs2024.bgl_airport import decode_lat, decode_lon, read_airports
from fs2024.geo import LocalFrame
from fs2024.pavement import spawn_point


def encode_lon(lon):
    return round((lon + 180.0) * (3 * 2**28) / 360.0)


def encode_lat(lat):
    return round((90.0 - lat) * (2 * 2**28) / 180.0)


def runway(number, lon, lat, alt_m, length, width, heading):
    body = struct.pack("<H4B", 0, number, 0, (number + 18) % 36, 0)
    body += struct.pack("<II", 0, 0)
    body += struct.pack("<IIifff", encode_lon(lon), encode_lat(lat),
                        round(alt_m * 1000), length, width, heading)
    body += bytes(144 - 6 - len(body))
    return struct.pack("<HI", 0xCE, 144) + body


def apron(points, triangles):
    body = bytes(42) + struct.pack("<HH", len(points), len(triangles))
    for lon, lat in points:
        body += struct.pack("<II", encode_lon(lon), encode_lat(lat))
    for tri in triangles:
        body += struct.pack("<3H", *tri)
    return struct.pack("<HI", 0xD0, 6 + len(body)) + body


def airport_file(path, subs):
    header = bytearray(0x5C)
    struct.pack_into("<IIi", header, 12, encode_lon(11.35),
                     encode_lat(47.26), 576_841)
    record = header + b"".join(subs)
    struct.pack_into("<HI", record, 0, 0x113, len(record))

    data_at = 0x38 + 20 + 16
    sub_table = struct.pack("<4I", 0, 1, data_at, len(record))
    section = struct.pack("<5I", 0x03, 1, 1, 0x38 + 20, 16)
    head = bytearray(0x38)
    struct.pack_into("<I", head, 0x14, 1)
    path.write_bytes(bytes(head) + section + sub_table + bytes(record))


def test_coordinates_round_trip():
    assert decode_lon(encode_lon(11.3439)) == pytest.approx(11.3439, abs=1e-6)
    assert decode_lat(encode_lat(47.2602)) == pytest.approx(47.2602, abs=1e-6)


def test_reads_runways_and_aprons(tmp_path):
    path = tmp_path / "test.bgl"
    airport_file(path, [
        runway(26, 11.344, 47.2602, 579.7, 2002.0, 45.0, 261.0),
        struct.pack("<HI", 0x19, 10) + b"LOWI",  # a name: skipped
        apron([(11.34, 47.26), (11.35, 47.26), (11.35, 47.25)],
              [(0, 1, 2)]),
    ])
    (airport,) = read_airports(str(path), "LOWI")
    assert airport.altitude == pytest.approx(576.841)
    assert airport.lat == pytest.approx(47.26, abs=1e-6)
    (rwy,) = airport.runways
    assert (rwy.number, rwy.length, rwy.heading) == (26, 2002.0, 261.0)
    assert rwy.altitude == pytest.approx(579.7)
    (pad,) = airport.aprons
    assert pad.triangles == [(0, 1, 2)]
    assert pad.points[2] == pytest.approx((11.35, 47.25), abs=1e-6)


def test_spawn_is_at_the_threshold_facing_down_the_runway(tmp_path):
    path = tmp_path / "test.bgl"
    airport_file(path, [runway(9, 11.35, 47.26, 580.0, 2000.0, 45.0, 90.0)])
    (airport,) = read_airports(str(path), "TEST")
    frame = LocalFrame(airport.lon, airport.lat, airport.altitude)
    spawn = spawn_point(airport, frame)
    # Runway 09 lands eastward, so its threshold is the west end.
    assert spawn["x"] == pytest.approx(-960.0, abs=0.5)
    assert spawn["z"] == pytest.approx(0.0, abs=0.5)
    assert spawn["y"] == pytest.approx(580.0 - 576.841, abs=1e-3)
    assert spawn["runway"] == "09"
