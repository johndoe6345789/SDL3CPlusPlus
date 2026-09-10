#pragma once

#include <string>
#include <vector>

namespace sdl3cpp::q3 {

/**
 * @brief Builds the ordered list of pk3 paths to try for a surface's texture.
 *
 * Quake resolves a surface's texture from its .skin file first, so that entry
 * (and its .jpg/.tga variants) leads.  The MD3's own shader name comes next,
 * then an explicitly configured skin, and finally the model path with texture
 * extensions substituted.
 *
 * @param skinnedTexture Texture the .skin file names for this surface, if any.
 * @param shaderName     Shader name recorded in the MD3 surface, if any.
 * @param configuredSkin The step's `skin` parameter, if any.
 * @param md3Path        Pk3 path of the MD3 itself, used for the fallback.
 */
std::vector<std::string> BuildMd3TextureCandidates(
    const std::string& skinnedTexture, const std::string& shaderName,
    const std::string& configuredSkin, const std::string& md3Path);

}  // namespace sdl3cpp::q3
