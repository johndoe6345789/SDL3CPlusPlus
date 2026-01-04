#include "script_service.hpp"
#include "../../script/script_engine.hpp"
#include "../../logging/logging.hpp"
#include <stdexcept>

namespace sdl3cpp::services::impl {

ScriptService::ScriptService(const std::filesystem::path& scriptPath)
    : scriptPath_(scriptPath) {
    logging::TraceGuard trace("ScriptService::ScriptService");
}

ScriptService::~ScriptService() {
    logging::TraceGuard trace("ScriptService::~ScriptService");
    if (initialized_) {
        Shutdown();
    }
}

void ScriptService::Initialize() {
    logging::TraceGuard trace("ScriptService::Initialize");

    if (initialized_) {
        throw std::runtime_error("Script service already initialized");
    }

    try {
        scriptEngine_ = std::make_unique<script::ScriptEngine>(scriptPath_);
        initialized_ = true;
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to initialize script engine: ") + e.what());
    }
}

void ScriptService::Shutdown() noexcept {
    logging::TraceGuard trace("ScriptService::Shutdown");

    if (scriptEngine_) {
        scriptEngine_.reset();
    }
    initialized_ = false;
}

std::vector<script::SceneManager::SceneObject> ScriptService::LoadSceneObjects() {
    logging::TraceGuard trace("ScriptService::LoadSceneObjects");

    if (!initialized_) {
        throw std::runtime_error("Script service not initialized");
    }

    return scriptEngine_->LoadSceneObjects();
}

std::array<float, 16> ScriptService::ComputeModelMatrix(int functionRef, float time) {
    logging::TraceGuard trace("ScriptService::ComputeModelMatrix");

    if (!initialized_) {
        throw std::runtime_error("Script service not initialized");
    }

    return scriptEngine_->ComputeModelMatrix(functionRef, time);
}

std::array<float, 16> ScriptService::GetViewProjectionMatrix(float aspect) {
    logging::TraceGuard trace("ScriptService::GetViewProjectionMatrix");

    if (!initialized_) {
        throw std::runtime_error("Script service not initialized");
    }

    return scriptEngine_->GetViewProjectionMatrix(aspect);
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