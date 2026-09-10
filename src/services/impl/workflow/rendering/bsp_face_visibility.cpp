#include "services/interfaces/workflow/rendering/bsp_face_visibility.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr const char* kNonRenderingSubstrings[] = {
    // "sky" catches textures/sky/...; ioquake3 maps' sky textures more
    // commonly live under textures/skies/... (q3dm1's tim_hell, etc.),
    // which "sky" alone doesn't match as a substring.
    "sky", "skies", "trigger", "hint", "caulk", "clip", "nodraw",
    "areaportal",
};

}  // namespace

bool IsBspFaceTextureVisible(const std::string& textureName) {
    for (const char* substring : kNonRenderingSubstrings) {
        if (textureName.find(substring) != std::string::npos) {
            return false;
        }
    }
    return true;
}

}  // namespace sdl3cpp::services::impl
