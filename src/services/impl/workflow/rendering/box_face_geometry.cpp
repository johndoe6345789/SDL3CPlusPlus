#include "services/interfaces/workflow/rendering/box_face_geometry.hpp"

#include <nlohmann/json.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstring>
#include <vector>

namespace sdl3cpp::services::impl {

std::array<BoxFace, 6> BuildBoxFaces(float sizeX, float sizeY, float sizeZ,
                                     float uvDensity) {
    const float hx = sizeX * 0.5f;
    const float hy = sizeY * 0.5f;
    const float hz = sizeZ * 0.5f;

    const glm::mat4 rotNone(1.0f);
    const glm::mat4 rotDown = glm::rotate(
        glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1, 0, 0));
    const glm::mat4 rotNorth = glm::rotate(
        glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0));
    const glm::mat4 rotSouth = glm::rotate(
        glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0));

    glm::mat4 rotEast(1.0f);
    rotEast[0] = glm::vec4(0, 0, 1, 0);
    rotEast[1] = glm::vec4(1, 0, 0, 0);
    rotEast[2] = glm::vec4(0, 1, 0, 0);
    rotEast[3] = glm::vec4(0, 0, 0, 1);

    glm::mat4 rotWest(1.0f);
    rotWest[0] = glm::vec4(0, 0, -1, 0);
    rotWest[1] = glm::vec4(-1, 0, 0, 0);
    rotWest[2] = glm::vec4(0, 1, 0, 0);
    rotWest[3] = glm::vec4(0, 0, 0, 1);

    return {{
        {glm::vec3(0, hy, 0), glm::vec3(0, 1, 0), rotNone, sizeX, sizeZ,
         sizeX * uvDensity, sizeZ * uvDensity},
        {glm::vec3(0, -hy, 0), glm::vec3(0, -1, 0), rotDown, sizeX, sizeZ,
         sizeX * uvDensity, sizeZ * uvDensity},
        {glm::vec3(0, 0, -hz), glm::vec3(0, 0, -1), rotNorth, sizeX, sizeY,
         sizeX * uvDensity, sizeY * uvDensity},
        {glm::vec3(0, 0, hz), glm::vec3(0, 0, 1), rotSouth, sizeX, sizeY,
         sizeX * uvDensity, sizeY * uvDensity},
        {glm::vec3(hx, 0, 0), glm::vec3(1, 0, 0), rotEast, sizeZ, sizeY,
         sizeZ * uvDensity, sizeY * uvDensity},
        {glm::vec3(-hx, 0, 0), glm::vec3(-1, 0, 0), rotWest, sizeZ, sizeY,
         sizeZ * uvDensity, sizeY * uvDensity},
    }};
}

void BindBoxTextures(SDL_GPURenderPass* pass, SDL_GPUTexture* texture,
                     SDL_GPUSampler* sampler, SDL_GPUTexture* shadowTex,
                     SDL_GPUSampler* shadowSamp) {
    if (shadowTex && shadowSamp) {
        SDL_GPUTextureSamplerBinding bindings[2] = {};
        bindings[0].texture = texture;
        bindings[0].sampler = sampler;
        bindings[1].texture = shadowTex;
        bindings[1].sampler = shadowSamp;
        SDL_BindGPUFragmentSamplers(pass, 0, bindings, 2);
    } else {
        SDL_GPUTextureSamplerBinding tex_binding = {};
        tex_binding.texture = texture;
        tex_binding.sampler = sampler;
        SDL_BindGPUFragmentSamplers(pass, 0, &tex_binding, 1);
    }
}

