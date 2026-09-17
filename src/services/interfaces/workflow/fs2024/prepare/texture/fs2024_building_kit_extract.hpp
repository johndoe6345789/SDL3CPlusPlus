#pragma once

#include <string>

namespace sdl3cpp::tools::fs2024 {

/// Decodes FS2024's own wall and roof DDS textures once into
/// `<outDir>/building_kit/wall.png` and `roof.png`, shared, global
/// data every OSM building citywide is textured with (unless already
/// there -- these never change bake to bake, only the game install
/// they came from could). No mesh or model library involved: unlike a
/// landmark, a generic wall/roof texture is just a plain file FS2024
/// ships directly (e.g. under a `*-modellib-texture` package's own
/// `Texture/` folder).
void ExtractBuildingKit(const std::string& wallDdsPath,
                       const std::string& roofDdsPath,
                       const std::string& outDir);

}  // namespace sdl3cpp::tools::fs2024
