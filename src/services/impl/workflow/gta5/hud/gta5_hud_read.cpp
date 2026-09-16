#include "services/interfaces/workflow/gta5/hud/gta5_hud_state.hpp"

#include "services/interfaces/workflow/gta5/stream/gta5_stream_lead.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"
#include "services/interfaces/workflow/quake3/pmove/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <cmath>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {

/// Where: the car when seated, else the player. Which way: the camera.
void Place(const WorkflowContext& context, const Gta5StreamState& state,
           Gta5HudState& s) {
    const auto* ps = context.TryGet<Q3PlayerState>("q3.ps");
    const glm::vec3 at =
        Gta5StreamOrigin(state, ps ? ps->origin : state.centreOrigin);
    s.where = glm::vec2(at.x, -at.z);
    const auto view =
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.f));
    const glm::vec3 ahead(-view[0][2], -view[1][2], -view[2][2]);
    s.heading = std::atan2(ahead.x, -ahead.z);
}

}  // namespace

Gta5HudState ReadGta5HudState(const WorkflowContext& context,
                              const Gta5StreamState& state) {
    Gta5HudState s;
    s.health   = context.Get<float>("gta5.player.health", 100.f);
    s.armour   = context.Get<float>("gta5.player.armour", 0.f);
    s.weapon   = context.GetString("gta5.weapon.name", "");
    s.clip     = context.Get<int>("gta5.weapon.clip", -1);
    s.reserve  = context.Get<int>("gta5.weapon.reserve", 0);
    s.aiming   = context.GetBool("gta5.weapon.aiming", false);
    s.driving  = state.seated >= 0;
    s.kmh      = std::abs(context.Get<float>("gta5.car.speed", 0.f)) * 3.6f;
    s.revs     = context.Get<float>("gta5.car.revs", 0.f);
    s.gear     = context.Get<int>("gta5.car.gear", 1);
    s.prompt   = context.GetString("gta5.prompt", "");
    s.menuOpen = context.GetBool("gta5.menu.open", false);
    if (s.menuOpen) s.menu = context.Get<Gta5Menu>("gta5.menu", Gta5Menu{});
    s.wheel.open = context.GetBool("gta5.wheel.open", false);
    if (s.wheel.open) {
        s.wheel.items =
            context.Get<std::vector<std::string>>("gta5.wheel.items", {});
        s.wheel.selected = context.Get<int>("gta5.wheel.selected", 0);
    }
    Place(context, state, s);
    return s;
}

}  // namespace sdl3cpp::services::impl
