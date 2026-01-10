#include "workflow_soundboard_gui_step.hpp"

#include "../config/json_config_document_parser.hpp"
#include "../soundboard/soundboard_path_resolver.hpp"
#include "workflow_step_io_resolver.hpp"

#include <rapidjson/document.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

GuiColor ReadColor(const rapidjson::Value& value, const GuiColor& fallback) {
    if (!value.IsArray() || (value.Size() != 3 && value.Size() != 4)) {
        return fallback;
    }
    GuiColor color = fallback;
    color.r = static_cast<float>(value[0].GetDouble());
    color.g = static_cast<float>(value[1].GetDouble());
    color.b = static_cast<float>(value[2].GetDouble());
    color.a = value.Size() == 4 ? static_cast<float>(value[3].GetDouble()) : fallback.a;
    return color;
}

void ReadColorField(const rapidjson::Value& object, const char* name, GuiColor& target) {
    if (!object.HasMember(name)) {
        return;
    }
    const auto& value = object[name];
    if (!value.IsArray()) {
        throw std::runtime_error(std::string("soundboard.gui: '") + name + "' must be an array");
    }
    target = ReadColor(value, target);
}

bool ReadFloatField(const rapidjson::Value& object, const char* name, float& target) {
    if (!object.HasMember(name)) {
        return false;
    }
    const auto& value = object[name];
    if (!value.IsNumber()) {
        throw std::runtime_error(std::string("soundboard.gui: '") + name + "' must be a number");
    }
    target = static_cast<float>(value.GetDouble());
    return true;
}

bool ReadIntField(const rapidjson::Value& object, const char* name, int& target) {
    if (!object.HasMember(name)) {
        return false;
    }
    const auto& value = object[name];
    if (!value.IsInt()) {
        throw std::runtime_error(std::string("soundboard.gui: '") + name + "' must be an integer");
    }
    target = value.GetInt();
    return true;
}

bool ReadStringField(const rapidjson::Value& object, const char* name, std::string& target) {
    if (!object.HasMember(name)) {
        return false;
    }
    const auto& value = object[name];
    if (!value.IsString()) {
        throw std::runtime_error(std::string("soundboard.gui: '") + name + "' must be a string");
    }
    target = value.GetString();
    return true;
}

GuiCommand BuildRectCommand(const GuiCommand::RectData& rect, const GuiColor& color,
                            const GuiColor& border, float borderWidth) {
    GuiCommand command{};
    command.type = GuiCommand::Type::Rect;
    command.rect = rect;
    command.color = color;
    command.borderColor = border;
    command.borderWidth = borderWidth;
    return command;
}

GuiCommand BuildTextCommand(const std::string& text, float x, float y, float fontSize,
                            const GuiColor& color, const std::string& alignX,
                            const std::string& alignY) {
    GuiCommand command{};
    command.type = GuiCommand::Type::Text;
    command.text = text;
    command.rect.x = x;
    command.rect.y = y;
    command.fontSize = fontSize;
    command.color = color;
    command.alignX = alignX;
    command.alignY = alignY;
    return command;
}

bool IsMouseOver(const GuiCommand::RectData& rect, float x, float y) {
    return x >= rect.x && x <= rect.x + rect.width && y >= rect.y && y <= rect.y + rect.height;
}

}  // namespace

WorkflowSoundboardGuiStep::WorkflowSoundboardGuiStep(std::shared_ptr<IInputService> inputService,
                                                     std::shared_ptr<IConfigService> configService,
                                                     std::shared_ptr<ISoundboardStateService> stateService,
                                                     std::shared_ptr<ILogger> logger)
    : inputService_(std::move(inputService)),
      configService_(std::move(configService)),
      stateService_(std::move(stateService)),
      logger_(std::move(logger)) {}

std::string WorkflowSoundboardGuiStep::GetPluginId() const {
    return "soundboard.gui";
}

void WorkflowSoundboardGuiStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!inputService_) {
        throw std::runtime_error("soundboard.gui requires an IInputService");
    }
    EnsureConfigLoaded();

    WorkflowStepIoResolver resolver;
    const std::string catalogKey = resolver.GetRequiredInputKey(step, "catalog");
    const std::string selectionKey = resolver.GetRequiredOutputKey(step, "selection");
    const std::string commandsKey = resolver.GetRequiredOutputKey(step, "gui_commands");

    const auto* catalog = context.TryGet<SoundboardCatalog>(catalogKey);
    if (!catalog) {
        throw std::runtime_error("soundboard.gui missing catalog input");
    }

    const std::string status = stateService_
        ? stateService_->GetStatusMessage()
        : std::string("Select a clip to play");

    std::optional<SoundboardSelection> selection;
    std::vector<GuiCommand> commands = BuildCommands(*catalog, *cachedConfig_, status, selection);
    const std::size_t commandCount = commands.size();

    SoundboardSelection output{};
    if (selection) {
        output = *selection;
        if (logger_) {
            logger_->Trace("WorkflowSoundboardGuiStep", "Execute",
                           "selection=" + output.label +
                               ", requestId=" + std::to_string(output.requestId),
                           "Soundboard selection updated");
        }
    }
    context.Set(selectionKey, output);
    context.Set(commandsKey, std::move(commands));
    if (logger_) {
        logger_->Trace("WorkflowSoundboardGuiStep", "Execute",
                       "commands=" + std::to_string(commandCount),
                       "GUI commands prepared");
    }
}

void WorkflowSoundboardGuiStep::EnsureConfigLoaded() {
    if (!cachedConfig_) {
        cachedConfig_ = LoadConfig();
        if (logger_) {
            logger_->Trace("WorkflowSoundboardGuiStep", "EnsureConfigLoaded",
                           "title=" + cachedConfig_->title);
        }
    }
}

WorkflowSoundboardGuiStep::SoundboardGuiConfig WorkflowSoundboardGuiStep::LoadConfig() const {
    const std::filesystem::path packageRoot = ResolveSoundboardPackageRoot(configService_);
    const std::filesystem::path guiPath = packageRoot / "assets" / "soundboard_gui.json";
    json_config::JsonConfigDocumentParser parser;
    auto document = parser.Parse(guiPath, "soundboard gui config");

    SoundboardGuiConfig config{};
    if (document.HasMember("panel") && document["panel"].IsObject()) {
        const auto& panel = document["panel"];
        ReadStringField(panel, "title", config.title);
        ReadStringField(panel, "description", config.description);
        if (panel.HasMember("rect") && panel["rect"].IsObject()) {
            const auto& rect = panel["rect"];
            ReadFloatField(rect, "x", config.panelRect.x);
            ReadFloatField(rect, "y", config.panelRect.y);
            ReadFloatField(rect, "width", config.panelRect.width);
            ReadFloatField(rect, "height", config.panelRect.height);
        }
        if (panel.HasMember("padding") && panel["padding"].IsObject()) {
            const auto& padding = panel["padding"];
            ReadFloatField(padding, "x", config.panelPaddingX);
            ReadFloatField(padding, "y", config.panelPaddingY);
        }
        if (panel.HasMember("colors") && panel["colors"].IsObject()) {
            const auto& colors = panel["colors"];
            ReadColorField(colors, "background", config.panelColor);
            ReadColorField(colors, "border", config.panelBorder);
            ReadColorField(colors, "title", config.titleColor);
            ReadColorField(colors, "description", config.descriptionColor);
            ReadColorField(colors, "category", config.categoryColor);
            ReadColorField(colors, "status", config.statusColor);
        }
    }
    if (document.HasMember("buttons") && document["buttons"].IsObject()) {
        const auto& buttons = document["buttons"];
        ReadFloatField(buttons, "width", config.buttonWidth);
        ReadFloatField(buttons, "height", config.buttonHeight);
        ReadFloatField(buttons, "spacing", config.buttonSpacing);
        if (buttons.HasMember("colors") && buttons["colors"].IsObject()) {
            const auto& colors = buttons["colors"];
            ReadColorField(colors, "text", config.buttonTextColor);
            ReadColorField(colors, "border", config.buttonBorder);
            ReadColorField(colors, "background", config.buttonBackground);
            ReadColorField(colors, "hover", config.buttonHover);
            ReadColorField(colors, "active", config.buttonActive);
        }
    }
    if (document.HasMember("layout") && document["layout"].IsObject()) {
        const auto& layout = document["layout"];
        ReadIntField(layout, "columns", config.columns);
        ReadFloatField(layout, "columnSpacing", config.columnSpacing);
        ReadFloatField(layout, "initialY", config.columnStartY);
        ReadFloatField(layout, "statusOffsetY", config.statusOffsetY);
    }
    if (document.HasMember("typography") && document["typography"].IsObject()) {
        const auto& type = document["typography"];
        ReadFloatField(type, "titleSize", config.titleFontSize);
        ReadFloatField(type, "descriptionSize", config.descriptionFontSize);
        ReadFloatField(type, "categorySize", config.categoryFontSize);
        ReadFloatField(type, "buttonSize", config.buttonFontSize);
        ReadFloatField(type, "statusSize", config.statusFontSize);
        ReadFloatField(type, "headerSpacing", config.headerSpacing);
    }
    return config;
}

