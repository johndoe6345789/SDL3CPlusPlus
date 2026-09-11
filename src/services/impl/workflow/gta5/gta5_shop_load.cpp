#include "services/interfaces/workflow/gta5/gta5_shop_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <algorithm>
#include <fstream>

namespace sdl3cpp::services::impl {

void WorkflowGta5ShopStep::Load(const WorkflowStepDefinition& step,
                                WorkflowContext& context) {
    loaded_ = true;
    roadsDir_ = Gta5ParameterOr(step, "roads_dir", "");
    garage_ = LoadGta5Garage(Gta5ResolvePath(
        step, context, "garage_file", "packages/gta5/data/garage.json"));
    weapons_ = LoadGta5Weapons(Gta5ResolvePath(
        step, context, "weapons_file", "packages/gta5/data/weapons.json"));
    if (LoadGta5Settings(state_->settings)) {
        Gta5Inventory& kept = state_->settings.inventory;
        colour_ = state_->settings.colour;
        kept.Fit(weapons_.size());
        // Nothing kept leaves the pistol gta5.weapon hands out, rather
        // than a player standing there empty handed.
        if (std::any_of(kept.clip.begin(), kept.clip.end(),
                        [](int c) { return c >= 0; })) {
            context.Set("gta5.inventory", kept);
        }
        context.Set<float>("gta5.player.armour",
                           state_->settings.armour);
        if (logger_) logger_->Info("gta5.shop: kept in " + Gta5SettingsPath());
    }
    // The shops are the map's own points: every LS Customs and every
    // Ammu-Nation, GTA (x, y) to engine (x, ?, -y).
    std::ifstream in(Gta5ResolvePath(step, context, "poi_file",
                                     "packages/gta5/config/map_poi.json"));
    const auto doc = nlohmann::json::parse(in, nullptr, false);
    const auto points = doc.is_object()
                            ? doc.value("points", nlohmann::json::array())
                            : nlohmann::json::array();
    for (const auto& p : points) {
        const std::string category = p.value("category", std::string());
        if (category != "customs" && category != "ammu") continue;
        const bool garage = category == "customs";
        shops_.push_back({garage, garage ? "LS CUSTOMS" : "AMMU-NATION",
                          glm::vec3(p.value("x", 0.f), 0.f,
                                    -p.value("y", 0.f))});
    }
    if (logger_) {
        logger_->Info("gta5.shop: " + std::to_string(shops_.size()) +
                      " shops, " + std::to_string(garage_.cars.size()) +
                      " cars, " + std::to_string(weapons_.size()) +
                      " weapons");
    }
}

int WorkflowGta5ShopStep::NearestShop(const glm::vec3& at) const {
    if (state_->seated >= 0) return -1;  // on foot, at the door
    int best = -1;
    float nearest = 10.f;
    for (std::size_t i = 0; i < shops_.size(); ++i) {
        const float d = glm::distance(glm::vec2(at.x, at.z),
                                      glm::vec2(shops_[i].at.x,
                                                shops_[i].at.z));
        if (d < nearest) {
            nearest = d;
            best = static_cast<int>(i);
        }
    }
    return best;
}

}  // namespace sdl3cpp::services::impl
