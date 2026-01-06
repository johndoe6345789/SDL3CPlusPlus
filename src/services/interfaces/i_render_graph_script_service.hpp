#pragma once

#include "graphics_types.hpp"

namespace sdl3cpp::services {

/**
 * @brief Loads render graph definitions from Lua.
 */
class IRenderGraphScriptService {
public:
    virtual ~IRenderGraphScriptService() = default;

    /**
     * @brief Check if render graph execution is enabled.
     */
    virtual bool IsEnabled() const = 0;

    /**
     * @brief Check whether the configured Lua function exists.
     */
    virtual bool HasRenderGraphFunction() const = 0;

    /**
     * @brief Load the render graph definition from Lua.
     */
    virtual RenderGraphDefinition LoadRenderGraph() = 0;
};

}  // namespace sdl3cpp::services
