#include "services/interfaces/workflow/rendering/shadow_pass_helpers.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstring>
#include <vector>

namespace sdl3cpp::services::impl {

namespace {

/// Shadow uniform: lightVP + model (2 mat4 = 128 bytes).
struct ShadowUniform {
    float light_vp[16];
    float model[16];
};

struct FaceRot {
    glm::vec3 offset;
    glm::mat4 rot;
    float sw, sd;
};

}  // namespace

ShadowFaceRotations BuildShadowFaceRotations() {
    ShadowFaceRotations r;
    r.none = glm::mat4(1.0f);
    r.down =
        glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1, 0, 0));
    r.north =
        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0));
    r.south =
        glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0));

    r.east    = glm::mat4(1.0f);
    r.east[0] = glm::vec4(0, 0, 1, 0);
    r.east[1] = glm::vec4(1, 0, 0, 0);
    r.east[2] = glm::vec4(0, 1, 0, 0);
    r.east[3] = glm::vec4(0, 0, 0, 1);

    r.west    = glm::mat4(1.0f);
    r.west[0] = glm::vec4(0, 0, -1, 0);
    r.west[1] = glm::vec4(-1, 0, 0, 0);
    r.west[2] = glm::vec4(0, 1, 0, 0);
    r.west[3] = glm::vec4(0, 0, 0, 1);
    return r;
}

void DrawShadowCasterBody(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                          const WorkflowContext& context,
                          const std::string& name, const glm::mat4& lightVP,
                          const ShadowFaceRotations& rotations,
                          uint32_t indexCount) {
    const auto* sync = context.TryGet<nlohmann::json>("body_sync_" + name);
    if (!sync) return;

    auto pos_arr  = (*sync)["pos"].get<std::vector<float>>();
    auto size_arr = (*sync)["size"].get<std::vector<float>>();
    auto rot_arr  = (*sync)["rotation"].get<std::vector<float>>();

    const float sx = size_arr[0], sy = size_arr[1], sz = size_arr[2];

    // Skip very large bodies (floor, walls, ceiling — not shadow casters).
    if (sx > 15.0f || sy > 15.0f || sz > 15.0f) return;

    const glm::mat4 bodyRot = glm::make_mat4(rot_arr.data());
    const glm::vec3 center(pos_arr[0], pos_arr[1], pos_arr[2]);

    const float hx = sx * 0.5f, hy = sy * 0.5f, hz = sz * 0.5f;

    const FaceRot faces[6] = {
        {glm::vec3(0, hy, 0), rotations.none, sx, sz},
        {glm::vec3(0, -hy, 0), rotations.down, sx, sz},
        {glm::vec3(0, 0, -hz), rotations.north, sx, sy},
        {glm::vec3(0, 0, hz), rotations.south, sx, sy},
        {glm::vec3(hx, 0, 0), rotations.east, sz, sy},
        {glm::vec3(-hx, 0, 0), rotations.west, sz, sy},
    };

    for (int f = 0; f < 6; ++f) {
        const glm::mat4 model =
            glm::translate(glm::mat4(1.0f), center) * bodyRot *
            glm::translate(glm::mat4(1.0f), faces[f].offset) * faces[f].rot *
            glm::scale(glm::mat4(1.0f),
                       glm::vec3(faces[f].sw, 1.0f, faces[f].sd));

        ShadowUniform su = {};
        std::memcpy(su.light_vp, glm::value_ptr(lightVP), 64);
        std::memcpy(su.model, glm::value_ptr(model), 64);

        SDL_PushGPUVertexUniformData(cmd, 0, &su, sizeof(su));
        SDL_DrawGPUIndexedPrimitives(pass, indexCount, 1, 0, 0, 0);
    }
}

}  // namespace sdl3cpp::services::impl
