#include "services/interfaces/workflow/stunts/data/stunts_car.hpp"

#include "services/interfaces/workflow/stunts/data/stunts_container.hpp"
#include "services/interfaces/workflow/stunts/data/stunts_resource.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr std::size_t kEngineFields = 7;

std::uint16_t Field(const std::vector<std::uint8_t>& simd, std::size_t index) {
    const std::size_t at = index * 2;
    if (at + 1 >= simd.size()) return 0;
    return static_cast<std::uint16_t>(simd[at] | (simd[at + 1] << 8));
}

StuntsEngine ReadEngine(const std::vector<std::uint8_t>& simd) {
    StuntsEngine engine;
    if (simd.size() < kEngineFields * 2) return engine;
    engine.gears = Field(simd, 0);
    engine.performance = Field(simd, 1);
    engine.idleRpm = Field(simd, 3);
    engine.powerRpm = Field(simd, 4);
    engine.redLine = Field(simd, 5);
    engine.revLimit = Field(simd, 6);
    return engine;
}

/// The showroom text is one NUL-terminated string whose lines are
/// separated by ']'; empty runs are the blank lines the screen shows.
std::vector<std::string> SplitLines(const std::vector<std::uint8_t>& edes) {
    std::vector<std::string> lines;
    std::string current;
    for (const std::uint8_t byte : edes) {
        if (byte == 0) break;
        if (byte == ']') {
            if (!current.empty()) lines.push_back(current);
            current.clear();
            continue;
        }
        if (byte >= 0x20 && byte < 0x7f) current.push_back(
            static_cast<char>(byte));
    }
    if (!current.empty()) lines.push_back(current);
    return lines;
}

}  // namespace

StuntsCar LoadStuntsCar(const std::string& path, const std::string& id) {
    StuntsCar car;
    car.id = id;
    const StuntsResources res = ParseStuntsResources(LoadStuntsFile(path));
    const auto simd = res.find("simd");
    if (simd == res.end()) return car;

    car.engine = ReadEngine(simd->second);
    const auto edes = res.find("edes");
    if (edes != res.end()) {
        car.spec = SplitLines(edes->second);
        if (!car.spec.empty()) {
            car.name = car.spec.front();
            car.spec.erase(car.spec.begin());
        }
    }
    if (car.name.empty()) car.name = id;
    car.loaded = true;
    return car;
}

}  // namespace sdl3cpp::services::impl
