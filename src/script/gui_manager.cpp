#include "script/gui_manager.hpp"
#include "logging/logger.hpp"

#include <lua.hpp>

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace sdl3cpp::script {

GuiManager::GuiManager(lua_State* L) : L_(L) {
    lua_getglobal(L_, "gui_input");
    if (!lua_isnil(L_, -1)) {
        guiInputRef_ = luaL_ref(L_, LUA_REGISTRYINDEX);
    } else {
        lua_pop(L_, 1);
    }

    lua_getglobal(L_, "get_gui_commands");
    if (lua_isfunction(L_, -1)) {
        guiCommandsFnRef_ = luaL_ref(L_, LUA_REGISTRYINDEX);
    } else {
        lua_pop(L_, 1);
    }
}

std::vector<GuiCommand> GuiManager::LoadGuiCommands() {
    std::vector<GuiCommand> commands;
    if (guiCommandsFnRef_ == LUA_REFNIL) {
        return commands;
    }
    lua_rawgeti(L_, LUA_REGISTRYINDEX, guiCommandsFnRef_);
    if (lua_pcall(L_, 0, 1, 0) != LUA_OK) {
        std::string message = GetLuaError();
        lua_pop(L_, 1);
        LOG_ERROR("Lua get_gui_commands failed: " + message);
        throw std::runtime_error("Lua get_gui_commands failed: " + message);
    }
    if (!lua_istable(L_, -1)) {
        lua_pop(L_, 1);
        LOG_ERROR("'get_gui_commands' did not return a table");
        throw std::runtime_error("'get_gui_commands' did not return a table");
    }

    size_t count = lua_rawlen(L_, -1);
    commands.reserve(count);

    for (size_t i = 1; i <= count; ++i) {
        lua_rawgeti(L_, -1, static_cast<int>(i));
        if (!lua_istable(L_, -1)) {
            lua_pop(L_, 1);
            LOG_ERROR("GUI command at index " + std::to_string(i) + " is not a table");
            throw std::runtime_error("GUI command at index " + std::to_string(i) + " is not a table");
        }
        int commandIndex = lua_gettop(L_);
        lua_getfield(L_, commandIndex, "type");
        const char* typeName = lua_tostring(L_, -1);
        if (!typeName) {
            lua_pop(L_, 2);
            LOG_ERROR("GUI command at index " + std::to_string(i) + " is missing a type");
            throw std::runtime_error("GUI command at index " + std::to_string(i) + " is missing a type");
        }
        GuiCommand command{};
        if (std::strcmp(typeName, "rect") == 0) {
            command.type = GuiCommand::Type::Rect;
            command.rect = ReadRect(commandIndex);
            command.color = ReadColor(commandIndex, GuiColor{0.0f, 0.0f, 0.0f, 1.0f});
            command.borderColor = ReadColor(commandIndex, GuiColor{0.0f, 0.0f, 0.0f, 0.0f});
            lua_getfield(L_, commandIndex, "borderWidth");
            if (lua_isnumber(L_, -1)) {
                command.borderWidth = static_cast<float>(lua_tonumber(L_, -1));
            }
            lua_pop(L_, 1);
        } else if (std::strcmp(typeName, "text") == 0) {
            command.type = GuiCommand::Type::Text;
            ReadStringField(commandIndex, "text", command.text);
            lua_getfield(L_, commandIndex, "fontSize");
            if (lua_isnumber(L_, -1)) {
                command.fontSize = static_cast<float>(lua_tonumber(L_, -1));
            }
            lua_pop(L_, 1);
            std::string align;
            if (ReadStringField(commandIndex, "alignX", align)) {
                command.alignX = align;
            }
            if (ReadStringField(commandIndex, "alignY", align)) {
                command.alignY = align;
            }
            lua_getfield(L_, commandIndex, "clipRect");
            if (lua_istable(L_, -1)) {
                command.clipRect = ReadRect(-1);
                command.hasClipRect = true;
            }
            lua_pop(L_, 1);
            lua_getfield(L_, commandIndex, "bounds");
            if (lua_istable(L_, -1)) {
                command.bounds = ReadRect(-1);
                command.hasBounds = true;
            }
            lua_pop(L_, 1);
            command.color = ReadColor(commandIndex, GuiColor{1.0f, 1.0f, 1.0f, 1.0f});
        } else if (std::strcmp(typeName, "clip_push") == 0) {
            command.type = GuiCommand::Type::ClipPush;
            command.rect = ReadRect(commandIndex);
        } else if (std::strcmp(typeName, "clip_pop") == 0) {
            command.type = GuiCommand::Type::ClipPop;
        } else if (std::strcmp(typeName, "svg") == 0) {
            command.type = GuiCommand::Type::Svg;
            ReadStringField(commandIndex, "path", command.svgPath);
            command.rect = ReadRect(commandIndex);
            command.svgTint = ReadColor(commandIndex, GuiColor{1.0f, 1.0f, 1.0f, 0.0f});
            lua_getfield(L_, commandIndex, "tint");
            if (lua_istable(L_, -1)) {
                command.svgTint = ReadColor(-1, command.svgTint);
            }
            lua_pop(L_, 1);
        }
        lua_pop(L_, 1);
        lua_pop(L_, 1);
        commands.push_back(std::move(command));
    }

    lua_pop(L_, 1);
    return commands;
}

void GuiManager::UpdateGuiInput(const GuiInputSnapshot& input) {
    if (guiInputRef_ == LUA_REFNIL) {
        return;
    }
    lua_rawgeti(L_, LUA_REGISTRYINDEX, guiInputRef_);
    int stateIndex = lua_gettop(L_);

    lua_getfield(L_, stateIndex, "resetTransient");
    lua_pushvalue(L_, stateIndex);
    lua_call(L_, 1, 0);

    lua_getfield(L_, stateIndex, "setMouse");
    lua_pushvalue(L_, stateIndex);
    lua_pushnumber(L_, input.mouseX);
    lua_pushnumber(L_, input.mouseY);
    lua_pushboolean(L_, input.mouseDown);
    lua_call(L_, 4, 0);

    lua_getfield(L_, stateIndex, "setWheel");
    lua_pushvalue(L_, stateIndex);
    lua_pushnumber(L_, input.wheel);
    lua_call(L_, 2, 0);

    if (!input.textInput.empty()) {
        lua_getfield(L_, stateIndex, "addTextInput");
        lua_pushvalue(L_, stateIndex);
        lua_pushstring(L_, input.textInput.c_str());
        lua_call(L_, 2, 0);
    }

    for (const auto& [key, pressed] : input.keyStates) {
        lua_getfield(L_, stateIndex, "setKey");
        lua_pushvalue(L_, stateIndex);
        lua_pushstring(L_, key.c_str());
        lua_pushboolean(L_, pressed);
        lua_call(L_, 3, 0);
    }

    lua_pop(L_, 1);
}

bool GuiManager::HasGuiCommands() const {
    return guiCommandsFnRef_ != LUA_REFNIL;
}

GuiCommand::RectData GuiManager::ReadRect(int index) {
    GuiCommand::RectData rect{};
    if (!lua_istable(L_, index)) {
        return rect;
    }
    int absIndex = lua_absindex(L_, index);
    auto readField = [&](const char* name, float defaultValue) -> float {
        lua_getfield(L_, absIndex, name);
        float value = defaultValue;
        if (lua_isnumber(L_, -1)) {
            value = static_cast<float>(lua_tonumber(L_, -1));
        }
        lua_pop(L_, 1);
        return value;
    };
    rect.x = readField("x", rect.x);
    rect.y = readField("y", rect.y);
    rect.width = readField("width", rect.width);
    rect.height = readField("height", rect.height);
    return rect;
}

GuiColor GuiManager::ReadColor(int index, const GuiColor& defaultColor) {
    GuiColor color = defaultColor;
    if (!lua_istable(L_, index)) {
        return color;
    }
    int absIndex = lua_absindex(L_, index);
    for (int component = 0; component < 4; ++component) {
        lua_rawgeti(L_, absIndex, component + 1);
        if (lua_isnumber(L_, -1)) {
            float value = static_cast<float>(lua_tonumber(L_, -1));
            switch (component) {
                case 0: color.r = value; break;
                case 1: color.g = value; break;
                case 2: color.b = value; break;
                case 3: color.a = value; break;
            }
        }
        lua_pop(L_, 1);
    }
    return color;
}

bool GuiManager::ReadStringField(int index, const char* name, std::string& outString) {
    int absIndex = lua_absindex(L_, index);
    lua_getfield(L_, absIndex, name);
    if (lua_isstring(L_, -1)) {
        outString = lua_tostring(L_, -1);
        lua_pop(L_, 1);
        return true;
    }
    lua_pop(L_, 1);
    return false;
}

std::string GuiManager::GetLuaError() {
    const char* message = lua_tostring(L_, -1);
    return message ? message : "unknown lua error";
}

} // namespace sdl3cpp::script