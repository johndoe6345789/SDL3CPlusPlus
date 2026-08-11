#include "services/interfaces/workflow/quake3/workflow_q3_pickups_draw_step.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>

#include <cstring>
#include <string>

namespace sdl3cpp::services::impl {

namespace {

bool HasPrefix(const std::string& value, const std::string& prefix) {
    return value.rfind(prefix, 0) == 0;
}

bool IsPickup(const std::string& classname) {
    return HasPrefix(classname, "weapon_") || HasPrefix(classname, "ammo_") ||
           HasPrefix(classname, "item_") || HasPrefix(classname, "holdable_");
}

std::string TextureKeyForClass(const std::string& classname) {
    if (HasPrefix(classname, "weapon_")) return "q3_pickup_weapon";
    if (HasPrefix(classname, "ammo_")) return "q3_pickup_ammo";
    if (classname.find("health") != std::string::npos) return "q3_pickup_health";
    if (classname.find("armor") != std::string::npos) return "q3_pickup_armor";
    return "q3_pickup_powerup";
}

bool ReadVec3(const nlohmann::json& value, glm::vec3& out) {
    if (!value.is_array() || value.size() != 3) return false;
    out = glm::vec3(value[0].get<float>(), value[1].get<float>(), value[2].get<float>());
    return true;
}

}  // namespace

WorkflowQ3PickupsDrawStep::WorkflowQ3PickupsDrawStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

WorkflowQ3PickupsDrawStep::~WorkflowQ3PickupsDrawStep() {
    if (device_) {
        if (quad_vb_) SDL_ReleaseGPUBuffer(device_, quad_vb_);
        if (quad_ib_) SDL_ReleaseGPUBuffer(device_, quad_ib_);
        if (transfer_) SDL_ReleaseGPUTransferBuffer(device_, transfer_);
    }
}

std::string WorkflowQ3PickupsDrawStep::GetPluginId() const {
    return "q3.pickups.draw";
}

SDL_GPUTexture* WorkflowQ3PickupsDrawStep::CreateColorTexture(SDL_GPUDevice* device, WorkflowContext& context,
                                                              const std::string& key,
                                                              uint8_t r, uint8_t g, uint8_t b) {
    auto* existing = context.Get<SDL_GPUTexture*>(key + "_gpu", nullptr);
    if (existing) return existing;

    SDL_GPUTextureCreateInfo ti = {};
    ti.type = SDL_GPU_TEXTURETYPE_2D;
    ti.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    ti.width = 1;
    ti.height = 1;
    ti.layer_count_or_depth = 1;
    ti.num_levels = 1;
    ti.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    auto* tex = SDL_CreateGPUTexture(device, &ti);
    if (!tex) return nullptr;

    SDL_GPUTransferBufferCreateInfo tbi = {};
    tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbi.size = 4;
    auto* tb = SDL_CreateGPUTransferBuffer(device, &tbi);
    auto* mapped = static_cast<uint8_t*>(SDL_MapGPUTransferBuffer(device, tb, false));
    mapped[0] = r; mapped[1] = g; mapped[2] = b; mapped[3] = 230;
    SDL_UnmapGPUTransferBuffer(device, tb);

    auto* cmd = SDL_AcquireGPUCommandBuffer(device);
    auto* copy = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTextureTransferInfo src = {};
    src.transfer_buffer = tb;
    SDL_GPUTextureRegion dst = {};
    dst.texture = tex; dst.w = 1; dst.h = 1; dst.d = 1;
    SDL_UploadToGPUTexture(copy, &src, &dst, false);
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, tb);

    SDL_GPUSamplerCreateInfo si = {};
    si.min_filter = SDL_GPU_FILTER_NEAREST;
    si.mag_filter = SDL_GPU_FILTER_NEAREST;
    si.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    si.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    si.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    auto* sampler = SDL_CreateGPUSampler(device, &si);
    context.Set<SDL_GPUTexture*>(key + "_gpu", tex);
    context.Set<SDL_GPUSampler*>(key + "_sampler", sampler);
    return tex;
}

void WorkflowQ3PickupsDrawStep::EnsureBuffers(SDL_GPUDevice* device) {
    if (quad_vb_ && quad_ib_) return;
    device_ = device;
    struct V { float x, y, z, u, v; };
    const V verts[4] = {
        {-0.5f, -0.5f, 0.0f, 0.0f, 1.0f},
        { 0.5f, -0.5f, 0.0f, 1.0f, 1.0f},
        { 0.5f,  0.5f, 0.0f, 1.0f, 0.0f},
        {-0.5f,  0.5f, 0.0f, 0.0f, 0.0f},
    };
    const uint16_t indices[6] = {0, 1, 2, 0, 2, 3};
    const uint32_t total = sizeof(verts) + sizeof(indices);

    SDL_GPUBufferCreateInfo vbInfo = {};
    vbInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vbInfo.size = sizeof(verts);
    quad_vb_ = SDL_CreateGPUBuffer(device, &vbInfo);
    SDL_GPUBufferCreateInfo ibInfo = {};
    ibInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    ibInfo.size = sizeof(indices);
    quad_ib_ = SDL_CreateGPUBuffer(device, &ibInfo);
    SDL_GPUTransferBufferCreateInfo tbInfo = {};
    tbInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbInfo.size = total;
    transfer_ = SDL_CreateGPUTransferBuffer(device, &tbInfo);
    auto* mapped = static_cast<uint8_t*>(SDL_MapGPUTransferBuffer(device, transfer_, false));
    std::memcpy(mapped, verts, sizeof(verts));
    std::memcpy(mapped + sizeof(verts), indices, sizeof(indices));
    SDL_UnmapGPUTransferBuffer(device, transfer_);

    auto* cmd = SDL_AcquireGPUCommandBuffer(device);
    auto* copy = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTransferBufferLocation srcV = {transfer_, 0};
    SDL_GPUBufferRegion dstV = {quad_vb_, 0, sizeof(verts)};
    SDL_UploadToGPUBuffer(copy, &srcV, &dstV, false);
    SDL_GPUTransferBufferLocation srcI = {transfer_, sizeof(verts)};
    SDL_GPUBufferRegion dstI = {quad_ib_, 0, sizeof(indices)};
    SDL_UploadToGPUBuffer(copy, &srcI, &dstI, false);
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(cmd);
}

