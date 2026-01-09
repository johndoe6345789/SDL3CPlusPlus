#pragma once

namespace sdl3cpp::services {

class IFrameWorkflowService {
public:
    virtual ~IFrameWorkflowService() = default;
    virtual void ExecuteFrame(float deltaTime, float elapsedTime) = 0;
};

}  // namespace sdl3cpp::services
