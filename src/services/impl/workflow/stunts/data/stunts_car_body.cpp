#include "services/interfaces/workflow/stunts/data/stunts_car_body.hpp"

#include "services/interfaces/workflow/stunts/data/stunts_container.hpp"
#include "services/interfaces/workflow/stunts/data/stunts_shape_archive.hpp"

#include <algorithm>
#include <cctype>

namespace sdl3cpp::services::impl {

std::string StuntsCarBodyFileFor(const std::string& carResFile) {
    std::string name = carResFile;
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    if (name.rfind("CAR", 0) != 0) return {};
    const auto dot = name.find('.');
    if (dot == std::string::npos) return {};
    return "ST" + name.substr(3, dot - 3) + ".P3S";
}

StuntsShape LoadStuntsCarBody(const std::string& path) {
    const StuntsShapeArchive archive =
        ParseStuntsShapeArchive(LoadStuntsFile(path));
    const auto car0 = archive.find("car0");
    if (car0 == archive.end()) return StuntsShape{};
    return ParseStuntsShape(car0->second);
}

}  // namespace sdl3cpp::services::impl
