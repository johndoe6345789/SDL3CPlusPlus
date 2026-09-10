#pragma once

#include <map>
#include <string>

// Forward-declared to avoid pulling <zip.h> into every includer.
typedef struct zip zip_t;

namespace sdl3cpp::services::impl {

/**
 * @brief Maps Q3 shader names to the image each shader's stages reference.
 *
 * A BSP face names a shader, not an image directly. Most of id's shaders
 * share a name with a file on disk, but ones defined only in
 * scripts/*.shader do not — on q3dm1 that's the tongue, the sky, the lava
 * and six others, every one of which would otherwise fall back to white.
 *
 * Scans every scripts/*.shader entry in the pk3 and records, per shader, the
 * first image its `map`/`clampmap` stages actually reference. The extension
 * is dropped because a script commonly says `.tga` where the pk3 ships
 * `.jpg` — the caller tries extensions itself.
 */
std::map<std::string, std::string> LoadShaderImages(zip_t* archive);

}  // namespace sdl3cpp::services::impl
