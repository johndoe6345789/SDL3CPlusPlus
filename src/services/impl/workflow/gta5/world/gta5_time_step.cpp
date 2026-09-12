#include "services/interfaces/workflow/gta5/world/gta5_time_step.hpp"

#include "services/interfaces/workflow/gta5/render/gta5_daylight.hpp"
#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_input.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_timer.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5TimeStep::WorkflowGta5TimeStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGta5TimeStep::GetPluginId() const { return "gta5.time"; }

void WorkflowGta5TimeStep::Execute(const WorkflowStepDefinition& step,
                                   WorkflowContext& context) {
    if (hours_ < 0.f) {
        hours_ = std::fmod(Gta5NumberOr(step, "start_hour", 12.f), 24.f);
    }
    // Real time, not the physics step's: a hitch must not stop the sun.
    const std::uint64_t now = SDL_GetTicks();
    const float dt =
        lastMs_ ? std::min(static_cast<float>(now - lastMs_) / 1000.f, 0.25f)
                : 0.f;
    lastMs_ = now;
    const bool fast = Gta5KeyDown(
        context.TryGet<nlohmann::json>("input.keyboard.state"), "T");
    const float rate = Gta5NumberOr(step, "minutes_per_second", 1.f) *
                       (fast ? 60.f : 1.f);
    hours_ = std::fmod(hours_ + dt * rate / 60.f, 24.f);

    const Gta5Daylight d = ComputeGta5Daylight(hours_);
    nlohmann::json light = context.Get<nlohmann::json>(
        "lighting.directional", nlohmann::json::object());
    light["direction"] = {d.lightDir.x, d.lightDir.y, d.lightDir.z};
    light["color"] = {d.lightColor.r, d.lightColor.g, d.lightColor.b};
    light["ambient"] = {d.ambient.r, d.ambient.g, d.ambient.b};
    light["exposure"] = d.exposure;
    context.Set("lighting.directional", light);
    context.Set<glm::vec4>("gta5.time.horizon", d.horizon);
    context.Set<glm::vec4>("gta5.time.zenith", d.zenith);
    context.Set<float>("gta5.time.hours", hours_);
    context.Set<float>("gta5.time.night", d.night);

    const int minutes = static_cast<int>(hours_ * 60.f);
    char text[8];
    std::snprintf(text, sizeof(text), "%02d:%02d", minutes / 60 % 24,
                  minutes % 60);
    context.Set<std::string>("gta5.clock.text", text);
    if (logger_ && minutes / 60 != shownHour_) {
        shownHour_ = minutes / 60;
        logger_->Info(std::string("gta5.time: ") + text);
    }
}

}  // namespace sdl3cpp::services::impl
