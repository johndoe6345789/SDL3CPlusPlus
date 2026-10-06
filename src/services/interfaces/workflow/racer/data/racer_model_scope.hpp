#pragma once

namespace sdl3cpp::services::impl {

/// What to keep while walking a model's node tree.
enum class RacerModelScope {
    /// Every reachable mesh, with every node transform applied. Right
    /// for tracks and scenery.
    Everything,
    /// A pod: only its parts (two engines and a cockpit, each under a
    /// 0.02 scale node the game replaces at run time), each from that
    /// node down, laid out engines ahead and cockpit behind. The shadow
    /// quads and afterburner cones around them are left out. The game
    /// spaces the parts from per-racer data not yet decoded, so the
    /// layout here is an approximation.
    PodParts,
    /// A track without its skybox (header slot 2 of a 'Trak' model).
    TrackWithoutSky,
    /// Only a track's skybox: a small dome the game keeps centred on
    /// the camera.
    SkyOnly,
};

}  // namespace sdl3cpp::services::impl
