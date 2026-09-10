#include "services/interfaces/workflow/rendering/gpu_text_overlay_shader_loader.hpp"

#include <cstdint>
#include <fstream>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {

constexpr const char* kVertSpvPath =
    "packages/quake3/shaders/spirv/overlay.vert.spv";
constexpr const char* kFragSpvPath =
    "packages/quake3/shaders/spirv/overlay.frag.spv";

std::vector<uint8_t> LoadSpirv(const char* path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return {};
    }
    const auto size = file.tellg();
    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

SDL_GPUShader* CreateShader(SDL_GPUDevice* device,
                            const std::vector<uint8_t>& code,
                            SDL_GPUShaderStage stage, Uint32 numSamplers) {
    SDL_GPUShaderCreateInfo info = {};
    info.code                    = code.data();
    info.code_size               = code.size();
    info.entrypoint              = "main";
    info.format                  = SDL_GPU_SHADERFORMAT_SPIRV;
    info.stage                   = stage;
    info.num_samplers            = numSamplers;
    return SDL_CreateGPUShader(device, &info);
}

}  // namespace

const char* LoadOverlayShaderPair(SDL_GPUDevice* device,
                                  SDL_GPUShader*& outVertex,
                                  SDL_GPUShader*& outFragment) {
    const std::vector<uint8_t> vertCode = LoadSpirv(kVertSpvPath);
    const std::vector<uint8_t> fragCode = LoadSpirv(kFragSpvPath);
    if (vertCode.empty() || fragCode.empty()) {
        return "overlay shaders not found";
    }

    SDL_GPUShader* vertex =
        CreateShader(device, vertCode, SDL_GPU_SHADERSTAGE_VERTEX, 0);
    SDL_GPUShader* fragment =
        CreateShader(device, fragCode, SDL_GPU_SHADERSTAGE_FRAGMENT, 1);
    if (!vertex || !fragment) {
        if (vertex) SDL_ReleaseGPUShader(device, vertex);
        if (fragment) SDL_ReleaseGPUShader(device, fragment);
        return "overlay shader creation failed";
    }

    outVertex   = vertex;
    outFragment = fragment;
    return "";
}

}  // namespace sdl3cpp::services::impl
