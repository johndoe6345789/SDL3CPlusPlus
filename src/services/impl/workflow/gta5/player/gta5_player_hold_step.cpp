#include "services/interfaces/workflow/gta5/player/gta5_player_hold_step.hpp"

#include "services/interfaces/workflow/gta5/stream/gta5_load_progress.hpp"
#include "services/interfaces/workflow/gta5/player/gta5_player_pin.hpp"
#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_input.hpp"

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

void WorkflowGta5PlayerHoldStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    if (!state_) return;
    roadsDir_ = Gta5ParameterOr(step, "roads_dir", "");
    btRigidBody* player = Gta5PlayerBody(context);
    if (!player) return;
    // A trip starts held over the destination: streaming, after this,
    // then wants its tiles, and the ground there is waited for.
    if (Travel(context, player)) {
        PinGta5Player(context, player, hold_);
        return;
    }
    if (released_) return;
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
        // Let go on the ground, not at the hold's height, set high to
        // clear terrain of unknown height. The search reaches 500 m down:
        // from high over the map, look again lower until it meets ground.
        for (glm::vec3 at = hold_; progress.done && at.y > -100.f;
             at.y -= 490.f) {
            if (DropGta5PlayerToGround(context, player, at)) break;
        }
        if (travelling_) Arrive(context, player);
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
