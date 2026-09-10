#include "services/interfaces/workflow/rendering/overlay_sw_end_resources.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

namespace sdl3cpp::services::impl {
namespace {

std::string StringParameterOr(const WorkflowStepParameterResolver& params,
                              const WorkflowStepDefinition& step,
                              const char* key, const char* fallback) {
    const auto* p       = params.FindParameter(step, key);
    const bool isString = p && p->type == WorkflowParameterValue::Type::String;
    return isString ? p->stringValue : fallback;
}

}  // namespace

void ResolveOverlaySwEndShaderPaths(const WorkflowStepDefinition& step,
                                    SDL_GPUDevice* device,
                                    std::string& vertPath,
                                    std::string& fragPath) {
    WorkflowStepParameterResolver params;
    const char* driver           = SDL_GetGPUDeviceDriver(device);
    const std::string driverName = driver ? driver : "";
    if (driverName == "metal") {
        vertPath =
            StringParameterOr(params, step, "vert_shader_path_msl",
                              "packages/quake3/shaders/msl/overlay.vert.metal");
        fragPath =
            StringParameterOr(params, step, "frag_shader_path_msl",
                              "packages/quake3/shaders/msl/overlay.frag.metal");
    } else {
        vertPath =
            StringParameterOr(params, step, "vert_shader_path_spirv",
                              "packages/quake3/shaders/spirv/overlay.vert.spv");
        fragPath =
            StringParameterOr(params, step, "frag_shader_path_spirv",
                              "packages/quake3/shaders/spirv/overlay.frag.spv");
    }
}

}  // namespace sdl3cpp::services::impl
