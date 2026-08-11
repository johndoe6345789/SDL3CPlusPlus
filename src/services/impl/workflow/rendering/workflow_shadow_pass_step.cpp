#include "services/interfaces/workflow/rendering/workflow_shadow_pass_step.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>
#include <cstring>

namespace sdl3cpp::services::impl {

WorkflowShadowPassStep::WorkflowShadowPassStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowShadowPassStep::GetPluginId() const {
    return "shadow.pass";
}

void WorkflowShadowPassStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;

    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* shadow_tex = context.Get<SDL_GPUTexture*>("shadow_depth_texture", nullptr);
    auto* shadow_pipeline = context.Get<SDL_GPUGraphicsPipeline*>("shadow_pipeline", nullptr);
    if (!device || !shadow_tex || !shadow_pipeline) return;

    // Get light VP matrix
    const auto* shadow_state = context.TryGet<nlohmann::json>("shadow.state");
    if (!shadow_state || !shadow_state->contains("light_vp")) return;

    auto vp_data = (*shadow_state)["light_vp"].get<std::vector<float>>();
    glm::mat4 lightVP = glm::make_mat4(vp_data.data());

    // Get unit plane for box rendering
    auto* vb = context.Get<SDL_GPUBuffer*>("plane_unit_vb", nullptr);
    auto* ib = context.Get<SDL_GPUBuffer*>("plane_unit_ib", nullptr);
    const auto* mesh_meta = context.TryGet<nlohmann::json>("plane_unit");
    if (!vb || !ib || !mesh_meta) return;
    uint32_t index_count = (*mesh_meta)["index_count"];

    // Get body list from physics.sync_transforms
    auto bodies = context.Get<nlohmann::json>("physics_bodies", nlohmann::json::array());
    if (bodies.empty()) return;

    // Begin shadow render pass (depth-only)
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd) return;

    SDL_GPUDepthStencilTargetInfo ds_target = {};
    ds_target.texture = shadow_tex;
    ds_target.clear_depth = 1.0f;
    ds_target.load_op = SDL_GPU_LOADOP_CLEAR;
    ds_target.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, nullptr, 0, &ds_target);
    if (!pass) {
        SDL_SubmitGPUCommandBuffer(cmd);
        return;
    }

    SDL_BindGPUGraphicsPipeline(pass, shadow_pipeline);

    SDL_GPUBufferBinding vb_bind = {};
    vb_bind.buffer = vb;
    SDL_BindGPUVertexBuffers(pass, 0, &vb_bind, 1);

    SDL_GPUBufferBinding ib_bind = {};
    ib_bind.buffer = ib;
    SDL_BindGPUIndexBuffer(pass, &ib_bind, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    // Shadow uniform: lightVP + model (2 mat4 = 128 bytes)
    struct ShadowUniform {
        float light_vp[16];
        float model[16];
    };

    // Face rotations for box rendering
    glm::mat4 rotNone(1.0f);
    glm::mat4 rotDown = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1, 0, 0));
    glm::mat4 rotNorth = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0));
    glm::mat4 rotSouth = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0));

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

    struct FaceRot { glm::vec3 offset; glm::mat4 rot; float sw, sd; };

    // Render each body as shadow-casting box using pre-computed transforms
    for (const auto& nameVal : bodies) {
        std::string name = nameVal.get<std::string>();

        const auto* sync = context.TryGet<nlohmann::json>("body_sync_" + name);
        if (!sync) continue;

        auto pos_arr = (*sync)["pos"].get<std::vector<float>>();
        auto size_arr = (*sync)["size"].get<std::vector<float>>();
        auto rot_arr = (*sync)["rotation"].get<std::vector<float>>();

        float sx = size_arr[0], sy = size_arr[1], sz = size_arr[2];

        // Skip very large bodies (floor, walls, ceiling — not shadow casters)
        if (sx > 15.0f || sy > 15.0f || sz > 15.0f) continue;

        glm::mat4 bodyRot = glm::make_mat4(rot_arr.data());
        glm::vec3 center(pos_arr[0], pos_arr[1], pos_arr[2]);

        float hx = sx * 0.5f, hy = sy * 0.5f, hz = sz * 0.5f;

        FaceRot faces[6] = {
            { glm::vec3(0, hy, 0),   rotNone,  sx, sz },
            { glm::vec3(0, -hy, 0),  rotDown,  sx, sz },
            { glm::vec3(0, 0, -hz),  rotNorth, sx, sy },
            { glm::vec3(0, 0, hz),   rotSouth, sx, sy },
            { glm::vec3(hx, 0, 0),   rotEast,  sz, sy },
            { glm::vec3(-hx, 0, 0),  rotWest,  sz, sy },
        };

        for (int f = 0; f < 6; ++f) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), center)
                            * bodyRot
                            * glm::translate(glm::mat4(1.0f), faces[f].offset)
                            * faces[f].rot
                            * glm::scale(glm::mat4(1.0f), glm::vec3(faces[f].sw, 1.0f, faces[f].sd));

            ShadowUniform su = {};
            std::memcpy(su.light_vp, glm::value_ptr(lightVP), 64);
            std::memcpy(su.model, glm::value_ptr(model), 64);

            SDL_PushGPUVertexUniformData(cmd, 0, &su, sizeof(su));
            SDL_DrawGPUIndexedPrimitives(pass, index_count, 1, 0, 0, 0);
        }
    }

    SDL_EndGPURenderPass(pass);
    SDL_SubmitGPUCommandBuffer(cmd);
}

}  // namespace sdl3cpp::services::impl
