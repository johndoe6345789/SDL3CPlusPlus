#include "services/interfaces/workflow/quake3/workflow_q3_mapselect_step.hpp"
#include "services/interfaces/workflow/quake3/q3_mapselect_draw.hpp"

#include <SDL3/SDL_render.h>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

WorkflowQ3MapSelectStep::WorkflowQ3MapSelectStep(std::shared_ptr<ILogger> l)
    : logger_(std::move(l)) {}

std::string WorkflowQ3MapSelectStep::GetPluginId() const {
    return "q3.mapselect";
}

void WorkflowQ3MapSelectStep::Execute(const WorkflowStepDefinition&,
                                      WorkflowContext& context) {
    if (!context.GetBool("overlay.ready", false)) return;
    if (!context.GetBool("q3.menu_open", false)) return;
    if (context.Get<std::string>("q3.menu_screen", "main") != "map_select") {
        return;
    }

    Q3MapSelectAssets assets;
    assets.renderer = context.Get<SDL_Renderer*>("overlay.renderer", nullptr);
    if (!assets.renderer) return;

    assets.prop = context.Get<SDL_Texture*>("overlay.tex.prop", nullptr);
    assets.bigchars =
        context.Get<SDL_Texture*>("overlay.tex.bigchars", nullptr);
    assets.arrowLeft =
        context.Get<SDL_Texture*>("overlay.tex.arrow_l", nullptr);
    assets.arrowRight =
        context.Get<SDL_Texture*>("overlay.tex.arrow_r", nullptr);
    assets.btnBack =
        context.Get<SDL_Texture*>("overlay.tex.btn_back", nullptr);
    assets.btnFight =
        context.Get<SDL_Texture*>("overlay.tex.btn_fight", nullptr);
    assets.btnSkirmish =
        context.Get<SDL_Texture*>("overlay.tex.btn_skirmish", nullptr);

    assets.arenas = context.Get<std::shared_ptr<q3overlay::ArenaMap>>(
        "overlay.arena_data", nullptr);
    assets.shots = context.Get<std::shared_ptr<q3overlay::LevelshotCache>>(
        "overlay.levelshot_cache", nullptr);

    const auto bspCfg =
        context.Get<nlohmann::json>("bsp_config", nlohmann::json{});
    assets.pk3Path = bspCfg.value("pk3_path", std::string(""));
    assets.maps = context.Get<nlohmann::json>(
        "q3.maps", nlohmann::json::array({"q3dm7"}));
    assets.selectedItem = context.Get<int>("q3.menu_selected_item", 0);

    DrawQ3MapSelectScreen(assets);
}

}  // namespace sdl3cpp::services::impl
