#include "services/interfaces/workflow/rendering/workflow_draw_map_step.hpp"
#include "services/interfaces/workflow/rendering/draw_map_helpers.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

WorkflowDrawMapStep::WorkflowDrawMapStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowDrawMapStep::GetPluginId() const {
    return "draw.map";
}

void WorkflowDrawMapStep::Execute(const WorkflowStepDefinition& step,
                                  WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;

    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    if (!pass || !cmd) return;

    const auto* mapNodes = context.TryGet<nlohmann::json>("map.nodes");
    if (!mapNodes || !mapNodes->is_array() || mapNodes->empty()) return;

    // Detect BSP mode: prefer gpu_pipeline_bsp if available.
    auto* bspPipeline =
        context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline_bsp", nullptr);
    auto* texPipeline =
        context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline_textured", nullptr);
    const bool isBsp = (bspPipeline != nullptr);
    auto* pipeline   = isBsp ? bspPipeline : texPipeline;
    if (!pipeline) return;

    const DrawMapTextureConfig config = ReadDrawMapTextureConfig(step);

    auto view   = context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.0f));
    auto proj   = context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.0f));
    auto camPos = context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.0f));
    auto shadowVP = context.Get<glm::mat4>("render.shadow_vp", glm::mat4(1.0f));

    auto fu = context.Get<rendering::FragmentUniformData>(
        "render.frag_uniforms", rendering::FragmentUniformData{});
    fu.material[0] = config.roughness;
    fu.material[1] = config.metallic;

    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    const rendering::VertexUniformData vu =
        BuildDrawMapVertexUniforms(view, proj, camPos, shadowVP);

    if (isBsp) {
        DrawBspMapGeometry(pass, cmd, context, *mapNodes, fu, vu,
                           config.defaultTexture);
    } else {
        DrawLegacyMapGeometry(pass, cmd, context, *mapNodes, vu, fu, config);
    }
}

}  // namespace sdl3cpp::services::impl
