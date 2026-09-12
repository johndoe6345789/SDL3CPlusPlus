#include "services/interfaces/workflow/gta5/stream/gta5_lod.hpp"

namespace sdl3cpp::services::impl {

std::string Gta5LodName(Gta5Lod lod) {
    switch (lod) {
        case Gta5Lod::Hd:    return "hd";
        case Gta5Lod::Lod:   return "lod";
        case Gta5Lod::Slod1: return "slod1";
        case Gta5Lod::Slod2: return "slod2";
    }
    return "slod2";
}

Gta5Lod Gta5LodFromName(const std::string& name) {
    if (name == "hd")    return Gta5Lod::Hd;
    if (name == "lod")   return Gta5Lod::Lod;
    if (name == "slod1") return Gta5Lod::Slod1;
    return Gta5Lod::Slod2;
}

}  // namespace sdl3cpp::services::impl
