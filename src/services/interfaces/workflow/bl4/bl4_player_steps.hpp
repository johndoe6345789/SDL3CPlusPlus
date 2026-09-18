#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/bl4/bl4_tile_stream_state.hpp"

#include "services/interfaces/workflow/quake3/pmove/q3_pm_types.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: bl4.player.spawn
 *
 * Stands the player body at (x, y, z), facing a compass heading. bl4x
 * writes this point into world.json a few metres above the nearest
 * mesh placement it walked, so (unlike fs2024, which has an analytic
 * heightfield to query) this step trusts that y outright rather than
 * probing the ground -- the q3.pm.* chain settles the player onto
 * whatever is actually below by the next few frames.
 *
 * Parameters: x, y, z (metres), heading (degrees), pitch (degrees,
 *             negative looks down; default 0).
 * Writes: camera_yaw, camera_pitch, q3.ps (when it already exists)
 */
/**
 * Plugin ID: bl4.player.hold
 *
 * Holds the player where bl4.player.spawn put them until the tiles
 * around that point have streamed in, publishing bl4.loading.text while
 * it waits. Read on demand, a tile takes a second or more -- 506 meshes
 * for the HLOD overview bake -- and unheld the player falls through a
 * world that is not there yet, which on the full map means falling out
 * of it. Both the rigid body and the q3 movement state are reset each
 * frame it holds: q3 keeps its own velocity, which would otherwise build
 * a fall speed through the hold and release it all at once.
 *
 * Runs after the physics group. Parameters: give_up_seconds (default
 * 180) -- release anyway, with a warning, rather than hang forever.
 * Writes: bl4.loading.text (empty once the tiles are in)
 */
class WorkflowBl4PlayerHoldStep final : public IWorkflowStep {
public:
    WorkflowBl4PlayerHoldStep(std::shared_ptr<ILogger> logger,
                              std::shared_ptr<Bl4TileStreamState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Bl4TileStreamState> state_;
    glm::vec3 hold_{0.f};
    std::uint64_t startMs_ = 0;
    bool recorded_ = false;
    bool released_ = false;
};

/**
 * Plugin ID: bl4.camera.orbit
 *
 * A scripted camera for showing the map's scale: circles a centre point
 * at a fixed radius and height, always looking at it, pinning the player
 * there each frame. Replaces flying by hand for a flyover -- free flight
 * moves along the look direction, so looking down at the map means
 * diving into it, and a 45 s pass ended up under or past the world with
 * only sky in view.
 *
 * Off unless radius > 0. Each parameter has a BL4_ORBIT_* environment
 * override via its `<name>_env` sibling (see Bl4NumberOrEnv): radius,
 * height (above centre_y), centre_x/y/z, seconds_per_orbit (default 90).
 * Runs after bl4.player.hold and before camera.fps.update.
 */
class WorkflowBl4CameraOrbitStep final : public IWorkflowStep {
public:
    explicit WorkflowBl4CameraOrbitStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    float angle_ = 0.f;
    bool announced_ = false;
};

/**
 * Plugin ID: bl4.player.respawn
 *
 * Borderlands' answer to falling out of the world: remembers the last
 * walkable ground the player stood on, and on falling more than
 * fall_distance metres below it (or below kill_y at all) puts them back
 * there. Covers every hole the bake cannot fill -- the city's designed
 * bottomless pit most of all, which the game guards with kill volumes
 * rather than a floor.
 *
 * Only a real fall triggers it (downward velocity): free flight zeroes
 * velocity each frame, and bl4.player.hold / bl4.camera.orbit pin the
 * player. Before first touching ground there is no safe spot, so an
 * altitude drop into a hole goes back to bl4.spawn_origin instead.
 *
 * Parameters: fall_distance (default 60), kill_y (default -1000),
 *             report_seconds (default 0, off; BL4_RESPAWN_REPORT) --
 *             log the position every that many seconds, for debugging.
 * Runs after bl4.player.hold, before the camera update.
 */
class WorkflowBl4PlayerRespawnStep final : public IWorkflowStep {
public:
    explicit WorkflowBl4PlayerRespawnStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    glm::vec3 spawnOrigin(const WorkflowContext& context) const;

    std::shared_ptr<ILogger> logger_;
    glm::vec3 safe_{0.f};
    bool haveSafe_ = false;
    int respawns_ = 0;
    float reportIn_ = 0.f;
};

class WorkflowBl4PlayerSpawnStep final : public IWorkflowStep {
public:
    WorkflowBl4PlayerSpawnStep(std::shared_ptr<ILogger> logger,
                              std::shared_ptr<Bl4TileStreamState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Bl4TileStreamState> state_;
};

}  // namespace sdl3cpp::services::impl
