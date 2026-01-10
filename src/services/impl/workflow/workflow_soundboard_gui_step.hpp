#pragma once

#include "services/interfaces/gui_types.hpp"
#include "services/interfaces/i_config_service.hpp"
#include "services/interfaces/i_input_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_soundboard_state_service.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/soundboard_types.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

class WorkflowSoundboardGuiStep final : public IWorkflowStep {
public:
    WorkflowSoundboardGuiStep(std::shared_ptr<IInputService> inputService,
                              std::shared_ptr<IConfigService> configService,
                              std::shared_ptr<ISoundboardStateService> stateService,
                              std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    struct SoundboardGuiConfig {
        GuiCommand::RectData panelRect{16.0f, 16.0f, 664.0f, 520.0f};
        float panelPaddingX = 20.0f;
        float panelPaddingY = 20.0f;
        float headerSpacing = 26.0f;
        float columnStartY = 120.0f;
        float statusOffsetY = 34.0f;
        int columns = 2;
        float columnSpacing = 24.0f;
        float buttonWidth = 300.0f;
        float buttonHeight = 36.0f;
        float buttonSpacing = 12.0f;
        float titleFontSize = 24.0f;
        float descriptionFontSize = 14.0f;
        float categoryFontSize = 20.0f;
        float buttonFontSize = 16.0f;
        float statusFontSize = 14.0f;
        GuiColor panelColor{0.06f, 0.07f, 0.09f, 0.95f};
        GuiColor panelBorder{0.35f, 0.38f, 0.42f, 1.0f};
        GuiColor titleColor{0.96f, 0.96f, 0.97f, 1.0f};
        GuiColor descriptionColor{0.7f, 0.75f, 0.8f, 1.0f};
        GuiColor categoryColor{0.9f, 0.9f, 0.95f, 1.0f};
        GuiColor statusColor{0.6f, 0.8f, 1.0f, 1.0f};
        GuiColor buttonTextColor{1.0f, 1.0f, 1.0f, 1.0f};
        GuiColor buttonBorder{0.45f, 0.52f, 0.6f, 1.0f};
        GuiColor buttonBackground{0.2f, 0.24f, 0.28f, 1.0f};
        GuiColor buttonHover{0.26f, 0.3f, 0.36f, 1.0f};
        GuiColor buttonActive{0.16f, 0.22f, 0.28f, 1.0f};
        std::string title = "Audio Soundboard";
        std::string description = "Trigger SFX or speech clips from bundled audio assets.";
    };

    SoundboardGuiConfig LoadConfig() const;
    void EnsureConfigLoaded();
    std::vector<GuiCommand> BuildCommands(const SoundboardCatalog& catalog,
                                          const SoundboardGuiConfig& config,
                                          const std::string& statusMessage,
                                          std::optional<SoundboardSelection>& selectionOut);

    std::shared_ptr<IInputService> inputService_;
    std::shared_ptr<IConfigService> configService_;
    std::shared_ptr<ISoundboardStateService> stateService_;
    std::shared_ptr<ILogger> logger_;
    std::optional<SoundboardGuiConfig> cachedConfig_;
    std::string activeWidget_;
    bool lastMouseDown_ = false;
    std::uint64_t nextRequestId_ = 1;
};

}  // namespace sdl3cpp::services::impl
