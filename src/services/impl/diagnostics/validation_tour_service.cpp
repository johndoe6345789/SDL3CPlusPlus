#include "validation_tour_service.hpp"

#include "services/interfaces/probe_types.hpp"
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
    checkpointIndexById_.clear();
    for (size_t i = 0; i < config_.checkpoints.size(); ++i) {
        const auto& checkpoint = config_.checkpoints[i];
        if (checkpoint.id.empty()) {
            continue;
        }
        checkpointIndexById_.emplace(checkpoint.id, i);
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

bool ValidationTourService::RequestCheckpoint(const std::string& checkpointId) {
    if (!config_.enabled) {
        if (logger_) {
            logger_->Trace("ValidationTourService", "RequestCheckpoint",
                           "checkpoint=" + checkpointId,
                           "Validation tour disabled");
        }
        return true;
    }
    if (checkpointId.empty()) {
        if (logger_) {
            logger_->Error("ValidationTourService::RequestCheckpoint: checkpoint id is empty");
        }
        return false;
    }
    auto it = checkpointIndexById_.find(checkpointId);
    if (it == checkpointIndexById_.end()) {
        if (logger_) {
            logger_->Error("ValidationTourService::RequestCheckpoint: checkpoint not found id=" + checkpointId);
        }
        return false;
    }
    checkpointIndex_ = it->second;
    pendingCapture_.reset();
    failed_ = false;
    completed_ = false;
    active_ = true;
    AdvanceCheckpoint();
    if (logger_) {
        logger_->Trace("ValidationTourService", "RequestCheckpoint",
                       "checkpoint=" + checkpointId +
                           ", index=" + std::to_string(checkpointIndex_),
                       "Checkpoint requested");
    }
    return true;
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
        pending.checkpointIndex = checkpointIndex_;
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
        bool ok = AnalyzeCapture(pending, errorMessage);
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

bool ValidationTourService::AnalyzeCapture(const PendingCapture& pending, std::string& errorMessage) const {
    if (pending.checkpointIndex >= config_.checkpoints.size()) {
        errorMessage = "Validation checkpoint index out of range";
        return false;
    }
    const ValidationCheckpointConfig& checkpoint = config_.checkpoints[pending.checkpointIndex];

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
        ReportMismatch(checkpoint.id, "Capture missing", errorMessage);
        return false;
    }

    bool expectedOk = CompareExpectedImage(checkpoint, actualWidth, actualHeight, actualPixels, errorMessage);
    bool checksOk = expectedOk
        ? ApplyChecks(checkpoint, actualWidth, actualHeight, actualPixels, errorMessage)
        : false;

    stbi_image_free(actualPixels);
    if (!expectedOk || !checksOk) {
        return false;
    }

    return true;
}

bool ValidationTourService::ApplyChecks(const ValidationCheckpointConfig& checkpoint,
                                        int width,
                                        int height,
                                        const unsigned char* pixels,
                                        std::string& errorMessage) const {
    if (checkpoint.checks.empty()) {
        return true;
    }
    if (width <= 0 || height <= 0) {
        errorMessage = "Validation check failed: invalid capture dimensions";
        ReportMismatch(checkpoint.id, "Invalid capture", errorMessage);
        return false;
    }

    struct NonBlackState {
        float threshold = 0.05f;
        float minRatio = 0.0f;
        float maxRatio = 1.0f;
        size_t count = 0;
        size_t checkIndex = 0;
    };

    std::vector<NonBlackState> nonBlackStates;
    nonBlackStates.reserve(checkpoint.checks.size());
    for (size_t index = 0; index < checkpoint.checks.size(); ++index) {
        const auto& check = checkpoint.checks[index];
        if (check.type == "non_black_ratio") {
            NonBlackState state{};
            state.threshold = check.threshold;
            state.minRatio = check.minValue;
            state.maxRatio = check.maxValue;
            state.checkIndex = index;
            nonBlackStates.push_back(state);
        }
    }

    const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
    const size_t totalChannels = pixelCount * 4;
    double sumR = 0.0;
    double sumG = 0.0;
    double sumB = 0.0;
    double sumLuma = 0.0;

    for (size_t idx = 0; idx < totalChannels; idx += 4) {
        const double r = static_cast<double>(pixels[idx]) / 255.0;
        const double g = static_cast<double>(pixels[idx + 1]) / 255.0;
        const double b = static_cast<double>(pixels[idx + 2]) / 255.0;
        sumR += r;
        sumG += g;
        sumB += b;
        const double luma = r * 0.2126 + g * 0.7152 + b * 0.0722;
        sumLuma += luma;
        if (!nonBlackStates.empty()) {
            for (auto& state : nonBlackStates) {
                if (luma > static_cast<double>(state.threshold)) {
                    ++state.count;
                }
            }
        }
    }

    const double denom = pixelCount == 0 ? 1.0 : static_cast<double>(pixelCount);
    const double avgLuma = sumLuma / denom;
    const double avgR = sumR / denom;
    const double avgG = sumG / denom;
    const double avgB = sumB / denom;

    for (const auto& state : nonBlackStates) {
        const double ratio = static_cast<double>(state.count) / denom;
        if (ratio < static_cast<double>(state.minRatio) || ratio > static_cast<double>(state.maxRatio)) {
            const auto& check = checkpoint.checks[state.checkIndex];
            errorMessage = "Validation check failed (non_black_ratio) checkpoint '" + checkpoint.id +
                           "' ratio=" + std::to_string(ratio) +
                           " min=" + std::to_string(check.minValue) +
                           " max=" + std::to_string(check.maxValue) +
                           " threshold=" + std::to_string(check.threshold);
            ReportMismatch(checkpoint.id, "Non-black ratio mismatch", errorMessage);
            return false;
        }
    }

    for (const auto& check : checkpoint.checks) {
        if (check.type == "luma_range") {
            if (avgLuma < static_cast<double>(check.minValue) ||
                avgLuma > static_cast<double>(check.maxValue)) {
                errorMessage = "Validation check failed (luma_range) checkpoint '" + checkpoint.id +
                               "' avgLuma=" + std::to_string(avgLuma) +
                               " min=" + std::to_string(check.minValue) +
                               " max=" + std::to_string(check.maxValue);
                ReportMismatch(checkpoint.id, "Luma range mismatch", errorMessage);
                return false;
            }
        } else if (check.type == "mean_color") {
            const double tol = static_cast<double>(check.tolerance);
            const double dr = std::abs(avgR - static_cast<double>(check.color[0]));
            const double dg = std::abs(avgG - static_cast<double>(check.color[1]));
            const double db = std::abs(avgB - static_cast<double>(check.color[2]));
            if (dr > tol || dg > tol || db > tol) {
                errorMessage = "Validation check failed (mean_color) checkpoint '" + checkpoint.id +
                               "' avgColor=[" + std::to_string(avgR) + "," +
                               std::to_string(avgG) + "," + std::to_string(avgB) +
                               "] target=[" + std::to_string(check.color[0]) + "," +
                               std::to_string(check.color[1]) + "," +
                               std::to_string(check.color[2]) + "] tolerance=" +
                               std::to_string(check.tolerance);
                ReportMismatch(checkpoint.id, "Mean color mismatch", errorMessage);
                return false;
            }
        } else if (check.type == "sample_points") {
            for (size_t pointIndex = 0; pointIndex < check.points.size(); ++pointIndex) {
                const auto& point = check.points[pointIndex];
                const double xPos = std::round(static_cast<double>(point.x) *
                                               static_cast<double>(width - 1));
                const double yPos = std::round(static_cast<double>(point.y) *
                                               static_cast<double>(height - 1));
                const int x = std::clamp(static_cast<int>(xPos), 0, width - 1);
                const int y = std::clamp(static_cast<int>(yPos), 0, height - 1);
                const size_t pixelIndex = (static_cast<size_t>(y) * static_cast<size_t>(width) +
                                           static_cast<size_t>(x)) * 4;
                const double r = static_cast<double>(pixels[pixelIndex]) / 255.0;
                const double g = static_cast<double>(pixels[pixelIndex + 1]) / 255.0;
                const double b = static_cast<double>(pixels[pixelIndex + 2]) / 255.0;
                const double tol = static_cast<double>(point.tolerance);
                const double dr = std::abs(r - static_cast<double>(point.color[0]));
                const double dg = std::abs(g - static_cast<double>(point.color[1]));
                const double db = std::abs(b - static_cast<double>(point.color[2]));
                if (dr > tol || dg > tol || db > tol) {
                    errorMessage = "Validation check failed (sample_points) checkpoint '" + checkpoint.id +
                                   "' point=" + std::to_string(pointIndex) +
                                   " actual=[" + std::to_string(r) + "," +
                                   std::to_string(g) + "," + std::to_string(b) +
                                   "] target=[" + std::to_string(point.color[0]) + "," +
                                   std::to_string(point.color[1]) + "," +
                                   std::to_string(point.color[2]) + "] tolerance=" +
                                   std::to_string(point.tolerance);
                    ReportMismatch(checkpoint.id, "Sample point mismatch", errorMessage);
                    return false;
                }
            }
        }
    }

    if (logger_) {
        logger_->Trace("ValidationTourService", "ApplyChecks",
                       "checkpoint=" + checkpoint.id +
                           ", avgLuma=" + std::to_string(avgLuma) +
                           ", avgColor=[" + std::to_string(avgR) + "," +
                           std::to_string(avgG) + "," + std::to_string(avgB) + "]");
    }

    return true;
}

bool ValidationTourService::CompareExpectedImage(const ValidationCheckpointConfig& checkpoint,
                                                 int width,
                                                 int height,
                                                 const unsigned char* pixels,
                                                 std::string& errorMessage) const {
    if (!checkpoint.expected.enabled) {
        return true;
    }
    std::filesystem::path expectedPath = ResolvePath(checkpoint.expected.imagePath);
    if (expectedPath.empty()) {
        errorMessage = "Expected image path missing for checkpoint '" + checkpoint.id + "'";
        ReportMismatch(checkpoint.id, "Expected missing", errorMessage);
        return false;
    }

    int expectedWidth = 0;
    int expectedHeight = 0;
    int expectedChannels = 0;
    stbi_uc* expectedPixels = stbi_load(expectedPath.string().c_str(),
                                        &expectedWidth,
                                        &expectedHeight,
                                        &expectedChannels,
                                        STBI_rgb_alpha);
    if (!expectedPixels) {
        errorMessage = "Expected image missing or unreadable: " + expectedPath.string();
        ReportMismatch(checkpoint.id, "Expected missing", errorMessage);
        return false;
    }

    if (width != expectedWidth || height != expectedHeight) {
        stbi_image_free(expectedPixels);
        errorMessage = "Validation image size mismatch for checkpoint '" + checkpoint.id +
                       "' actual=" + std::to_string(width) + "x" + std::to_string(height) +
                       " expected=" + std::to_string(expectedWidth) + "x" + std::to_string(expectedHeight);
        ReportMismatch(checkpoint.id, "Image size mismatch", errorMessage);
        return false;
    }

    const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
    const size_t totalChannels = pixelCount * 4;
    size_t mismatchCount = 0;
    double totalDiff = 0.0;
    double maxChannelDiff = 0.0;
    const double tolerance = static_cast<double>(checkpoint.expected.tolerance);

    for (size_t idx = 0; idx < totalChannels; idx += 4) {
        bool pixelMismatch = false;
        for (size_t channel = 0; channel < 4; ++channel) {
            int actual = pixels[idx + channel];
            int expected = expectedPixels[idx + channel];
            double diff = static_cast<double>(std::abs(actual - expected)) / 255.0;
            totalDiff += diff;
            if (diff > maxChannelDiff) {
                maxChannelDiff = diff;
            }
            if (diff > tolerance) {
                pixelMismatch = true;
            }
        }
        if (pixelMismatch) {
            ++mismatchCount;
        }
    }

    stbi_image_free(expectedPixels);

    const double averageDiff = totalChannels == 0 ? 0.0 : totalDiff / static_cast<double>(totalChannels);
    if (mismatchCount > checkpoint.expected.maxDiffPixels) {
        errorMessage = "Validation mismatch for checkpoint '" + checkpoint.id +
                       "' mismatchedPixels=" + std::to_string(mismatchCount) +
                       " maxAllowed=" + std::to_string(checkpoint.expected.maxDiffPixels) +
                       " tolerance=" + std::to_string(checkpoint.expected.tolerance) +
                       " avgDiff=" + std::to_string(averageDiff) +
                       " maxChannelDiff=" + std::to_string(maxChannelDiff) +
                       " expected=" + expectedPath.string();
        ReportMismatch(checkpoint.id, "Image mismatch", errorMessage);
        return false;
    }

    if (logger_) {
        logger_->Trace("ValidationTourService", "CompareExpectedImage",
                       "checkpoint=" + checkpoint.id +
                           ", mismatchedPixels=" + std::to_string(mismatchCount) +
                           ", avgDiff=" + std::to_string(averageDiff));
    }
    return true;
}

void ValidationTourService::ReportMismatch(const std::string& checkpointId,
                                           const std::string& summary,
                                           const std::string& details) const {
    if (logger_) {
        logger_->Error("ValidationTourService::AnalyzeCapture: " + details);
    }
    if (!probeService_) {
        return;
    }
    ProbeReport report{};
    report.severity = ProbeSeverity::Error;
    report.code = "VALIDATION_MISMATCH";
    report.resourceId = checkpointId;
    report.message = summary;
    report.details = details;
    probeService_->Report(report);
}

}  // namespace sdl3cpp::services::impl
