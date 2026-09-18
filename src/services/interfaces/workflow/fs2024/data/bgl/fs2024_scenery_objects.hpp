#pragma once

#include <string>
#include <vector>

namespace sdl3cpp::fs2024 {

/// One library-object placement from a BGL's SceneryObject section
/// (type 0x25, record 0x000B, 64 bytes): where FS2024 itself puts one
/// of its model-library models.
struct SceneryObjectPlacement {
    double lat = 0.0, lon = 0.0;
    float headingDegrees = 0.f;
    float scale = 1.f;
    std::string guid;  ///< the model's, as HexGuid formats it
};

/// Every library-object placement in `path`. Record layout: u16 id
/// 0x000B, u16 size 64, u32 lon (v * 360 / 0x30000000 - 180), u32 lat
/// (90 - v * 180 / 0x20000000), u32 altitude, u16 flags, i16 pitch,
/// i16 bank, u16 heading (v * 360 / 65536), then the model GUID at
/// +0x2C and a float scale at +0x3C. Other record kinds are skipped by
/// their own size field.
std::vector<SceneryObjectPlacement> ReadSceneryObjects(
    const std::string& path);

}  // namespace sdl3cpp::fs2024
