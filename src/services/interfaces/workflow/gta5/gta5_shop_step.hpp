#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_garage.hpp"
#include "services/interfaces/workflow/gta5/gta5_menu.hpp"
#include "services/interfaces/workflow/gta5/gta5_roads.hpp"
#include "services/interfaces/workflow/gta5/gta5_settings.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"
#include "services/interfaces/workflow/gta5/gta5_weapons.hpp"

#include <nlohmann/json.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::services::impl {

/// A shop on the map: Los Santos Customs (a garage) or Ammu-Nation.
struct Gta5Shop {
    bool garage{false};
    std::string name;
    glm::vec3 at{0.f};  // engine space; y unknown, so compared flat
};

enum class Gta5ShopPage { Closed, Root, Cars, Colours, Weapons };

/**
 * Plugin ID: gta5.shop
 *
 * Walk up to a Los Santos Customs or an Ammu-Nation (poi_file) and E --
 * the pad's D-pad right -- opens its menu, which the HUD draws
 * (gta5.menu): Up and Down choose, Enter takes, Backspace goes back,
 * and the player stands still meanwhile. The garage swaps the car for
 * any in garage_file, parked on the nearest road (roads_dir), or
 * resprays it; Ammu-Nation hands over guns, ammunition and armour.
 */
class WorkflowGta5ShopStep final : public IWorkflowStep {
public:
    WorkflowGta5ShopStep(std::shared_ptr<ILogger> logger,
                         std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    void Load(const WorkflowStepDefinition& step, WorkflowContext& context);
    int NearestShop(const glm::vec3& at) const;
    bool Edge(const nlohmann::json* keys, const char* name);
    Gta5Menu Menu(WorkflowContext& context) const;
    void Navigate(WorkflowContext& context, const nlohmann::json* keys);
    void Choose(WorkflowContext& context);
    void SwapCar(WorkflowContext& context, std::size_t car);
    void Repaint(WorkflowContext& context, std::size_t colour);
    void GiveWeapon(WorkflowContext& context, std::size_t weapon);
    void FillAmmo(WorkflowContext& context);
    /// Keeps the car, colour, guns and armour (Gta5SettingsPath).
    void Remember(WorkflowContext& context);

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    std::vector<Gta5Shop> shops_;
    Gta5Garage garage_;
    std::vector<Gta5Weapon> weapons_;
    Gta5Roads roads_;
    Gta5Settings settings_;
    std::string roadsDir_;
    std::unordered_map<std::string, bool> held_;
    Gta5ShopPage page_{Gta5ShopPage::Closed};
    int shop_{-1};
    int selected_{0};
    int colour_{-1};  // the last respray, kept for the next car
    glm::vec3 standAt_{0.f};
    bool loaded_{false};
};

}  // namespace sdl3cpp::services::impl
