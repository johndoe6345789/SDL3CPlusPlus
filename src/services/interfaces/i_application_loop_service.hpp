#pragma once

namespace sdl3cpp::services {

class IApplicationLoopService {
public:
    virtual ~IApplicationLoopService() = default;

    virtual void Run() = 0;
};

}  // namespace sdl3cpp::services
