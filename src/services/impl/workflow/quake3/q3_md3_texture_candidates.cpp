#include "services/interfaces/workflow/quake3/q3_md3_texture_candidates.hpp"

namespace sdl3cpp::q3 {
namespace {

std::string StripExtension(const std::string& path) {
    const auto dot = path.rfind('.');
    return dot == std::string::npos ? path : path.substr(0, dot);
}

bool HasExtension(const std::string& path) {
    return path.rfind('.') != std::string::npos;
}

}  // namespace

std::vector<std::string> BuildMd3TextureCandidates(
    const std::string& skinnedTexture, const std::string& shaderName,
    const std::string& configuredSkin, const std::string& md3Path) {
    std::vector<std::string> candidates;

    // The .skin entry wins: it is what Quake itself uses.
    if (!skinnedTexture.empty()) {
        candidates.push_back(skinnedTexture);
        if (HasExtension(skinnedTexture)) {
            const std::string stem = StripExtension(skinnedTexture);
            candidates.push_back(stem + ".jpg");
            candidates.push_back(stem + ".tga");
        }
    }

    if (!shaderName.empty()) {
        candidates.push_back(shaderName + ".tga");
        candidates.push_back(shaderName + ".jpg");
        candidates.push_back(shaderName);
    }

    if (!configuredSkin.empty()) {
        candidates.push_back(configuredSkin);
        if (HasExtension(configuredSkin)) {
            candidates.push_back(StripExtension(configuredSkin) + ".jpg");
        }
    }

    const std::string stem = StripExtension(md3Path);
    candidates.push_back(stem + ".tga");
    candidates.push_back(stem + ".jpg");
    return candidates;
}

}  // namespace sdl3cpp::q3
