#include "services/interfaces/workflow/fs2024/prepare/fs2024_local_frame.hpp"

#include <cmath>

namespace sdl3cpp::tools::fs2024 {

namespace {
constexpr double kEarthRadius = 6371008.8;
}

LocalFrame::LocalFrame(double lon, double lat, float altitude)
    : lon_(lon), lat_(lat), altitude_(altitude) {
    metresPerDegree_ = (3.14159265358979323846 / 180.0) * kEarthRadius;
    cosLat_ = std::cos(lat * 3.14159265358979323846 / 180.0);
}

void LocalFrame::ToEngine(double lon, double lat, float& x, float& z) const {
    x = static_cast<float>((lon - lon_) * metresPerDegree_ * cosLat_);
    z = static_cast<float>(-(lat - lat_) * metresPerDegree_);
}

void LocalFrame::ToGeo(float x, float z, double& lon, double& lat) const {
    lon = lon_ + x / (metresPerDegree_ * cosLat_);
    lat = lat_ - z / metresPerDegree_;
}

}  // namespace sdl3cpp::tools::fs2024
