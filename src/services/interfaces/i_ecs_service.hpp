#pragma once

#include <entt/entt.hpp>

namespace sdl3cpp::services {

class IEcsService {
public:
    virtual ~IEcsService() = default;

    virtual entt::registry& GetRegistry() = 0;
    virtual const entt::registry& GetRegistry() const = 0;
};

}  // namespace sdl3cpp::services
