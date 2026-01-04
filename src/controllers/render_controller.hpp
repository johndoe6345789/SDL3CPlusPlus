#pragma once

#include "../di/service_registry.hpp"
#include <memory>

namespace sdl3cpp::controllers {

/**
 * @brief Render controller.
 *
 * Coordinates the rendering pipeline across graphics, scene, GUI, and script services.
 * Extracted from Sdl3App rendering logic.
 */
class RenderController {
public:
    explicit RenderController(di::ServiceRegistry& registry);
    ~RenderController();

    RenderController(const RenderController&) = delete;
    RenderController& operator=(const RenderController&) = delete;

    /**
     * @brief Render a single frame.
     *
     * @param time Current time in seconds for animations
     */
    void RenderFrame(float time);

private:
    di::ServiceRegistry& registry_;
};

}  // namespace sdl3cpp::controllers
