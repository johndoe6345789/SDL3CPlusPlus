#include "services/interfaces/workflow/gta5/gta5_player_hold_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_load_progress.hpp"
#include "services/interfaces/workflow/gta5/gta5_player_pin.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_input.hpp"

#include <SDL3/SDL_timer.h>

#include <utility>

namespace sdl3cpp::services::impl {
namespace {

constexpr std::uint64_t kGiveUpMs = 120000;

}  // namespace

WorkflowGta5PlayerHoldStep::WorkflowGta5PlayerHoldStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5PlayerHoldStep::GetPluginId() const {
    return "gta5.player.hold";
}

void WorkflowGta5PlayerHoldStep::Execute(const WorkflowStepDefinition&,
                                         WorkflowContext& context) {
    if (!state_ || released_) return;
    btRigidBody* player = Gta5PlayerBody(context);
    if (!player) return;
    if (!recorded_) {
        const btVector3& origin = player->getWorldTransform().getOrigin();
        hold_ = glm::vec3(origin.x(), origin.y(), origin.z());
        startMs_ = SDL_GetTicks();
        recorded_ = true;
    }

    const Gta5LoadProgress progress = MeasureGta5LoadProgress(*state_, hold_);
    const std::uint64_t waited = SDL_GetTicks() - startMs_;
    if (progress.done || waited > kGiveUpMs) {
        released_ = true;
        // Let go on the ground, not at the spawn height, which may be set
        // high to clear terrain of unknown height.
        if (progress.done) DropGta5PlayerToGround(context, player, hold_);
        context.Set<std::string>("gta5.loading.text", "");
        if (logger_) {
            const std::string after = std::to_string(waited / 1000) + " s";
            if (progress.done) {
                logger_->Info("gta5.player.hold: ground in after " + after);
            } else {
                logger_->Warn("gta5.player.hold: gave up after " + after);
            }
        }
        return;
    }
    context.Set<std::string>("gta5.loading.text", progress.text);
    PinGta5Player(context, player, hold_);
}

}  // namespace sdl3cpp::services::impl
