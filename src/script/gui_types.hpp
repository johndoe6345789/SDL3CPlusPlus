#ifndef SDL3CPP_SCRIPT_GUI_TYPES_HPP
#define SDL3CPP_SCRIPT_GUI_TYPES_HPP

#include <string>
#include <unordered_map>

namespace sdl3cpp::script {

struct GuiInputSnapshot {
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    bool mouseDown = false;
    float wheel = 0.0f;
    std::string textInput;
    std::unordered_map<std::string, bool> keyStates;
};

struct GuiColor {
    float r = 0;
    float g = 0;
    float b = 0;
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
        float x = 0;
        float y = 0;
        float width = 0;
        float height = 0;
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

} // namespace sdl3cpp::script

#endif // SDL3CPP_SCRIPT_GUI_TYPES_HPP
