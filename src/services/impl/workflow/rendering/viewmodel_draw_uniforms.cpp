#include "services/interfaces/workflow/rendering/viewmodel_draw.hpp"
#include "services/interfaces/workflow/rendering/viewmodel_transform.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstring>

namespace sdl3cpp::services::impl {

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
    const auto basis      = rendering::ExtractCameraBasis(viewMatrix);
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
    vu                               = {};
    std::memcpy(vu.mvp, glm::value_ptr(mvp), sizeof(float) * 16);
    std::memcpy(vu.model_mat, glm::value_ptr(model), sizeof(float) * 16);
    vu.normal[0]     = surfaceNormal.x;
    vu.normal[1]     = surfaceNormal.y;
    vu.normal[2]     = surfaceNormal.z;
    vu.uv_scale[0]   = 1.0f;
    vu.uv_scale[1]   = 1.0f;
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

}  // namespace sdl3cpp::services::impl
