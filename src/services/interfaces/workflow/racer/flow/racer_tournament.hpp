#pragma once

#include "services/interfaces/workflow/racer/data/racer_track_table.hpp"
#include "services/interfaces/workflow/racer/flow/racer_flow.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// The original's four circuits and their tracks, in race order.
const char* RacerCircuitName(int circuit);
int RacerCircuitTrackCount(int circuit);
const char* RacerCircuitTrackName(int circuit, int slot);

/// A circuit track's index in the track table, or -1.
int RacerCircuitTrackIndex(const RacerTrackTable& table, int circuit,
                           int slot);

const char* RacerSplitName(RacerPurseSplit split);

/// Truguts for finishing `position` (1 = first) in `circuit` with the
/// purse shared by `split`: four places paid, three, or the winner.
int RacerPurseFor(int circuit, RacerPurseSplit split, int position);

/// Tournament points for a place: 12, 8, 6, 4, 3, 2, 1.
int RacerPointsFor(int position);

/// What a tournament race changed.
struct RacerTournamentOutcome {
    int prize = 0;
    int points = 0;
    std::string unlocked;   ///< the next track or circuit now open
    int newRacer = -1;      ///< a racer opened by a first win, or -1
};

/// Books a finished tournament race into the profile: prize, points,
/// best place, and what a podium (or a first win) opens up. A podium on
/// the newest track opens the next; podiums on all of a circuit open
/// the next circuit; a first win on a track opens a new racer.
RacerTournamentOutcome RecordRacerTournamentRace(RacerProfile& profile,
                                                 int circuit, int slot,
                                                 int position,
                                                 RacerPurseSplit split,
                                                 int racerCount);

}  // namespace sdl3cpp::services::impl
