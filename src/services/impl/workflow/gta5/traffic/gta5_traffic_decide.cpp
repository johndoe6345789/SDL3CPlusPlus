#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_ai.hpp"

#include <cstdio>
#include <cstdlib>
#include <iterator>

namespace sdl3cpp::services::impl {
namespace {

/// The net, loaded once from the first call. Empty means the baseline.
const Gta5DriverNet& Net() {
    static const Gta5DriverNet net = [] {
        Gta5DriverNet loaded;
        const char* path = std::getenv("SDL3CPP_TRAFFIC_POLICY");
        if (path && !loaded.Load(path)) {
            std::fprintf(stderr, "traffic: cannot load policy %s\n", path);
        }
        return loaded;
    }();
    return net;
}

/// Rows of kGta5DriverInputs + kGta5DriverOutputs raw float32s: the
/// sense as the driver saw it, then the baseline's controls.
std::FILE* Log() {
    static std::FILE* file = [] {
        const char* path = std::getenv("SDL3CPP_TRAFFIC_LOG");
        return path ? std::fopen(path, "ab") : nullptr;
    }();
    return file;
}

}  // namespace

Gta5DriverAction Gta5DecideDrive(const Gta5DriverSense& s) {
    const Gta5DriverAction taught = Gta5BaselineDriver(s);
    if (std::FILE* log = Log()) {
        const float row[kGta5DriverInputs + kGta5DriverOutputs] = {
            s.speed,  s.off,          s.turning,   s.gap,
            s.line,   s.want,         s.follow,    s.nerve,
            taught.steer, taught.throttle, taught.brake};
        std::fwrite(row, sizeof(float), std::size(row), log);
    }
    return Net().Ready() ? Net().Run(s) : taught;
}

}  // namespace sdl3cpp::services::impl
