#include "services/interfaces/workflow/rendering/workflow_frame_draw_bodies_step.hpp"
#include "services/interfaces/workflow/rendering/frame_draw_bodies_helpers.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

WorkflowFrameDrawBodiesStep::WorkflowFrameDrawBodiesStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowFrameDrawBodiesStep::GetPluginId() const {
    return "frame.gpu.draw_bodies";
}

void WorkflowFrameDrawBodiesStep::Execute(const WorkflowStepDefinition& step,
                                          WorkflowContext& context) {
    // Skip if frame wasn't acquired.
    if (context.GetBool("frame_skip", false)) return;

    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline =
        context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline", nullptr);
    auto* vbuf = context.Get<SDL_GPUBuffer*>("gpu_vertex_buffer", nullptr);
    auto* ibuf = context.Get<SDL_GPUBuffer*>("gpu_index_buffer", nullptr);
    if (!pass || !cmd || !pipeline || !vbuf || !ibuf) return;

    // Get camera matrices (pre-computed by render.prepare step).
    auto view = context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.0f));
    auto proj = context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.0f));
    const glm::mat4 viewProj = proj * view;

    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    SDL_GPUBufferBinding vbufBinding = {};
    vbufBinding.buffer = vbuf;
    vbufBinding.offset = 0;
    SDL_BindGPUVertexBuffers(pass, 0, &vbufBinding, 1);

    SDL_GPUBufferBinding ibufBinding = {};
    ibufBinding.buffer = ibuf;
    ibufBinding.offset = 0;
    SDL_BindGPUIndexBuffer(pass, &ibufBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    auto bodies =
        context.Get<nlohmann::json>("physics_bodies", nlohmann::json::array());
    const float time =
        static_cast<float>(context.GetDouble("frame.elapsed", 0.0));
    uint32_t drawCalls = 0;

    for (const auto& nameVal : bodies) {
        if (DrawOnePhysicsBody(pass, cmd, context, nameVal.get<std::string>(),
                              viewProj, time)) {
            drawCalls++;
        }
    }

    context.Set<uint32_t>("frame_draw_calls", drawCalls);

    // Track elapsed time.
    double elapsed = context.GetDouble("frame.elapsed", 0.0);
    context.Set<double>("frame.elapsed", elapsed + 1.0 / 60.0);
}

}  // namespace sdl3cpp::services::impl
