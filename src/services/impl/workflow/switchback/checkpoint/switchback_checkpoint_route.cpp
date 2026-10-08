#include "services/interfaces/workflow/switchback/checkpoint/switchback_checkpoint_route.hpp"

#include <cmath>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

float HorizontalDistance(const glm::vec3& a, const glm::vec3& b) {
    return std::hypot(a.x - b.x, a.z - b.z);
}

}  // namespace

void SwitchbackCheckpointRoute::SetPoints(std::vector<glm::vec3> points) {
    points_ = std::move(points);
    passed_ = 0;
}

bool SwitchbackCheckpointRoute::Update(const glm::vec3& car, float radius) {
    if (Finished()) return false;
    if (HorizontalDistance(car, Target()) > radius) return false;
    ++passed_;
    return true;
}

void SwitchbackCheckpointRoute::Reset() { passed_ = 0; }

bool SwitchbackCheckpointRoute::Finished() const {
    return points_.empty() || passed_ >= GateCount();
}

std::size_t SwitchbackCheckpointRoute::GateCount() const {
    return points_.empty() ? 0 : points_.size() - 1;
}

std::size_t SwitchbackCheckpointRoute::Passed() const { return passed_; }

std::size_t SwitchbackCheckpointRoute::TargetIndex() const {
    return Finished() ? points_.size() - 1 : passed_ + 1;
}

glm::vec3 SwitchbackCheckpointRoute::Target() const {
    return points_[TargetIndex()];
}

}  // namespace sdl3cpp::services::impl
