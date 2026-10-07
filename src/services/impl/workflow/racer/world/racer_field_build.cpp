#include "services/interfaces/workflow/racer/world/racer_field_build.hpp"

#include "services/interfaces/workflow/racer/player/racer_pod_handling.hpp"
#include "services/interfaces/workflow/racer/world/racer_gpu_upload.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_build.hpp"

#include <algorithm>
#include <string>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kGridSpacing = 6.f;    // metres either side of the line
constexpr int kRowPoints = 5;         // lap points (~40 m) between rows
constexpr int kPlayerSlot = 3;        // second row, right

/// Pace from 1.0 down to 0.95 across the field, varied but repeatable.
float PaceFor(int index) {
    static const float kPaces[] = {1.0f, 0.98f, 0.97f, 0.99f, 0.96f,
                                   0.97f, 0.95f, 0.96f, 0.95f};
    return kPaces[index % 9];
}

void PlaceOnGrid(RacerWorldState& state, RacerPodState& pod, int slot,
                 int rows) {
    const int row = slot / 2;
    const float side = slot % 2 == 0 ? -kGridSpacing : kGridSpacing;
    PlaceRacerPod(state, pod, (rows - 1 - row) * kRowPoints, 0.f, side);
}

}  // namespace

void BuildRacerField(SDL_GPUDevice* device, RacerWorldState& state,
                     const RacerTrackTable& table, int count,
                     const std::shared_ptr<ILogger>& logger) {
    state.opponents.clear();
    for (const RacerPodInfo& racer : table.racers) {
        if (static_cast<int>(state.opponents.size()) >= count) break;
        if (racer.podd == state.racer.podd) continue;
        RacerOpponent opponent;
        opponent.racer = racer;
        const float pace = PaceFor(static_cast<int>(state.opponents.size()));
        // Each rival flies their own pod's stock handling; the shop's
        // upgrades are the player's alone.
        opponent.spec = RacerSpecFromHandling(racer.handling);
        opponent.spec.topSpeed *= pace;
        opponent.spec.boostSpeed *= pace;
        opponent.race = state.race;
        const RacerModel model = LoadRacerModel(
            state.library, racer.podd, RacerModelScope::PodParts);
        opponent.model = UploadRacerModel(device, state, model, nullptr,
                                          RacerVertexShading::Normal);
        opponent.rig = BuildRacerPodRig(device, model, state.white,
                                        RacerBinderColour(racer.name));
        SetRacerPodBody(opponent.pod, opponent.rig, opponent.spec);
        if (logger) {
            logger->Trace("racer.world.load: opponent " + racer.name + ", " +
                          std::to_string(model.triangleCount) + " triangles");
        }
        state.opponents.push_back(std::move(opponent));
    }
    const int total = 1 + static_cast<int>(state.opponents.size());
    const int rows = (total + 1) / 2;
    const int playerSlot = std::min(kPlayerSlot, total - 1);
    int slot = 0;
    for (RacerOpponent& opponent : state.opponents) {
        if (slot == playerSlot) ++slot;
        PlaceOnGrid(state, opponent.pod, slot++, rows);
    }
    PlaceOnGrid(state, state.pod, playerSlot, rows);
    if (logger) {
        logger->Info("racer.world.load: field of " + std::to_string(total) +
                     ", player in slot " + std::to_string(playerSlot + 1));
    }
}

}  // namespace sdl3cpp::services::impl
