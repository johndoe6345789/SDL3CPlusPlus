#include "racer_hazard_rules.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

bool RacerHazardErupting(const RacerHazard& hazard) {
    return hazard.kind == RacerHazardKind::Eruption &&
           hazard.clock < kRacerEruptSeconds;
}

float RacerRockFall(const RacerHazard& hazard) {
    return hazard.clock / kRacerRockFallSeconds;
}

std::uint32_t UpdateRacerHazards(std::vector<RacerHazard>& hazards,
                                 const std::vector<RacerPodState*>& pods,
                                 float dt) {
    std::uint32_t sounds = 0;
    for (RacerHazard& h : hazards) {
        const float before = h.clock;
        h.clock += dt;
        const bool wrapped = h.clock >= h.period;
        if (wrapped) h.clock -= h.period;
        h.flash = std::max(0.f, h.flash - dt);
        switch (h.kind) {
        case RacerHazardKind::Blaster:
            if (wrapped && FireRacerBlaster(h, pods)) {
                sounds |= kRacerSoundBlaster;
            }
            break;
        case RacerHazardKind::Eruption:
            if (wrapped) sounds |= kRacerSoundEruption;
            if (RacerHazardErupting(h)) RunRacerEruption(h, pods, dt);
            break;
        case RacerHazardKind::Rockfall:
            if (before < kRacerRockFallSeconds &&
                h.clock >= kRacerRockFallSeconds) {
                sounds |= kRacerSoundRock;
                DropRacerRock(h, pods);
            }
            break;
        }
    }
    return sounds;
}

}  // namespace sdl3cpp::services::impl
