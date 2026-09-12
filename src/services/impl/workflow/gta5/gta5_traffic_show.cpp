#include "services/interfaces/workflow/gta5/gta5_traffic_path.hpp"

#include "services/interfaces/workflow/gta5/gta5_effects_spawn.hpp"

namespace sdl3cpp::services::impl {

void ShowGta5Lights(Gta5Effects& effects, const Gta5Traffic& traffic,
                    const Gta5Roads& roads) {
    // On the map's own signal heads, never in mid air: a crossing whose
    // lights have not streamed in yet simply shows none. GTA's props
    // have no bulb to switch, so the state is a lamp laid over the head.
    effects.lamps.clear();
    for (const Gta5Junction& junction : traffic.junctions) {
        if (junction.heads.empty()) continue;
        const float half = traffic.cycle * 0.5f;
        const bool firstHalf = junction.clock < half;
        const float into = firstHalf ? junction.clock : junction.clock - half;
        const bool changing = into > half - traffic.amber;
        for (const glm::vec3& head : junction.heads) {
            Gta5Particle lamp;
            lamp.at = head;
            lamp.colour = changing ? glm::vec3(1.f, 0.62f, 0.f)
                                   : glm::vec3(0.1f, 1.f, 0.2f);
            lamp.size = 0.30f;
            lamp.life = 1.f;
            lamp.fade = 1.f;
            lamp.sprite = kGta5Flash;
            effects.lamps.push_back(lamp);
            // The arm being held shows red a little above the green, so
            // a crossing reads the same whichever road you come in on.
            lamp.at += glm::vec3(0.f, 0.42f, 0.f);
            lamp.colour = glm::vec3(1.f, 0.1f, 0.1f);
            effects.lamps.push_back(lamp);
        }
    }
    (void)roads;
}

}  // namespace sdl3cpp::services::impl
