#pragma once

#include <string>
#include <unordered_map>

namespace sdl3cpp::services {

struct GuiInputSnapshot {
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;
    bool mouseDown = false;
    float wheel = 0.0f;
    std::string textInput;
    std::unordered_map<std::string, bool> keyStates;
    bool gamepadConnected = false;
    float gamepadLeftX = 0.0f;
    float gamepadLeftY = 0.0f;
    float gamepadRightX = 0.0f;
    float gamepadRightY = 0.0f;
    bool gamepadTogglePressed = false;
};

struct GuiColor {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
};

struct GuiCommand {
    enum class Type {
        Rect,
        Text,
        ClipPush,
        ClipPop,
        Svg,
    };

    struct RectData {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
    };

    Type type = Type::Rect;
    RectData rect;
    GuiColor color;
    GuiColor borderColor;
    float borderWidth = 0.0f;
    bool hasClipRect = false;
    RectData clipRect{};
    std::string text;
    float fontSize = 16.0f;
    std::string alignX = "left";
    std::string alignY = "center";
    std::string svgPath;
    GuiColor svgTint;
    RectData bounds{};
    bool hasBounds = false;
};

}  // namespace sdl3cpp::services
