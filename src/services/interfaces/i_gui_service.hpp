#pragma once

#include <cstdint>
#include <vector>

namespace sdl3cpp::services {

// Forward declare GUI types
struct GuiCommand;

/**
 * @brief GUI rendering service interface.
 *
 * Consumes GUI commands and prepares per-frame data.
 */
class IGuiService {
public:
    virtual ~IGuiService() = default;

    /**
     * @brief Prepare GUI commands for rendering.
     *
     * Processes GUI commands and prepares rendering data.
     *
     * @param commands Vector of GUI commands to render
     * @param width Viewport width in pixels
     * @param height Viewport height in pixels
     */
    virtual void PrepareFrame(const std::vector<GuiCommand>& commands,
                             uint32_t width,
                             uint32_t height) = 0;
};

}  // namespace sdl3cpp::services
