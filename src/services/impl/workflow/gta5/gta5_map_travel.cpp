#include "services/interfaces/workflow/gta5/gta5_map_travel.hpp"

#include "services/interfaces/workflow/gta5/gta5_map_frame.hpp"

#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

#include <btBulletDynamicsCommon.h>

#include <algorithm>

namespace sdl3cpp::services::impl {

bool TravelGta5Map(WorkflowContext& context, Gta5MapTravel& travel,
                   bool open, const Gta5MapRect& rect) {
    context.Set<bool>("gta5.map.open", open);
    auto* window = context.Get<SDL_Window*>("sdl_window", nullptr);
    if (!open || !window ||
        !context.GetBool("input_mouse_left_pressed", false)) {
        return false;
    }
    // The cursor is in window points; the map is laid out in the frame's
    // pixels, which differ on a scaled display.
    float mx = 0.f, my = 0.f;
    SDL_GetMouseState(&mx, &my);
    int ww = 1, wh = 1;
    SDL_GetWindowSize(window, &ww, &wh);
    const float fw = float(context.Get<uint32_t>("frame_width", 1280u));
    const float fh = float(context.Get<uint32_t>("frame_height", 960u));
    const glm::vec2 at(mx * fw / float(std::max(ww, 1)),
                       my * fh / float(std::max(wh, 1)));
    const std::uint64_t now = SDL_GetTicks();
    const bool twice = now - travel.clickMs < 400 &&
                       glm::distance(at, travel.click) < 10.f;
    travel.clickMs = now;
    travel.click = at;
    if (!twice) return false;
    const Gta5MapLayout l = FitGta5Map(int(fw), int(fh));
    const float u = (at.x - l.left) / (2.f * l.tile);
    const float v = (at.y - l.top) / (3.f * l.tile);
    if (u < 0.f || u > 1.f || v < 0.f || v > 1.f) return false;
    // The map's u runs east with GTA's x, v south against its y; GTA's y
    // is engine -z.
    const float x = rect.minX + u * rect.width;
    const float y = rect.maxY - v * rect.height;
    context.Set<glm::vec2>("gta5.player.teleport", glm::vec2(x, -y));
    context.Set<int>("gta5.player.teleport_seq", ++travel.sequence);
    travel.clickMs = 0;  // a third click starts afresh
    return true;
}

std::vector<glm::vec2> Gta5MapCars(const Gta5StreamState& state) {
    std::vector<glm::vec2> cars;
    for (const Gta5Vehicle& car : state.vehicles) {
        if (!car.chassis) continue;
        const btVector3& at = car.chassis->getWorldTransform().getOrigin();
        cars.emplace_back(at.x(), -at.z());  // GTA's y is engine -z
    }
    return cars;
}

// Engine x is east and -z north: GTA's y, the map's up.
Gta5MapRect Gta5MapRectFor(const WorkflowStepDefinition& step) {
    Gta5MapRect rect;
    rect.minX = Gta5NumberOr(step, "map_min_x", rect.minX);
    rect.maxY = Gta5NumberOr(step, "map_max_y", rect.maxY);
    rect.width = Gta5NumberOr(step, "map_width", rect.width);
    rect.height = Gta5NumberOr(step, "map_height", rect.height);
    return rect;
}

}  // namespace sdl3cpp::services::impl
