#include "lua_script_service.hpp"
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

LuaScriptService::LuaScriptService(std::shared_ptr<IScriptEngineService> engineService, std::shared_ptr<ILogger> logger)
    : engineService_(std::move(engineService)), logger_(std::move(logger)) {
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

    if (!engineService_ || !engineService_->IsInitialized()) {
        throw std::runtime_error("Script engine service not initialized");
    }

    initialized_ = true;

    logger_->Info("Script service initialized");
}

void LuaScriptService::Shutdown() noexcept {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        return;
    }

    initialized_ = false;

    logger_->Info("Script service shutdown");
}

std::vector<script::SceneManager::SceneObject> LuaScriptService::LoadSceneObjects() {
    logger_->TraceFunction(__func__);

    if (!engineService_ || !engineService_->IsInitialized()) {
        throw std::runtime_error("Script service not initialized");
    }

    return engineService_->GetEngine().LoadSceneObjects();
}

std::array<float, 16> LuaScriptService::ComputeModelMatrix(int functionRef, float time) {
    if (!engineService_ || !engineService_->IsInitialized()) {
        throw std::runtime_error("Script service not initialized");
    }

    return engineService_->GetEngine().ComputeModelMatrix(functionRef, time);
}

std::array<float, 16> LuaScriptService::GetViewProjectionMatrix(float aspect) {
    if (!engineService_ || !engineService_->IsInitialized()) {
        throw std::runtime_error("Script service not initialized");
    }

    return engineService_->GetEngine().GetViewProjectionMatrix(aspect);
}

std::unordered_map<std::string, sdl3cpp::services::ShaderPaths> LuaScriptService::LoadShaderPathsMap() {
    logger_->TraceFunction(__func__);

    if (!engineService_ || !engineService_->IsInitialized()) {
        throw std::runtime_error("Script service not initialized");
    }

    return engineService_->GetEngine().LoadShaderPathsMap();
}

std::vector<script::GuiCommand> LuaScriptService::LoadGuiCommands() {
    if (!engineService_ || !engineService_->IsInitialized()) {
        return {};
    }

    return engineService_->GetEngine().LoadGuiCommands();
}

void LuaScriptService::UpdateGuiInput(const script::GuiInputSnapshot& input) {
    if (!engineService_ || !engineService_->IsInitialized()) {
        return;
    }

    engineService_->GetEngine().UpdateGuiInput(input);
}

bool LuaScriptService::HasGuiCommands() const {
    if (!engineService_ || !engineService_->IsInitialized()) {
        return false;
    }

    return engineService_->GetEngine().HasGuiCommands();
}

script::PhysicsBridge& LuaScriptService::GetPhysicsBridge() {
    if (!engineService_ || !engineService_->IsInitialized()) {
        throw std::runtime_error("Script service not initialized");
    }

    return engineService_->GetEngine().GetPhysicsBridge();
}

std::filesystem::path LuaScriptService::GetScriptDirectory() const {
    if (!engineService_ || !engineService_->IsInitialized()) {
        return {};
    }

    return engineService_->GetEngine().GetScriptDirectory();
}

std::string LuaScriptService::GetLuaError() {
    if (!engineService_ || !engineService_->IsInitialized()) {
        return "Script service not initialized";
    }

    return engineService_->GetEngine().GetLuaError();
}

}  // namespace sdl3cpp::services::impl
