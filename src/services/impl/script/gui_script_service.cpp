#include "gui_script_service.hpp"

#include "lua_helpers.hpp"
#include "services/interfaces/i_logger.hpp"

#include <lua.hpp>

#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

GuiScriptService::GuiScriptService(std::shared_ptr<IScriptEngineService> engineService,
                                   std::shared_ptr<ILogger> logger)
    : engineService_(std::move(engineService)),
      logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("GuiScriptService", "GuiScriptService",
                       "engineService=" + std::string(engineService_ ? "set" : "null"));
    }
}

void GuiScriptService::Initialize() {
    if (logger_) {
        logger_->Trace("GuiScriptService", "Initialize");
    }
    lua_State* L = GetLuaState();

    lua_getglobal(L, "gui_input");
    if (!lua_isnil(L, -1)) {
        guiInputRef_ = luaL_ref(L, LUA_REGISTRYINDEX);
    } else {
        lua_pop(L, 1);
    }

    lua_getglobal(L, "get_gui_commands");
    if (lua_isfunction(L, -1)) {
        guiCommandsFnRef_ = luaL_ref(L, LUA_REGISTRYINDEX);
    } else {
        lua_pop(L, 1);
    }
}

void GuiScriptService::Shutdown() noexcept {
    if (logger_) {
        logger_->Trace("GuiScriptService", "Shutdown");
    }

    if (!engineService_ || !engineService_->IsInitialized()) {
        guiInputRef_ = -1;
        guiCommandsFnRef_ = -1;
        return;
    }

    lua_State* L = engineService_->GetLuaState();
    if (!L) {
        guiInputRef_ = -1;
        guiCommandsFnRef_ = -1;
        return;
    }

    if (guiInputRef_ >= 0) {
        luaL_unref(L, LUA_REGISTRYINDEX, guiInputRef_);
    }
    if (guiCommandsFnRef_ >= 0) {
        luaL_unref(L, LUA_REGISTRYINDEX, guiCommandsFnRef_);
    }
    guiInputRef_ = -1;
    guiCommandsFnRef_ = -1;
}

