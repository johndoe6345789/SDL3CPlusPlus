#include "services/interfaces/workflow/rendering/workflow_lighting_setup_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <nlohmann/json.hpp>
#include <cmath>

namespace sdl3cpp::services::impl {

WorkflowLightingSetupStep::WorkflowLightingSetupStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowLightingSetupStep::GetPluginId() const {
    return "lighting.setup";
}

void WorkflowLightingSetupStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepParameterResolver params;

    auto getNum = [&](const char* name, float def) -> float {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::Number) ? static_cast<float>(p->numberValue) : def;
    };

    // Directional light (like sun) — direction points FROM light TO scene
    float dir_x = getNum("light_dir_x", -0.3f);
    float dir_y = getNum("light_dir_y", -0.8f);
    float dir_z = getNum("light_dir_z", -0.4f);
    float intensity = getNum("light_intensity", 1.5f);
    float color_r = getNum("light_color_r", 1.0f);
    float color_g = getNum("light_color_g", 0.95f);
    float color_b = getNum("light_color_b", 0.9f);

    // Ambient light — fills shadows, prevents pure black
    float ambient_r = getNum("ambient_r", 0.15f);
    float ambient_g = getNum("ambient_g", 0.15f);
    float ambient_b = getNum("ambient_b", 0.18f);
    float ambient_intensity = getNum("ambient_intensity", 1.0f);

    // Exposure control (UE5 EV100-inspired)
    float exposure = getNum("exposure", 1.0f);

    // Normalize direction
    float len = std::sqrt(dir_x * dir_x + dir_y * dir_y + dir_z * dir_z);
    if (len > 0.0001f) {
        dir_x /= len;
        dir_y /= len;
        dir_z /= len;
    }

    nlohmann::json lighting = {
        {"direction", {dir_x, dir_y, dir_z}},
        {"color", {color_r * intensity, color_g * intensity, color_b * intensity}},
        {"ambient", {ambient_r * ambient_intensity, ambient_g * ambient_intensity, ambient_b * ambient_intensity}},
        {"exposure", exposure}
    };

    context.Set("lighting.directional", lighting);

    if (logger_) {
        logger_->Trace("WorkflowLightingSetupStep", "Execute",
                       "dir=(" + std::to_string(dir_x) + "," + std::to_string(dir_y) + "," + std::to_string(dir_z) +
                       ") intensity=" + std::to_string(intensity) +
                       " exposure=" + std::to_string(exposure),
                       "Lighting configured");
    }
}

}  // namespace sdl3cpp::services::impl
