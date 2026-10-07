#pragma once

#include "services/interfaces/workflow/racer/player/racer_pod_physics.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// Hazards along a track, by planet: Tusken Raiders firing from the
/// canyon sides (Tatooine), lava and methane vents (Baroonda,
/// Malastare), and rockfalls (Ando Prime, Ord Ibanna, Mon Gazza). The
/// original's own hazard triggers are not decoded; these are new.
enum class RacerHazardKind { Blaster, Eruption, Rockfall };

struct RacerHazard {
    RacerHazardKind kind = RacerHazardKind::Blaster;
    glm::vec3 at{0.f};      ///< sniper's perch, vent, or where rocks land
    float radius = 6.f;     ///< metres a vent or rock reaches
    float period = 6.f;     ///< seconds per cycle
    float clock = 0.f;      ///< seconds into the cycle
    glm::vec3 target{0.f};  ///< a bolt's end
    float flash = 0.f;      ///< seconds a bolt stays visible
    std::uint32_t seed = 1; ///< repeatable aim
};

/// What the hazards did this tick, for the sound.
enum RacerHazardSound : std::uint32_t {
    kRacerSoundBlaster = 1u, kRacerSoundEruption = 2u, kRacerSoundRock = 4u
};

/// Places a planet's hazards at points spread round the lap (lap
/// points in engine space); none on planets without any.
std::vector<RacerHazard> PlaceRacerHazards(
    const std::string& planet, const std::vector<glm::vec3>& lap);

/// Runs the hazards for `dt` seconds against the pods; returns the
/// sounds to play (RacerHazardSound bits).
std::uint32_t UpdateRacerHazards(std::vector<RacerHazard>& hazards,
                                 const std::vector<RacerPodState*>& pods,
                                 float dt);

/// While a vent erupts, and where a rock is in its fall (0 = above,
/// 1 = landed; below 0 before it drops, above 1 once it has).
bool RacerHazardErupting(const RacerHazard& hazard);
float RacerRockFall(const RacerHazard& hazard);

}  // namespace sdl3cpp::services::impl
