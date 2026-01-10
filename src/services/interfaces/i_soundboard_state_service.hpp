#pragma once

#include <string>

namespace sdl3cpp::services {

class ISoundboardStateService {
public:
    virtual ~ISoundboardStateService() = default;

    virtual void SetStatusMessage(std::string message) = 0;
    virtual const std::string& GetStatusMessage() const = 0;
};

}  // namespace sdl3cpp::services
