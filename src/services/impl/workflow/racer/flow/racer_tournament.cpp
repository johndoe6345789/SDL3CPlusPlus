#include "services/interfaces/workflow/racer/flow/racer_tournament.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

constexpr int kPodium = 3;

/// True once every track of `circuit` has a podium.
bool CircuitConquered(const RacerProfile& profile, int circuit) {
    for (int slot = 0; slot < RacerCircuitTrackCount(circuit); ++slot) {
        const int best = profile.best[circuit * kRacerCircuitTracks + slot];
        if (best < 1 || best > kPodium) return false;
    }
    return true;
}

/// The first racer not yet open, or -1.
int NextLockedRacer(const RacerProfile& profile, int racerCount) {
    for (int i = 0; i < racerCount && i < 32; ++i) {
        if (!(profile.racers & (1u << i))) return i;
    }
    return -1;
}

}  // namespace

RacerTournamentOutcome RecordRacerTournamentRace(RacerProfile& profile,
                                                 int circuit, int slot,
                                                 int position,
                                                 RacerPurseSplit split,
                                                 int racerCount) {
    RacerTournamentOutcome out;
    circuit = std::clamp(circuit, 0, kRacerCircuits - 1);
    out.prize = RacerPurseFor(circuit, split, position);
    out.points = RacerPointsFor(position);
    profile.truguts += out.prize;
    profile.points[circuit] += out.points;
    ++profile.racesRun;
    if (position < 1 || position > kPodium) return out;
    ++profile.podiums;
    int& best = profile.best[circuit * kRacerCircuitTracks + slot];
    const bool firstWin = position == 1 && best != 1;
    if (best == 0 || position < best) best = position;
    const int count = RacerCircuitTrackCount(circuit);
    if (slot + 1 == profile.tracksOpen[circuit] && slot + 1 < count) {
        ++profile.tracksOpen[circuit];
        out.unlocked = std::string("NEXT TRACK OPEN: ") +
                       RacerCircuitTrackName(circuit, slot + 1);
    }
    if (CircuitConquered(profile, circuit) &&
        profile.circuitsOpen == circuit + 1 &&
        circuit + 1 < kRacerCircuits) {
        ++profile.circuitsOpen;
        out.unlocked = std::string(RacerCircuitName(circuit + 1)) +
                       " CIRCUIT OPEN";
    }
    if (firstWin) {
        out.newRacer = NextLockedRacer(profile, racerCount);
        if (out.newRacer >= 0) profile.racers |= 1u << out.newRacer;
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
