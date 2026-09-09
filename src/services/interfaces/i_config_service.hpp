#pragma once

#include <cstdint>
#include <string>

namespace sdl3cpp::services {

class IConfigService {
public:
    virtual ~IConfigService() = default;
    virtual uint32_t GetWindowWidth() const = 0;
    virtual uint32_t GetWindowHeight() const = 0;

    /**
     * @brief Raw configuration document backing this service.
     *
     * Steps that need settings beyond window size (camera placement,
     * shader selection) parse this rather than widening the interface
     * once per setting. Implementations return an empty string when no
     * document has been loaded; callers are expected to check.
     */
    virtual const std::string& GetConfigJson() const = 0;
};

}  // namespace sdl3cpp::services
