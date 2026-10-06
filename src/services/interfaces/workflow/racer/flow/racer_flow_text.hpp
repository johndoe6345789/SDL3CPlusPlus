#pragma once

#include "services/interfaces/workflow/racer/flow/racer_flow.hpp"
#include "services/interfaces/workflow/racer/render/racer_panel.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <vector>

namespace sdl3cpp::services::impl {

/// The menu screens' text panel: 480 x 270 pixels of SDL's 8 px font,
/// shown full screen (4x at 1920 x 1080).
inline constexpr int kRacerScreenWidth = 480;
inline constexpr int kRacerScreenHeight = 270;

/// Lines for each screen, laid out on the 480 x 270 panel.
std::vector<RacerPanelLine> RacerMenuLines(const RacerFlow& flow,
                                           const RacerTrackTable& table);
std::vector<RacerPanelLine> RacerShopLines(const RacerFlow& flow);
std::vector<RacerPanelLine> RacerPauseLines(const RacerFlow& flow);
std::vector<RacerPanelLine> RacerResultsLines(const RacerFlow& flow,
                                              const RacerWorldState& state);
std::vector<RacerPanelLine> RacerLoadingLines(const RacerFlow& flow,
                                              const RacerTrackTable& table);

/// A line centred across the panel at row `y`.
RacerPanelLine RacerCentred(const std::string& text, float y,
                            SDL_Color colour);

/// `text` in capitals (SDL's debug font has no lower case worth using).
std::string RacerUpper(std::string text);

/// A translucent box across the panel behind rows `top` to
/// `bottom`, for legibility over busy art.
RacerPanelLine RacerBand(float top, float bottom);

/// Highlight for the selected row, plain otherwise.
SDL_Color RacerRowColour(bool selected);

}  // namespace sdl3cpp::services::impl
