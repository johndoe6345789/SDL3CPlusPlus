#pragma once

// Umbrella header: LightmapAtlas/BuildLightmapAtlas live in
// bsp_lightmap_atlas_build.hpp, LightmapAtlasGpu/UploadLightmapAtlas in
// bsp_lightmap_atlas_upload.hpp. Kept so existing includers of this name
// don't need to know about the split.
#include "services/interfaces/workflow/rendering/bsp_lightmap_atlas_build.hpp"
#include "services/interfaces/workflow/rendering/bsp_lightmap_atlas_upload.hpp"
