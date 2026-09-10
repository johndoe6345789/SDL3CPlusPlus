#include "services/interfaces/workflow/rendering/bsp_model_lump.hpp"

#include <cstdio>

namespace sdl3cpp::services::impl {

bool ParseBspVec3(const std::string& text, float& x, float& y, float& z) {
    return std::sscanf(text.c_str(), "%f %f %f", &x, &y, &z) == 3;
}

std::vector<BspModel> ReadBspModels(const std::vector<uint8_t>& bspData,
                                    const BspLump& modelLump) {
    std::vector<BspModel> models;
    const bool inBounds =
        modelLump.length >= 0 && modelLump.offset >= 0 &&
        static_cast<size_t>(modelLump.offset + modelLump.length) <=
            bspData.size();
    if (!inBounds) {
        return models;
    }
    const auto* modelData =
        reinterpret_cast<const BspModel*>(bspData.data() + modelLump.offset);
    const size_t modelCount =
        static_cast<size_t>(modelLump.length) / sizeof(BspModel);
    models.assign(modelData, modelData + modelCount);
    return models;
}

}  // namespace sdl3cpp::services::impl