std::vector<GuiCommand> GuiScriptService::LoadGuiCommands() {
    if (logger_) {
        logger_->Trace("GuiScriptService", "LoadGuiCommands");
    }
    if (guiCommandsFnRef_ < 0) {
        return {};
    }
    lua_State* L = GetLuaState();

    std::vector<GuiCommand> commands;
    lua_rawgeti(L, LUA_REGISTRYINDEX, guiCommandsFnRef_);
    if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        std::string message = lua::GetLuaError(L);
        lua_pop(L, 1);
        if (logger_) {
            logger_->Error("Lua get_gui_commands failed: " + message);
        }
        throw std::runtime_error("Lua get_gui_commands failed: " + message);
    }
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        if (logger_) {
            logger_->Error("'get_gui_commands' did not return a table");
        }
        throw std::runtime_error("'get_gui_commands' did not return a table");
    }

    size_t count = lua_rawlen(L, -1);
    commands.reserve(count);

    for (size_t i = 1; i <= count; ++i) {
        lua_rawgeti(L, -1, static_cast<int>(i));
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            if (logger_) {
                logger_->Error("GUI command at index " + std::to_string(i) + " is not a table");
            }
            throw std::runtime_error("GUI command at index " + std::to_string(i) + " is not a table");
        }
        int commandIndex = lua_gettop(L);
        lua_getfield(L, commandIndex, "type");
        const char* typeName = lua_tostring(L, -1);
        if (!typeName) {
            lua_pop(L, 2);
            if (logger_) {
                logger_->Error("GUI command at index " + std::to_string(i) + " is missing a type");
            }
            throw std::runtime_error("GUI command at index " + std::to_string(i) + " is missing a type");
        }
        GuiCommand command{};
        if (std::strcmp(typeName, "rect") == 0) {
            command.type = GuiCommand::Type::Rect;
            command.rect = ReadRect(L, commandIndex);
            command.color = ReadColorField(L, commandIndex, "color", GuiColor{0.0f, 0.0f, 0.0f, 1.0f});
            command.borderColor = ReadColorField(L, commandIndex, "borderColor",
                                                 GuiColor{0.0f, 0.0f, 0.0f, 0.0f});
            lua_getfield(L, commandIndex, "borderWidth");
            if (lua_isnumber(L, -1)) {
                command.borderWidth = static_cast<float>(lua_tonumber(L, -1));
            }
            lua_pop(L, 1);
        } else if (std::strcmp(typeName, "text") == 0) {
            command.type = GuiCommand::Type::Text;
            ReadStringField(L, commandIndex, "text", command.text);
            bool hasX = false;
            bool hasY = false;
            lua_getfield(L, commandIndex, "x");
            if (lua_isnumber(L, -1)) {
                command.rect.x = static_cast<float>(lua_tonumber(L, -1));
                hasX = true;
            }
            lua_pop(L, 1);
            lua_getfield(L, commandIndex, "y");
            if (lua_isnumber(L, -1)) {
                command.rect.y = static_cast<float>(lua_tonumber(L, -1));
                hasY = true;
            }
            lua_pop(L, 1);
            if (logger_ && (!hasX || !hasY)) {
                logger_->Trace("GuiScriptService", "LoadGuiCommands",
                               "Text command missing x/y; defaulting to 0");
            }
            lua_getfield(L, commandIndex, "fontSize");
            if (lua_isnumber(L, -1)) {
                command.fontSize = static_cast<float>(lua_tonumber(L, -1));
            }
            lua_pop(L, 1);
            std::string align;
            if (ReadStringField(L, commandIndex, "alignX", align)) {
                command.alignX = align;
            }
            if (ReadStringField(L, commandIndex, "alignY", align)) {
                command.alignY = align;
            }
            lua_getfield(L, commandIndex, "clipRect");
            if (lua_istable(L, -1)) {
                command.clipRect = ReadRect(L, -1);
                command.hasClipRect = true;
            }
            lua_pop(L, 1);
            lua_getfield(L, commandIndex, "bounds");
            if (lua_istable(L, -1)) {
                command.bounds = ReadRect(L, -1);
                command.hasBounds = true;
            }
            lua_pop(L, 1);
            command.color = ReadColorField(L, commandIndex, "color", GuiColor{1.0f, 1.0f, 1.0f, 1.0f});
        } else if (std::strcmp(typeName, "clip_push") == 0) {
            command.type = GuiCommand::Type::ClipPush;
            lua_getfield(L, commandIndex, "rect");
            if (lua_istable(L, -1)) {
                command.rect = ReadRect(L, -1);
            } else {
                command.rect = ReadRect(L, commandIndex);
                if (logger_) {
                    logger_->Trace("GuiScriptService", "LoadGuiCommands",
                                   "clipPushFallback=true");
                }
            }
            lua_pop(L, 1);
        } else if (std::strcmp(typeName, "clip_pop") == 0) {
            command.type = GuiCommand::Type::ClipPop;
        } else if (std::strcmp(typeName, "svg") == 0) {
            command.type = GuiCommand::Type::Svg;
            ReadStringField(L, commandIndex, "path", command.svgPath);
            command.rect = ReadRect(L, commandIndex);
            command.svgTint = ReadColorField(L, commandIndex, "tint", GuiColor{1.0f, 1.0f, 1.0f, 0.0f});
            command.svgTint = ReadColorField(L, commandIndex, "color", command.svgTint);
        }
        lua_pop(L, 1);
        lua_pop(L, 1);
        commands.push_back(std::move(command));
    }

    lua_pop(L, 1);
    return commands;
}

void GuiScriptService::UpdateGuiInput(const GuiInputSnapshot& input) {
    if (logger_) {
        logger_->Trace("GuiScriptService", "UpdateGuiInput",
                       "mouseX=" + std::to_string(input.mouseX) +
                       ", mouseY=" + std::to_string(input.mouseY) +
                       ", mouseDeltaX=" + std::to_string(input.mouseDeltaX) +
                       ", mouseDeltaY=" + std::to_string(input.mouseDeltaY) +
                       ", mouseDown=" + std::string(input.mouseDown ? "true" : "false") +
                       ", wheel=" + std::to_string(input.wheel) +
                       ", textInput.size=" + std::to_string(input.textInput.size()) +
                       ", keyStates.size=" + std::to_string(input.keyStates.size()));
    }
    if (guiInputRef_ < 0) {
        return;
    }
    lua_State* L = GetLuaState();

    lua_rawgeti(L, LUA_REGISTRYINDEX, guiInputRef_);
    int stateIndex = lua_gettop(L);

    lua_getfield(L, stateIndex, "resetTransient");
    lua_pushvalue(L, stateIndex);
    lua_call(L, 1, 0);

    lua_getfield(L, stateIndex, "setMouse");
    lua_pushvalue(L, stateIndex);
    lua_pushnumber(L, input.mouseX);
    lua_pushnumber(L, input.mouseY);
    lua_pushboolean(L, input.mouseDown);
    lua_pushnumber(L, input.mouseDeltaX);
    lua_pushnumber(L, input.mouseDeltaY);
    lua_call(L, 6, 0);

    lua_getfield(L, stateIndex, "setWheel");
    lua_pushvalue(L, stateIndex);
    lua_pushnumber(L, input.wheel);
    lua_call(L, 2, 0);

    lua_getfield(L, stateIndex, "setGamepad");
    if (lua_isfunction(L, -1)) {
        lua_pushvalue(L, stateIndex);
        lua_pushboolean(L, input.gamepadConnected);
        lua_pushnumber(L, input.gamepadLeftX);
        lua_pushnumber(L, input.gamepadLeftY);
        lua_pushnumber(L, input.gamepadRightX);
        lua_pushnumber(L, input.gamepadRightY);
        lua_pushboolean(L, input.gamepadTogglePressed);
        lua_call(L, 7, 0);
    } else {
        lua_pop(L, 1);
    }

    if (!input.textInput.empty()) {
        lua_getfield(L, stateIndex, "addTextInput");
        lua_pushvalue(L, stateIndex);
        lua_pushstring(L, input.textInput.c_str());
        lua_call(L, 2, 0);
    }

    for (const auto& [key, pressed] : input.keyStates) {
        lua_getfield(L, stateIndex, "setKey");
        lua_pushvalue(L, stateIndex);
        lua_pushstring(L, key.c_str());
        lua_pushboolean(L, pressed);
        lua_call(L, 3, 0);
    }

    lua_pop(L, 1);
}

