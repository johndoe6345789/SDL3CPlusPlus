#pragma once

#include "../interfaces/i_script_engine_service.hpp"
#include "../interfaces/i_logger.hpp"
#include "../../di/lifecycle.hpp"
#include <filesystem>
#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Service wrapper around ScriptEngine.
 */
class ScriptEngineService : public IScriptEngineService,
                            public di::IInitializable,
                            public di::IShutdownable {
public:
    ScriptEngineService(const std::filesystem::path& scriptPath,
                        std::shared_ptr<ILogger> logger,
                        bool debugEnabled = false);
    ~ScriptEngineService() override;

    // Lifecycle
    void Initialize() override;
    void Shutdown() noexcept override;

    // IScriptEngineService interface
    script::ScriptEngine& GetEngine() override;
    std::filesystem::path GetScriptDirectory() const override;
    bool IsInitialized() const override { return initialized_; }

private:
    std::shared_ptr<ILogger> logger_;
    std::filesystem::path scriptPath_;
    bool debugEnabled_ = false;
    bool initialized_ = false;
    std::unique_ptr<script::ScriptEngine> engine_;
};

}  // namespace sdl3cpp::services::impl
