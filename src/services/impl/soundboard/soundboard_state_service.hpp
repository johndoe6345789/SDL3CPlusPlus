#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_soundboard_state_service.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

class SoundboardStateService final : public ISoundboardStateService {
public:
    explicit SoundboardStateService(std::shared_ptr<ILogger> logger);

    void SetStatusMessage(std::string message) override;
    const std::string& GetStatusMessage() const override;

private:
    std::shared_ptr<ILogger> logger_;
    std::string statusMessage_;
};

}  // namespace sdl3cpp::services::impl
