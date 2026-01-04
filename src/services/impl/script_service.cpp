#include "script_service.hpp"
#include "../../script/script_engine.hpp"
#include "../../logging/logging.hpp"
#include <stdexcept>

namespace sdl3cpp::services::impl {

ScriptService::ScriptService(const std::filesystem::path& scriptPath)
    : scriptPath_(scriptPath) {
    logging::Logger::GetInstance().Trace("ScriptService::ScriptService: Created");
}

ScriptService::~ScriptService() {
    logging::Logger::GetInstance().Trace("ScriptService::~ScriptService: Destroyed");
    if (initialized_) {
        Shutdown();
    }
}

void ScriptService::Initialize() {
    logging::Logger::GetInstance().Trace("ScriptService::Initialize: Entering");

    if (initialized_) {
        throw std::runtime_error("Script service already initialized");
    }

    try {
        scriptEngine_ = std::make_unique<script::ScriptEngine>(scriptPath_);
        initialized_ = true;
        logging::Logger::GetInstance().Trace("ScriptService::Initialize: Exiting");
    } catch (const std::exception& e) {
        logging::Logger::GetInstance().Trace("ScriptService::Initialize: Exiting with error");
        throw std::runtime_error(std::string("Failed to initialize script engine: ") + e.what());
    }
}

void ScriptService::Shutdown() noexcept {
    logging::Logger::GetInstance().Trace("ScriptService::Shutdown: Entering");

    if (scriptEngine_) {
        scriptEngine_.reset();
    }
    initialized_ = false;

    logging::Logger::GetInstance().Trace("ScriptService::Shutdown: Exiting");
}

std::vector<script::SceneManager::SceneObject> ScriptService::LoadSceneObjects() {
    logging::Logger::GetInstance().Trace("ScriptService::LoadSceneObjects: Entering");

    if (!initialized_) {
        throw std::runtime_error("Script service not initialized");
    }

    auto result = scriptEngine_->LoadSceneObjects();
    logging::Logger::GetInstance().Trace("ScriptService::LoadSceneObjects: Exiting");
    return result;
}

std::array<float, 16> ScriptService::ComputeModelMatrix(int functionRef, float time) {
    logging::TraceGuard trace("ScriptService::ComputeModelMatrix");

    if (!initialized_) {
        throw std::runtime_error("Script service not initialized");
    }

    return scriptEngine_->ComputeModelMatrix(functionRef, time);
}

std::array<float, 16> ScriptService::GetViewProjectionMatrix(float aspect) {
    logging::Logger::GetInstance().Trace("ScriptService::GetViewProjectionMatrix: Entering");

    if (!initialized_) {
        throw std::runtime_error("Script service not initialized");
    }

    auto result = scriptEngine_->GetViewProjectionMatrix(aspect);
    logging::Logger::GetInstance().Trace("ScriptService::GetViewProjectionMatrix: Exiting");
    return result;
}

std::unordered_map<std::string, script::ShaderManager::ShaderPaths> ScriptService::LoadShaderPathsMap() {
    logging::TraceGuard trace("ScriptService::LoadShaderPathsMap");

    if (!initialized_) {
        throw std::runtime_error("Script service not initialized");
    }

    return scriptEngine_->LoadShaderPathsMap();
}

std::vector<script::GuiCommand> ScriptService::LoadGuiCommands() {
    logging::TraceGuard trace("ScriptService::LoadGuiCommands");

    if (!initialized_) {
        throw std::runtime_error("Script service not initialized");
    }

    return scriptEngine_->LoadGuiCommands();
}

void ScriptService::UpdateGuiInput(const script::GuiInputSnapshot& input) {
    logging::TraceGuard trace("ScriptService::UpdateGuiInput");

    if (!initialized_) {
        throw std::runtime_error("Script service not initialized");
    }

    scriptEngine_->UpdateGuiInput(input);
}

bool ScriptService::HasGuiCommands() const {
    logging::TraceGuard trace("ScriptService::HasGuiCommands");

    if (!initialized_) {
        return false;
    }

    return scriptEngine_->HasGuiCommands();
}

script::PhysicsBridge& ScriptService::GetPhysicsBridge() {
    logging::TraceGuard trace("ScriptService::GetPhysicsBridge");

    if (!initialized_) {
        throw std::runtime_error("Script service not initialized");
    }

    return scriptEngine_->GetPhysicsBridge();
}

void ScriptService::SetAudioPlayer(app::AudioPlayer* audioPlayer) {
    logging::TraceGuard trace("ScriptService::SetAudioPlayer");

    if (!initialized_) {
        throw std::runtime_error("Script service not initialized");
    }

    scriptEngine_->SetAudioPlayer(audioPlayer);
}

std::filesystem::path ScriptService::GetScriptDirectory() const {
    logging::TraceGuard trace("ScriptService::GetScriptDirectory");

    if (!initialized_) {
        throw std::runtime_error("Script service not initialized");
    }

    return scriptEngine_->GetScriptDirectory();
}

std::string ScriptService::GetLuaError() {
    logging::TraceGuard trace("ScriptService::GetLuaError");

    if (!initialized_) {
        return "Script service not initialized";
    }

    return scriptEngine_->GetLuaError();
}

}  // namespace sdl3cpp::services::impl