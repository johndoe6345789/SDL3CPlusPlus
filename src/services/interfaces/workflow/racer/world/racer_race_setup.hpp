#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// Where the install and the track table are, from the workflow.
struct RacerSetupPaths {
    std::string racerDir;
    std::string trackTable;
    int textureScale = 4;
};

/// First run: opens the install's blocks, reads the track table and the
/// saved profile. With RACER_TRACK, RACER_POD or RACER_AUTOPILOT set the
/// race starts straight away (scripted and headless runs); otherwise the
/// menu opens.
void InitRacerFlow(RacerWorldState& state, const RacerSetupPaths& paths,
                   const std::shared_ptr<ILogger>& logger);

/// Builds the race the menu chose: track, racer, laps, rivals, and the
/// player's upgraded pod. False (and back to the menu) on failure.
bool LoadRacerRace(SDL_GPUDevice* device, RacerWorldState& state,
                   const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
