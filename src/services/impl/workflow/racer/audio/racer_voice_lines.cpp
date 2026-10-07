#include "services/interfaces/workflow/racer/audio/racer_voice_lines.hpp"

#include <cstdio>

namespace sdl3cpp::services::impl {

std::string RacerVoiceLineFile(const std::string& voice,
                               RacerVoiceEvent event, int variety) {
    // Anakin has two lines about repairs where the others have one, so
    // his later lines are numbered one on.
    const int late = voice == "as" ? 1 : 0;
    const int taunts = 10 - late;  // up to line 025
    int line = 1;
    switch (event) {
    case RacerVoiceEvent::Bump: line = 1; break;
    case RacerVoiceEvent::Whoop: line = 2; break;
    case RacerVoiceEvent::Damage: line = 4; break;
    case RacerVoiceEvent::Hit: line = 5 + variety % 2; break;
    case RacerVoiceEvent::Air: line = 7; break;
    case RacerVoiceEvent::Scream: line = 9; break;
    case RacerVoiceEvent::Repaired: line = 10 + late; break;
    case RacerVoiceEvent::Win: line = 14 + late; break;
    case RacerVoiceEvent::Lose: line = 15 + late; break;
    case RacerVoiceEvent::Taunt: line = 16 + late + variety % taunts; break;
    }
    char name[32];
    std::snprintf(name, sizeof(name), "%ssp%03d.wav", voice.c_str(), line);
    return name;
}

}  // namespace sdl3cpp::services::impl
