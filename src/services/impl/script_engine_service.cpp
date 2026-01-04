#include "script_engine_service.hpp"

#include "../../script/script_engine.hpp"
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

ScriptEngineService::ScriptEngineService(const std::filesystem::path& scriptPath,
                                         std::shared_ptr<ILogger> logger,
                                         bool debugEnabled)
    : logger_(std::move(logger)),
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

    engine_ = std::make_unique<script::ScriptEngine>(scriptPath_, debugEnabled_);
    initialized_ = true;

    logger_->Info("Script engine service initialized");
}

void ScriptEngineService::Shutdown() noexcept {
    if (!initialized_) {
        return;
    }
    logger_->TraceFunction(__func__);

    engine_.reset();
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