bool GuiScriptService::HasGuiCommands() const {
    if (logger_) {
        logger_->Trace("GuiScriptService", "HasGuiCommands");
    }
    return guiCommandsFnRef_ >= 0;
}

GuiCommand::RectData GuiScriptService::ReadRect(lua_State* L, int index) const {
    if (logger_) {
        logger_->Trace("GuiScriptService", "ReadRect",
                       "index=" + std::to_string(index));
    }
    GuiCommand::RectData rect{};
    if (!lua_istable(L, index)) {
        return rect;
    }
    int absIndex = lua_absindex(L, index);
    auto readField = [&](const char* name, float defaultValue) -> float {
        lua_getfield(L, absIndex, name);
        float value = defaultValue;
        if (lua_isnumber(L, -1)) {
            value = static_cast<float>(lua_tonumber(L, -1));
        }
        lua_pop(L, 1);
        return value;
    };
    rect.x = readField("x", rect.x);
    rect.y = readField("y", rect.y);
    rect.width = readField("width", rect.width);
    rect.height = readField("height", rect.height);
    return rect;
}

GuiColor GuiScriptService::ReadColor(lua_State* L, int index, const GuiColor& defaultColor) const {
    if (logger_) {
        logger_->Trace("GuiScriptService", "ReadColor",
                       "index=" + std::to_string(index));
    }
    GuiColor color = defaultColor;
    if (!lua_istable(L, index)) {
        return color;
    }
    int absIndex = lua_absindex(L, index);
    for (int component = 0; component < 4; ++component) {
        lua_rawgeti(L, absIndex, component + 1);
        if (lua_isnumber(L, -1)) {
            float value = static_cast<float>(lua_tonumber(L, -1));
            switch (component) {
                case 0: color.r = value; break;
                case 1: color.g = value; break;
                case 2: color.b = value; break;
                case 3: color.a = value; break;
            }
        }
        lua_pop(L, 1);
    }
    return color;
}

GuiColor GuiScriptService::ReadColorField(lua_State* L, int index, const char* name,
                                          const GuiColor& defaultColor) const {
    if (logger_) {
        logger_->Trace("GuiScriptService", "ReadColorField",
                       "index=" + std::to_string(index) +
                       ", name=" + std::string(name ? name : ""));
    }
    if (!lua_istable(L, index) || !name) {
        return defaultColor;
    }
    int absIndex = lua_absindex(L, index);
    lua_getfield(L, absIndex, name);
    GuiColor color = defaultColor;
    if (lua_istable(L, -1)) {
        color = ReadColor(L, -1, defaultColor);
    } else if (logger_) {
        logger_->Trace("GuiScriptService", "ReadColorField",
                       "Field not found or not table: " + std::string(name));
    }
    lua_pop(L, 1);
    return color;
}

bool GuiScriptService::ReadStringField(lua_State* L, int index, const char* name, std::string& outString) const {
    if (logger_) {
        logger_->Trace("GuiScriptService", "ReadStringField",
                       "index=" + std::to_string(index) +
                       ", name=" + std::string(name ? name : ""));
    }
    int absIndex = lua_absindex(L, index);
    lua_getfield(L, absIndex, name);
    if (lua_isstring(L, -1)) {
        outString = lua_tostring(L, -1);
        lua_pop(L, 1);
        return true;
    }
    lua_pop(L, 1);
    return false;
}

lua_State* GuiScriptService::GetLuaState() const {
    if (logger_) {
        logger_->Trace("GuiScriptService", "GetLuaState");
    }
    if (!engineService_) {
        throw std::runtime_error("GUI script service is missing script engine service");
    }
    lua_State* state = engineService_->GetLuaState();
    if (!state) {
        throw std::runtime_error("Lua state is not initialized");
    }
    return state;
}

}  // namespace sdl3cpp::services::impl
