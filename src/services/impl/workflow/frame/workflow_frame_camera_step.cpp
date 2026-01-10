#include "workflow_frame_camera_step.hpp"
#include "../workflow_step_io_resolver.hpp"

#include "services/interfaces/graphics_types.hpp"

#include <rapidjson/document.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstring>
#include <stdexcept>

namespace sdl3cpp::services::impl {
namespace {

struct CameraConfig {
    std::array<float, 3> position{0.0f, 0.0f, 5.0f};
    std::array<float, 3> lookAt{0.0f, 0.0f, 0.0f};
    std::array<float, 3> up{0.0f, 1.0f, 0.0f};
    float fov = 0.78f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
};

std::array<float, 16> ToArray(const glm::mat4& matrix) {
    std::array<float, 16> result{};
    std::memcpy(result.data(), glm::value_ptr(matrix), sizeof(float) * result.size());
    return result;
}

bool ReadVec3(const rapidjson::Value& object, const char* name, std::array<float, 3>& out) {
    if (!object.HasMember(name)) {
        return false;
    }
    const auto& value = object[name];
    if (!value.IsArray() || value.Size() != 3) {
        throw std::runtime_error(std::string("frame.camera: '") + name + "' must be a 3-element array");
    }
    for (rapidjson::SizeType i = 0; i < 3; ++i) {
        if (!value[i].IsNumber()) {
            throw std::runtime_error(std::string("frame.camera: '") + name + "' must contain numbers");
        }
        out[i] = static_cast<float>(value[i].GetDouble());
    }
    return true;
}

bool ReadNumber(const rapidjson::Value& object, const char* name, float& out) {
    if (!object.HasMember(name)) {
        return false;
    }
    const auto& value = object[name];
    if (!value.IsNumber()) {
        throw std::runtime_error(std::string("frame.camera: '") + name + "' must be a number");
    }
    out = static_cast<float>(value.GetDouble());
    return true;
}

CameraConfig ReadCameraConfig(const std::string& json) {
    rapidjson::Document document;
    document.Parse(json.c_str());
    if (document.HasParseError()) {
        throw std::runtime_error("frame.camera: failed to parse config JSON");
    }
    CameraConfig config{};
    if (!document.HasMember("scene") || !document["scene"].IsObject()) {
        return config;
    }
    const auto& scene = document["scene"];
    if (!scene.HasMember("camera") || !scene["camera"].IsObject()) {
        return config;
    }
    const auto& camera = scene["camera"];
    ReadVec3(camera, "position", config.position);
    bool hasLookAt = ReadVec3(camera, "look_at", config.lookAt);
    if (!hasLookAt && camera.HasMember("lookAt")) {
        ReadVec3(camera, "lookAt", config.lookAt);
        hasLookAt = true;
    }
    if (!hasLookAt) {
        config.lookAt = {config.position[0], config.position[1], config.position[2] - 1.0f};
    }
    ReadVec3(camera, "up", config.up);
    if (ReadNumber(camera, "fov_degrees", config.fov)) {
        config.fov = glm::radians(config.fov);
    } else if (ReadNumber(camera, "fov", config.fov) && config.fov > 3.2f) {
        config.fov = glm::radians(config.fov);
    }
    ReadNumber(camera, "near", config.nearPlane);
    ReadNumber(camera, "far", config.farPlane);
    return config;
}

ViewState BuildViewState(const CameraConfig& config, float aspect) {
    glm::vec3 position(config.position[0], config.position[1], config.position[2]);
    glm::vec3 lookAt(config.lookAt[0], config.lookAt[1], config.lookAt[2]);
    glm::vec3 up(config.up[0], config.up[1], config.up[2]);
    glm::mat4 view = glm::lookAt(position, lookAt, up);
    float safeAspect = aspect <= 0.0f ? 1.0f : aspect;
    glm::mat4 proj = glm::perspective(config.fov, safeAspect, config.nearPlane, config.farPlane);

    ViewState state{};
    state.view = ToArray(view);
    state.proj = ToArray(proj);
    state.viewProj = ToArray(proj * view);
    state.cameraPosition = config.position;
    return state;
}

}  // namespace

WorkflowFrameCameraStep::WorkflowFrameCameraStep(std::shared_ptr<IConfigService> configService,
                                                 std::shared_ptr<ILogger> logger)
    : configService_(std::move(configService)),
      logger_(std::move(logger)) {}

std::string WorkflowFrameCameraStep::GetPluginId() const {
    return "frame.camera";
}

void WorkflowFrameCameraStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!configService_) {
        throw std::runtime_error("frame.camera requires an IConfigService");
    }
    const std::string& configJson = configService_->GetConfigJson();
    if (configJson.empty()) {
        throw std::runtime_error("frame.camera requires config JSON to be available");
    }
    WorkflowStepIoResolver resolver;
    const std::string deltaKey = resolver.GetRequiredInputKey(step, "delta");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "view_state");
    const auto* delta = context.TryGet<double>(deltaKey);
    if (!delta) {
        throw std::runtime_error("frame.camera missing delta input");
    }

    CameraConfig cameraConfig = ReadCameraConfig(configJson);
    float aspect = 1.0f;
    uint32_t width = configService_->GetWindowWidth();
    uint32_t height = configService_->GetWindowHeight();
    if (width > 0 && height > 0) {
        aspect = static_cast<float>(width) / static_cast<float>(height);
    }
    ViewState viewState = BuildViewState(cameraConfig, aspect);
    context.Set(outputKey, viewState);

    if (logger_) {
        logger_->Trace("WorkflowFrameCameraStep", "Execute",
                       "delta=" + std::to_string(*delta) +
                           ", output=" + outputKey +
                           ", aspect=" + std::to_string(aspect),
                       "Camera view state updated");
    }
}

}  // namespace sdl3cpp::services::impl
