#include "render_graph_script_service.hpp"

#include "lua_helpers.hpp"
#include "services/interfaces/i_logger.hpp"

#include <lua.hpp>

#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

RenderGraphScriptService::RenderGraphScriptService(std::shared_ptr<IScriptEngineService> engineService,
                                                   std::shared_ptr<IConfigService> configService,
                                                   std::shared_ptr<ILogger> logger)
    : engineService_(std::move(engineService)),
      configService_(std::move(configService)),
      logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("RenderGraphScriptService", "RenderGraphScriptService",
                       "engineService=" + std::string(engineService_ ? "set" : "null") +
                       ", configService=" + std::string(configService_ ? "set" : "null"));
    }
}

bool RenderGraphScriptService::IsEnabled() const {
    if (!configService_) {
        return false;
    }
    return configService_->GetRenderGraphConfig().enabled;
}

bool RenderGraphScriptService::HasRenderGraphFunction() const {
    if (!IsEnabled()) {
        return false;
    }
    lua_State* L = GetLuaState();
    const auto& config = GetRenderGraphConfig();
    lua_getglobal(L, config.functionName.c_str());
    bool isFunction = lua_isfunction(L, -1);
    lua_pop(L, 1);
    return isFunction;
}

RenderGraphDefinition RenderGraphScriptService::LoadRenderGraph() {
    if (logger_) {
        logger_->Trace("RenderGraphScriptService", "LoadRenderGraph");
    }

    if (!IsEnabled()) {
        if (logger_) {
            logger_->Trace("RenderGraphScriptService", "LoadRenderGraph",
                           "renderGraphEnabled=false");
        }
        return {};
    }

    if (graphLoaded_) {
        return cachedGraph_;
    }

    lua_State* L = GetLuaState();
    const auto& config = GetRenderGraphConfig();
    lua_getglobal(L, config.functionName.c_str());
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        if (logger_) {
            logger_->Error("Lua render graph function '" + config.functionName + "' is missing");
        }
        throw std::runtime_error("Lua render graph function '" + config.functionName + "' is missing");
    }

    if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        std::string message = lua::GetLuaError(L);
        lua_pop(L, 1);
        if (logger_) {
            logger_->Error("Lua render graph failed: " + message);
        }
        throw std::runtime_error("Lua render graph failed: " + message);
    }

    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        if (logger_) {
            logger_->Error("Render graph function did not return a table");
        }
        throw std::runtime_error("Render graph function did not return a table");
    }

    cachedGraph_ = ParseRenderGraph(L, lua_gettop(L));
    graphLoaded_ = true;
    lua_pop(L, 1);

    if (logger_) {
        logger_->Info("Loaded render graph with resources=" +
                      std::to_string(cachedGraph_.resources.size()) +
                      ", passes=" + std::to_string(cachedGraph_.passes.size()));
    }

    return cachedGraph_;
}

lua_State* RenderGraphScriptService::GetLuaState() const {
    if (logger_) {
        logger_->Trace("RenderGraphScriptService", "GetLuaState");
    }
    if (!engineService_) {
        throw std::runtime_error("Render graph script service is missing script engine service");
    }
    lua_State* state = engineService_->GetLuaState();
    if (!state) {
        throw std::runtime_error("Lua state is not initialized");
    }
    return state;
}

const RenderGraphConfig& RenderGraphScriptService::GetRenderGraphConfig() const {
    if (!configService_) {
        throw std::runtime_error("Render graph script service is missing config service");
    }
    return configService_->GetRenderGraphConfig();
}

RenderGraphDefinition RenderGraphScriptService::ParseRenderGraph(lua_State* L, int index) const {
    RenderGraphDefinition graph;
    int graphIndex = lua_absindex(L, index);

    lua_getfield(L, graphIndex, "resources");
    if (lua_istable(L, -1)) {
        int resourcesIndex = lua_absindex(L, -1);
        lua_pushnil(L);
        while (lua_next(L, resourcesIndex) != 0) {
            if (lua_isstring(L, -2) && lua_istable(L, -1)) {
                std::string name = lua_tostring(L, -2);
                graph.resources.push_back(ParseResource(L, lua_gettop(L), name));
            } else if (logger_) {
                logger_->Warn("RenderGraphScriptService: Skipping invalid resource entry");
            }
            lua_pop(L, 1);
        }
    } else if (!lua_isnil(L, -1)) {
        lua_pop(L, 1);
        throw std::runtime_error("Render graph 'resources' must be a table when provided");
    }
    lua_pop(L, 1);

    lua_getfield(L, graphIndex, "passes");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        throw std::runtime_error("Render graph 'passes' must be a table");
    }

    int passesIndex = lua_absindex(L, -1);
    size_t count = lua_rawlen(L, passesIndex);
    graph.passes.reserve(count);
    for (size_t i = 1; i <= count; ++i) {
        lua_rawgeti(L, passesIndex, static_cast<int>(i));
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            throw std::runtime_error("Render graph pass at index " +
                                     std::to_string(i) + " must be a table");
        }
        graph.passes.push_back(ParsePass(L, lua_gettop(L), i));
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    if (graph.passes.empty()) {
        throw std::runtime_error("Render graph must define at least one pass");
    }

    return graph;
}

