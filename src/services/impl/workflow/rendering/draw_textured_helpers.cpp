#include "services/interfaces/workflow/rendering/draw_textured_helpers.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>

#include <cstring>

namespace sdl3cpp::services::impl {

DrawTexturedParams ReadDrawTexturedParams(const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver params;

    auto getStr = [&](const char* name, const std::string& def) {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::String)
                   ? p->stringValue
                   : def;
    };
    auto getNum = [&](const char* name, float def) -> float {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::Number)
                   ? static_cast<float>(p->numberValue)
                   : def;
    };

    DrawTexturedParams result;
    result.meshName    = getStr("mesh", "plane");
    result.textureName = getStr("texture", "texture");
    result.facing       = getStr("facing", "");
    result.posX = getNum("pos_x", 0.0f);
    result.posY = getNum("pos_y", 0.0f);
    result.posZ = getNum("pos_z", 0.0f);
    result.rotX = getNum("rot_x", 0.0f);
    result.rotY = getNum("rot_y", 0.0f);
    result.rotZ = getNum("rot_z", 0.0f);
    result.scale     = getNum("scale", 1.0f);
    result.roughness = getNum("roughness", 0.8f);
    result.metallic  = getNum("metallic", 0.0f);
    return result;
}

DrawTexturedTransform BuildDrawTexturedTransform(
    const DrawTexturedParams& params) {
    DrawTexturedTransform result;
    result.model = glm::mat4(1.0f);
    result.normal = glm::vec3(0.0f, 1.0f, 0.0f);

    if (!params.facing.empty()) {
        const glm::vec3 pos(params.posX, params.posY, params.posZ);
        const std::string& facing = params.facing;

        if (facing == "up") {
            result.model = glm::translate(glm::mat4(1.0f), pos);
            result.normal = glm::vec3(0, 1, 0);
        } else if (facing == "down") {
            result.model = glm::translate(glm::mat4(1.0f), pos);
            result.model = glm::rotate(result.model, glm::radians(180.0f),
                                       glm::vec3(1, 0, 0));
            result.normal = glm::vec3(0, -1, 0);
        } else if (facing == "north") {
            result.model = glm::translate(glm::mat4(1.0f), pos);
            result.model = glm::rotate(result.model, glm::radians(-90.0f),
                                       glm::vec3(1, 0, 0));
            result.normal = glm::vec3(0, 0, -1);
        } else if (facing == "south") {
            result.model = glm::translate(glm::mat4(1.0f), pos);
            result.model = glm::rotate(result.model, glm::radians(90.0f),
                                       glm::vec3(1, 0, 0));
            result.normal = glm::vec3(0, 0, 1);
        } else if (facing == "east") {
            glm::mat4 rot(1.0f);
            rot[0] = glm::vec4(0, 0, 1, 0);
            rot[1] = glm::vec4(-1, 0, 0, 0);
            rot[2] = glm::vec4(0, 1, 0, 0);
            rot[3] = glm::vec4(0, 0, 0, 1);
            result.model = glm::translate(glm::mat4(1.0f), pos) * rot;
            result.normal = glm::vec3(1, 0, 0);
        } else if (facing == "west") {
            glm::mat4 rot(1.0f);
            rot[0] = glm::vec4(0, 0, -1, 0);
            rot[1] = glm::vec4(1, 0, 0, 0);
            rot[2] = glm::vec4(0, 1, 0, 0);
            rot[3] = glm::vec4(0, 0, 0, 1);
            result.model = glm::translate(glm::mat4(1.0f), pos) * rot;
            result.normal = glm::vec3(-1, 0, 0);
        }

        if (params.scale != 1.0f) {
            result.model = glm::scale(result.model, glm::vec3(params.scale));
        }
    } else {
        result.model = glm::translate(result.model,
                                      glm::vec3(params.posX, params.posY,
                                               params.posZ));
        if (params.rotX != 0.0f) {
            result.model = glm::rotate(result.model, glm::radians(params.rotX),
                                       glm::vec3(1, 0, 0));
        }
        if (params.rotY != 0.0f) {
            result.model = glm::rotate(result.model, glm::radians(params.rotY),
                                       glm::vec3(0, 1, 0));
        }
        if (params.rotZ != 0.0f) {
            result.model = glm::rotate(result.model, glm::radians(params.rotZ),
                                       glm::vec3(0, 0, 1));
        }
        if (params.scale != 1.0f) {
            result.model = glm::scale(result.model, glm::vec3(params.scale));
        }
    }
    return result;
}

