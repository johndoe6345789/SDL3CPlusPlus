#include "services/interfaces/workflow/gta5/stream/gta5_proxy.hpp"

#include <algorithm>
#include <cctype>
#include <initializer_list>

namespace sdl3cpp::services::impl {
namespace {

bool HasAny(const std::string& name, std::initializer_list<const char*> of) {
    return std::any_of(of.begin(), of.end(), [&](const char* part) {
        return name.find(part) != std::string::npos;
    });
}

}  // namespace

Gta5ProxyKind ClassifyGta5Proxy(const std::string& archetype) {
    std::string name = archetype;
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (name.find("proxy") == std::string::npos) return Gta5ProxyKind::None;
    // Water first: "wtrproxy" holds "rproxy". Water reflection
    // ("watrelf", sic) holds neither, and is a reflection.
    if (HasAny(name, {"wtr", "water"})) return Gta5ProxyKind::Water;
    if (HasAny(name, {"refl", "rfl", "relf", "_ref_", "refproxy", "rproxy",
                      "mirror"})) {
        return Gta5ProxyKind::Reflection;
    }
    if (HasAny(name, {"shadow", "shad", "shdw"})) return Gta5ProxyKind::Shadow;
    if (HasAny(name, {"light", "lproxy", "_emi_", "emi_proxy", "emissive"})) {
        return Gta5ProxyKind::Light;
    }
    if (name.find("rain") != std::string::npos) return Gta5ProxyKind::Rain;
    if (name.find("smoke") != std::string::npos) return Gta5ProxyKind::Smoke;
    return Gta5ProxyKind::None;
}

const char* Gta5ProxyKindName(Gta5ProxyKind kind) {
    switch (kind) {
        case Gta5ProxyKind::Reflection: return "reflection";
        case Gta5ProxyKind::Shadow: return "shadow";
        case Gta5ProxyKind::Water: return "water";
        case Gta5ProxyKind::Light: return "light";
        case Gta5ProxyKind::Rain: return "rain";
        case Gta5ProxyKind::Smoke: return "smoke";
        default: return "none";
    }
}

}  // namespace sdl3cpp::services::impl
