#include "gui_script_service.hpp"
#include <utility>

namespace sdl3cpp::services::impl {

GuiScriptService::GuiScriptService(std::shared_ptr<IScriptEngineService> engineService)
    : engineService_(std::move(engineService)) {
}

std::vector<script::GuiCommand> GuiScriptService::LoadGuiCommands() {
    return engineService_->GetEngine().LoadGuiCommands();
}

void GuiScriptService::UpdateGuiInput(const script::GuiInputSnapshot& input) {
    engineService_->GetEngine().UpdateGuiInput(input);
}

bool GuiScriptService::HasGuiCommands() const {
    return engineService_->GetEngine().HasGuiCommands();
}

}  // namespace sdl3cpp::services::impl
