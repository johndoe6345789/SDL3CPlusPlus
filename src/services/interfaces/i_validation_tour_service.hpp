#pragma once

#include "graphics_types.hpp"
#include <filesystem>
#include <string>

namespace sdl3cpp::services {

struct ValidationFramePlan {
    bool active = false;
    bool overrideView = false;
    bool requestScreenshot = false;
    ViewState viewState{};
    std::filesystem::path screenshotPath;
};

struct ValidationFrameResult {
    bool shouldAbort = false;
    std::string message;
};

/**
 * @brief Validation tour interface for startup visual checks.
 */
class IValidationTourService {
public:
    virtual ~IValidationTourService() = default;

    /**
     * @brief Request a specific checkpoint by id.
     *
     * @param checkpointId Checkpoint identifier from validation_tour config
     * @return true if the checkpoint was accepted or validation is disabled
     */
    virtual bool RequestCheckpoint(const std::string& checkpointId) = 0;

    /**
     * @brief Prepare validation state for the upcoming frame.
     *
     * @param aspect Aspect ratio for view/projection math
     * @return Frame plan including optional view override and screenshot request
     */
    virtual ValidationFramePlan BeginFrame(float aspect) = 0;

    /**
     * @brief Finalize validation work after a frame completes.
     *
     * @return Result indicating whether to abort runtime
     */
    virtual ValidationFrameResult EndFrame() = 0;

    /**
     * @brief Whether validation tour is enabled and active.
     */
    virtual bool IsActive() const = 0;
};

}  // namespace sdl3cpp::services
