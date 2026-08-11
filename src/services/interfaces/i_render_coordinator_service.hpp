#pragma once

#include "services/interfaces/graphics_types.hpp"
#include "gui_types.hpp"

#include <vector>

namespace sdl3cpp::services {

class IRenderCoordinatorService {
public:
    virtual ~IRenderCoordinatorService() = default;

    virtual void RenderFrame(float time) = 0;
    virtual void RenderFrameWithViewState(float time, const ViewState& viewState) = 0;
    virtual void RenderFrameWithOverrides(float time,
                                          const ViewState* viewState,
                                          const std::vector<GuiCommand>* guiCommands) = 0;
};

}  // namespace sdl3cpp::services
