#include "lua_script_service.hpp"
#include <stdexcept>

namespace sdl3cpp::services::impl {

LuaScriptService::LuaScriptService(const std::filesystem::path& scriptPath, std::shared_ptr<ILogger> logger, bool debugEnabled)
    : scriptPath_(scriptPath), logger_(logger), debugEnabled_(debugEnabled) {
}

LuaScriptService::~LuaScriptService() {
    if (initialized_) {
        Shutdown();
    }
}

void LuaScriptService::Initialize() {
    logger_->TraceFunction(__func__);

    if (initialized_) {
        return;
    }

    engine_ = std::make_unique<script::ScriptEngine>(scriptPath_, debugEnabled_);
    initialized_ = true;

    logger_->Info("Script service initialized");
}

void LuaScriptService::Shutdown() noexcept {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        return;
    }

    engine_.reset();
    initialized_ = false;

    logger_->Info("Script service shutdown");
}

std::vector<script::SceneManager::SceneObject> LuaScriptService::LoadSceneObjects() {
    logger_->TraceFunction(__func__);

    if (!engine_) {
        throw std::runtime_error("Script service not initialized");
    }

    return engine_->LoadSceneObjects();
}

std::array<float, 16> LuaScriptService::ComputeModelMatrix(int functionRef, float time) {
    if (!engine_) {
        throw std::runtime_error("Script service not initialized");
    }

    return engine_->ComputeModelMatrix(functionRef, time);
}

std::array<float, 16> LuaScriptService::GetViewProjectionMatrix(float aspect) {
    if (!engine_) {
        throw std::runtime_error("Script service not initialized");
    }

    return engine_->GetViewProjectionMatrix(aspect);
}

std::unordered_map<std::string, sdl3cpp::services::ShaderPaths> LuaScriptService::LoadShaderPathsMap() {
    logger_->TraceFunction(__func__);

    if (!engine_) {
        throw std::runtime_error("Script service not initialized");
    }

    return engine_->LoadShaderPathsMap();
}

std::vector<script::GuiCommand> LuaScriptService::LoadGuiCommands() {
    if (!engine_) {
        return {};
    }

    return engine_->LoadGuiCommands();
}

void LuaScriptService::UpdateGuiInput(const script::GuiInputSnapshot& input) {
    if (!engine_) {
        return;
    }

    engine_->UpdateGuiInput(input);
}

bool LuaScriptService::HasGuiCommands() const {
    if (!engine_) {
        return false;
    }

    return engine_->HasGuiCommands();
}

script::PhysicsBridge& LuaScriptService::GetPhysicsBridge() {
    if (!engine_) {
        throw std::runtime_error("Script service not initialized");
    }

    return engine_->GetPhysicsBridge();
}

void LuaScriptService::SetAudioPlayer(app::AudioPlayer* audioPlayer) {
    // Stub - audio functionality now handled through services
    logger_->Trace("LuaScriptService::SetAudioPlayer: Stub implementation - using services now");
}

std::filesystem::path LuaScriptService::GetScriptDirectory() const {
    if (!engine_) {
        return {};
    }

    return engine_->GetScriptDirectory();
}

std::string LuaScriptService::GetLuaError() {
    if (!engine_) {
        return "Script service not initialized";
    }

    return engine_->GetLuaError();
}

}  // namespace sdl3cpp::services::impl
