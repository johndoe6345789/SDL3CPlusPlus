#include "services/interfaces/workflow/quake3/workflow_q3_bots_damage_step.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowQ3BotsDamageStep::WorkflowQ3BotsDamageStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3BotsDamageStep::GetPluginId() const { return "q3.damage.bots"; }

void WorkflowQ3BotsDamageStep::Execute(const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    const auto* botDamagePtr = context.TryGet<nlohmann::json>("q3.bot_damage");
    if (!botDamagePtr || !botDamagePtr->is_object() || botDamagePtr->empty()) return;

    const auto* botsPtr = context.TryGet<nlohmann::json>("q3.bots");
    if (!botsPtr || !botsPtr->is_array()) return;

    nlohmann::json botDamage = *botDamagePtr;
    nlohmann::json bots      = *botsPtr;

    for (auto it = botDamage.begin(); it != botDamage.end(); ++it) {
        const std::string& key    = it.key();
        const int          amount = it.value().get<int>();
        if (amount <= 0) continue;

        // Find the bot whose id matches the key suffix (e.g. "bot_0" → id==0)
        for (auto& bot : bots) {
            const std::string botKey = "bot_" + std::to_string(bot.value("id", -1));
            if (botKey != key) continue;

            int hp = bot.value("health", 0) - amount;
            bot["health"] = hp;
            if (hp <= 0) {
                bot["state"] = "dead";
                if (logger_) logger_->Info("q3.damage.bots: " + key + " killed");
            }
            break;
        }
    }

    context.Set("q3.bots",       bots);
    context.Set("q3.bot_damage", nlohmann::json::object());
}

}  // namespace sdl3cpp::services::impl
