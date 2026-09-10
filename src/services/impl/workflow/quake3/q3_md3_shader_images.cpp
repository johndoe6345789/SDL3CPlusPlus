#include "services/interfaces/workflow/quake3/q3_md3_shader_images.hpp"

#include "services/interfaces/workflow/rendering/bsp_shader_script.hpp"

#include <zip.h>

namespace sdl3cpp::services::impl {
namespace {

constexpr const char* kCacheKey = "q3.md3.shader_images";

using ShaderImages = std::map<std::string, std::string>;

ShaderImages ParseArchive(const std::string& pk3Path) {
    int err       = 0;
    zip_t* handle = zip_open(pk3Path.c_str(), ZIP_RDONLY, &err);
    if (!handle) {
        return {};
    }
    ShaderImages images = LoadShaderImages(handle);
    zip_close(handle);
    return images;
}

}  // namespace

const ShaderImages& Q3Md3ShaderImages(const std::string& pk3Path,
                                      WorkflowContext& context) {
    if (const auto* cached = context.TryGet<ShaderImages>(kCacheKey)) {
        return *cached;
    }
    context.Set<ShaderImages>(kCacheKey, ParseArchive(pk3Path));
    return *context.TryGet<ShaderImages>(kCacheKey);
}

}  // namespace sdl3cpp::services::impl