std::vector<GuiCommand> WorkflowSoundboardGuiStep::BuildCommands(
    const SoundboardCatalog& catalog,
    const SoundboardGuiConfig& config,
    const std::string& statusMessage,
    std::optional<SoundboardSelection>& selectionOut) {
    std::vector<GuiCommand> commands;

    const auto& rect = config.panelRect;
    commands.push_back(BuildRectCommand(rect, config.panelColor, config.panelBorder, 1.0f));

    const float titleX = rect.x + config.panelPaddingX;
    const float titleY = rect.y + config.panelPaddingY;
    commands.push_back(BuildTextCommand(config.title, titleX, titleY, config.titleFontSize,
                                        config.titleColor, "left", "top"));
    commands.push_back(BuildTextCommand(config.description, titleX,
                                        titleY + config.headerSpacing,
                                        config.descriptionFontSize, config.descriptionColor,
                                        "left", "top"));

    const auto inputState = inputService_->GetState();
    const float mouseX = inputState.mouseX;
    const float mouseY = inputState.mouseY;
    const bool mouseDown = inputService_->IsMouseButtonPressed(SDL_BUTTON_LEFT);
    const bool justPressed = mouseDown && !lastMouseDown_;
    const bool justReleased = !mouseDown && lastMouseDown_;

    const float columnStartY = rect.y + config.columnStartY;
    int columnCount = std::max(1, config.columns);
    for (size_t index = 0; index < catalog.categories.size(); ++index) {
        const SoundboardCategory& category = catalog.categories[index];
        int columnIndex = static_cast<int>(index % static_cast<size_t>(columnCount));
        float columnX = rect.x + config.panelPaddingX +
                        columnIndex * (config.buttonWidth + config.columnSpacing);
        commands.push_back(BuildTextCommand(category.name, columnX, columnStartY,
                                            config.categoryFontSize, config.categoryColor,
                                            "left", "top"));

        float buttonY = columnStartY + config.categoryFontSize + config.buttonSpacing;
        if (category.clips.empty()) {
            commands.push_back(BuildTextCommand("No clips available", columnX, buttonY,
                                                config.buttonFontSize, config.descriptionColor,
                                                "left", "top"));
            continue;
        }
        for (const auto& clip : category.clips) {
            GuiCommand::RectData buttonRect{columnX, buttonY, config.buttonWidth, config.buttonHeight};
            const std::string widgetId = category.id + "::" + clip.id;
            const bool hovered = IsMouseOver(buttonRect, mouseX, mouseY);
            if (justPressed && hovered) {
                activeWidget_ = widgetId;
            }
            bool clicked = false;
            if (justReleased && hovered && activeWidget_ == widgetId) {
                clicked = true;
            }
            if (justReleased && activeWidget_ == widgetId) {
                activeWidget_.clear();
            }

            GuiColor fill = config.buttonBackground;
            if (activeWidget_ == widgetId && mouseDown) {
                fill = config.buttonActive;
            } else if (hovered) {
                fill = config.buttonHover;
            }
            commands.push_back(BuildRectCommand(buttonRect, fill, config.buttonBorder, 1.0f));
            const float textX = buttonRect.x + (buttonRect.width * 0.5f);
            const float textY = buttonRect.y + (buttonRect.height * 0.5f);
            commands.push_back(BuildTextCommand(clip.label, textX, textY, config.buttonFontSize,
                                                config.buttonTextColor, "center", "center"));

            if (clicked) {
                SoundboardSelection selection{};
                selection.hasSelection = true;
                selection.requestId = nextRequestId_++;
                selection.categoryId = category.id;
                selection.clipId = clip.id;
                selection.label = clip.label;
                selection.path = clip.path;
                selectionOut = selection;
                if (stateService_) {
                    stateService_->SetStatusMessage("Playing \"" + clip.label + "\"");
                }
            }

            buttonY += config.buttonHeight + config.buttonSpacing;
        }
    }

    if (justReleased && !activeWidget_.empty()) {
        activeWidget_.clear();
    }

    const float statusY = rect.y + rect.height - config.statusOffsetY;
    commands.push_back(BuildTextCommand(statusMessage, rect.x + config.panelPaddingX,
                                        statusY, config.statusFontSize,
                                        config.statusColor, "left", "center"));

    lastMouseDown_ = mouseDown;
    return commands;
}

}  // namespace sdl3cpp::services::impl
