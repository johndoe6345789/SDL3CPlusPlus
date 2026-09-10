#include "services/interfaces/workflow/rendering/render_prepare_helpers.hpp"

#include <nlohmann/json.hpp>

#include <vector>

namespace sdl3cpp::services::impl {

void PrepareRenderLightingState(WorkflowContext& context) {
    rendering::FragmentUniformData fu = {};

    fu.light_dir[1]   = -1.0f;
    fu.light_color[0] = 1.0f;
    fu.light_color[1] = 1.0f;
    fu.light_color[2] = 1.0f;
    fu.light_color[3] = 1.0f;
    fu.ambient[0]     = 0.2f;
    fu.ambient[1]     = 0.2f;
    fu.ambient[2]     = 0.2f;
    fu.material[0]    = 0.8f;
    fu.material[1]    = 0.0f;

    const auto* lighting =
        context.TryGet<nlohmann::json>("lighting.directional");
    if (lighting) {
        if (lighting->contains("direction")) {
            auto dir = (*lighting)["direction"].get<std::vector<float>>();
            if (dir.size() >= 3) {
                fu.light_dir[0] = dir[0];
                fu.light_dir[1] = dir[1];
                fu.light_dir[2] = dir[2];
            }
        }
        if (lighting->contains("color")) {
            auto col = (*lighting)["color"].get<std::vector<float>>();
            if (col.size() >= 3) {
                fu.light_color[0] = col[0];
                fu.light_color[1] = col[1];
                fu.light_color[2] = col[2];
            }
        }
        if (lighting->contains("ambient")) {
            auto amb = (*lighting)["ambient"].get<std::vector<float>>();
            if (amb.size() >= 3) {
                fu.ambient[0] = amb[0];
                fu.ambient[1] = amb[1];
                fu.ambient[2] = amb[2];
            }
        }
        fu.light_color[3] = lighting->value("exposure", 1.0f);
    }

    context.Set<rendering::FragmentUniformData>("render.frag_uniforms", fu);
}

}  // namespace sdl3cpp::services::impl
