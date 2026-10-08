#include "services/interfaces/workflow/switchback/checkpoint/switchback_checkpoint_route.hpp"

#include <nlohmann/json.hpp>

#include <cmath>
#include <fstream>

namespace sdl3cpp::services::impl {
namespace {

float HorizontalDistance(const glm::vec3& a, const glm::vec3& b) {
    return std::hypot(a.x - b.x, a.z - b.z);
}

}  // namespace

bool SwitchbackCheckpointRoute::Load(const std::string& path) {
    points_.clear();
    passed_ = 0;
    std::ifstream file(path);
    if (!file) return false;
    nlohmann::json doc;
    try {
        doc = nlohmann::json::parse(file);
    } catch (const nlohmann::json::exception&) {
        return false;
    }
    if (!doc.contains("checkpoints") || !doc["checkpoints"].is_array()) {
        return false;
    }
    for (const nlohmann::json& entry : doc["checkpoints"]) {
        points_.emplace_back(entry.value("x", 0.f), entry.value("y", 0.f),
                             entry.value("z", 0.f));
    }
    return points_.size() >= 2;
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
