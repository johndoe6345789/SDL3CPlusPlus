#include "services/interfaces/workflow/rendering/frame_draw_bodies_helpers.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstring>
#include <vector>

namespace sdl3cpp::services::impl {

namespace {

struct UniformData {
    float mvp[16];
};

}  // namespace

bool DrawOnePhysicsBody(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                        const WorkflowContext& context,
                        const std::string& name, const glm::mat4& viewProj,
                        float time) {
    auto visual = context.Get<nlohmann::json>("physics_visual_" + name,
                                              nlohmann::json::object());
    if (!visual.value("visible", true)) return false;

    auto* body = context.Get<btRigidBody*>("physics_body_" + name, nullptr);
    if (!body) return false;

    btTransform transform;
    body->getMotionState()->getWorldTransform(transform);
    btVector3 pos = transform.getOrigin();
    btQuaternion rot = transform.getRotation();

    // Build model matrix from physics transform.
    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(pos.x(), pos.y(), pos.z()));

    // Apply rotation from physics.
    btMatrix3x3 rotMat(rot);
    glm::mat4 rotGlm(1.0f);
    for (int r = 0; r < 3; ++r) {
        btVector3 row = rotMat.getRow(r);
        rotGlm[0][r] = row.x();
        rotGlm[1][r] = row.y();
        rotGlm[2][r] = row.z();
    }
    model = model * rotGlm;

    // Apply spinning animation if flagged.
    if (visual.value("spinning", false)) {
        float spinX = visual.value("spin_speed_x", 1.0f);
        float spinY = visual.value("spin_speed_y", 0.7f);
        model = glm::rotate(model, time * spinX, glm::vec3(1, 0, 0));
        model = glm::rotate(model, time * spinY, glm::vec3(0, 1, 0));
    }

    // Apply scale from visual info.
    auto scaleArr = visual.value("scale", std::vector<float>{0.5f, 0.5f, 0.5f});
    if (scaleArr.size() >= 3) {
        model = glm::scale(model, glm::vec3(scaleArr[0], scaleArr[1],
                                            scaleArr[2]));
    }

    const glm::mat4 mvp = viewProj * model;
    UniformData uniforms;
    std::memcpy(uniforms.mvp, glm::value_ptr(mvp), sizeof(uniforms.mvp));
    SDL_PushGPUVertexUniformData(cmd, 0, &uniforms, sizeof(uniforms));

    SDL_DrawGPUIndexedPrimitives(pass, 36, 1, 0, 0, 0);
    return true;
}

}  // namespace sdl3cpp::services::impl
