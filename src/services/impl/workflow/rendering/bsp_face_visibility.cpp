#include "services/interfaces/workflow/rendering/bsp_face_visibility.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr const char* kNonRenderingSubstrings[] = {
    "sky", "trigger", "hint", "caulk", "clip", "nodraw", "areaportal",
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
