#include "services/interfaces/workflow/quake3/workflow_q3_bots_draw_step.hpp"
#include "services/interfaces/workflow/quake3/q3_bot_model_render.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <string>

namespace sdl3cpp::services::impl {

WorkflowQ3BotsDrawStep::WorkflowQ3BotsDrawStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3BotsDrawStep::GetPluginId() const {
    return "q3.bots.draw";
}

void WorkflowQ3BotsDrawStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;

    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        "gpu_pipeline_textured", nullptr);
    const auto* botsPtr = context.TryGet<nlohmann::json>("q3.bots");
    if (!pass || !cmd || !pipeline || !botsPtr || !botsPtr->is_array()) {
        return;
    }

    WorkflowStepParameterResolver params;
    auto getStr = [&](const char* k, const std::string& def) -> std::string {
        const auto* p = params.FindParameter(step, k);
        return (p && p->type == WorkflowParameterValue::Type::String)
                   ? p->stringValue : def;
    };
    BotModelPrefixes prefixes;
    prefixes.lower  = getStr("lower_prefix",  "lower");
    prefixes.upper  = getStr("upper_prefix",  "upper");
    prefixes.head   = getStr("head_prefix",   "head");
    prefixes.weapon = getStr("weapon_prefix", "weapon_mg");

    auto hasFrames = [&](const std::string& pfx) {
        return context.Get<int>("q3.md3." + pfx + "_num_frames", 0) > 0;
    };
    prefixes.hasUpper  = hasFrames(prefixes.upper);
    prefixes.hasHead   = hasFrames(prefixes.head);
    prefixes.hasWeapon = hasFrames(prefixes.weapon);
    if (!hasFrames(prefixes.lower)) return;  // nothing without lower

    auto view = context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.0f));
    auto proj = context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.0f));
    auto camPos =
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.0f));
    auto shadowVP =
        context.Get<glm::mat4>("render.shadow_vp", glm::mat4(1.0f));
    auto fu = context.Get<rendering::FragmentUniformData>(
        "render.frag_uniforms", rendering::FragmentUniformData{});
    fu.material[0] = 0.6f;
    fu.material[1] = 0.1f;  // roughness/metallic

    auto* shadowTex =
        context.Get<SDL_GPUTexture*>("shadow_depth_texture", nullptr);
    auto* shadowSamp =
        context.Get<SDL_GPUSampler*>("shadow_depth_sampler", nullptr);

    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    for (const auto& bot : *botsPtr) {
        DrawBotModelChain(bot, prefixes, view, proj, camPos, shadowVP, fu,
                          pass, cmd, shadowTex, shadowSamp, context);
    }
}

}  // namespace sdl3cpp::services::impl
