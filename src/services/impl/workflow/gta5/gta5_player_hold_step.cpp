#include "services/interfaces/workflow/gta5/gta5_player_hold_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_load_progress.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_input.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"

#include <SDL3/SDL_timer.h>

#include <utility>

namespace sdl3cpp::services::impl {
namespace {

constexpr std::uint64_t kGiveUpMs = 120000;

/// Put the player back at `at`, at rest, in both the body and q3.ps.
void Pin(WorkflowContext& context, btRigidBody* body, const glm::vec3& at) {
    btTransform transform = body->getWorldTransform();
    transform.setOrigin(btVector3(at.x, at.y, at.z));
    body->setWorldTransform(transform);
    if (body->getMotionState()) {
        body->getMotionState()->setWorldTransform(transform);
    }
    body->setLinearVelocity(btVector3(0.f, 0.f, 0.f));
    if (!context.Contains("q3.ps")) return;
    auto ps = context.Get<Q3PlayerState>("q3.ps", Q3PlayerState{});
    ps.origin = at;
    ps.velocity = glm::vec3(0.f);
    context.Set("q3.ps", ps);
    context.Set("q3.player_pos", at);
}

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
    Pin(context, player, hold_);
}

}  // namespace sdl3cpp::services::impl
