#include "services/interfaces/workflow/rendering/bsp_pmove_brush_model.hpp"

#include "services/interfaces/workflow/quake3/q3_brush_collision.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

size_t AttachPmoveBrushModel(btDiscreteDynamicsWorld* world,
                             const std::vector<uint8_t>& bspData, float scale,
                             WorkflowContext& context) {
    auto model = std::make_shared<BrushCollisionModel>(
        BuildBrushCollisionModel(bspData, scale));
    const size_t count = model->brushes.size();

    world->setWorldUserInfo(model->empty() ? nullptr : model.get());
    context.Set("q3.brush_model", model);
    return count;
}

}  // namespace sdl3cpp::services::impl
