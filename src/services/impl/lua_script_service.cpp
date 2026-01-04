#include "lua_script_service.hpp"
#include "../../logging/logger.hpp"
#include <stdexcept>

namespace sdl3cpp::services::impl {

LuaScriptService::LuaScriptService(const std::filesystem::path& scriptPath, bool debugEnabled)
    : scriptPath_(scriptPath), debugEnabled_(debugEnabled) {
}

LuaScriptService::~LuaScriptService() {
    if (initialized_) {
        Shutdown();
    }
}

void LuaScriptService::Initialize() {
    logging::TraceGuard trace;

    if (initialized_) {
        return;
    }

    engine_ = std::make_unique<script::ScriptEngine>(scriptPath_, debugEnabled_);
    initialized_ = true;

    logging::Logger::GetInstance().Info("Script service initialized");
}

void LuaScriptService::Shutdown() noexcept {
    logging::TraceGuard trace;

    if (!initialized_) {
        return;
    }

    engine_.reset();
    initialized_ = false;

    logging::Logger::GetInstance().Info("Script service shutdown");
}

std::vector<script::SceneManager::SceneObject> LuaScriptService::LoadSceneObjects() {
    logging::TraceGuard trace;

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
    logging::TraceGuard trace;

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
    if (!engine_) {
        return;
    }

    engine_->SetAudioPlayer(audioPlayer);
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
