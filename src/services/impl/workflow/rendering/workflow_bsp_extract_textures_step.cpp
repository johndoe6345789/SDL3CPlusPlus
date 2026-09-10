#include "services/interfaces/workflow/rendering/workflow_bsp_extract_textures_step.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>
#include <stb_image.h>
#include <zip.h>
#include <algorithm>
#include <cstring>
#include <memory>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

// A BSP names a shader, not an image. Most of id's shaders share a name
// with a file on disk, but the ones defined only in scripts/*.shader do
// not: on q3dm1 that is the tongue, the sky, the lava and six others,
// every one of which fell back to white. Take the first image a shader's
// stages actually reference. The extension is dropped because a script
// says .tga where the pk3 ships .jpg.
static void ParseShaderScript(const std::string& text,
                              std::map<std::string, std::string>& out) {
    std::string shader;
    int depth = 0;
    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line)) {
        const auto comment = line.find("//");
        if (comment != std::string::npos) line.erase(comment);
        std::istringstream words(line);
        std::string word;
        if (!(words >> word)) continue;
        if (word == "{") { ++depth; continue; }
        if (word == "}") { if (depth > 0) --depth; continue; }
        if (depth == 0) { shader = word; continue; }
        if (shader.empty() || out.count(shader)) continue;
        std::string lowered = word;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (lowered != "map" && lowered != "clampmap") continue;
        std::string image;
        if (!(words >> image) || image.empty() || image[0] == '$') continue;
        const auto dot = image.rfind('.');
        if (dot != std::string::npos) image.erase(dot);
        out[shader] = image;
    }
}

static std::map<std::string, std::string> LoadShaderImages(zip_t* archive) {
    std::map<std::string, std::string> out;
    const zip_int64_t count = zip_get_num_entries(archive, 0);
    for (zip_int64_t i = 0; i < count; ++i) {
        const char* raw = zip_get_name(archive, i, 0);
        if (!raw) continue;
        const std::string entry(raw);
        if (entry.rfind("scripts/", 0) != 0) continue;
        if (entry.size() < 7 ||
            entry.compare(entry.size() - 7, 7, ".shader") != 0) continue;
        zip_stat_t st;
        if (zip_stat_index(archive, i, 0, &st) != 0) continue;
        zip_file_t* file = zip_fopen_index(archive, i, 0);
        if (!file) continue;
        std::string text(static_cast<size_t>(st.size), '\0');
        zip_fread(file, text.data(), st.size);
        zip_fclose(file);
        ParseShaderScript(text, out);
    }
    return out;
}

// Helper: create a 1x1 white RGBA texture + sampler as fallback
static void CreateWhiteTexture(SDL_GPUDevice* device, const std::string& texKey, const std::string& sampKey,
                               WorkflowContext& context) {
    SDL_GPUTextureCreateInfo ti = {};
    ti.type = SDL_GPU_TEXTURETYPE_2D;
    ti.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    ti.width = 1; ti.height = 1;
    ti.layer_count_or_depth = 1;
    ti.num_levels = 1;
    ti.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    auto* tex = SDL_CreateGPUTexture(device, &ti);

    SDL_GPUTransferBufferCreateInfo tbi = {};
    tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbi.size = 4;
    auto* tb = SDL_CreateGPUTransferBuffer(device, &tbi);
    auto* m = static_cast<uint8_t*>(SDL_MapGPUTransferBuffer(device, tb, false));
    m[0] = 255; m[1] = 255; m[2] = 255; m[3] = 255;
    SDL_UnmapGPUTransferBuffer(device, tb);

    auto* cmd = SDL_AcquireGPUCommandBuffer(device);
    auto* cp = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTextureTransferInfo src = {}; src.transfer_buffer = tb;
    SDL_GPUTextureRegion dst = {}; dst.texture = tex; dst.w = 1; dst.h = 1; dst.d = 1;
    SDL_UploadToGPUTexture(cp, &src, &dst, false);
    SDL_EndGPUCopyPass(cp);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, tb);

    SDL_GPUSamplerCreateInfo si = {};
    si.min_filter = SDL_GPU_FILTER_LINEAR;
    si.mag_filter = SDL_GPU_FILTER_LINEAR;
    si.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    si.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    si.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    si.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    auto* samp = SDL_CreateGPUSampler(device, &si);

    context.Set<SDL_GPUTexture*>(texKey, tex);
    context.Set<SDL_GPUSampler*>(sampKey, samp);
}

WorkflowBspExtractTexturesStep::WorkflowBspExtractTexturesStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBspExtractTexturesStep::GetPluginId() const {
    return "bsp.extract_textures";
}

void WorkflowBspExtractTexturesStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    auto bspDataPtr = context.Get<std::shared_ptr<std::vector<uint8_t>>>("bsp_raw_data", nullptr);
    if (!bspDataPtr) throw std::runtime_error("bsp.extract_textures: bsp_raw_data not in context");

    auto bspConfig = context.Get<nlohmann::json>("bsp_config", nlohmann::json{});
    std::string pk3_path = bspConfig.value("pk3_path", std::string(""));
    if (pk3_path.empty()) throw std::runtime_error("bsp.extract_textures: pk3_path not in config");

    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) throw std::runtime_error("bsp.extract_textures: GPU device not found");

    auto usedTextures = context.Get<std::shared_ptr<std::set<int>>>("bsp_used_textures", nullptr);
    if (!usedTextures) throw std::runtime_error("bsp.extract_textures: bsp_used_textures not in context");

    const auto& bspData = *bspDataPtr;
    auto* lumps = reinterpret_cast<const BspLump*>(bspData.data() + sizeof(BspHeader));
    const auto& texLump = lumps[LUMP_TEXTURES];
    int numTextures = texLump.length / static_cast<int>(sizeof(BspTexture));
    auto* bspTextures = reinterpret_cast<const BspTexture*>(bspData.data() + texLump.offset);

    // Reopen pk3 for texture extraction
    int zip_err = 0;
    zip_t* archive = zip_open(pk3_path.c_str(), ZIP_RDONLY, &zip_err);
    if (!archive) throw std::runtime_error("bsp.extract_textures: Failed to open pk3: " + pk3_path);

    const auto shaderImages = LoadShaderImages(archive);
    int loadedTextures = 0;
    int missingTextures = 0;
    int viaShader = 0;

    for (int texIdx : *usedTextures) {
        if (texIdx < 0 || texIdx >= numTextures) continue;

        std::string texName(bspTextures[texIdx].name);
        std::string texKey = "bsp_tex_" + std::to_string(texIdx) + "_gpu";
        std::string sampKey = "bsp_tex_" + std::to_string(texIdx) + "_sampler";

        static const char* extensions[] = { ".jpg", ".tga", ".png" };
        bool found = false;

        std::vector<std::string> bases{texName};
        const auto viaScript = shaderImages.find(texName);
        if (viaScript != shaderImages.end() && viaScript->second != texName) {
            bases.push_back(viaScript->second);
        }
        for (const std::string& base : bases) {
        for (const char* ext : extensions) {
            std::string entryName = base + ext;
            zip_stat_t texStat;
            if (zip_stat(archive, entryName.c_str(), 0, &texStat) != 0) continue;

            zip_file_t* texFile = zip_fopen(archive, entryName.c_str(), 0);
            if (!texFile) continue;

            std::vector<uint8_t> texData(texStat.size);
            zip_fread(texFile, texData.data(), texStat.size);
            zip_fclose(texFile);

            int w = 0, h = 0, channels = 0;
            unsigned char* pixels = stbi_load_from_memory(texData.data(),
                static_cast<int>(texData.size()), &w, &h, &channels, 4);
            if (!pixels) continue;

            int maxDim = std::max(w, h);
            Uint32 numLevels = 1;
            while (maxDim > 1) { maxDim >>= 1; numLevels++; }

            SDL_GPUTextureCreateInfo ti = {};
            ti.type = SDL_GPU_TEXTURETYPE_2D;
            ti.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
            ti.width = static_cast<Uint32>(w);
            ti.height = static_cast<Uint32>(h);
            ti.layer_count_or_depth = 1;
            ti.num_levels = numLevels;
            ti.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER |
                        (numLevels > 1 ? SDL_GPU_TEXTUREUSAGE_COLOR_TARGET : 0);
            auto* tex = SDL_CreateGPUTexture(device, &ti);

            Uint32 dataSize = static_cast<Uint32>(w * h * 4);
            SDL_GPUTransferBufferCreateInfo tbi = {};
            tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            tbi.size = dataSize;
            auto* tb = SDL_CreateGPUTransferBuffer(device, &tbi);
            auto* mapped = static_cast<uint8_t*>(SDL_MapGPUTransferBuffer(device, tb, false));
            std::memcpy(mapped, pixels, dataSize);
            SDL_UnmapGPUTransferBuffer(device, tb);
            stbi_image_free(pixels);

            auto* cmd = SDL_AcquireGPUCommandBuffer(device);
            auto* cp = SDL_BeginGPUCopyPass(cmd);
            SDL_GPUTextureTransferInfo srcInfo = {}; srcInfo.transfer_buffer = tb;
            SDL_GPUTextureRegion dstRegion = {};
            dstRegion.texture = tex;
            dstRegion.w = static_cast<Uint32>(w);
            dstRegion.h = static_cast<Uint32>(h);
            dstRegion.d = 1;
            SDL_UploadToGPUTexture(cp, &srcInfo, &dstRegion, false);
            SDL_EndGPUCopyPass(cp);

            if (numLevels > 1) {
                SDL_GenerateMipmapsForGPUTexture(cmd, tex);
            }

            SDL_SubmitGPUCommandBuffer(cmd);
            SDL_ReleaseGPUTransferBuffer(device, tb);

            SDL_GPUSamplerCreateInfo si = {};
            si.min_filter = SDL_GPU_FILTER_LINEAR;
            si.mag_filter = SDL_GPU_FILTER_LINEAR;
            si.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
            si.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
            si.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
            si.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
            si.enable_anisotropy = true;
            si.max_anisotropy = 16.0f;
            si.mip_lod_bias = 0.5f;
            si.min_lod = 0.0f;
            si.max_lod = static_cast<float>(numLevels);
            auto* samp = SDL_CreateGPUSampler(device, &si);

            context.Set<SDL_GPUTexture*>(texKey, tex);
            context.Set<SDL_GPUSampler*>(sampKey, samp);
            found = true;
            ++loadedTextures;
            if (base != texName) ++viaShader;
            break;
        }
        if (found) break;
        }

        if (!found) {
            CreateWhiteTexture(device, texKey, sampKey, context);
            ++missingTextures;
        }
    }

    zip_close(archive);

    if (logger_) {
        logger_->Info("bsp.extract_textures: Loaded " + std::to_string(loadedTextures) +
                     " (" + std::to_string(viaShader) + " via shader script)" +
                     ", missing (white fallback): " + std::to_string(missingTextures));
    }
}

}  // namespace sdl3cpp::services::impl
