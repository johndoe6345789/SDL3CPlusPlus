#include "services/interfaces/workflow/switchback/checkpoint/switchback_checkpoint_arrow_step.hpp"

#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow/switchback/checkpoint/switchback_arrow_transform.hpp"
#include "services/interfaces/workflow/switchback/checkpoint/switchback_marquee_mesh.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

void WorkflowSwitchbackCheckpointArrowStep::LoadMarquees(
    SDL_GPUDevice* device) {
    const GeometryPlaneMesh mesh =
        BuildSwitchbackMarqueeMesh(route_.Points());
    try {
        marquee_ = UploadGeometryPlaneMesh(device, mesh);
    } catch (const std::runtime_error& error) {
        if (logger_) {
            logger_->Error(std::string("switchback.checkpoint.marquee: ") +
                           error.what());
        }
        return;
    }
    marqueeIndexCount_ = static_cast<std::uint32_t>(mesh.indices.size());
    if (logger_) {
        logger_->Trace("WorkflowSwitchbackCheckpointArrowStep", "LoadMarquees",
                       "gantries=" + std::to_string(route_.Points().size()),
                       "Checkpoint marquees ready");
    }
}

void WorkflowSwitchbackCheckpointArrowStep::DrawMarquees(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (marqueeIndexCount_ == 0) return;
    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        Gta5ParameterOr(step, "marquee_pipeline_key", "gpu_pipeline_marquee"),
        nullptr);
    if (!pass || !cmd || !pipeline) return;

    const rendering::VertexUniformData vertex = BuildSwitchbackArrowUniforms(
        glm::mat4(1.f),
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.f)),
        context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.f)),
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.f)));

    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_PushGPUVertexUniformData(cmd, 0, &vertex, sizeof(vertex));
    SDL_GPUBufferBinding vertices = {};
    vertices.buffer = marquee_.vertexBuffer;
    SDL_BindGPUVertexBuffers(pass, 0, &vertices, 1);
    SDL_GPUBufferBinding indices = {};
    indices.buffer = marquee_.indexBuffer;
    SDL_BindGPUIndexBuffer(pass, &indices, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_DrawGPUIndexedPrimitives(pass, marqueeIndexCount_, 1, 0, 0, 0);
}

}  // namespace sdl3cpp::services::impl
