#include "services/interfaces/workflow/stunts/render/stunts_car_draw_step.hpp"

#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow/stunts/stunts_step_params.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstring>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

void BindTexture(SDL_GPURenderPass* pass, const WorkflowContext& context,
                 const std::string& key) {
    auto* texture = context.Get<SDL_GPUTexture*>(key + "_gpu", nullptr);
    auto* sampler = context.Get<SDL_GPUSampler*>(key + "_sampler", nullptr);
    if (!texture || !sampler) return;
    const SDL_GPUTextureSamplerBinding binding = {texture, sampler};
    SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
}

void DrawMesh(SDL_GPURenderPass* pass, const StuntsGpuMesh& mesh) {
    if (!mesh.vertexBuffer || !mesh.indexBuffer || mesh.indexCount == 0) {
        return;
    }
    SDL_GPUBufferBinding vertices = {};
    vertices.buffer = mesh.vertexBuffer;
    SDL_BindGPUVertexBuffers(pass, 0, &vertices, 1);
    SDL_GPUBufferBinding indices = {};
    indices.buffer = mesh.indexBuffer;
    SDL_BindGPUIndexBuffer(pass, &indices, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_DrawGPUIndexedPrimitives(pass, mesh.indexCount, 1, 0, 0, 0);
}

}  // namespace

WorkflowStuntsCarDrawStep::WorkflowStuntsCarDrawStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<StuntsWorldState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowStuntsCarDrawStep::GetPluginId() const {
    return "stunts.car.draw";
}

void WorkflowStuntsCarDrawStep::Execute(const WorkflowStepDefinition& step,
                                        WorkflowContext& context) {
    if (context.GetBool("frame_skip", false) || !state_->carBodyLoaded) {
        return;
    }
    // A driver does not see their own car's body from inside it.
    if (context.Get<std::string>("stunts.camera_mode", "chase") ==
        "cockpit") {
        return;
    }
    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline =
        context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline_textured", nullptr);
    if (!pass || !cmd || !pipeline) return;

    const auto* at = context.TryGet<glm::vec3>("stunts.car_pos");
    if (!at) return;
    const float heading = context.Get<float>("stunts.car_heading", 0.f);
    // The drive model's `heading` moves the car along
    // (cos(heading), 0, sin(heading)) in world space. The shape's own
    // +Z is its forward axis, and glm::rotate(_, theta, +Y) sends
    // local +Z to world (sin(theta), 0, cos(theta)) -- so matching
    // the two needs theta = pi/2 - heading, not heading - pi/2: that
    // sign was backwards, turning the model the wrong way as it
    // steered and facing it opposite its travel direction at spawn.
    const glm::mat4 model =
        glm::translate(glm::mat4(1.f), *at) *
        glm::rotate(glm::mat4(1.f), 1.57079632679f - heading,
                   glm::vec3(0.f, 1.f, 0.f));
    const glm::mat4 view =
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.f));
    const glm::mat4 proj =
        context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.f));
    const glm::vec3 camPos =
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.f));

    rendering::VertexUniformData vertex = {};
    std::memcpy(vertex.mvp, glm::value_ptr(proj * view * model),
               sizeof(float) * 16);
    std::memcpy(vertex.model_mat, glm::value_ptr(model), sizeof(float) * 16);
    vertex.normal[1] = 1.f;
    vertex.uv_scale[0] = vertex.uv_scale[1] = 1.f;
    vertex.camera_pos[0] = camPos.x;
    vertex.camera_pos[1] = camPos.y;
    vertex.camera_pos[2] = camPos.z;
    std::memcpy(vertex.shadow_vp,
               glm::value_ptr(context.Get<glm::mat4>("render.shadow_vp",
                                                    glm::mat4(1.f))),
               sizeof(float) * 16);
    auto fragment = context.Get<rendering::FragmentUniformData>(
        "render.frag_uniforms", rendering::FragmentUniformData{});

    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_PushGPUVertexUniformData(cmd, 0, &vertex, sizeof(vertex));
    SDL_PushGPUFragmentUniformData(cmd, 0, &fragment, sizeof(fragment));
    BindTexture(pass, context,
               StuntsStringOr(step, "texture_key", nullptr,
                             "stunts_palette"));

    if (!traced_ && logger_) {
        traced_ = true;
        logger_->Info("stunts.car.draw: first frame, panels " +
                      std::to_string(state_->carPanels.indexCount) +
                      " indices, wheels " +
                      std::to_string(state_->carWheels.indexCount));
    }
    DrawMesh(pass, state_->carPanels);
    DrawMesh(pass, state_->carWheels);
}

}  // namespace sdl3cpp::services::impl
