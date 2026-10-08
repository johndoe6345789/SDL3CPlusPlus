#include "services/interfaces/workflow/switchback/checkpoint/switchback_checkpoint_arrow_step.hpp"

#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow/switchback/checkpoint/switchback_arrow_transform.hpp"
#include "services/interfaces/workflow_context.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr float kArrowHeight = 3.2f;

}  // namespace

void WorkflowSwitchbackCheckpointArrowStep::DrawArrow(
    const WorkflowStepDefinition& step, WorkflowContext& context,
    const glm::vec3& car) {
    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        Gta5ParameterOr(step, "pipeline_key", "gpu_pipeline_arrow"), nullptr);
    if (!pass || !cmd || !pipeline) return;

    const glm::mat4 model =
        BuildSwitchbackArrowModel(car, route_.Target(), kArrowHeight);
    const rendering::VertexUniformData vertex = BuildSwitchbackArrowUniforms(
        model,
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.f)),
        context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.f)),
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.f)));

    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_PushGPUVertexUniformData(cmd, 0, &vertex, sizeof(vertex));
    SDL_GPUBufferBinding vertices = {};
    vertices.buffer = arrow_.vertexBuffer;
    SDL_BindGPUVertexBuffers(pass, 0, &vertices, 1);
    SDL_GPUBufferBinding indices = {};
    indices.buffer = arrow_.indexBuffer;
    SDL_BindGPUIndexBuffer(pass, &indices, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_DrawGPUIndexedPrimitives(pass, arrowIndexCount_, 1, 0, 0, 0);
}

}  // namespace sdl3cpp::services::impl
