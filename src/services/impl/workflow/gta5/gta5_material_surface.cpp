#include "services/interfaces/workflow/gta5/gta5_material_surface.hpp"

#include <assimp/GltfMaterial.h>

#include <cstring>

namespace sdl3cpp::services::impl {

std::array<float, 4> ReadGta5MaterialSurface(const aiMaterial& material) {
    std::array<float, 4> out{1.f, 1.f, 1.f, 0.f};

    // glTF baseColorFactor. The converter writes it only for shaders in
    // the vehicle_paint family, whose texture is a few white pixels
    // because the game colours a car from carcols.ymt rather than from
    // the drawable.
    aiColor4D base;
    if (material.Get(AI_MATKEY_BASE_COLOR, base) == AI_SUCCESS) {
        out[0] = base.r;
        out[1] = base.g;
        out[2] = base.b;
    }

    // alphaMode MASK. BLEND is deliberately not honoured: it would want
    // back-to-front sorting across the whole streamed world, and every
    // surface the converter marks is a cutout anyway -- its alpha says
    // "not here", not "see through".
    aiString mode;
    if (material.Get(AI_MATKEY_GLTF_ALPHAMODE, mode) == AI_SUCCESS &&
        std::strcmp(mode.C_Str(), "MASK") == 0) {
        float cutoff = 0.5f;
        material.Get(AI_MATKEY_GLTF_ALPHACUTOFF, cutoff);
        // A zero threshold is the "off" value, so a material asking for
        // one would silently draw its whole rectangle.
        out[3] = (cutoff > 0.f) ? cutoff : 0.5f;
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