void BuildDrawTexturedUniforms(const WorkflowContext& context,
                               const DrawTexturedTransform& transform,
                               float roughness, float metallic,
                               rendering::VertexUniformData& vu,
                               rendering::FragmentUniformData& fu) {
    auto view = context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.0f));
    auto proj = context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.0f));
    auto camPos =
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.0f));
    const glm::mat4 mvp = proj * view * transform.model;
    auto shadowVP =
        context.Get<glm::mat4>("render.shadow_vp", glm::mat4(1.0f));

    vu = {};
    std::memcpy(vu.mvp, glm::value_ptr(mvp), sizeof(float) * 16);
    std::memcpy(vu.model_mat, glm::value_ptr(transform.model),
               sizeof(float) * 16);
    vu.normal[0] = transform.normal.x;
    vu.normal[1] = transform.normal.y;
    vu.normal[2] = transform.normal.z;
    vu.uv_scale[0] = 1.0f;
    vu.uv_scale[1] = 1.0f;
    vu.camera_pos[0] = camPos.x;
    vu.camera_pos[1] = camPos.y;
    vu.camera_pos[2] = camPos.z;
    std::memcpy(vu.shadow_vp, glm::value_ptr(shadowVP), sizeof(float) * 16);

    fu = context.Get<rendering::FragmentUniformData>(
        "render.frag_uniforms", rendering::FragmentUniformData{});
    fu.material[0] = roughness;
    fu.material[1] = metallic;
}

void BindDrawTexturedSamplers(SDL_GPURenderPass* pass,
                              const WorkflowContext& context,
                              SDL_GPUTexture* texture,
                              SDL_GPUSampler* sampler) {
    auto* shadow_tex =
        context.Get<SDL_GPUTexture*>("shadow_depth_texture", nullptr);
    auto* shadow_samp =
        context.Get<SDL_GPUSampler*>("shadow_depth_sampler", nullptr);
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
}

bool ResolveDrawTexturedResources(const WorkflowContext& context,
                                  const DrawTexturedParams& params,
                                  const std::shared_ptr<ILogger>& logger,
                                  DrawTexturedResources& out) {
    out.vb = context.Get<SDL_GPUBuffer*>("plane_" + params.meshName + "_vb",
                                         nullptr);
    out.ib = context.Get<SDL_GPUBuffer*>("plane_" + params.meshName + "_ib",
                                         nullptr);
    const auto* mesh_meta =
        context.TryGet<nlohmann::json>("plane_" + params.meshName);
    if (!out.vb || !out.ib || !mesh_meta) {
        if (logger) {
            logger->Warn("draw.textured: Mesh '" + params.meshName +
                        "' not found in context");
        }
        return false;
    }
    out.indexCount = (*mesh_meta)["index_count"];

    out.texture = context.Get<SDL_GPUTexture*>(
        params.textureName + "_gpu", nullptr);
    out.sampler = context.Get<SDL_GPUSampler*>(
        params.textureName + "_sampler", nullptr);
    if (!out.texture || !out.sampler) {
        if (logger) {
            logger->Warn("draw.textured: Texture '" + params.textureName +
                        "' not found in context");
        }
        return false;
    }
    return true;
}

}  // namespace sdl3cpp::services::impl
