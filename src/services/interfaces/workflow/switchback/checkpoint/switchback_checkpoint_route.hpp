#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <vector>

namespace sdl3cpp::services::impl {

/// The course's checkpoints in driving order. Index 0 is the start line; the
/// race is over once the last checkpoint is passed.
class SwitchbackCheckpointRoute {
public:
    /// Replaces the course and starts the race from the first point.
    void SetPoints(std::vector<glm::vec3> points);

    /// Passes the target when the car is within `radius` of it on the
    /// ground. Returns true when a checkpoint was passed.
    bool Update(const glm::vec3& car, float radius);

    /// Starts the race again from the start line.
    void Reset();

    bool Finished() const;
    /// Checkpoints to pass after the start line.
    std::size_t GateCount() const;
    std::size_t Passed() const;
    std::size_t TargetIndex() const;
    glm::vec3 Target() const;
    const std::vector<glm::vec3>& Points() const { return points_; }

private:
    std::vector<glm::vec3> points_;
    std::size_t passed_ = 0;
};

}  // namespace sdl3cpp::services::impl
