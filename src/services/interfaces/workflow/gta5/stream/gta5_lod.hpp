#pragma once

#include <string>

namespace sdl3cpp::services::impl {

/// Detail bands from config/gta5_world.json, nearest first.
enum class Gta5Lod : int { Hd = 0, Lod = 1, Slod1 = 2, Slod2 = 3 };

std::string Gta5LodName(Gta5Lod lod);

/// Parse a band name as the importer writes it. Unknown names fall back to
/// the outermost band, which draws least.
Gta5Lod Gta5LodFromName(const std::string& name);

}  // namespace sdl3cpp::services::impl
