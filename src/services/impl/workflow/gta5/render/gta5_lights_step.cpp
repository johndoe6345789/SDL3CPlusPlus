#include "services/interfaces/workflow/gta5/render/gta5_lights_step.hpp"

#include "services/interfaces/workflow/gta5/render/gta5_lights.hpp"
#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5LightsDrawStep::WorkflowGta5LightsDrawStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5LightsDrawStep::GetPluginId() const {
    return "gta5.lights.draw";
}

void WorkflowGta5LightsDrawStep::Load(const WorkflowStepDefinition& step,
                                      SDL_GPUDevice* device) {
    tried_ = true;
    const std::string dir = Gta5ParameterOr(step, "lights_dir", "");
    const auto lights = LoadGta5DistantLights(dir);
    const auto sprites = BuildGta5LightSprites(lights);
    if (logger_) {
        logger_->Info("gta5.lights.draw: " + std::to_string(lights.size()) +
                      " distant lights from '" + dir + "'");
    }
    if (sprites.empty()) return;
    const auto bytes = static_cast<Uint32>(sprites.size() * sizeof(sprites[0]));
    SDL_GPUBufferCreateInfo info = {};
    info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    info.size = bytes;
    vertices_ = SDL_CreateGPUBuffer(device, &info);
    // Staged now, copied when gta5.tiles.cull next submits.
    if (vertices_ && state_->uploads.StageBuffer(device, sprites.data(), bytes,
                                                 vertices_, 0)) {
        count_ = static_cast<std::uint32_t>(sprites.size());
    }
}

void WorkflowGta5LightsDrawStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    if (!state_ || context.GetBool("frame_skip", false)) return;
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        "gpu_pipeline_gta5_lights", nullptr);
    if (!device || !pass || !cmd || !pipeline) return;
    if (!tried_) Load(step, device);  // at start-up, not at dusk
    const float night = context.Get<float>("gta5.time.night", 0.f);
    const bool ready = staged_;
    staged_ = count_ > 0;
    if (!ready || night < 0.02f) return;
    const auto view =
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.f));
    Gta5LightUniforms u;
    u.viewProj =
        context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.f)) * view;
    u.right = glm::vec4(view[0][0], view[1][0], view[2][0], 0.f);
    u.up = glm::vec4(view[0][1], view[1][1], view[2][1], 0.f);
    u.camera = glm::vec4(
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.f)), 1.f);
    u.params = glm::vec4(night, Gta5NumberOr(step, "size", 0.0022f),
                         Gta5NumberOr(step, "fade_from", 40.f),
                         Gta5NumberOr(step, "fade_to", 120.f));
    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    const SDL_GPUBufferBinding binding = {vertices_, 0};
    SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
    SDL_PushGPUVertexUniformData(cmd, 0, &u, sizeof(u));
    SDL_DrawGPUPrimitives(pass, count_, 1, 0, 0);
}

}  // namespace sdl3cpp::services::impl
