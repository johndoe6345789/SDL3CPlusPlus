#pragma once

#include "config_ir_types.hpp"

namespace sdl3cpp::services {

/**
 * @brief Builds and validates render graphs.
 */
class IRenderGraphService {
public:
    virtual ~IRenderGraphService() = default;

    /**
     * @brief Build a render graph schedule.
     *
     * @param renderGraph Render graph IR
     * @return Schedule and diagnostics
     */
    virtual RenderGraphBuildResult BuildGraph(const RenderGraphIR& renderGraph) = 0;
};

}  // namespace sdl3cpp::services
