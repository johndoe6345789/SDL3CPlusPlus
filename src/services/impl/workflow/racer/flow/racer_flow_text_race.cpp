#include "services/interfaces/workflow/racer/flow/racer_flow_text.hpp"

#include "services/interfaces/workflow/racer/flow/racer_flow_rules.hpp"
#include "services/interfaces/workflow/racer/player/racer_field_rules.hpp"

#include <algorithm>
#include <cstdio>

namespace sdl3cpp::services::impl {
namespace {

constexpr SDL_Color kGold{255, 205, 70, 255};
constexpr SDL_Color kGrey{200, 200, 210, 255};

std::string Clock(float seconds) {
    const int minutes = static_cast<int>(seconds / 60.f);
    char text[16];
    std::snprintf(text, sizeof(text), "%d:%05.2f", minutes,
                  seconds - 60.f * minutes);
    return text;
}

struct Row {
    int position;
    std::string name;
    const RacerRaceState* race;
};

}  // namespace

std::vector<RacerPanelLine> RacerPauseLines(const RacerFlow& flow) {
    std::vector<RacerPanelLine> lines;
    lines.push_back(RacerCentred("PAUSED", 80.f, kGold));
    const char* rows[] = {"RESUME", "RESTART RACE", "QUIT TO MENU"};
    for (int i = 0; i < 3; ++i) {
        const bool on = flow.pauseRow == i;
        lines.push_back(RacerCentred(std::string(on ? "> " : "  ") + rows[i],
                                     120.f + 20.f * i, RacerRowColour(on)));
    }
    return lines;
}

std::vector<RacerPanelLine> RacerResultsLines(const RacerFlow& flow,
                                              const RacerWorldState& state) {
    std::vector<Row> rows{{state.race.position, state.racer.name,
                           &state.race}};
    for (const RacerOpponent& o : state.opponents) {
        rows.push_back({o.race.position, o.racer.name, &o.race});
    }
    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
        return a.position < b.position;
    });
    std::vector<RacerPanelLine> lines;
    lines.push_back(RacerCentred("RESULTS", 24.f, kGold));
    float y = 54.f;
    for (const Row& row : rows) {
        const bool me = row.race == &state.race;
        char text[64];
        std::snprintf(text, sizeof(text), "%d  %-24.24s %s", row.position,
                      row.name.c_str(),
                      row.race->finished ? Clock(row.race->raceTime).c_str()
                                         : "RACING");
        lines.push_back(RacerCentred(text, y, RacerRowColour(me)));
        y += 14.f;
    }
    lines.push_back(RacerCentred("BEST LAP " + Clock(state.race.bestLap),
                                 y + 10.f, kGrey));
    lines.push_back(RacerCentred("PRIZE  " + std::to_string(flow.prize) +
                                     " TRUGUTS", y + 28.f, kGold));
    lines.push_back(RacerCentred("ENTER TO CONTINUE", 248.f, kGrey));
    return lines;
}

}  // namespace sdl3cpp::services::impl
