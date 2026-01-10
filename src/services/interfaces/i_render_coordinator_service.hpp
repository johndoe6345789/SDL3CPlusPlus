#pragma once

#include "graphics_types.hpp"

namespace sdl3cpp::services {

class IRenderCoordinatorService {
public:
    virtual ~IRenderCoordinatorService() = default;

    virtual void RenderFrame(float time) = 0;
    virtual void RenderFrameWithViewState(float time, const ViewState& viewState) = 0;
};

}  // namespace sdl3cpp::services
