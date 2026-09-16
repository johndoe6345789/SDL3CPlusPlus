"""Local metric frame around an airport, and elevation sampling.

Engine space is Y-up with x = east and z = south (so -z is north), in
metres, with the origin on the airport reference point at its elevation.
Over the ~20 km a bake covers an equirectangular tangent frame is good
to well under a metre, which is far finer than the 30 m source data.
"""

from __future__ import annotations

import math

import numpy as np

EARTH_RADIUS = 6371008.8


class LocalFrame:
    def __init__(self, lon: float, lat: float, altitude: float):
        self.lon = lon
        self.lat = lat
        self.altitude = altitude
        self._per_deg = math.radians(1.0) * EARTH_RADIUS
        self._cos = math.cos(math.radians(lat))

    def to_engine(self, lon, lat):
        """(lon, lat) in degrees -> engine (x, z) in metres."""
        x = (np.asarray(lon) - self.lon) * self._per_deg * self._cos
        z = -(np.asarray(lat) - self.lat) * self._per_deg
        return x, z

    def to_geo(self, x, z):
        """Engine (x, z) in metres -> (lon, lat) in degrees."""
        lon = self.lon + np.asarray(x) / (self._per_deg * self._cos)
        lat = self.lat - np.asarray(z) / self._per_deg
        return lon, lat


class DemTile:
    """A Copernicus GLO-30 COG tile (pixel-is-point, EPSG:4326)."""

    def __init__(self, path: str):
        import tifffile

        with tifffile.TiffFile(path) as tif:
            page = tif.pages[0]
            tie = page.tags["ModelTiepointTag"].value
            scale = page.tags["ModelPixelScaleTag"].value
            self.heights = page.asarray().astype(np.float32)
        self.lon0, self.lat0 = tie[3], tie[4]
        self.dlon, self.dlat = scale[0], scale[1]

    def sample(self, lon, lat):
        """Bilinear elevation in metres at arrays of lon/lat."""
        col = (np.asarray(lon) - self.lon0) / self.dlon
        row = (self.lat0 - np.asarray(lat)) / self.dlat
        rows, cols = self.heights.shape
        if col.min() < 0 or row.min() < 0 or \
                col.max() > cols - 1 or row.max() > rows - 1:
            raise ValueError("bake extent leaves the DEM tile")
        c0 = np.minimum(np.floor(col).astype(int), cols - 2)
        r0 = np.minimum(np.floor(row).astype(int), rows - 2)
        fc = (col - c0).astype(np.float32)
        fr = (row - r0).astype(np.float32)
        h = self.heights
        top = h[r0, c0] * (1 - fc) + h[r0, c0 + 1] * fc
        bottom = h[r0 + 1, c0] * (1 - fc) + h[r0 + 1, c0 + 1] * fc
        return top * (1 - fr) + bottom * fr
