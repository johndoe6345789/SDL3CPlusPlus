#pragma once

namespace sdl3cpp::services {

class IRenderCoordinatorService {
public:
    virtual ~IRenderCoordinatorService() = default;

    virtual void RenderFrame(float time) = 0;
};

}  // namespace sdl3cpp::services
