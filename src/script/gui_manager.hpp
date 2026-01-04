#pragma once

#include "script/gui_types.hpp"
#include "script/lua_helpers.hpp"

#include <lua.hpp>

#include <memory>
#include <vector>

namespace sdl3cpp::services {
class ILogger;
}

namespace sdl3cpp::script {

class GuiManager {
public:
    GuiManager(lua_State* L, std::shared_ptr<services::ILogger> logger);

    std::vector<GuiCommand> LoadGuiCommands();
    void UpdateGuiInput(const GuiInputSnapshot& input);
    bool HasGuiCommands() const;

private:
    lua_State* L_;
    int guiInputRef_ = LUA_REFNIL;
    int guiCommandsFnRef_ = LUA_REFNIL;
    std::shared_ptr<services::ILogger> logger_;

    GuiCommand::RectData ReadRect(int index);
    GuiColor ReadColor(int index, const GuiColor& defaultColor);
    bool ReadStringField(int index, const char* name, std::string& outString);
    std::string GetLuaError();
};

} // namespace sdl3cpp::script
