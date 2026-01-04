#include "script_engine_service.hpp"

#include "../../script/lua_bindings.hpp"
#include "../../script/script_engine.hpp"
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

ScriptEngineService::ScriptEngineService(const std::filesystem::path& scriptPath,
                                         std::shared_ptr<ILogger> logger,
                                         std::shared_ptr<IMeshService> meshService,
                                         std::shared_ptr<IAudioCommandService> audioCommandService,
                                         std::shared_ptr<IPhysicsBridgeService> physicsBridgeService,
                                         bool debugEnabled)
    : logger_(std::move(logger)),
      meshService_(std::move(meshService)),
      audioCommandService_(std::move(audioCommandService)),
      physicsBridgeService_(std::move(physicsBridgeService)),
      scriptPath_(scriptPath),
      debugEnabled_(debugEnabled) {
}

ScriptEngineService::~ScriptEngineService() {
    if (initialized_) {
        Shutdown();
    }
}

void ScriptEngineService::Initialize() {
    if (initialized_) {
        return;
    }
    logger_->TraceFunction(__func__);

    bindingContext_ = std::make_shared<script::LuaBindingContext>();
    bindingContext_->meshService = meshService_;
    bindingContext_->audioCommandService = audioCommandService_;
    bindingContext_->physicsBridgeService = physicsBridgeService_;

    engine_ = std::make_unique<script::ScriptEngine>(scriptPath_, bindingContext_.get(), debugEnabled_);
    initialized_ = true;

    logger_->Info("Script engine service initialized");
}

void ScriptEngineService::Shutdown() noexcept {
    if (!initialized_) {
        return;
    }
    logger_->TraceFunction(__func__);

    engine_.reset();
    bindingContext_.reset();
    initialized_ = false;

    logger_->Info("Script engine service shutdown");
}

script::ScriptEngine& ScriptEngineService::GetEngine() {
    if (!engine_) {
        throw std::runtime_error("Script engine service not initialized");
    }
    return *engine_;
}

std::filesystem::path ScriptEngineService::GetScriptDirectory() const {
    if (!engine_) {
        throw std::runtime_error("Script engine service not initialized");
    }
    return engine_->GetScriptDirectory();
}

}  // namespace sdl3cpp::services::impl
