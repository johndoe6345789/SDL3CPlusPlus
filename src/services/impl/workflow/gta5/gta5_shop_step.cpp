#include "services/interfaces/workflow/gta5/gta5_shop_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_player_pin.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_input.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5ShopStep::WorkflowGta5ShopStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5ShopStep::GetPluginId() const { return "gta5.shop"; }

bool WorkflowGta5ShopStep::Edge(const nlohmann::json* keys,
                                const char* name) {
    const bool down = Gta5KeyDown(keys, name);
    bool& was = held_[name];
    const bool pressed = down && !was;
    was = down;
    return pressed;
}

void WorkflowGta5ShopStep::Execute(const WorkflowStepDefinition& step,
                                   WorkflowContext& context) {
    if (!state_) return;
    if (!loaded_) Load(step, context);
    const auto* keys = context.TryGet<nlohmann::json>("input.keyboard.state");
    const auto ps = context.Get<Q3PlayerState>("q3.ps", Q3PlayerState{});
    std::string prompt;
    if (page_ == Gta5ShopPage::Closed) {
        const int near = NearestShop(ps.origin);
        if (near >= 0) prompt = "E  " + shops_[near].name;
        if (Edge(keys, "E") && near >= 0) {
            shop_ = near;
            page_ = Gta5ShopPage::Root;
            selected_ = 0;
            standAt_ = ps.origin;
            // Enter, Up and Down start afresh in the menu.
            for (const char* key : {"Return", "Up", "Down", "Backspace"}) {
                Edge(keys, key);
            }
        }
    } else {
        // Standing at the counter, not walking off with the arrow keys.
        if (btRigidBody* body = Gta5PlayerBody(context)) {
            PinGta5Player(context, body, standAt_);
        }
        Navigate(context, keys);
    }
    context.Set<std::string>("gta5.prompt", prompt);
    const bool open = page_ != Gta5ShopPage::Closed;
    context.Set<bool>("gta5.menu.open", open);
    if (open) context.Set<Gta5Menu>("gta5.menu", Menu(context));
}

}  // namespace sdl3cpp::services::impl
