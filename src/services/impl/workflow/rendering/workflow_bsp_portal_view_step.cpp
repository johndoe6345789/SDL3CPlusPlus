#include "services/interfaces/workflow/rendering/workflow_bsp_portal_view_step.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>

#include <cmath>
#include <cstring>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

namespace {

bool ReadVec3(const nlohmann::json& value, glm::vec3& out) {
    if (!value.is_array() || value.size() != 3) return false;
    out = glm::vec3(value[0].get<float>(), value[1].get<float>(), value[2].get<float>());
    return true;
}

bool FindPortalDestination(const nlohmann::json& entities, glm::vec3& out) {
    if (!entities.is_array()) return false;
    for (const auto& ent : entities) {
        if (ent.value("classname", std::string{}) != "trigger_teleport") continue;
        if (ent.contains("target_position") && ReadVec3(ent["target_position"], out)) return true;
    }
    return false;
}

}  // namespace

WorkflowBspPortalViewStep::WorkflowBspPortalViewStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBspPortalViewStep::GetPluginId() const {
    return "bsp.portal_view";
}

void WorkflowBspPortalViewStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;

    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    SDL_Window* window = context.Get<SDL_Window*>("sdl_window", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline_bsp", nullptr);
    auto* lmTex = context.Get<SDL_GPUTexture*>("bsp_lightmap_atlas_gpu", nullptr);
    auto* lmSamp = context.Get<SDL_GPUSampler*>("bsp_lightmap_atlas_sampler", nullptr);
    const auto* mapNodes = context.TryGet<nlohmann::json>("map.nodes");
    const auto* entities = context.TryGet<nlohmann::json>("bsp.entities");

    if (!device || !window || !pipeline || !lmTex || !lmSamp ||
        !mapNodes || !mapNodes->is_array() || mapNodes->empty() || !entities) {
        return;
    }

    glm::vec3 dest;
    if (!FindPortalDestination(*entities, dest)) return;
    dest += glm::vec3(0.0f, 1.4f, 0.0f);

    constexpr uint32_t kPortalSize = 512;
    auto* portalTex = context.Get<SDL_GPUTexture*>("bsp_portal_view_texture", nullptr);
    auto* portalDepth = context.Get<SDL_GPUTexture*>("bsp_portal_view_depth", nullptr);
    auto* portalSampler = context.Get<SDL_GPUSampler*>("bsp_portal_view_sampler", nullptr);

    if (!portalTex) {
        SDL_GPUTextureCreateInfo ti = {};
        ti.type = SDL_GPU_TEXTURETYPE_2D;
        ti.format = SDL_GetGPUSwapchainTextureFormat(device, window);
        ti.width = kPortalSize;
        ti.height = kPortalSize;
        ti.layer_count_or_depth = 1;
        ti.num_levels = 1;
        ti.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
        portalTex = SDL_CreateGPUTexture(device, &ti);
        if (!portalTex) return;
        context.Set<SDL_GPUTexture*>("bsp_portal_view_texture", portalTex);
    }

    if (!portalDepth) {
        SDL_GPUTextureCreateInfo di = {};
        di.type = SDL_GPU_TEXTURETYPE_2D;
        di.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        di.width = kPortalSize;
        di.height = kPortalSize;
        di.layer_count_or_depth = 1;
        di.num_levels = 1;
        di.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        portalDepth = SDL_CreateGPUTexture(device, &di);
        if (!portalDepth) return;
        context.Set<SDL_GPUTexture*>("bsp_portal_view_depth", portalDepth);
    }

    if (!portalSampler) {
        SDL_GPUSamplerCreateInfo si = {};
        si.min_filter = SDL_GPU_FILTER_LINEAR;
        si.mag_filter = SDL_GPU_FILTER_LINEAR;
        si.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        si.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        si.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        si.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        portalSampler = SDL_CreateGPUSampler(device, &si);
        if (!portalSampler) return;
        context.Set<SDL_GPUSampler*>("bsp_portal_view_sampler", portalSampler);
    }

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd) return;

    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = portalTex;
    colorTarget.clear_color = {0.02f, 0.03f, 0.05f, 1.0f};
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo depthTarget = {};
    depthTarget.texture = portalDepth;
    depthTarget.clear_depth = 1.0f;
    depthTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    depthTarget.store_op = SDL_GPU_STOREOP_DONT_CARE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorTarget, 1, &depthTarget);
    if (!pass) {
        SDL_CancelGPUCommandBuffer(cmd);
        return;
    }

    const float yaw = context.Get<float>("camera_yaw", 0.0f);
    const float pitch = context.Get<float>("camera_pitch", 0.0f);
    glm::vec3 front;
    front.x = std::cos(pitch) * (-std::sin(yaw));
    front.y = std::sin(pitch);
    front.z = std::cos(pitch) * (-std::cos(yaw));
    front = glm::normalize(front);

    glm::mat4 view = glm::lookAt(dest, dest + front, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 500.0f);
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 mvp = proj * view * model;

    rendering::VertexUniformData vu = {};
    std::memcpy(vu.mvp, glm::value_ptr(mvp), sizeof(float) * 16);
    std::memcpy(vu.model_mat, glm::value_ptr(model), sizeof(float) * 16);
    vu.normal[1] = 1.0f;
    vu.uv_scale[0] = 1.0f;
    vu.uv_scale[1] = 1.0f;
    vu.camera_pos[0] = dest.x;
    vu.camera_pos[1] = dest.y;
    vu.camera_pos[2] = dest.z;
    auto shadowVP = context.Get<glm::mat4>("render.shadow_vp", glm::mat4(1.0f));
    std::memcpy(vu.shadow_vp, glm::value_ptr(shadowVP), sizeof(float) * 16);

    auto fu = context.Get<rendering::FragmentUniformData>("render.frag_uniforms", rendering::FragmentUniformData{});
    fu.material[0] = 0.7f;
    fu.material[1] = static_cast<float>(context.GetDouble("frame.elapsed", 0.0));
    fu.material[2] = 2.0f;
    fu.material[3] = 0.0f;

    const auto& firstNode = (*mapNodes)[0];
    std::string meshName = firstNode["name"];
    auto* vb = context.Get<SDL_GPUBuffer*>("plane_" + meshName + "_vb", nullptr);
    auto* ib = context.Get<SDL_GPUBuffer*>("plane_" + meshName + "_ib", nullptr);
    if (!vb || !ib) {
        SDL_EndGPURenderPass(pass);
        SDL_SubmitGPUCommandBuffer(cmd);
        return;
    }

    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_GPUBufferBinding vbBind = {};
    vbBind.buffer = vb;
    SDL_BindGPUVertexBuffers(pass, 0, &vbBind, 1);
    SDL_GPUBufferBinding ibBind = {};
    ibBind.buffer = ib;
    SDL_BindGPUIndexBuffer(pass, &ibBind, SDL_GPU_INDEXELEMENTSIZE_32BIT);
    SDL_PushGPUVertexUniformData(cmd, 0, &vu, sizeof(vu));
    SDL_PushGPUFragmentUniformData(cmd, 0, &fu, sizeof(fu));

    for (const auto& node : *mapNodes) {
        const int texIdx = node.value("texture_index", -1);
        SDL_GPUTexture* albedoTex = nullptr;
        SDL_GPUSampler* albedoSamp = nullptr;
        if (texIdx >= 0) {
            const std::string texKey = "bsp_tex_" + std::to_string(texIdx);
            albedoTex = context.Get<SDL_GPUTexture*>(texKey + "_gpu", nullptr);
            albedoSamp = context.Get<SDL_GPUSampler*>(texKey + "_sampler", nullptr);
        }
        if (!albedoTex || !albedoSamp) continue;

        SDL_GPUTextureSamplerBinding bindings[4] = {};
        bindings[0].texture = albedoTex;
        bindings[0].sampler = albedoSamp;
        bindings[1].texture = albedoTex;
        bindings[1].sampler = albedoSamp;
        bindings[2].texture = lmTex;
        bindings[2].sampler = lmSamp;
        bindings[3].texture = albedoTex;
        bindings[3].sampler = albedoSamp;
        SDL_BindGPUFragmentSamplers(pass, 0, bindings, 4);

        SDL_DrawGPUIndexedPrimitives(pass,
            node["index_count"].get<uint32_t>(),
            1,
            node.value("index_offset", 0u),
            0,
            0);
    }

    SDL_EndGPURenderPass(pass);
    SDL_SubmitGPUCommandBuffer(cmd);
}

}  // namespace sdl3cpp::services::impl
