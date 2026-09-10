#include "services/interfaces/workflow/rendering/workflow_geometry_create_flashlight_step.hpp"
#include "services/interfaces/workflow/rendering/flashlight_mesh.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>

#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowGeometryCreateFlashlightStep::WorkflowGeometryCreateFlashlightStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGeometryCreateFlashlightStep::GetPluginId() const {
    return "geometry.create_flashlight";
}

void WorkflowGeometryCreateFlashlightStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepParameterResolver params;
    auto getStr = [&](const char* pname, const std::string& def) {
        const auto* p = params.FindParameter(step, pname);
        return (p && p->type == WorkflowParameterValue::Type::String)
                   ? p->stringValue
                   : def;
    };
    auto getNum = [&](const char* pname, float def) {
        const auto* p = params.FindParameter(step, pname);
        return (p && p->type == WorkflowParameterValue::Type::Number)
                   ? static_cast<float>(p->numberValue)
                   : def;
    };

    const std::string name = getStr("name", "flashlight");
    const int segments = static_cast<int>(getNum("segments", 12));
    const float body_radius = getNum("body_radius", 0.025f);
    const float body_length = getNum("body_length", 0.25f);
    const float head_radius = getNum("head_radius", 0.04f);
    const float head_length = getNum("head_length", 0.08f);
    const float lens_radius = getNum("lens_radius", 0.035f);

    const FlashlightMesh mesh = BuildFlashlightMesh(
        segments, body_radius, body_length, head_radius, head_length,
        lens_radius);

    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        throw std::runtime_error(
            "geometry.create_flashlight: GPU device not found");
    }

    const FlashlightMeshBuffers buffers = UploadFlashlightMesh(device, mesh);

    const uint32_t vertex_count =
        static_cast<uint32_t>(mesh.vertices.size());
    const uint32_t index_count = static_cast<uint32_t>(mesh.indices.size());

    context.Set<SDL_GPUBuffer*>("plane_" + name + "_vb", buffers.vertexBuffer);
    context.Set<SDL_GPUBuffer*>("plane_" + name + "_ib", buffers.indexBuffer);
    // The barrel runs along +Y and the lens cap sits at its far end, so
    // publish that so a spotlight can be placed at the lens rather than
    // guessing an offset that drifts from the model.
    context.Set("plane_" + name,
               nlohmann::json{{"vertex_count", vertex_count},
                             {"index_count", index_count},
                             {"stride", 20},
                             {"lens_y", mesh.lensY},
                             {"emit_axis", "y"}});

    if (logger_) {
        logger_->Info("geometry.create_flashlight: '" + name + "' created (" +
                     std::to_string(vertex_count) + " verts, " +
                     std::to_string(index_count) + " indices)");
    }
}

}  // namespace sdl3cpp::services::impl
