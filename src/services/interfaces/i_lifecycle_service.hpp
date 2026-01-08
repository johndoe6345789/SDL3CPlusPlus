#pragma once

namespace sdl3cpp::services {

class ILifecycleService {
public:
    virtual ~ILifecycleService() = default;

    virtual void InitializeAll() = 0;
    virtual void ShutdownAll() noexcept = 0;
};

}  // namespace sdl3cpp::services
