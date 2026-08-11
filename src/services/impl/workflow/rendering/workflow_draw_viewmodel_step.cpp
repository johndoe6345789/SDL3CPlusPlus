#include "services/interfaces/workflow/rendering/workflow_draw_viewmodel_step.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>
#include <cstring>

namespace sdl3cpp::services::impl {

WorkflowDrawViewmodelStep::WorkflowDrawViewmodelStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowDrawViewmodelStep::GetPluginId() const {
    return "draw.viewmodel";
}

void WorkflowDrawViewmodelStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;

    WorkflowStepParameterResolver params;

    auto getStr = [&](const char* name, const std::string& def) -> std::string {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::String) ? p->stringValue : def;
    };
    auto getNum = [&](const char* name, float def) -> float {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::Number) ? static_cast<float>(p->numberValue) : def;
    };

    const std::string mesh_name = getStr("mesh", "model");
    const std::string tex_name = getStr("texture", "");
    // Viewmodel offset from camera (right, down, forward)
    const float offset_x = getNum("offset_x", 0.35f);
    const float offset_y = getNum("offset_y", -0.3f);
    const float offset_z = getNum("offset_z", -0.5f);
    const float model_scale = getNum("scale", 0.15f);
    const float rot_x = getNum("rot_x", 0.0f);
    const float rot_y = getNum("rot_y", 0.0f);
    const float rot_z = getNum("rot_z", 0.0f);
    const float roughness = getNum("roughness", 0.6f);
    const float metallic = getNum("metallic", 0.4f);

    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd = context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline_textured", nullptr);
    if (!pass || !cmd || !pipeline) return;

    // Get mesh buffers
    auto* vb = context.Get<SDL_GPUBuffer*>("plane_" + mesh_name + "_vb", nullptr);
    auto* ib = context.Get<SDL_GPUBuffer*>("plane_" + mesh_name + "_ib", nullptr);
    const auto* mesh_meta = context.TryGet<nlohmann::json>("plane_" + mesh_name);
    if (!vb || !ib || !mesh_meta) {
        if (logger_) logger_->Warn("draw.viewmodel: Mesh '" + mesh_name + "' not found");
        return;
    }
    uint32_t index_count = (*mesh_meta)["index_count"];

    // Build viewmodel MVP: rendered in camera-local space
    // The viewmodel uses its own near-field projection to prevent clipping
    auto viewMatrix = context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.0f));
    auto projMatrix = context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.0f));
    auto camPos = context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.0f));

    // Extract camera basis vectors
    glm::vec3 camRight = glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
    glm::vec3 camUp = glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);
    glm::vec3 camForward = -glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);

    // Position the viewmodel relative to the camera (locked to view)
    glm::vec3 modelPos = camPos + camRight * offset_x + camUp * offset_y + camForward * (-offset_z);

    // Build local rotation first (to orient the mesh correctly)
    glm::mat4 localRot = glm::mat4(1.0f);
    if (rot_x != 0.0f) localRot = glm::rotate(localRot, glm::radians(rot_x), glm::vec3(1, 0, 0));
    if (rot_y != 0.0f) localRot = glm::rotate(localRot, glm::radians(rot_y), glm::vec3(0, 1, 0));
    if (rot_z != 0.0f) localRot = glm::rotate(localRot, glm::radians(rot_z), glm::vec3(0, 0, 1));

    // Camera orientation matrix (maps local space to world-aligned-with-camera)
    glm::mat4 camOrient = glm::mat4(1.0f);
    camOrient[0] = glm::vec4(camRight, 0.0f);
    camOrient[1] = glm::vec4(camUp, 0.0f);
    camOrient[2] = glm::vec4(-camForward, 0.0f);  // -forward because camera looks down -Z

    // Final model: translate to position, orient to camera, apply local rotation, then scale
    glm::mat4 model = glm::translate(glm::mat4(1.0f), modelPos) * camOrient * localRot * glm::scale(glm::mat4(1.0f), glm::vec3(model_scale));

    glm::mat4 mvp = projMatrix * viewMatrix * model;

    // Surface normal pointing up from the viewmodel
    glm::vec3 surfaceNormal = camUp;

    rendering::VertexUniformData vu = {};
    std::memcpy(vu.mvp, glm::value_ptr(mvp), sizeof(float) * 16);
    std::memcpy(vu.model_mat, glm::value_ptr(model), sizeof(float) * 16);
    vu.normal[0] = surfaceNormal.x; vu.normal[1] = surfaceNormal.y; vu.normal[2] = surfaceNormal.z;
    vu.uv_scale[0] = 1.0f; vu.uv_scale[1] = 1.0f;
    vu.camera_pos[0] = camPos.x; vu.camera_pos[1] = camPos.y; vu.camera_pos[2] = camPos.z;
    auto shadowVP = context.Get<glm::mat4>("render.shadow_vp", glm::mat4(1.0f));
    std::memcpy(vu.shadow_vp, glm::value_ptr(shadowVP), sizeof(float) * 16);

    auto fu = context.Get<rendering::FragmentUniformData>("render.frag_uniforms", rendering::FragmentUniformData{});
    fu.material[0] = roughness;
    fu.material[1] = metallic;

    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    // Bind texture if specified, else use a default
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUSampler* sampler = nullptr;
    if (!tex_name.empty()) {
        texture = context.Get<SDL_GPUTexture*>(tex_name + "_gpu", nullptr);
        sampler = context.Get<SDL_GPUSampler*>(tex_name + "_sampler", nullptr);
    }
    // Fall back to floor texture or any available texture
    if (!texture) texture = context.Get<SDL_GPUTexture*>("floor_texture_gpu", nullptr);
    if (!sampler) sampler = context.Get<SDL_GPUSampler*>("floor_texture_sampler", nullptr);

    if (texture && sampler) {
        auto* shadow_tex = context.Get<SDL_GPUTexture*>("shadow_depth_texture", nullptr);
        auto* shadow_samp = context.Get<SDL_GPUSampler*>("shadow_depth_sampler", nullptr);
        if (shadow_tex && shadow_samp) {
            SDL_GPUTextureSamplerBinding bindings[2] = {};
            bindings[0].texture = texture;
            bindings[0].sampler = sampler;
            bindings[1].texture = shadow_tex;
            bindings[1].sampler = shadow_samp;
            SDL_BindGPUFragmentSamplers(pass, 0, bindings, 2);
        } else {
            SDL_GPUTextureSamplerBinding binding = {};
            binding.texture = texture;
            binding.sampler = sampler;
            SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
        }
    }

    SDL_GPUBufferBinding vb_binding = {};
    vb_binding.buffer = vb;
    SDL_BindGPUVertexBuffers(pass, 0, &vb_binding, 1);
    SDL_GPUBufferBinding ib_binding = {};
    ib_binding.buffer = ib;
    SDL_BindGPUIndexBuffer(pass, &ib_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_PushGPUVertexUniformData(cmd, 0, &vu, sizeof(vu));
    SDL_PushGPUFragmentUniformData(cmd, 0, &fu, sizeof(fu));
    SDL_DrawGPUIndexedPrimitives(pass, index_count, 1, 0, 0, 0);
}

}  // namespace sdl3cpp::services::impl
