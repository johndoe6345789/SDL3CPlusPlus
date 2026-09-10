#include "services/interfaces/workflow/rendering/box_face_context.hpp"

#include "services/interfaces/workflow/rendering/box_face_resources.hpp"

#include <nlohmann/json.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <vector>

namespace sdl3cpp::services::impl {
namespace {

/// Overrides `out.center`/`out.bodyRotation` with `params.body`'s synced
/// transform, when one is present in `context`. Left untouched (identity
/// rotation about `params.pos`) if `params.body` is empty or not yet synced.
void ResolveBodyTransform(WorkflowContext& context,
                          const DrawTexturedBoxParams& params,
                          TexturedBoxDrawContext& out) {
    if (params.body.empty()) return;
    const auto* sync =
        context.TryGet<nlohmann::json>("body_sync_" + params.body);
    if (!sync) return;

    auto p = (*sync)["pos"].get<std::vector<float>>();
    out.center = glm::vec3(p[0], p[1], p[2]);

    auto rot = (*sync)["rotation"].get<std::vector<float>>();
    if (rot.size() == 16) out.bodyRotation = glm::make_mat4(rot.data());
}

}  // namespace

bool ResolveTexturedBoxDraw(WorkflowContext& context, ILogger* logger,
                            const DrawTexturedBoxParams& params,
                            TexturedBoxDrawContext& out) {
    if (!ResolveTexturedBoxResources(context, logger, params, out)) {
        return false;
    }

    out.view = context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.0f));
    out.proj = context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.0f));
    out.camPos =
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.0f));

    // Pre-computed body transform from physics.sync_transforms step
    out.center = params.pos;
    ResolveBodyTransform(context, params, out);

    // Pre-computed PBR lighting from context + per-draw material
    out.fu = context.Get<rendering::FragmentUniformData>(
        "render.frag_uniforms", rendering::FragmentUniformData{});
    out.fu.material[0] = params.roughness;
    out.fu.material[1] = params.metallic;

    return true;
}

}  // namespace sdl3cpp::services::impl