void DrawBoxFaces(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                  const std::array<BoxFace, 6>& faces,
                  const glm::vec3& center, const glm::mat4& bodyRotation,
                  const glm::mat4& view, const glm::mat4& proj,
                  const glm::vec3& camPos, const glm::mat4& shadowVP,
                  const rendering::FragmentUniformData& fu,
                  uint32_t indexCount) {
    for (const auto& f : faces) {
        glm::mat4 model_mat = glm::translate(glm::mat4(1.0f), center) *
                             bodyRotation *
                             glm::translate(glm::mat4(1.0f), f.offset) *
                             f.rotation *
                             glm::scale(glm::mat4(1.0f),
                                       glm::vec3(f.scaleW, 1.0f, f.scaleD));

        glm::mat4 mvp = proj * view * model_mat;
        glm::vec3 worldNormal =
            glm::vec3(bodyRotation * glm::vec4(f.normal, 0.0f));

        rendering::VertexUniformData vu = {};
        std::memcpy(vu.mvp, glm::value_ptr(mvp), sizeof(float) * 16);
        std::memcpy(vu.model_mat, glm::value_ptr(model_mat),
                   sizeof(float) * 16);
        vu.normal[0] = worldNormal.x;
        vu.normal[1] = worldNormal.y;
        vu.normal[2] = worldNormal.z;
        vu.uv_scale[0] = f.uvW;
        vu.uv_scale[1] = f.uvH;
        vu.camera_pos[0] = camPos.x;
        vu.camera_pos[1] = camPos.y;
        vu.camera_pos[2] = camPos.z;
        std::memcpy(vu.shadow_vp, glm::value_ptr(shadowVP),
                   sizeof(float) * 16);

        SDL_PushGPUVertexUniformData(cmd, 0, &vu, sizeof(vu));
        SDL_PushGPUFragmentUniformData(cmd, 0, &fu, sizeof(fu));
        SDL_DrawGPUIndexedPrimitives(pass, indexCount, 1, 0, 0, 0);
    }
}

void DrawTexturedBox(WorkflowContext& context, ILogger* logger,
                    const DrawTexturedBoxParams& params) {
    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        "gpu_pipeline_textured", nullptr);
    if (!pass || !cmd || !pipeline) return;

    // Unit plane buffers (1x1 plane on XZ, normal +Y)
    auto* vb = context.Get<SDL_GPUBuffer*>("plane_unit_vb", nullptr);
    auto* ib = context.Get<SDL_GPUBuffer*>("plane_unit_ib", nullptr);
    const auto* mesh_meta = context.TryGet<nlohmann::json>("plane_unit");
    if (!vb || !ib || !mesh_meta) {
        if (logger) {
            logger->Warn("draw.textured_box: unit plane not found in "
                        "context");
        }
        return;
    }
    uint32_t index_count = (*mesh_meta)["index_count"];

    auto* texture =
        context.Get<SDL_GPUTexture*>(params.texture + "_gpu", nullptr);
    auto* sampler =
        context.Get<SDL_GPUSampler*>(params.texture + "_sampler", nullptr);
    if (!texture || !sampler) {
        if (logger) {
            logger->Warn("draw.textured_box: texture '" + params.texture +
                        "' not found");
        }
        return;
    }

    auto view = context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.0f));
    auto proj = context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.0f));
    auto camPos =
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.0f));

    // Pre-computed body transform from physics.sync_transforms step
    glm::vec3 center = params.pos;
    glm::mat4 bodyRotation(1.0f);
    if (!params.body.empty()) {
        const auto* sync =
            context.TryGet<nlohmann::json>("body_sync_" + params.body);
        if (sync) {
            auto p = (*sync)["pos"].get<std::vector<float>>();
            center = glm::vec3(p[0], p[1], p[2]);

            auto rot = (*sync)["rotation"].get<std::vector<float>>();
            if (rot.size() == 16) bodyRotation = glm::make_mat4(rot.data());
        }
    }

    // Pre-computed PBR lighting from context + per-draw material
    auto fu = context.Get<rendering::FragmentUniformData>(
        "render.frag_uniforms", rendering::FragmentUniformData{});
    fu.material[0] = params.roughness;
    fu.material[1] = params.metallic;

    const auto faces = BuildBoxFaces(params.size.x, params.size.y,
                                     params.size.z, params.uvDensity);

    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    auto* shadow_tex =
        context.Get<SDL_GPUTexture*>("shadow_depth_texture", nullptr);
    auto* shadow_samp =
        context.Get<SDL_GPUSampler*>("shadow_depth_sampler", nullptr);
    BindBoxTextures(pass, texture, sampler, shadow_tex, shadow_samp);

    SDL_GPUBufferBinding vb_binding = {};
    vb_binding.buffer = vb;
    SDL_BindGPUVertexBuffers(pass, 0, &vb_binding, 1);
    SDL_GPUBufferBinding ib_binding = {};
    ib_binding.buffer = ib;
    SDL_BindGPUIndexBuffer(pass, &ib_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    auto shadowVP =
        context.Get<glm::mat4>("render.shadow_vp", glm::mat4(1.0f));

    DrawBoxFaces(pass, cmd, faces, center, bodyRotation, view, proj, camPos,
                shadowVP, fu, index_count);
}

}  // namespace sdl3cpp::services::impl
