#pragma once

#include <cstdint>

namespace sdl3cpp::services {

class IConfigService {
public:
    virtual ~IConfigService() = default;
    virtual uint32_t GetWindowWidth() const = 0;
    virtual uint32_t GetWindowHeight() const = 0;
};

}  // namespace sdl3cpp::services
