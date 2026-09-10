#include "services/interfaces/workflow/quake3/workflow_q3_pickups_draw_step.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <string>

namespace sdl3cpp::services::impl {

WorkflowQ3PickupsDrawStep::WorkflowQ3PickupsDrawStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

WorkflowQ3PickupsDrawStep::~WorkflowQ3PickupsDrawStep() {
    ReleasePickupQuadBuffers(quadBuffers_);
}

std::string WorkflowQ3PickupsDrawStep::GetPluginId() const {
    return "q3.pickups.draw";
}

void WorkflowQ3PickupsDrawStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;
    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        "gpu_pipeline_textured", nullptr);
    const auto* entities = context.TryGet<nlohmann::json>("bsp.entities");
    if (!pass || !cmd || !device || !pipeline || !entities ||
        !entities->is_array()) {
        return;
    }

    EnsurePickupQuadBuffers(device, quadBuffers_);
    EnsurePickupColorTexture(device, context, "q3_pickup_weapon", 255, 210,
                             40);
    EnsurePickupColorTexture(device, context, "q3_pickup_ammo", 255, 120,
                             45);
    EnsurePickupColorTexture(device, context, "q3_pickup_health", 40, 235,
                             85);
    EnsurePickupColorTexture(device, context, "q3_pickup_armor", 70, 150,
                             255);
    EnsurePickupColorTexture(device, context, "q3_pickup_powerup", 190, 90,
                             255);

    auto collected =
        context.Get<nlohmann::json>("q3.collected", nlohmann::json::object());
    auto view = context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.0f));
    auto proj = context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.0f));
    auto camPos =
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.0f));
    auto shadowVP =
        context.Get<glm::mat4>("render.shadow_vp", glm::mat4(1.0f));
    const float time =
        static_cast<float>(context.GetDouble("frame.elapsed", 0.0));

    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    DrawPickupEntities(*entities, collected, view, proj, camPos, shadowVP,
                       time, pass, cmd, quadBuffers_, context);
}

}  // namespace sdl3cpp::services::impl
