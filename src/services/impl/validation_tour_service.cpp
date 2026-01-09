#include "validation_tour_service.hpp"

#include "../interfaces/probe_types.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>

namespace sdl3cpp::services::impl {
namespace {

std::array<float, 16> ToArray(const glm::mat4& matrix) {
    std::array<float, 16> result{};
    std::memcpy(result.data(), glm::value_ptr(matrix), sizeof(float) * result.size());
    return result;
}

}

ValidationTourService::ValidationTourService(std::shared_ptr<IConfigService> configService,
                                             std::shared_ptr<IProbeService> probeService,
                                             std::shared_ptr<ILogger> logger)
    : probeService_(std::move(probeService)),
      logger_(std::move(logger)) {
    if (configService) {
        config_ = configService->GetValidationTourConfig();
    }

    if (config_.enabled && config_.checkpoints.empty()) {
        if (logger_) {
            logger_->Warn("ValidationTourService: validation_tour enabled with no checkpoints");
        }
        active_ = false;
        completed_ = true;
        return;
    }

    if (config_.enabled && config_.captureFrames == 0) {
        if (logger_) {
            logger_->Warn("ValidationTourService: capture_frames was 0; defaulting to 1");
        }
        config_.captureFrames = 1;
    }

    active_ = config_.enabled;
    if (!active_) {
        completed_ = true;
        return;
    }

    resolvedOutputDir_ = ResolvePath(config_.outputDir);
    if (!resolvedOutputDir_.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(resolvedOutputDir_, ec);
        if (ec && logger_) {
            logger_->Warn("ValidationTourService: Failed to create output dir " +
                         resolvedOutputDir_.string() + " error=" + ec.message());
        }
    }

    checkpointIndex_ = 0;
    AdvanceCheckpoint();
}

ValidationFramePlan ValidationTourService::BeginFrame(float aspect) {
    ValidationFramePlan plan{};
    if (!IsActive()) {
        return plan;
    }

    if (!pendingCapture_ && warmupRemaining_ == 0 && capturesRemaining_ == 0) {
        ++checkpointIndex_;
        AdvanceCheckpoint();
        if (!IsActive()) {
            return plan;
        }
    }

    const auto& checkpoint = config_.checkpoints[checkpointIndex_];
    plan.active = true;
    plan.overrideView = true;
    plan.viewState = BuildViewState(checkpoint.camera, aspect);

    if (!pendingCapture_ && warmupRemaining_ == 0 && capturesRemaining_ > 0) {
        std::filesystem::path outputDir = resolvedOutputDir_.empty()
            ? ResolvePath(config_.outputDir)
            : resolvedOutputDir_;
        std::string captureName = checkpoint.id + "_capture_" + std::to_string(captureIndex_) + ".png";
        std::filesystem::path actualPath = outputDir.empty()
            ? std::filesystem::path(captureName)
            : outputDir / captureName;
        plan.requestScreenshot = true;
        plan.screenshotPath = actualPath;

        PendingCapture pending{};
        pending.actualPath = actualPath;
        pending.expectedPath = ResolvePath(checkpoint.expected.imagePath);
        pending.checkpointId = checkpoint.id;
        pending.tolerance = checkpoint.expected.tolerance;
        pending.maxDiffPixels = checkpoint.expected.maxDiffPixels;
        pending.captureIndex = captureIndex_;
        pendingCapture_ = std::move(pending);

        if (logger_) {
            logger_->Trace("ValidationTourService", "BeginFrame",
                           "checkpoint=" + checkpoint.id +
                               ", captureIndex=" + std::to_string(captureIndex_) +
                               ", output=" + actualPath.string());
        }
    } else if (logger_) {
        logger_->Trace("ValidationTourService", "BeginFrame",
                       "checkpoint=" + checkpoint.id +
                           ", warmupRemaining=" + std::to_string(warmupRemaining_) +
                           ", capturesRemaining=" + std::to_string(capturesRemaining_) +
                           ", pendingCapture=" + std::string(pendingCapture_.has_value() ? "true" : "false"));
    }

    return plan;
}

ValidationFrameResult ValidationTourService::EndFrame() {
    ValidationFrameResult result{};
    if (!IsActive()) {
        if (failed_) {
            result.shouldAbort = config_.failOnMismatch;
            result.message = failureMessage_;
        }
        return result;
    }

    if (pendingCapture_) {
        const auto& pending = *pendingCapture_;
        if (!std::filesystem::exists(pending.actualPath)) {
            return result;
        }

        std::string errorMessage;
        bool ok = CompareImages(pending, errorMessage);
        if (!ok) {
            failed_ = true;
            failureMessage_ = errorMessage;
            result.shouldAbort = config_.failOnMismatch;
            result.message = failureMessage_;
            pendingCapture_.reset();
            return result;
        }

        pendingCapture_.reset();
        if (capturesRemaining_ > 0) {
            --capturesRemaining_;
        }
        ++captureIndex_;
        if (capturesRemaining_ == 0) {
            ++checkpointIndex_;
            AdvanceCheckpoint();
        }
        return result;
    }

    if (warmupRemaining_ > 0) {
        --warmupRemaining_;
    }

    return result;
}

void ValidationTourService::AdvanceCheckpoint() {
    if (checkpointIndex_ >= config_.checkpoints.size()) {
        completed_ = true;
        active_ = false;
        if (logger_) {
            logger_->Trace("ValidationTourService", "AdvanceCheckpoint", "", "Validation tour completed");
        }
        return;
    }

    warmupRemaining_ = config_.warmupFrames;
    capturesRemaining_ = config_.captureFrames;
    captureIndex_ = 0;
    if (logger_) {
        logger_->Trace("ValidationTourService", "AdvanceCheckpoint",
                       "checkpoint=" + config_.checkpoints[checkpointIndex_].id +
                           ", warmupFrames=" + std::to_string(warmupRemaining_) +
                           ", captureFrames=" + std::to_string(capturesRemaining_));
    }
}

ViewState ValidationTourService::BuildViewState(const ValidationCameraConfig& camera, float aspect) const {
    glm::vec3 position(camera.position[0], camera.position[1], camera.position[2]);
    glm::vec3 lookAt(camera.lookAt[0], camera.lookAt[1], camera.lookAt[2]);
    glm::vec3 up(camera.up[0], camera.up[1], camera.up[2]);
    glm::mat4 view = glm::lookAt(position, lookAt, up);
    float clampedAspect = aspect <= 0.0f ? 1.0f : aspect;
    glm::mat4 proj = glm::perspective(glm::radians(camera.fovDegrees), clampedAspect,
                                      camera.nearPlane, camera.farPlane);

    ViewState state{};
    state.view = ToArray(view);
    state.proj = ToArray(proj);
    glm::mat4 viewProj = proj * view;
    state.viewProj = ToArray(viewProj);
    state.cameraPosition = camera.position;
    return state;
}

std::filesystem::path ValidationTourService::ResolvePath(const std::filesystem::path& path) const {
    if (path.empty()) {
        return {};
    }
    if (path.is_absolute()) {
        return path;
    }
    return std::filesystem::current_path() / path;
}

bool ValidationTourService::CompareImages(const PendingCapture& pending, std::string& errorMessage) const {
    int actualWidth = 0;
    int actualHeight = 0;
    int actualChannels = 0;
    stbi_uc* actualPixels = stbi_load(pending.actualPath.string().c_str(),
                                      &actualWidth,
                                      &actualHeight,
                                      &actualChannels,
                                      STBI_rgb_alpha);
    if (!actualPixels) {
        errorMessage = "Validation capture missing or unreadable: " + pending.actualPath.string();
        ReportMismatch(pending, "Capture missing", errorMessage);
        return false;
    }

    int expectedWidth = 0;
    int expectedHeight = 0;
    int expectedChannels = 0;
    stbi_uc* expectedPixels = stbi_load(pending.expectedPath.string().c_str(),
                                        &expectedWidth,
                                        &expectedHeight,
                                        &expectedChannels,
                                        STBI_rgb_alpha);
    if (!expectedPixels) {
        stbi_image_free(actualPixels);
        errorMessage = "Expected image missing or unreadable: " + pending.expectedPath.string();
        ReportMismatch(pending, "Expected missing", errorMessage);
        return false;
    }

    if (actualWidth != expectedWidth || actualHeight != expectedHeight) {
        stbi_image_free(actualPixels);
        stbi_image_free(expectedPixels);
        errorMessage = "Validation image size mismatch for checkpoint '" + pending.checkpointId +
                       "' actual=" + std::to_string(actualWidth) + "x" + std::to_string(actualHeight) +
                       " expected=" + std::to_string(expectedWidth) + "x" + std::to_string(expectedHeight);
        ReportMismatch(pending, "Image size mismatch", errorMessage);
        return false;
    }

    const size_t pixelCount = static_cast<size_t>(actualWidth) * static_cast<size_t>(actualHeight);
    const size_t totalChannels = pixelCount * 4;
    size_t mismatchCount = 0;
    float maxChannelDiff = 0.0f;
    double totalDiff = 0.0;

    for (size_t idx = 0; idx < totalChannels; idx += 4) {
        bool pixelMismatch = false;
        for (size_t channel = 0; channel < 4; ++channel) {
            int actual = actualPixels[idx + channel];
            int expected = expectedPixels[idx + channel];
            float diff = static_cast<float>(std::abs(actual - expected)) / 255.0f;
            totalDiff += static_cast<double>(diff);
            if (diff > maxChannelDiff) {
                maxChannelDiff = diff;
            }
            if (diff > pending.tolerance) {
                pixelMismatch = true;
            }
        }
        if (pixelMismatch) {
            ++mismatchCount;
        }
    }

    stbi_image_free(actualPixels);
    stbi_image_free(expectedPixels);

    const double averageDiff = totalChannels == 0 ? 0.0 : totalDiff / static_cast<double>(totalChannels);
    if (mismatchCount > pending.maxDiffPixels) {
        errorMessage = "Validation mismatch for checkpoint '" + pending.checkpointId +
                       "' mismatchedPixels=" + std::to_string(mismatchCount) +
                       " maxAllowed=" + std::to_string(pending.maxDiffPixels) +
                       " tolerance=" + std::to_string(pending.tolerance) +
                       " avgDiff=" + std::to_string(averageDiff) +
                       " maxChannelDiff=" + std::to_string(maxChannelDiff) +
                       " actual=" + pending.actualPath.string() +
                       " expected=" + pending.expectedPath.string();
        ReportMismatch(pending, "Image mismatch", errorMessage);
        return false;
    }

    if (logger_) {
        logger_->Trace("ValidationTourService", "CompareImages",
                       "checkpoint=" + pending.checkpointId +
                           ", mismatchedPixels=" + std::to_string(mismatchCount) +
                           ", avgDiff=" + std::to_string(averageDiff));
    }
    return true;
}

void ValidationTourService::ReportMismatch(const PendingCapture& pending,
                                           const std::string& summary,
                                           const std::string& details) const {
    if (logger_) {
        logger_->Error("ValidationTourService::CompareImages: " + details);
    }
    if (!probeService_) {
        return;
    }
    ProbeReport report{};
    report.severity = ProbeSeverity::Error;
    report.code = "VALIDATION_MISMATCH";
    report.resourceId = pending.checkpointId;
    report.message = summary;
    report.details = details;
    probeService_->Report(report);
}

}  // namespace sdl3cpp::services::impl
