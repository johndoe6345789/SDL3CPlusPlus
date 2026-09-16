"""Read an airport out of an MSFS 2024 scenery BGL.

Only what the terrain bake needs: the airport reference point, its
runways and its apron polygons. Record ids are the ones observed in
FS2024's own LOWI package (runway 0xCE, apron 0xD0 inside airport record
0x113); anything else is skipped by size, so unknown records are safe.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass, field

SECTION_AIRPORT = 0x03
RECORD_AIRPORT = 0x113
SUB_RUNWAY = 0xCE
SUB_APRON = 0xD0
AIRPORT_HEADER_SIZE = 0x5C


def decode_lon(raw: int) -> float:
    return raw * 360.0 / (3 * 2**28) - 180.0


def decode_lat(raw: int) -> float:
    return 90.0 - raw * 180.0 / (2 * 2**28)


@dataclass
class Runway:
    number: int
    lon: float
    lat: float
    altitude: float  # metres above sea level
    length: float
    width: float
    heading: float  # degrees true


@dataclass
class Apron:
    points: list[tuple[float, float]]  # (lon, lat)
    triangles: list[tuple[int, int, int]]


@dataclass
class Airport:
    ident: str
    lon: float
    lat: float
    altitude: float
    runways: list[Runway] = field(default_factory=list)
    aprons: list[Apron] = field(default_factory=list)


def _read_runway(data: bytes, at: int) -> Runway:
    number = data[at + 8]
    lon, lat, alt, length, width, heading = struct.unpack_from(
        "<IIifff", data, at + 20)
    return Runway(number, decode_lon(lon), decode_lat(lat), alt / 1000.0,
                  length, width, heading)


def _read_apron(data: bytes, at: int) -> Apron:
    count, tris = struct.unpack_from("<HH", data, at + 48)
    raw = struct.unpack_from(f"<{count * 2}I", data, at + 52)
    points = [(decode_lon(raw[i]), decode_lat(raw[i + 1]))
              for i in range(0, len(raw), 2)]
    idx = struct.unpack_from(f"<{tris * 3}H", data, at + 52 + count * 8)
    return Apron(points, [tuple(idx[i:i + 3]) for i in range(0, len(idx), 3)])


def _read_airport(data: bytes, at: int, ident: str) -> Airport:
    size = struct.unpack_from("<I", data, at + 2)[0]
    lon, lat, alt = struct.unpack_from("<IIi", data, at + 12)
    airport = Airport(ident, decode_lon(lon), decode_lat(lat), alt / 1000.0)
    sub = at + AIRPORT_HEADER_SIZE
    while sub < at + size:
        sub_id, sub_size = struct.unpack_from("<HI", data, sub)
        if sub_size < 6:
            break
        if sub_id == SUB_RUNWAY:
            airport.runways.append(_read_runway(data, sub))
        elif sub_id == SUB_APRON:
            airport.aprons.append(_read_apron(data, sub))
        sub += sub_size
    return airport


def read_airports(path: str, ident: str) -> list[Airport]:
    """Every airport record that has runways, largest first."""
    data = open(path, "rb").read()
    found = []
    sections = struct.unpack_from("<I", data, 0x14)[0]
    for i in range(sections):
        kind, _, subs, offset, _ = struct.unpack_from(
            "<5I", data, 0x38 + i * 20)
        if kind != SECTION_AIRPORT:
            continue
        for s in range(subs):
            _, records, at, length = struct.unpack_from(
                "<4I", data, offset + s * 16)
            end = at + length
            while at < end:
                rec_id, rec_size = struct.unpack_from("<HI", data, at)
                if rec_id == RECORD_AIRPORT:
                    airport = _read_airport(data, at, ident)
                    if airport.runways:
                        found.append(airport)
                at += rec_size
    found.sort(key=lambda a: -len(a.aprons))
    return found