RenderGraphResource RenderGraphScriptService::ParseResource(lua_State* L, int index,
                                                            const std::string& name) const {
    RenderGraphResource resource;
    resource.name = name;
    int resourceIndex = lua_absindex(L, index);

    auto readRequiredString = [&](const char* field, std::string& target) {
        lua_getfield(L, resourceIndex, field);
        if (!lua_isstring(L, -1)) {
            lua_pop(L, 1);
            throw std::runtime_error("Render graph resource '" + name +
                                     "' field '" + field + "' must be a string");
        }
        target = lua_tostring(L, -1);
        lua_pop(L, 1);
    };

    auto readOptionalNumber = [&](const char* field, uint32_t& target) {
        lua_getfield(L, resourceIndex, field);
        if (lua_isnumber(L, -1)) {
            double value = lua_tonumber(L, -1);
            target = value < 0.0 ? 0u : static_cast<uint32_t>(value);
        } else if (!lua_isnil(L, -1)) {
            lua_pop(L, 1);
            throw std::runtime_error("Render graph resource '" + name +
                                     "' field '" + field + "' must be a number");
        }
        lua_pop(L, 1);
    };

    readRequiredString("type", resource.type);
    readRequiredString("format", resource.format);

    resource.size = "swapchain";
    lua_getfield(L, resourceIndex, "size");
    if (lua_isstring(L, -1)) {
        resource.size = lua_tostring(L, -1);
    } else if (lua_istable(L, -1)) {
        int sizeIndex = lua_absindex(L, -1);
        bool parsed = false;
        size_t sizeLen = lua_rawlen(L, sizeIndex);
        if (sizeLen >= 2) {
            lua_rawgeti(L, sizeIndex, 1);
            lua_rawgeti(L, sizeIndex, 2);
            if (lua_isnumber(L, -2) && lua_isnumber(L, -1)) {
                resource.explicitSize[0] = static_cast<uint32_t>(lua_tonumber(L, -2));
                resource.explicitSize[1] = static_cast<uint32_t>(lua_tonumber(L, -1));
                resource.hasExplicitSize = true;
                parsed = true;
            }
            lua_pop(L, 2);
        }
        if (!parsed) {
            lua_getfield(L, sizeIndex, "width");
            lua_getfield(L, sizeIndex, "height");
            if (lua_isnumber(L, -2) && lua_isnumber(L, -1)) {
                resource.explicitSize[0] = static_cast<uint32_t>(lua_tonumber(L, -2));
                resource.explicitSize[1] = static_cast<uint32_t>(lua_tonumber(L, -1));
                resource.hasExplicitSize = true;
                parsed = true;
            }
            lua_pop(L, 2);
        }
        if (!parsed) {
            lua_pop(L, 1);
            throw std::runtime_error("Render graph resource '" + name +
                                     "' size must be a string or {width,height}");
        }
    } else if (!lua_isnil(L, -1)) {
        lua_pop(L, 1);
        throw std::runtime_error("Render graph resource '" + name +
                                 "' size must be a string or table");
    }
    lua_pop(L, 1);

    readOptionalNumber("layers", resource.layers);
    readOptionalNumber("mips", resource.mips);

    return resource;
}

