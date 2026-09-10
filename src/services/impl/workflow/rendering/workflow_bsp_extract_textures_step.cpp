#include "services/interfaces/workflow/rendering/workflow_bsp_extract_textures_step.hpp"
#include "services/interfaces/workflow/rendering/bsp_texture_batch_loader.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>
#include <zip.h>
#include <set>
#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowBspExtractTexturesStep::WorkflowBspExtractTexturesStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBspExtractTexturesStep::GetPluginId() const {
    return "bsp.extract_textures";
}

void WorkflowBspExtractTexturesStep::Execute(const WorkflowStepDefinition&,
                                             WorkflowContext& context) {
    auto bspDataPtr =
        context.Get<std::shared_ptr<std::vector<uint8_t>>>("bsp_raw_data",
                                                            nullptr);
    if (!bspDataPtr) {
        throw std::runtime_error(
            "bsp.extract_textures: bsp_raw_data not in context");
    }

    const auto bspConfig =
        context.Get<nlohmann::json>("bsp_config", nlohmann::json{});
    const std::string pk3Path = bspConfig.value("pk3_path", std::string(""));
    if (pk3Path.empty()) {
        throw std::runtime_error("bsp.extract_textures: pk3_path not in config");
    }

    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        throw std::runtime_error("bsp.extract_textures: GPU device not found");
    }

    auto usedTextures =
        context.Get<std::shared_ptr<std::set<int>>>("bsp_used_textures",
                                                     nullptr);
    if (!usedTextures) {
        throw std::runtime_error(
            "bsp.extract_textures: bsp_used_textures not in context");
    }

    const auto& bspData = *bspDataPtr;
    const auto* lumps =
        reinterpret_cast<const BspLump*>(bspData.data() + sizeof(BspHeader));
    const auto& texLump = lumps[LUMP_TEXTURES];
    const int numTextures = texLump.length / static_cast<int>(sizeof(BspTexture));
    const auto* bspTextures =
        reinterpret_cast<const BspTexture*>(bspData.data() + texLump.offset);

    int zipError = 0;
    zip_t* archive = zip_open(pk3Path.c_str(), ZIP_RDONLY, &zipError);
    if (!archive) {
        throw std::runtime_error("bsp.extract_textures: Failed to open pk3: " +
                                 pk3Path);
    }

    const BspTextureBatchResult result = LoadBspTextureBatch(
        archive, device, *usedTextures, bspTextures, numTextures, context);
    zip_close(archive);

    if (logger_) {
        logger_->Info("bsp.extract_textures: Loaded " +
                      std::to_string(result.loadedCount) + " (" +
                      std::to_string(result.viaShaderCount) +
                      " via shader script)" + ", missing (white fallback): " +
                      std::to_string(result.missingCount));
    }
}

}  // namespace sdl3cpp::services::impl
