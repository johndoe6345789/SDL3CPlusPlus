#pragma once

#include "services/interfaces/workflow/racer/flow/racer_flow.hpp"
#include "services/interfaces/workflow/racer/render/racer_panel.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <string>
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
std::vector<RacerPanelLine> RacerJunkyardLines(const RacerFlow& flow);
std::vector<RacerPanelLine> RacerPitDroidLines(const RacerFlow& flow);
std::vector<RacerPanelLine> RacerTournamentLines(
    const RacerFlow& flow, const RacerTrackTable& table);
std::vector<RacerPanelLine> RacerFreeRaceLines(const RacerFlow& flow,
                                               const RacerTrackTable& table);
std::vector<RacerPanelLine> RacerPauseLines(const RacerFlow& flow);
std::vector<RacerPanelLine> RacerResultsLines(const RacerFlow& flow,
                                              const RacerWorldState& state);
std::vector<RacerPanelLine> RacerLoadingLines(const RacerFlow& flow,
                                              const RacerTrackTable& table);

/// The game's title across the top, with a line under it.
void RacerTitleLines(std::vector<RacerPanelLine>& lines,
                     const std::string& below);

/// Selectable rows, one under another from `top`, `selected` marked.
void RacerRowLines(std::vector<RacerPanelLine>& lines,
                   const std::vector<std::string>& rows, int selected,
                   float top, float spacing);

/// A line centred across the panel at row `y`.
RacerPanelLine RacerCentred(const std::string& text, float y,
                            SDL_Color colour);

/// `text` in capitals (SDL's debug font has no lower case worth using).
std::string RacerUpper(std::string text);

/// A translucent box across the panel behind rows `top` to
/// `bottom`, for legibility over busy art.
RacerPanelLine RacerBand(float top, float bottom);

/// A part type's row: its name, the fitted part, and its condition.
std::string RacerFittedPartRow(const RacerProfile& profile, int type);

/// "1ST", "2ND", "3RD", "4TH" ...
std::string RacerPlaceText(int place);

/// The chosen racer's name in capitals.
std::string RacerChosenRacer(const RacerFlow& flow,
                             const RacerTrackTable& table);

/// Highlight for the selected row, plain otherwise.
SDL_Color RacerRowColour(bool selected);

}  // namespace sdl3cpp::services::impl