void WorkflowQ3PickupsDrawStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;
    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd = context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline_textured", nullptr);
    const auto* entities = context.TryGet<nlohmann::json>("bsp.entities");
    if (!pass || !cmd || !device || !pipeline || !entities || !entities->is_array()) return;

    EnsureBuffers(device);
    CreateColorTexture(device, context, "q3_pickup_weapon", 255, 210, 40);
    CreateColorTexture(device, context, "q3_pickup_ammo", 255, 120, 45);
    CreateColorTexture(device, context, "q3_pickup_health", 40, 235, 85);
    CreateColorTexture(device, context, "q3_pickup_armor", 70, 150, 255);
    CreateColorTexture(device, context, "q3_pickup_powerup", 190, 90, 255);

    auto collected = context.Get<nlohmann::json>("q3.collected", nlohmann::json::object());
    auto view = context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.0f));
    auto proj = context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.0f));
    auto camPos = context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.0f));
    auto shadowVP = context.Get<glm::mat4>("render.shadow_vp", glm::mat4(1.0f));
    glm::vec3 camRight(view[0][0], view[1][0], view[2][0]);
    glm::vec3 camUp(view[0][1], view[1][1], view[2][1]);
    const float time = static_cast<float>(context.GetDouble("frame.elapsed", 0.0));

    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_GPUBufferBinding vb = {quad_vb_, 0};
    SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
    SDL_GPUBufferBinding ib = {quad_ib_, 0};
    SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    int drawn = 0;
    for (const auto& ent : *entities) {
        const std::string classname = ent.value("classname", std::string{});
        const std::string id = ent.value("id", std::string{});
        if (!IsPickup(classname) || collected.value(id, false)) continue;
        glm::vec3 pos;
        if (!ent.contains("position") || !ReadVec3(ent["position"], pos)) continue;
        if (++drawn > 96) break;

        const float bob = std::sin(time * 3.0f + static_cast<float>(drawn)) * 0.08f;
        const float size = HasPrefix(classname, "weapon_") ? 0.9f : 0.55f;
        pos.y += 0.45f + bob;
        glm::mat4 model(1.0f);
        model[0] = glm::vec4(camRight * size, 0.0f);
        model[1] = glm::vec4(camUp * size, 0.0f);
        model[2] = glm::vec4(glm::normalize(glm::cross(camRight, camUp)) * size, 0.0f);
        model[3] = glm::vec4(pos, 1.0f);

        rendering::VertexUniformData vu = {};
        glm::mat4 mvp = proj * view * model;
        std::memcpy(vu.mvp, glm::value_ptr(mvp), sizeof(float) * 16);
        std::memcpy(vu.model_mat, glm::value_ptr(model), sizeof(float) * 16);
        vu.normal[1] = 1.0f;
        vu.uv_scale[0] = 1.0f; vu.uv_scale[1] = 1.0f;
        vu.camera_pos[0] = camPos.x; vu.camera_pos[1] = camPos.y; vu.camera_pos[2] = camPos.z;
        std::memcpy(vu.shadow_vp, glm::value_ptr(shadowVP), sizeof(float) * 16);

        auto fu = context.Get<rendering::FragmentUniformData>("render.frag_uniforms", rendering::FragmentUniformData{});
        fu.material[0] = 0.35f;
        fu.material[1] = 0.0f;

        const std::string texKey = TextureKeyForClass(classname);
        auto* tex = context.Get<SDL_GPUTexture*>(texKey + "_gpu", nullptr);
        auto* samp = context.Get<SDL_GPUSampler*>(texKey + "_sampler", nullptr);
        if (!tex || !samp) continue;
        SDL_GPUTextureSamplerBinding bindings[2] = {};
        bindings[0].texture = tex; bindings[0].sampler = samp;
        bindings[1].texture = tex; bindings[1].sampler = samp;
        SDL_BindGPUFragmentSamplers(pass, 0, bindings, 2);
        SDL_PushGPUVertexUniformData(cmd, 0, &vu, sizeof(vu));
        SDL_PushGPUFragmentUniformData(cmd, 0, &fu, sizeof(fu));
        SDL_DrawGPUIndexedPrimitives(pass, 6, 1, 0, 0, 0);
    }
}

}  // namespace sdl3cpp::services::impl
