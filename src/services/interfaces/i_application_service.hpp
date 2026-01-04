#pragma once

#include "i_logger.hpp"
#include <memory>
#include <string>

namespace sdl3cpp::services {

class IApplicationService {
public:
    virtual ~IApplicationService() = default;

    virtual void Run() = 0;
    virtual void ConfigureLogging(LogLevel level, bool enableConsole, const std::string& outputFile = "") = 0;
    virtual std::shared_ptr<ILogger> GetLogger() const = 0;
};

}  // namespace sdl3cpp::services
