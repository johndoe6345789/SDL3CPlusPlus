#pragma once

#include "services/interfaces/i_platform_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

class PlatformService : public IPlatformService {
public:
    explicit PlatformService(std::shared_ptr<ILogger> logger = nullptr);
    ~PlatformService() override = default;

    std::optional<std::filesystem::path> GetUserConfigDirectory() const override;
    std::string GetPlatformError() const override;
    std::string GetPlatformName() const override;
    std::string GetCurrentVideoDriverName() const override;
    std::vector<std::string> GetAvailableVideoDrivers() const override;
    std::vector<std::string> GetAvailableRenderDrivers() const override;
    void LogSystemInfo() const override;

private:
    std::shared_ptr<ILogger> logger_;
#ifdef _WIN32
    std::string FormatWin32Error(unsigned long errorCode) const;
#endif
};

}  // namespace sdl3cpp::services::impl
