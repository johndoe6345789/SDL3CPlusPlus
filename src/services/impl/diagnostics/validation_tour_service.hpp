#pragma once

#include "services/interfaces/i_config_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_probe_service.hpp"
#include "services/interfaces/i_validation_tour_service.hpp"
#include <filesystem>
#include <optional>
#include <unordered_map>

namespace sdl3cpp::services::impl {

class ValidationTourService final : public IValidationTourService {
public:
    ValidationTourService(std::shared_ptr<IConfigService> configService,
                          std::shared_ptr<IProbeService> probeService,
                          std::shared_ptr<ILogger> logger);

    bool RequestCheckpoint(const std::string& checkpointId) override;
    ValidationFramePlan BeginFrame(float aspect) override;
    ValidationFrameResult EndFrame() override;
    bool IsActive() const override { return active_ && !completed_ && !failed_; }

private:
    struct PendingCapture {
        std::filesystem::path actualPath;
        size_t checkpointIndex = 0;
        size_t captureIndex = 0;
    };

    void AdvanceCheckpoint();
    ViewState BuildViewState(const ValidationCameraConfig& camera, float aspect) const;
    std::filesystem::path ResolvePath(const std::filesystem::path& path) const;
    bool AnalyzeCapture(const PendingCapture& pending, std::string& errorMessage) const;
    bool ApplyChecks(const ValidationCheckpointConfig& checkpoint,
                     int width,
                     int height,
                     const unsigned char* pixels,
                     std::string& errorMessage) const;
    bool CompareExpectedImage(const ValidationCheckpointConfig& checkpoint,
                              int width,
                              int height,
                              const unsigned char* pixels,
                              std::string& errorMessage) const;
    void ReportMismatch(const std::string& checkpointId,
                        const std::string& summary,
                        const std::string& details) const;

    std::shared_ptr<IProbeService> probeService_;
    std::shared_ptr<ILogger> logger_;
    ValidationTourConfig config_{};
    std::unordered_map<std::string, size_t> checkpointIndexById_{};
    std::filesystem::path resolvedOutputDir_{};
    size_t checkpointIndex_ = 0;
    uint32_t warmupRemaining_ = 0;
    uint32_t capturesRemaining_ = 0;
    size_t captureIndex_ = 0;
    bool active_ = false;
    bool completed_ = false;
    bool failed_ = false;
    std::string failureMessage_;
    std::optional<PendingCapture> pendingCapture_;
};

}  // namespace sdl3cpp::services::impl
