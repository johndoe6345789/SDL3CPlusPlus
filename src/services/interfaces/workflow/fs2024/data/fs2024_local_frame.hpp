#pragma once

namespace sdl3cpp::fs2024 {

/// Local metric frame around a point (an airport reference point, or a
/// spawn road's snap point): engine x = east metres, z = south metres,
/// y = altitude minus this point's own altitude. Good to well under a
/// metre over the few km one bake covers -- far finer than a 30 m DEM.
class LocalFrame {
public:
    LocalFrame(double lon, double lat, float altitude);

    double lon() const { return lon_; }
    double lat() const { return lat_; }
    float altitude() const { return altitude_; }
    void SetAltitude(float altitude) { altitude_ = altitude; }

    void ToEngine(double lon, double lat, float& x, float& z) const;
    void ToGeo(float x, float z, double& lon, double& lat) const;

private:
    double lon_, lat_;
    float altitude_;
    double metresPerDegree_, cosLat_;
};

}  // namespace sdl3cpp::fs2024
