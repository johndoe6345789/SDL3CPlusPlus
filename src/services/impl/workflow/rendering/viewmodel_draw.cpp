#include "services/interfaces/workflow/rendering/viewmodel_draw.hpp"
#include "services/interfaces/workflow/rendering/viewmodel_transform.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>
#include <cstring>

namespace sdl3cpp::services::impl {

namespace {

std::string GetStrParam(const WorkflowStepParameterResolver& params,
                        const WorkflowStepDefinition& step, const char* name,
                        const std::string& def) {
    const auto* p = params.FindParameter(step, name);
    return (p && p->type == WorkflowParameterValue::Type::String)
               ? p->stringValue : def;
}

float GetNumParam(const WorkflowStepParameterResolver& params,
                  const WorkflowStepDefinition& step, const char* name,
                  float def) {
    const auto* p = params.FindParameter(step, name);
    return (p && p->type == WorkflowParameterValue::Type::Number)
               ? static_cast<float>(p->numberValue) : def;
}

}  // namespace

ViewmodelDrawParams ReadViewmodelDrawParams(
    const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver params;
    ViewmodelDrawParams out;
    out.meshName  = GetStrParam(params, step, "mesh", out.meshName);
    out.texName   = GetStrParam(params, step, "texture", out.texName);
    out.offsetX   = GetNumParam(params, step, "offset_x", out.offsetX);
    out.offsetY   = GetNumParam(params, step, "offset_y", out.offsetY);
    out.offsetZ   = GetNumParam(params, step, "offset_z", out.offsetZ);
    out.scale     = GetNumParam(params, step, "scale", out.scale);
    out.rotX      = GetNumParam(params, step, "rot_x", out.rotX);
    out.rotY      = GetNumParam(params, step, "rot_y", out.rotY);
    out.rotZ      = GetNumParam(params, step, "rot_z", out.rotZ);
    out.roughness = GetNumParam(params, step, "roughness", out.roughness);
    out.metallic  = GetNumParam(params, step, "metallic", out.metallic);
    return out;
}

std::optional<ViewmodelMesh> TryGetViewmodelMesh(
    const WorkflowContext& context, const std::string& meshName,
    const std::shared_ptr<ILogger>& logger) {
    ViewmodelMesh mesh;
    mesh.vertexBuffer =
        context.Get<SDL_GPUBuffer*>("plane_" + meshName + "_vb", nullptr);
    mesh.indexBuffer =
        context.Get<SDL_GPUBuffer*>("plane_" + meshName + "_ib", nullptr);
    const auto* meta =
        context.TryGet<nlohmann::json>("plane_" + meshName);

    if (!mesh.vertexBuffer || !mesh.indexBuffer || !meta) {
        if (logger) {
            logger->Warn("draw.viewmodel: Mesh '" + meshName +
                        "' not found");
        }
        return std::nullopt;
    }
    mesh.indexCount = (*meta)["index_count"];
    return mesh;
}

ViewmodelUniforms BuildViewmodelUniforms(const WorkflowContext& context,
                                         const ViewmodelDrawParams& params) {
    // Build viewmodel MVP: rendered in camera-local space. The viewmodel
    // uses its own near-field projection to prevent clipping.
    auto viewMatrix =
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.0f));
    auto projMatrix =
        context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.0f));
    auto camPos = context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.0f));

    // Shared with spotlight.update so a light attached to this model
    // starts exactly where the model is drawn.
    const auto basis = rendering::ExtractCameraBasis(viewMatrix);
    const glm::vec3 camUp = basis.up;
    const glm::mat4 model = rendering::BuildViewmodelMatrix(
        viewMatrix, camPos,
        glm::vec3(params.offsetX, params.offsetY, params.offsetZ),
        glm::vec3(params.rotX, params.rotY, params.rotZ), params.scale);

    glm::mat4 mvp = projMatrix * viewMatrix * model;

    // Surface normal pointing up from the viewmodel
    glm::vec3 surfaceNormal = camUp;

    ViewmodelUniforms out;
    rendering::VertexUniformData& vu = out.vertex;
    vu = {};
    std::memcpy(vu.mvp, glm::value_ptr(mvp), sizeof(float) * 16);
    std::memcpy(vu.model_mat, glm::value_ptr(model), sizeof(float) * 16);
    vu.normal[0] = surfaceNormal.x;
    vu.normal[1] = surfaceNormal.y;
    vu.normal[2] = surfaceNormal.z;
    vu.uv_scale[0] = 1.0f;
    vu.uv_scale[1] = 1.0f;
    vu.camera_pos[0] = camPos.x;
    vu.camera_pos[1] = camPos.y;
    vu.camera_pos[2] = camPos.z;
    auto shadowVP = context.Get<glm::mat4>("render.shadow_vp", glm::mat4(1.0f));
    std::memcpy(vu.shadow_vp, glm::value_ptr(shadowVP), sizeof(float) * 16);

    out.fragment = context.Get<rendering::FragmentUniformData>(
        "render.frag_uniforms", rendering::FragmentUniformData{});
    out.fragment.material[0] = params.roughness;
    out.fragment.material[1] = params.metallic;
    return out;
}

void BindViewmodelTexture(const WorkflowContext& context,
                          SDL_GPURenderPass* pass,
                          const std::string& texName) {
    // Bind texture if specified, else use a default
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUSampler* sampler = nullptr;
    if (!texName.empty()) {
        texture = context.Get<SDL_GPUTexture*>(texName + "_gpu", nullptr);
        sampler = context.Get<SDL_GPUSampler*>(texName + "_sampler", nullptr);
    }
    // Fall back to floor texture or any available texture
    if (!texture) {
        texture = context.Get<SDL_GPUTexture*>("floor_texture_gpu", nullptr);
    }
    if (!sampler) {
        sampler =
            context.Get<SDL_GPUSampler*>("floor_texture_sampler", nullptr);
    }

    if (!texture || !sampler) return;

    auto* shadowTex =
        context.Get<SDL_GPUTexture*>("shadow_depth_texture", nullptr);
    auto* shadowSamp =
        context.Get<SDL_GPUSampler*>("shadow_depth_sampler", nullptr);
    if (shadowTex && shadowSamp) {
        SDL_GPUTextureSamplerBinding bindings[2] = {};
        bindings[0].texture = texture;
        bindings[0].sampler = sampler;
        bindings[1].texture = shadowTex;
        bindings[1].sampler = shadowSamp;
        SDL_BindGPUFragmentSamplers(pass, 0, bindings, 2);
    } else {
        SDL_GPUTextureSamplerBinding binding = {};
        binding.texture = texture;
        binding.sampler = sampler;
        SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
    }
}

}  // namespace sdl3cpp::services::impl
