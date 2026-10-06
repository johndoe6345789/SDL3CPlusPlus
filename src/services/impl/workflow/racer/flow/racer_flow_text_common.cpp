#include "services/interfaces/workflow/racer/flow/racer_flow_text.hpp"

#include <algorithm>
#include <cctype>

namespace sdl3cpp::services::impl {

/// A translucent box across the panel behind rows `top` to `bottom`.
RacerPanelLine RacerBand(float top, float bottom) {
    RacerPanelLine band;
    band.colour = {0, 0, 0, 165};
    band.x = 24.f;
    band.y = top;
    band.boxWidth = kRacerScreenWidth - 48.f;
    band.boxHeight = bottom - top;
    return band;
}

std::string RacerUpper(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](char c) {
        return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    });
    return text;
}

RacerPanelLine RacerCentred(const std::string& text, float y,
                            SDL_Color colour) {
    const float width = 8.f * static_cast<float>(text.size());
    return {text, colour, 0.5f * (kRacerScreenWidth - width), y};
}

SDL_Color RacerRowColour(bool selected) {
    return selected ? SDL_Color{255, 240, 120, 255}
                    : SDL_Color{170, 175, 190, 255};
}

}  // namespace sdl3cpp::services::impl
