#include "services/interfaces/workflow/gta5/gta5_draw_probe.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cstring>

namespace sdl3cpp::services::impl {
namespace {

std::string Fmt(float v) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.2f", v);
    return buffer;
}

}  // namespace

std::string ProbeGta5FirstInstance(const Gta5StreamState& state,
                                   const Gta5DrawContext& draw) {
    for (const auto& entry : state.resident) {
        for (const Gta5Instance& instance : entry.second.instances) {
            glm::mat4 model(1.f);
            std::memcpy(glm::value_ptr(model), instance.modelMatrix.data(),
                        sizeof(float) * 16);
            const glm::vec3 world = glm::vec3(model[3]);
            const glm::vec4 clip =
                draw.proj * draw.view * glm::vec4(world, 1.f);

            std::string out = "world(" + Fmt(world.x) + "," + Fmt(world.y) +
                              "," + Fmt(world.z) + ") clip.w=" + Fmt(clip.w);
            if (clip.w > 0.0001f) {
                out += " ndc(" + Fmt(clip.x / clip.w) + "," +
                       Fmt(clip.y / clip.w) + "," + Fmt(clip.z / clip.w) + ")";
            } else {
                out += " (behind camera)";
            }
            return out;
        }
    }
    return "no instances";
}

}  // namespace sdl3cpp::services::impl
