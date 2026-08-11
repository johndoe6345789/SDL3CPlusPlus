#include "services/interfaces/workflow/rendering/workflow_draw_textured_step.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>
#include <cstring>
#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowDrawTexturedStep::WorkflowDrawTexturedStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowDrawTexturedStep::GetPluginId() const {
    return "draw.textured";
}

void WorkflowDrawTexturedStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
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

    const std::string mesh_name = getStr("mesh", "plane");
    const std::string tex_name = getStr("texture", "texture");
    const std::string facing = getStr("facing", "");
    const float pos_x = getNum("pos_x", 0.0f);
    const float pos_y = getNum("pos_y", 0.0f);
    const float pos_z = getNum("pos_z", 0.0f);
    const float rot_x = getNum("rot_x", 0.0f);
    const float rot_y = getNum("rot_y", 0.0f);
    const float rot_z = getNum("rot_z", 0.0f);
    const float scale = getNum("scale", 1.0f);
    const float roughness = getNum("roughness", 0.8f);
    const float metallic = getNum("metallic", 0.0f);

    // Get render state from context
    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd = context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline_textured", nullptr);
    if (!pass || !cmd || !pipeline) {
        if (logger_) logger_->Warn("draw.textured: Missing render pass, command buffer, or textured pipeline");
        return;
    }

    // Get mesh buffers
    auto* vb = context.Get<SDL_GPUBuffer*>("plane_" + mesh_name + "_vb", nullptr);
    auto* ib = context.Get<SDL_GPUBuffer*>("plane_" + mesh_name + "_ib", nullptr);
    const auto* mesh_meta = context.TryGet<nlohmann::json>("plane_" + mesh_name);
    if (!vb || !ib || !mesh_meta) {
        if (logger_) logger_->Warn("draw.textured: Mesh '" + mesh_name + "' not found in context");
        return;
    }
    uint32_t index_count = (*mesh_meta)["index_count"];

    // Get texture and sampler
    auto* texture = context.Get<SDL_GPUTexture*>(tex_name + "_gpu", nullptr);
    auto* sampler = context.Get<SDL_GPUSampler*>(tex_name + "_sampler", nullptr);
    if (!texture || !sampler) {
        if (logger_) logger_->Warn("draw.textured: Texture '" + tex_name + "' not found in context");
        return;
    }

    // Build model matrix
    glm::mat4 model = glm::mat4(1.0f);

    if (!facing.empty()) {
        glm::vec3 pos(pos_x, pos_y, pos_z);

        if (facing == "up") {
            model = glm::translate(glm::mat4(1.0f), pos);
        } else if (facing == "down") {
            model = glm::translate(glm::mat4(1.0f), pos);
            model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1, 0, 0));
        } else if (facing == "north") {
            model = glm::translate(glm::mat4(1.0f), pos);
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
        } else if (facing == "south") {
            model = glm::translate(glm::mat4(1.0f), pos);
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1, 0, 0));
        } else if (facing == "east") {
            glm::mat4 rot(1.0f);
            rot[0] = glm::vec4( 0, 0, 1, 0);
            rot[1] = glm::vec4(-1, 0, 0, 0);
            rot[2] = glm::vec4( 0, 1, 0, 0);
            rot[3] = glm::vec4( 0, 0, 0, 1);
            model = glm::translate(glm::mat4(1.0f), pos) * rot;
        } else if (facing == "west") {
            glm::mat4 rot(1.0f);
            rot[0] = glm::vec4(0, 0, -1, 0);
            rot[1] = glm::vec4(1,  0, 0, 0);
            rot[2] = glm::vec4(0,  1, 0, 0);
            rot[3] = glm::vec4(0,  0, 0, 1);
            model = glm::translate(glm::mat4(1.0f), pos) * rot;
        }

        if (scale != 1.0f) model = glm::scale(model, glm::vec3(scale));
    } else {
        model = glm::translate(model, glm::vec3(pos_x, pos_y, pos_z));
        if (rot_x != 0.0f) model = glm::rotate(model, glm::radians(rot_x), glm::vec3(1, 0, 0));
        if (rot_y != 0.0f) model = glm::rotate(model, glm::radians(rot_y), glm::vec3(0, 1, 0));
        if (rot_z != 0.0f) model = glm::rotate(model, glm::radians(rot_z), glm::vec3(0, 0, 1));
        if (scale != 1.0f) model = glm::scale(model, glm::vec3(scale));
    }

    // Camera + shadow state from pre-computed context values
    auto view = context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.0f));
    auto proj = context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.0f));
    auto camPos = context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.0f));
    glm::mat4 mvp = proj * view * model;
    auto shadowVP = context.Get<glm::mat4>("render.shadow_vp", glm::mat4(1.0f));

    // Surface normal
    glm::vec3 surfaceNormal(0.0f, 1.0f, 0.0f);
    if (facing == "up")         surfaceNormal = glm::vec3( 0,  1,  0);
    else if (facing == "down")  surfaceNormal = glm::vec3( 0, -1,  0);
    else if (facing == "north") surfaceNormal = glm::vec3( 0,  0, -1);
    else if (facing == "south") surfaceNormal = glm::vec3( 0,  0,  1);
    else if (facing == "east")  surfaceNormal = glm::vec3( 1,  0,  0);
    else if (facing == "west")  surfaceNormal = glm::vec3(-1,  0,  0);

    // Vertex uniform: manual struct fill
    rendering::VertexUniformData vu = {};
    std::memcpy(vu.mvp, glm::value_ptr(mvp), sizeof(float) * 16);
    std::memcpy(vu.model_mat, glm::value_ptr(model), sizeof(float) * 16);
    vu.normal[0] = surfaceNormal.x; vu.normal[1] = surfaceNormal.y; vu.normal[2] = surfaceNormal.z;
    vu.uv_scale[0] = 1.0f; vu.uv_scale[1] = 1.0f;
    vu.camera_pos[0] = camPos.x; vu.camera_pos[1] = camPos.y; vu.camera_pos[2] = camPos.z;
    std::memcpy(vu.shadow_vp, glm::value_ptr(shadowVP), sizeof(float) * 16);

    // Fragment uniform: pre-computed PBR lighting from context + per-draw material
    auto fu = context.Get<rendering::FragmentUniformData>("render.frag_uniforms", rendering::FragmentUniformData{});
    fu.material[0] = roughness;
    fu.material[1] = metallic;

    // Bind pipeline
    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    // Bind albedo + shadow textures
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
        SDL_GPUTextureSamplerBinding tex_binding = {};
        tex_binding.texture = texture;
        tex_binding.sampler = sampler;
        SDL_BindGPUFragmentSamplers(pass, 0, &tex_binding, 1);
    }

    // Bind mesh buffers
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