RenderGraphPass RenderGraphScriptService::ParsePass(lua_State* L, int index, size_t passIndex) const {
    RenderGraphPass pass;
    int passTableIndex = lua_absindex(L, index);

    lua_getfield(L, passTableIndex, "name");
    if (lua_isstring(L, -1)) {
        pass.name = lua_tostring(L, -1);
    } else {
        pass.name = "pass_" + std::to_string(passIndex);
    }
    lua_pop(L, 1);

    lua_getfield(L, passTableIndex, "kind");
    if (!lua_isstring(L, -1)) {
        lua_pop(L, 1);
        throw std::runtime_error("Render graph pass '" + pass.name + "' missing 'kind' string");
    }
    pass.kind = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, passTableIndex, "shader");
    if (lua_isstring(L, -1)) {
        pass.shader = lua_tostring(L, -1);
    } else if (!lua_isnil(L, -1) && logger_) {
        logger_->Warn("RenderGraphScriptService: pass '" + pass.name + "' shader must be a string");
    }
    lua_pop(L, 1);

    lua_getfield(L, passTableIndex, "output");
    if (lua_isstring(L, -1)) {
        pass.output = lua_tostring(L, -1);
    } else if (!lua_isnil(L, -1) && logger_) {
        logger_->Warn("RenderGraphScriptService: pass '" + pass.name + "' output must be a string");
    }
    lua_pop(L, 1);

    std::string inputValue;
    lua_getfield(L, passTableIndex, "input");
    if (lua_isstring(L, -1)) {
        inputValue = lua_tostring(L, -1);
    } else if (!lua_isnil(L, -1) && logger_) {
        logger_->Warn("RenderGraphScriptService: pass '" + pass.name + "' input must be a string");
    }
    lua_pop(L, 1);

    lua_getfield(L, passTableIndex, "inputs");
    if (lua_istable(L, -1)) {
        pass.inputs = ParseStringMap(L, lua_gettop(L));
    } else if (!lua_isnil(L, -1) && logger_) {
        logger_->Warn("RenderGraphScriptService: pass '" + pass.name + "' inputs must be a table");
    }
    lua_pop(L, 1);
    if (!inputValue.empty() && pass.inputs.find("input") == pass.inputs.end()) {
        pass.inputs.emplace("input", std::move(inputValue));
    }

    lua_getfield(L, passTableIndex, "outputs");
    if (lua_istable(L, -1)) {
        pass.outputs = ParseStringMap(L, lua_gettop(L));
    } else if (!lua_isnil(L, -1) && logger_) {
        logger_->Warn("RenderGraphScriptService: pass '" + pass.name + "' outputs must be a table");
    }
    lua_pop(L, 1);

    lua_getfield(L, passTableIndex, "settings");
    if (lua_istable(L, -1)) {
        pass.settings = ParseSettingsMap(L, lua_gettop(L));
    } else if (!lua_isnil(L, -1) && logger_) {
        logger_->Warn("RenderGraphScriptService: pass '" + pass.name + "' settings must be a table");
    }
    lua_pop(L, 1);

    return pass;
}

std::unordered_map<std::string, std::string> RenderGraphScriptService::ParseStringMap(lua_State* L,
                                                                                       int index) const {
    std::unordered_map<std::string, std::string> result;
    int mapIndex = lua_absindex(L, index);

    lua_pushnil(L);
    while (lua_next(L, mapIndex) != 0) {
        if (lua_isstring(L, -2) && lua_isstring(L, -1)) {
            std::string key = lua_tostring(L, -2);
            std::string value = lua_tostring(L, -1);
            result.emplace(std::move(key), std::move(value));
        } else if (logger_) {
            logger_->Warn("RenderGraphScriptService: String map entry must be string->string");
        }
        lua_pop(L, 1);
    }

    return result;
}

std::unordered_map<std::string, RenderGraphValue> RenderGraphScriptService::ParseSettingsMap(lua_State* L,
                                                                                              int index) const {
    std::unordered_map<std::string, RenderGraphValue> result;
    int mapIndex = lua_absindex(L, index);

    lua_pushnil(L);
    while (lua_next(L, mapIndex) != 0) {
        if (!lua_isstring(L, -2)) {
            lua_pop(L, 1);
            continue;
        }
        std::string key = lua_tostring(L, -2);
        RenderGraphValue value;
        if (TryParseValue(L, lua_gettop(L), value)) {
            result.emplace(std::move(key), std::move(value));
        } else if (logger_) {
            logger_->Warn("RenderGraphScriptService: Unsupported settings value for key '" + key + "'");
        }
        lua_pop(L, 1);
    }

    return result;
}

bool RenderGraphScriptService::TryParseValue(lua_State* L, int index, RenderGraphValue& outValue) const {
    int absIndex = lua_absindex(L, index);
    if (lua_isnumber(L, absIndex)) {
        outValue.type = RenderGraphValue::Type::Number;
        outValue.number = lua_tonumber(L, absIndex);
        return true;
    }
    if (lua_isboolean(L, absIndex)) {
        outValue.type = RenderGraphValue::Type::Boolean;
        outValue.boolean = lua_toboolean(L, absIndex) != 0;
        return true;
    }
    if (lua_isstring(L, absIndex)) {
        outValue.type = RenderGraphValue::Type::String;
        outValue.string = lua_tostring(L, absIndex);
        return true;
    }
    if (lua_istable(L, absIndex)) {
        size_t len = lua_rawlen(L, absIndex);
        if (len == 0) {
            return false;
        }
        outValue.type = RenderGraphValue::Type::Array;
        outValue.array.clear();
        outValue.array.reserve(len);
        for (size_t i = 1; i <= len; ++i) {
            lua_rawgeti(L, absIndex, static_cast<int>(i));
            if (!lua_isnumber(L, -1)) {
                lua_pop(L, 1);
                outValue.array.clear();
                return false;
            }
            outValue.array.push_back(lua_tonumber(L, -1));
            lua_pop(L, 1);
        }
        return true;
    }

    return false;
}

}  // namespace sdl3cpp::services::impl
