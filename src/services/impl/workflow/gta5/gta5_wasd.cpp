#include "services/interfaces/workflow/gta5/gta5_vehicle_input.hpp"

#include "services/interfaces/workflow/gta5/gta5_look.hpp"

namespace sdl3cpp::services::impl {

glm::vec3 Gta5WasdDirection(const nlohmann::json* keys, float yaw) {
    const glm::vec3 front = Gta5LookFront(yaw, 0.f);
    const glm::vec3 right = Gta5LookRight(yaw);
    glm::vec3 way(0.f);
    if (Gta5KeyDown(keys, "W")) way += front;
    if (Gta5KeyDown(keys, "S")) way -= front;
    if (Gta5KeyDown(keys, "D")) way += right;
    if (Gta5KeyDown(keys, "A")) way -= right;
    return glm::length(way) > 0.f ? glm::normalize(way) : way;
}

}  // namespace sdl3cpp::services::impl
