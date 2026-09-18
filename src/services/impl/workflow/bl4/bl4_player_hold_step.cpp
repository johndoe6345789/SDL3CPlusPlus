#include "services/interfaces/workflow/bl4/bl4_player_steps.hpp"

#include "services/interfaces/workflow/bl4/bl4_load_progress.hpp"
#include "services/interfaces/workflow/bl4/bl4_player_pin.hpp"
#include "services/interfaces/workflow/bl4/bl4_step_params.hpp"

#include <SDL3/SDL_timer.h>

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowBl4PlayerHoldStep::WorkflowBl4PlayerHoldStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Bl4TileStreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowBl4PlayerHoldStep::GetPluginId() const { return "bl4.player.hold"; }

void WorkflowBl4PlayerHoldStep::Execute(const WorkflowStepDefinition& step,
                                        WorkflowContext& context) {
    if (released_ || !state_) return;
    if (!recorded_) {
        // Wherever bl4.player.spawn put them, height included: an
        // altitude spawn is meant to fall, once there is a map to fall to.
        if (auto ps = context.TryGet<Q3PlayerState>("q3.ps")) hold_ = ps->origin;
        startMs_ = SDL_GetTicks();
        recorded_ = true;
    }

    const Bl4LoadProgress progress = MeasureBl4LoadProgress(*state_, hold_);
    const auto giveUpMs =
        static_cast<std::uint64_t>(Bl4NumberOr(step, "give_up_seconds", 180.f) * 1000.f);
    const std::uint64_t waited = SDL_GetTicks() - startMs_;
    if (progress.done || waited > giveUpMs) {
        released_ = true;
        context.Set<std::string>("bl4.loading.text", "");
        if (logger_) {
            const std::string after = std::to_string(waited / 1000) + " s";
            if (progress.done) {
                logger_->Info("bl4.player.hold: tiles in after " + after);
            } else {
                logger_->Warn("bl4.player.hold: gave up waiting after " + after);
            }
        }
        return;
    }
    context.Set<std::string>("bl4.loading.text", progress.text);
    PinBl4Player(context, hold_);
}

}  // namespace sdl3cpp::services::impl
