#include "services/interfaces/workflow/rendering/workflow_frame_draw_bodies_step.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <btBulletDynamicsCommon.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <nlohmann/json.hpp>
#include <cstring>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowFrameDrawBodiesStep::WorkflowFrameDrawBodiesStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowFrameDrawBodiesStep::GetPluginId() const {
    return "frame.gpu.draw_bodies";
}

void WorkflowFrameDrawBodiesStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    // Skip if frame wasn't acquired
    if (context.GetBool("frame_skip", false)) return;

    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd = context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline", nullptr);
    auto* vbuf = context.Get<SDL_GPUBuffer*>("gpu_vertex_buffer", nullptr);
    auto* ibuf = context.Get<SDL_GPUBuffer*>("gpu_index_buffer", nullptr);

    if (!pass || !cmd || !pipeline || !vbuf || !ibuf) return;

    // Get camera matrices (pre-computed by render.prepare step)
    auto view = context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.0f));
    auto proj = context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.0f));
    glm::mat4 viewProj = proj * view;

    // Bind pipeline and buffers
    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    SDL_GPUBufferBinding vbufBinding = {};
    vbufBinding.buffer = vbuf;
    vbufBinding.offset = 0;
    SDL_BindGPUVertexBuffers(pass, 0, &vbufBinding, 1);

    SDL_GPUBufferBinding ibufBinding = {};
    ibufBinding.buffer = ibuf;
    ibufBinding.offset = 0;
    SDL_BindGPUIndexBuffer(pass, &ibufBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    // Get physics body registry
    auto bodies = context.Get<nlohmann::json>("physics_bodies", nlohmann::json::array());
    float time = static_cast<float>(context.GetDouble("frame.elapsed", 0.0));
    uint32_t drawCalls = 0;

    struct UniformData {
        float mvp[16];
    };

    for (const auto& nameVal : bodies) {
        std::string name = nameVal.get<std::string>();

        // Get visual info
        auto visual = context.Get<nlohmann::json>("physics_visual_" + name, nlohmann::json::object());
        if (!visual.value("visible", true)) continue;

        // Get physics body transform
        auto* body = context.Get<btRigidBody*>("physics_body_" + name, nullptr);
        if (!body) continue;

        btTransform transform;
        body->getMotionState()->getWorldTransform(transform);
        btVector3 pos = transform.getOrigin();
        btQuaternion rot = transform.getRotation();

        // Build model matrix from physics transform
        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(pos.x(), pos.y(), pos.z()));

        // Apply rotation from physics
        btMatrix3x3 rotMat(rot);
        glm::mat4 rotGlm(1.0f);
        for (int r = 0; r < 3; ++r) {
            btVector3 row = rotMat.getRow(r);
            rotGlm[0][r] = row.x();
            rotGlm[1][r] = row.y();
            rotGlm[2][r] = row.z();
        }
        model = model * rotGlm;

        // Apply spinning animation if flagged
        if (visual.value("spinning", false)) {
            float spinX = visual.value("spin_speed_x", 1.0f);
            float spinY = visual.value("spin_speed_y", 0.7f);
            model = glm::rotate(model, time * spinX, glm::vec3(1, 0, 0));
            model = glm::rotate(model, time * spinY, glm::vec3(0, 1, 0));
        }

        // Apply scale from visual info
        auto scaleArr = visual.value("scale", std::vector<float>{0.5f, 0.5f, 0.5f});
        if (scaleArr.size() >= 3) {
            model = glm::scale(model, glm::vec3(scaleArr[0], scaleArr[1], scaleArr[2]));
        }

        // Compute MVP and push
        glm::mat4 mvp = viewProj * model;
        UniformData uniforms;
        std::memcpy(uniforms.mvp, glm::value_ptr(mvp), sizeof(uniforms.mvp));
        SDL_PushGPUVertexUniformData(cmd, 0, &uniforms, sizeof(uniforms));

        SDL_DrawGPUIndexedPrimitives(pass, 36, 1, 0, 0, 0);
        drawCalls++;
    }

    context.Set<uint32_t>("frame_draw_calls", drawCalls);

    // Track elapsed time
    double elapsed = context.GetDouble("frame.elapsed", 0.0);
    context.Set<double>("frame.elapsed", elapsed + 1.0 / 60.0);
}

}  // namespace sdl3cpp::services::impl
