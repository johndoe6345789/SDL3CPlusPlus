#include "racer_pod_parts.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl::racer_model_detail {

void LayoutPodParts(ModelWalk& walk) {
    std::vector<RacerModel>& parts = walk.parts;
    RacerModel& model = walk.model;
    std::erase_if(parts, [](const RacerModel& p) { return p.batches.empty(); });
    if (parts.empty()) return;
    // The last part is the cockpit; every one before it is an engine.
    // Parts face game -y (their intakes point that way), so the engines
    // lead on the -y side and the cockpit trails on +y.
    const std::size_t engines = parts.size() > 1 ? parts.size() - 1 : 1;
    float width = 0.f, length = 0.f;
    for (std::size_t i = 0; i < engines; ++i) {
        width = std::max(width, BoundsOf(parts[i]).Size().x);
        length = std::max(length, BoundsOf(parts[i]).Size().y);
    }
    // The shadow quad gives the pod's true spread and cable length.
    const float spread = kPodPartScale * walk.shadowWidth;
    const float total = kPodPartScale * walk.shadowLength;
    for (std::size_t i = 0; i < engines; ++i) {
        const PodPartBounds b = BoundsOf(parts[i]);
        const float side = engines == 1 ? 0.f : (i % 2 == 0 ? -1.f : 1.f);
        const float pair = static_cast<float>(i / 2);  // four-engine pods
        const float outer = spread > width ? 0.5f * (spread - width)
                                           : 0.8f * width;
        const float x = side * std::max(0.5f * width,
                                        outer - pair * 1.05f * width);
        const glm::vec3 at(x, -0.5f * length, 0.f);
        MoveAndMerge(parts[i], at - b.Centre(), model);
        model.engineExhausts.push_back({at.x, 0.f, at.z});
    }
    model.engineRadius = 0.5f * width;
    float rear = length;
    if (parts.size() > 1) {
        // Cockpit at the shadow's far end, or one engine length back.
        const PodPartBounds b = BoundsOf(parts.back());
        const float cockpit = b.Size().y;
        rear = std::max(total - length, length + cockpit);
        const glm::vec3 at(0.f, rear - 0.5f * cockpit, 0.f);
        MoveAndMerge(parts.back(), at - b.Centre(), model);
        model.cockpitFront = {0.f, rear - cockpit, 0.f};
    }
    model.footprintWidth = std::max(spread, 2.f * std::fabs(
        model.engineExhausts.front()[0]) + width);
    model.footprintLength = rear + length;
}

}  // namespace sdl3cpp::services::impl::racer_model_detail
