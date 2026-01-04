#pragma once

#include "../interfaces/i_gui_script_service.hpp"
#include "../interfaces/i_script_engine_service.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Script-facing GUI service implementation.
 */
class GuiScriptService : public IGuiScriptService {
public:
    explicit GuiScriptService(std::shared_ptr<IScriptEngineService> engineService);

    std::vector<script::GuiCommand> LoadGuiCommands() override;
    void UpdateGuiInput(const script::GuiInputSnapshot& input) override;
    bool HasGuiCommands() const override;

private:
    std::shared_ptr<IScriptEngineService> engineService_;
};

}  // namespace sdl3cpp::services::impl
