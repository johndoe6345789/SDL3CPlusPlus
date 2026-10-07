#include "services/interfaces/workflow/racer/flow/racer_flow_rules.hpp"

#include "services/interfaces/workflow/racer/flow/racer_flow_text.hpp"
#include "services/interfaces/workflow/racer/flow/racer_parts.hpp"
#include "services/interfaces/workflow/racer/flow/racer_tournament.hpp"

namespace sdl3cpp::services::impl {

void BookRacerRace(RacerFlow& flow, const RacerWorldState& state) {
    if (flow.prizeAwarded) return;
    flow.prizeAwarded = true;
    flow.prize = 0;
    flow.pointsWon = 0;
    flow.unlocked.clear();
    flow.repairs.clear();
    if (!flow.tournament) return;
    const RacerTournamentOutcome out = RecordRacerTournamentRace(
        flow.profile, flow.circuit, flow.circuitTrack, state.race.position,
        flow.split, static_cast<int>(state.table.racers.size()));
    flow.prize = out.prize;
    flow.pointsWon = out.points;
    flow.unlocked = out.unlocked;
    if (out.newRacer >= 0) {
        if (!flow.unlocked.empty()) flow.unlocked += "  ";
        flow.unlocked += "NEW RACER: " +
                         RacerUpper(state.table.racers[out.newRacer].name);
    }
    flow.repairs = WearAndRepairRacerParts(
        flow.profile,
        0.5f * (state.pod.engineDamage[0] + state.pod.engineDamage[1]),
        state.pod.heat >= 0.95f);
    SaveRacerProfile(flow.profile, RacerProfilePath());
}

}  // namespace sdl3cpp::services::impl
