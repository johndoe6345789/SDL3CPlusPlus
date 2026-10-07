#include "services/interfaces/workflow/racer/flow/racer_tournament.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

const char* const kCircuits[kRacerCircuits] = {
    "AMATEUR", "SEMI-PRO", "GALACTIC", "INVITATIONAL"};

const char* const kTracks[kRacerCircuits][kRacerCircuitTracks] = {
    {"The Boonta Training Course", "Mon Gazza Speedway",
     "Beedo's Wild Ride", "Aquilaris Classic", "Malastare 100",
     "Vengeance", "Spice Mine Run"},
    {"Sunken City", "Howler Gorge", "Dug Derby", "Scrapper's Run",
     "Zugga Challenge", "Baroo Coast", "Bumpy's Breakers"},
    {"Executioner", "Sebulba's Legacy", "Grabvine Gateway",
     "Andobi Mountain Run", "Dethro's Revenge", "Fire Mountain Rally",
     "The Boonta Classic"},
    {"Ando Prime Centrum", "Abyss", "The Gauntlet", "Inferno", nullptr,
     nullptr, nullptr}};

// The purse for a race in each circuit, and each split's share of it.
const int kPurses[kRacerCircuits] = {1200, 2400, 5000, 10000};
const float kShares[3][4] = {{0.40f, 0.27f, 0.18f, 0.15f},
                             {0.55f, 0.30f, 0.15f, 0.f},
                             {1.f, 0.f, 0.f, 0.f}};
const float kRisk[3] = {1.f, 1.15f, 1.3f};

}  // namespace

const char* RacerCircuitName(int circuit) {
    return kCircuits[std::clamp(circuit, 0, kRacerCircuits - 1)];
}

int RacerCircuitTrackCount(int circuit) {
    return circuit == kRacerCircuits - 1 ? 4 : kRacerCircuitTracks;
}

const char* RacerCircuitTrackName(int circuit, int slot) {
    circuit = std::clamp(circuit, 0, kRacerCircuits - 1);
    slot = std::clamp(slot, 0, RacerCircuitTrackCount(circuit) - 1);
    return kTracks[circuit][slot];
}

int RacerCircuitTrackIndex(const RacerTrackTable& table, int circuit,
                           int slot) {
    const std::string name = RacerCircuitTrackName(circuit, slot);
    for (std::size_t i = 0; i < table.tracks.size(); ++i) {
        if (table.tracks[i].name == name) return static_cast<int>(i);
    }
    return -1;
}

const char* RacerSplitName(RacerPurseSplit split) {
    static const char* const kNames[] = {"FAIR", "SKILLED",
                                         "WINNER TAKES ALL"};
    return kNames[static_cast<int>(split)];
}

int RacerPurseFor(int circuit, RacerPurseSplit split, int position) {
    if (position < 1 || position > 4) return 0;
    const int s = static_cast<int>(split);
    const float purse = kPurses[std::clamp(circuit, 0, 3)] * kRisk[s];
    return 10 * static_cast<int>(purse * kShares[s][position - 1] / 10.f);
}

int RacerPointsFor(int position) {
    static const int kPoints[] = {12, 8, 6, 4, 3, 2, 1};
    return position >= 1 && position <= 7 ? kPoints[position - 1] : 0;
}

}  // namespace sdl3cpp::services::impl
