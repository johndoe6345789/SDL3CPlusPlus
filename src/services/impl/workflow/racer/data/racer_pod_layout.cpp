#include "racer_model_context.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl::racer_model_detail {
namespace {

struct Bounds {
    glm::vec3 lo{1e9f};
    glm::vec3 hi{-1e9f};
    glm::vec3 Size() const { return hi - lo; }
    glm::vec3 Centre() const { return 0.5f * (lo + hi); }
};

Bounds BoundsOf(const RacerModel& part) {
    Bounds b;
    for (const auto& batch : part.batches) {
        for (const auto& v : batch.vertices) {
            b.lo = glm::min(b.lo, glm::vec3(v.x, v.y, v.z));
            b.hi = glm::max(b.hi, glm::vec3(v.x, v.y, v.z));
        }
    }
    return b;
}

void MoveAndMerge(RacerModel& part, const glm::vec3& by, RacerModel& into) {
    for (auto& batch : part.batches) {
        for (auto& v : batch.vertices) {
            v.x += by.x;
            v.y += by.y;
            v.z += by.z;
        }
        into.batches.push_back(std::move(batch));
    }
    into.meshCount += part.meshCount;
    into.triangleCount += part.triangleCount;
}

}  // namespace

void LayoutPodParts(std::vector<RacerModel>& parts, RacerModel& model) {
    std::erase_if(parts, [](const RacerModel& p) { return p.batches.empty(); });
    if (parts.empty()) return;
    // The last part is the cockpit; every one before it is an engine.
    const std::size_t engines = parts.size() > 1 ? parts.size() - 1 : 1;
    float width = 0.f, length = 0.f;
    for (std::size_t i = 0; i < engines; ++i) {
        width = std::max(width, BoundsOf(parts[i]).Size().x);
        length = std::max(length, BoundsOf(parts[i]).Size().y);
    }
    for (std::size_t i = 0; i < engines; ++i) {
        const Bounds b = BoundsOf(parts[i]);
        const float side = engines == 1 ? 0.f : (i % 2 == 0 ? -1.f : 1.f);
        const glm::vec3 at(side * 0.8f * width, 0.5f * length, 0.f);
        MoveAndMerge(parts[i], at - b.Centre(), model);
        model.engineExhausts.push_back({at.x, 0.f, at.z});
    }
    model.engineRadius = 0.5f * width;
    if (parts.size() > 1) {
        // Cockpit trails the engines by about one engine length of cable.
        const Bounds b = BoundsOf(parts.back());
        const glm::vec3 at(0.f, -length - 0.5f * b.Size().y, 0.f);
        MoveAndMerge(parts.back(), at - b.Centre(), model);
        model.cockpitFront = {0.f, -length, 0.f};
    }
}

}  // namespace sdl3cpp::services::impl::racer_model_detail
