#pragma once

#include "services/interfaces/workflow/gta5/hud/gta5_map_build.hpp"
#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// The last click on the open map, to tell a double-click.
struct Gta5MapTravel {
    std::uint64_t clickMs{0};
    glm::vec2 click{-100.f};
    int sequence{0};
};

/// Publishes gta5.map.open, which frees the cursor and holds the view
/// still (gta5.map.look). A double-click on the map sends the player
/// there: gta5.player.teleport, engine (x, z), and a bumped
/// gta5.player.teleport_seq, which gta5.player.hold acts on. True when
/// it sent them, so the map closes.
bool TravelGta5Map(WorkflowContext& context, Gta5MapTravel& travel,
                   bool open, const Gta5MapRect& rect);

/// Where the map's picture lies in GTA's world: map_min_x, map_max_y,
/// map_width and map_height, else Gta5MapRect's.
Gta5MapRect Gta5MapRectFor(const WorkflowStepDefinition& step);

/// Where the cars are, in GTA's (x, y), for the map.
std::vector<glm::vec2> Gta5MapCars(const Gta5StreamState& state);

}  // namespace sdl3cpp::services::impl
