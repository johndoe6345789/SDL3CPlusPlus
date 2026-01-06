#pragma once

#include "../interfaces/i_render_graph_script_service.hpp"
#include "../interfaces/i_config_service.hpp"
#include "../interfaces/i_logger.hpp"
#include "../interfaces/i_script_engine_service.hpp"
#include <memory>

struct lua_State;

namespace sdl3cpp::services::impl {

/**
 * @brief Loads render graph definitions from the shared Lua runtime.
 */
class RenderGraphScriptService : public IRenderGraphScriptService {
public:
    RenderGraphScriptService(std::shared_ptr<IScriptEngineService> engineService,
                             std::shared_ptr<IConfigService> configService,
                             std::shared_ptr<ILogger> logger);

    bool IsEnabled() const override;
    bool HasRenderGraphFunction() const override;
    RenderGraphDefinition LoadRenderGraph() override;

private:
    lua_State* GetLuaState() const;
    const RenderGraphConfig& GetRenderGraphConfig() const;
    RenderGraphDefinition ParseRenderGraph(lua_State* L, int index) const;
    RenderGraphResource ParseResource(lua_State* L, int index, const std::string& name) const;
    RenderGraphPass ParsePass(lua_State* L, int index, size_t passIndex) const;
    bool TryParseValue(lua_State* L, int index, RenderGraphValue& outValue) const;
    std::unordered_map<std::string, std::string> ParseStringMap(lua_State* L, int index) const;
    std::unordered_map<std::string, RenderGraphValue> ParseSettingsMap(lua_State* L, int index) const;

    std::shared_ptr<IScriptEngineService> engineService_;
    std::shared_ptr<IConfigService> configService_;
    std::shared_ptr<ILogger> logger_;
    bool graphLoaded_ = false;
    RenderGraphDefinition cachedGraph_{};
};

}  // namespace sdl3cpp::services::impl
