#include "services/interfaces/workflow/rendering/workflow_draw_textured_box_step.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>
#include <cstring>

namespace sdl3cpp::services::impl {

WorkflowDrawTexturedBoxStep::WorkflowDrawTexturedBoxStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowDrawTexturedBoxStep::GetPluginId() const {
    return "draw.textured_box";
}

void WorkflowDrawTexturedBoxStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
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

    float pos_x = getNum("pos_x", 0.0f);
    float pos_y = getNum("pos_y", 0.0f);
    float pos_z = getNum("pos_z", 0.0f);
    float size_x = getNum("size_x", 1.0f);
    float size_y = getNum("size_y", 1.0f);
    float size_z = getNum("size_z", 1.0f);
    float uv_density = getNum("uv_density", 1.0f);
    float roughness = getNum("roughness", 0.8f);
    float metallic = getNum("metallic", 0.0f);
    const std::string tex_name = getStr("texture", "walls_texture");
    const std::string body_name = getStr("body", "");

    // Get render state
    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd = context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline_textured", nullptr);
    if (!pass || !cmd || !pipeline) return;

    // Get unit plane buffers (1x1 plane on XZ, normal +Y)
    auto* vb = context.Get<SDL_GPUBuffer*>("plane_unit_vb", nullptr);
    auto* ib = context.Get<SDL_GPUBuffer*>("plane_unit_ib", nullptr);
    const auto* mesh_meta = context.TryGet<nlohmann::json>("plane_unit");
    if (!vb || !ib || !mesh_meta) {
        if (logger_) logger_->Warn("draw.textured_box: unit plane not found in context");
        return;
    }
    uint32_t index_count = (*mesh_meta)["index_count"];

    // Get texture + sampler
    auto* texture = context.Get<SDL_GPUTexture*>(tex_name + "_gpu", nullptr);
    auto* sampler = context.Get<SDL_GPUSampler*>(tex_name + "_sampler", nullptr);
    if (!texture || !sampler) {
        if (logger_) logger_->Warn("draw.textured_box: texture '" + tex_name + "' not found");
        return;
    }

    // Camera matrices + world position from pre-computed context values
    auto view = context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.0f));
    auto proj = context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.0f));
    auto camPos = context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.0f));

    // Read pre-computed body transform from physics.sync_transforms step
    glm::mat4 bodyRotation(1.0f);
    if (!body_name.empty()) {
        const auto* sync = context.TryGet<nlohmann::json>("body_sync_" + body_name);
        if (sync) {
            auto p = (*sync)["pos"].get<std::vector<float>>();
            pos_x = p[0]; pos_y = p[1]; pos_z = p[2];

            auto rot = (*sync)["rotation"].get<std::vector<float>>();
            if (rot.size() == 16) bodyRotation = glm::make_mat4(rot.data());
        }
    }

    // Fragment uniforms: pre-computed PBR lighting from context + per-draw material
    auto fu = context.Get<rendering::FragmentUniformData>("render.frag_uniforms", rendering::FragmentUniformData{});
    fu.material[0] = roughness;
    fu.material[1] = metallic;

    // Half-extents
    float hx = size_x * 0.5f;
    float hy = size_y * 0.5f;
    float hz = size_z * 0.5f;

    // 6 box faces
    struct BoxFace {
        glm::vec3 offset;
        glm::vec3 normal;
        glm::mat4 rotation;
        float scaleW, scaleD;
        float uvW, uvH;
    };

    glm::mat4 rotNone(1.0f);
    glm::mat4 rotDown = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1, 0, 0));
    glm::mat4 rotNorth = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0));
    glm::mat4 rotSouth = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0));

    glm::mat4 rotEast(1.0f);
    rotEast[0] = glm::vec4( 0, 0, 1, 0);
    rotEast[1] = glm::vec4( 1, 0, 0, 0);
    rotEast[2] = glm::vec4( 0, 1, 0, 0);
    rotEast[3] = glm::vec4( 0, 0, 0, 1);

    glm::mat4 rotWest(1.0f);
    rotWest[0] = glm::vec4( 0, 0,-1, 0);
    rotWest[1] = glm::vec4(-1, 0, 0, 0);
    rotWest[2] = glm::vec4( 0, 1, 0, 0);
    rotWest[3] = glm::vec4( 0, 0, 0, 1);

    BoxFace faces[6] = {
        { glm::vec3(0, hy, 0),   glm::vec3(0,1,0),  rotNone,  size_x, size_z, size_x*uv_density, size_z*uv_density },
        { glm::vec3(0, -hy, 0),  glm::vec3(0,-1,0), rotDown,  size_x, size_z, size_x*uv_density, size_z*uv_density },
        { glm::vec3(0, 0, -hz),  glm::vec3(0,0,-1), rotNorth, size_x, size_y, size_x*uv_density, size_y*uv_density },
        { glm::vec3(0, 0, hz),   glm::vec3(0,0,1),  rotSouth, size_x, size_y, size_x*uv_density, size_y*uv_density },
        { glm::vec3(hx, 0, 0),   glm::vec3(1,0,0),  rotEast,  size_z, size_y, size_z*uv_density, size_y*uv_density },
        { glm::vec3(-hx, 0, 0),  glm::vec3(-1,0,0), rotWest,  size_z, size_y, size_z*uv_density, size_y*uv_density },
    };

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

    // Shadow VP matrix for vertex uniforms
    auto shadowVP = context.Get<glm::mat4>("render.shadow_vp", glm::mat4(1.0f));

    glm::vec3 center(pos_x, pos_y, pos_z);

    for (int i = 0; i < 6; ++i) {
        const auto& f = faces[i];

        glm::mat4 model_mat = glm::translate(glm::mat4(1.0f), center)
                        * bodyRotation
                        * glm::translate(glm::mat4(1.0f), f.offset)
                        * f.rotation
                        * glm::scale(glm::mat4(1.0f), glm::vec3(f.scaleW, 1.0f, f.scaleD));

        glm::mat4 mvp = proj * view * model_mat;
        glm::vec3 worldNormal = glm::vec3(bodyRotation * glm::vec4(f.normal, 0.0f));

        rendering::VertexUniformData vu = {};
        std::memcpy(vu.mvp, glm::value_ptr(mvp), sizeof(float) * 16);
        std::memcpy(vu.model_mat, glm::value_ptr(model_mat), sizeof(float) * 16);
        vu.normal[0] = worldNormal.x; vu.normal[1] = worldNormal.y; vu.normal[2] = worldNormal.z;
        vu.uv_scale[0] = f.uvW; vu.uv_scale[1] = f.uvH;
        vu.camera_pos[0] = camPos.x; vu.camera_pos[1] = camPos.y; vu.camera_pos[2] = camPos.z;
        std::memcpy(vu.shadow_vp, glm::value_ptr(shadowVP), sizeof(float) * 16);

        SDL_PushGPUVertexUniformData(cmd, 0, &vu, sizeof(vu));
        SDL_PushGPUFragmentUniformData(cmd, 0, &fu, sizeof(fu));
        SDL_DrawGPUIndexedPrimitives(pass, index_count, 1, 0, 0, 0);
    }
}

}  // namespace sdl3cpp::services::impl
