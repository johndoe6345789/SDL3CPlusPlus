#pragma once

#include "../interfaces/i_gui_script_service.hpp"
#include "../interfaces/i_script_engine_service.hpp"
#include "../interfaces/i_logger.hpp"
#include "../../di/lifecycle.hpp"
#include <memory>
#include <string>

struct lua_State;

namespace sdl3cpp::services::impl {

/**
 * @brief Script-facing GUI service implementation.
 */
class GuiScriptService : public IGuiScriptService,
                         public di::IInitializable,
                         public di::IShutdownable {
public:
    GuiScriptService(std::shared_ptr<IScriptEngineService> engineService,
                     std::shared_ptr<ILogger> logger);

    void Initialize() override;
    void Shutdown() noexcept override;

    std::vector<GuiCommand> LoadGuiCommands() override;
    void UpdateGuiInput(const GuiInputSnapshot& input) override;
    bool HasGuiCommands() const override;

private:
    lua_State* GetLuaState() const;
    GuiCommand::RectData ReadRect(lua_State* L, int index) const;
    GuiColor ReadColor(lua_State* L, int index, const GuiColor& defaultColor) const;
    bool ReadStringField(lua_State* L, int index, const char* name, std::string& outString) const;

    std::shared_ptr<IScriptEngineService> engineService_;
    std::shared_ptr<ILogger> logger_;
    int guiInputRef_ = -1;
    int guiCommandsFnRef_ = -1;
};

}  // namespace sdl3cpp::services::impl
