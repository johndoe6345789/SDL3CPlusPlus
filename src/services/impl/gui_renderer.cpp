#include "gui_renderer.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <shaderc/shaderc.hpp>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "font8x8_basic.h"

namespace sdl3cpp::services::impl {
namespace {

bool ExtractAttribute(const std::string& source, const char* name, std::string& outValue) {
    std::string key = name;
    size_t pos = source.find(key);
    while (pos != std::string::npos) {
        size_t eq = source.find('=', pos + key.size());
        if (eq == std::string::npos) {
            break;
        }
        size_t valueStart = eq + 1;
        while (valueStart < source.size() &&
               std::isspace(static_cast<unsigned char>(source[valueStart]))) {
            valueStart++;
        }
        if (valueStart >= source.size()) {
            break;
        }
        char quote = source[valueStart];
        if (quote != '\"' && quote != '\'') {
            break;
        }
        size_t valueEnd = source.find(quote, valueStart + 1);
        if (valueEnd == std::string::npos) {
            break;
        }
        outValue = source.substr(valueStart + 1, valueEnd - valueStart - 1);
        return true;
    }
    return false;
}

float ParseFloatValue(const std::string& text) {
    try {
        size_t idx = 0;
        return std::stof(text, &idx);
    } catch (...) {
        return 0.0f;
    }
}

GuiColor ParseColorString(const std::string& text, const GuiColor& fallback) {
    if (text.empty() || text[0] != '#') {
        return fallback;
    }
    try {
        if (text.size() == 7) {
            unsigned int rgb = std::stoul(text.substr(1), nullptr, 16);
            return {((rgb >> 16) & 0xFF) / 255.0f, ((rgb >> 8) & 0xFF) / 255.0f,
                    (rgb & 0xFF) / 255.0f, 1.0f};
        }
        if (text.size() == 9) {
            unsigned int rgba = std::stoul(text.substr(1), nullptr, 16);
            return {((rgba >> 24) & 0xFF) / 255.0f, ((rgba >> 16) & 0xFF) / 255.0f,
                    ((rgba >> 8) & 0xFF) / 255.0f, (rgba & 0xFF) / 255.0f};
        }
    } catch (...) {
    }
    return fallback;
}

ParsedSvg ParseSvgFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Failed to open SVG file: " + path.string());
    }
    std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    ParsedSvg result;
    std::string value;
    if (ExtractAttribute(data, "viewBox", value)) {
        float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
        std::sscanf(value.c_str(), "%f %f %f %f", &x, &y, &w, &h);
        if (w > 0.0f && h > 0.0f) {
            result.viewWidth = w;
            result.viewHeight = h;
        }
    }
    if (ExtractAttribute(data, "width", value)) {
        result.viewWidth = ParseFloatValue(value);
    }
    if (ExtractAttribute(data, "height", value)) {
        result.viewHeight = ParseFloatValue(value);
    }
    if (result.viewWidth <= 0.0f) {
        result.viewWidth = 128.0f;
    }
    if (result.viewHeight <= 0.0f) {
        result.viewHeight = 128.0f;
    }

    size_t search = 0;
    while (true) {
        size_t tagStart = data.find("<circle", search);
        if (tagStart == std::string::npos) {
            break;
        }
        size_t tagEnd = data.find('>', tagStart);
        if (tagEnd == std::string::npos) {
            break;
        }
        std::string tag = data.substr(tagStart, tagEnd - tagStart);
        SvgCircle circle;
        std::string attr;
        if (ExtractAttribute(tag, "cx", attr)) {
            circle.cx = ParseFloatValue(attr);
        }
        if (ExtractAttribute(tag, "cy", attr)) {
            circle.cy = ParseFloatValue(attr);
        }
        if (ExtractAttribute(tag, "r", attr)) {
            circle.r = ParseFloatValue(attr);
        }
        if (ExtractAttribute(tag, "fill", attr)) {
            circle.color = ParseColorString(attr, circle.color);
        }
        result.circles.push_back(circle);
        search = tagEnd + 1;
    }
    return result;
}

GuiCommand::RectData IntersectRect(const GuiCommand::RectData& a, const GuiCommand::RectData& b) {
    GuiCommand::RectData result;
    result.x = std::max(a.x, b.x);
    result.y = std::max(a.y, b.y);
    float right = std::min(a.x + a.width, b.x + b.width);
    float bottom = std::min(a.y + a.height, b.y + b.height);
    result.width = std::max(0.0f, right - result.x);
    result.height = std::max(0.0f, bottom - result.y);
    return result;
}

bool RectHasArea(const GuiCommand::RectData& rect) {
    return rect.width > 0.0f && rect.height > 0.0f;
}

struct ClipPoint {
    float x = 0.0f;
    float y = 0.0f;
};

enum class ClipEdge {
    Left,
    Right,
    Top,
    Bottom
};

bool IsInsideClip(const ClipPoint& point, const GuiCommand::RectData& rect, ClipEdge edge) {
    switch (edge) {
        case ClipEdge::Left:
            return point.x >= rect.x;
        case ClipEdge::Right:
            return point.x <= rect.x + rect.width;
        case ClipEdge::Top:
            return point.y >= rect.y;
        case ClipEdge::Bottom:
            return point.y <= rect.y + rect.height;
    }
    return false;
}

ClipPoint IntersectClipEdge(const ClipPoint& a, const ClipPoint& b,
                            const GuiCommand::RectData& rect, ClipEdge edge) {
    ClipPoint result = a;
    float dx = b.x - a.x;
    float dy = b.y - a.y;

    if (edge == ClipEdge::Left || edge == ClipEdge::Right) {
        float clipX = (edge == ClipEdge::Left) ? rect.x : (rect.x + rect.width);
        float t = (dx != 0.0f) ? (clipX - a.x) / dx : 0.0f;
        result.x = clipX;
        result.y = a.y + t * dy;
    } else {
        float clipY = (edge == ClipEdge::Top) ? rect.y : (rect.y + rect.height);
        float t = (dy != 0.0f) ? (clipY - a.y) / dy : 0.0f;
        result.x = a.x + t * dx;
        result.y = clipY;
    }
    return result;
}

std::vector<ClipPoint> ClipPolygonToRect(const std::vector<ClipPoint>& polygon,
                                         const GuiCommand::RectData& rect) {
    std::vector<ClipPoint> output = polygon;
    for (ClipEdge edge : {ClipEdge::Left, ClipEdge::Right, ClipEdge::Top, ClipEdge::Bottom}) {
        if (output.empty()) {
            break;
        }
        std::vector<ClipPoint> input = output;
        output.clear();

        for (size_t i = 0; i < input.size(); ++i) {
            const ClipPoint& current = input[i];
            const ClipPoint& previous = input[(i + input.size() - 1) % input.size()];
            bool currentInside = IsInsideClip(current, rect, edge);
            bool previousInside = IsInsideClip(previous, rect, edge);

            if (currentInside) {
                if (!previousInside) {
                    output.push_back(IntersectClipEdge(previous, current, rect, edge));
                }
                output.push_back(current);
            } else if (previousInside) {
                output.push_back(IntersectClipEdge(previous, current, rect, edge));
            }
        }
    }
    return output;
}

shaderc_shader_kind ShadercKindFromStage(VkShaderStageFlagBits stage) {
    switch (stage) {
        case VK_SHADER_STAGE_VERTEX_BIT:
            return shaderc_vertex_shader;
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            return shaderc_fragment_shader;
        case VK_SHADER_STAGE_GEOMETRY_BIT:
            return shaderc_geometry_shader;
        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
            return shaderc_tess_control_shader;
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
            return shaderc_tess_evaluation_shader;
        case VK_SHADER_STAGE_COMPUTE_BIT:
            return shaderc_compute_shader;
        default:
            return shaderc_glsl_infer_from_source;
    }
}

std::vector<uint8_t> ReadShaderFile(const std::filesystem::path& path,
                                    VkShaderStageFlagBits stage,
                                    ILogger* logger) {
    if (logger) {
        logger->Trace("GuiRenderer", "ReadShaderFile",
                      "path=" + path.string() + ", stage=" +
                      std::to_string(static_cast<int>(stage)));
    }

    if (path.empty()) {
        throw std::runtime_error("Shader path is empty");
    }

    std::filesystem::path shaderPath = path;
    if (shaderPath.extension() == ".spv") {
        std::filesystem::path sourcePath = shaderPath;
        sourcePath.replace_extension();
        if (logger) {
            logger->Trace("GuiRenderer", "ReadShaderFile",
                          "usingSource=" + sourcePath.string());
        }
        shaderPath = sourcePath;
    }

    if (!std::filesystem::exists(shaderPath)) {
        throw std::runtime_error("Shader file not found: " + shaderPath.string() +
            "\n\nPlease ensure the shader source (.vert/.frag/etc.) exists.");
    }

    if (!std::filesystem::is_regular_file(shaderPath)) {
        throw std::runtime_error("Path is not a regular file: " + shaderPath.string());
    }

    std::ifstream sourceFile(shaderPath);
    if (!sourceFile) {
        throw std::runtime_error("Failed to open shader source: " + shaderPath.string());
    }
    std::string source((std::istreambuf_iterator<char>(sourceFile)),
                       std::istreambuf_iterator<char>());
    sourceFile.close();

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);

    shaderc_shader_kind kind = ShadercKindFromStage(stage);
    auto result = compiler.CompileGlslToSpv(source, kind, shaderPath.string().c_str(), options);
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::string error = result.GetErrorMessage();
        if (logger) {
            logger->Error("GuiRenderer shader compilation failed: " + shaderPath.string() +
                          "\n" + error);
        }
        throw std::runtime_error("Shader compilation failed: " + shaderPath.string() +
            "\n" + error);
    }

    std::vector<uint32_t> spirv(result.cbegin(), result.cend());
    std::vector<uint8_t> buffer(spirv.size() * sizeof(uint32_t));
    if (!buffer.empty()) {
        std::memcpy(buffer.data(), spirv.data(), buffer.size());
    }
    if (logger) {
        logger->Trace("GuiRenderer", "ReadShaderFile",
                      "compiledBytes=" + std::to_string(buffer.size()));
    }
    return buffer;
}

std::vector<uint8_t> ReadShaderSource(const std::string& source,
                                      VkShaderStageFlagBits stage,
                                      const std::string& label,
                                      ILogger* logger) {
    if (logger) {
        logger->Trace("GuiRenderer", "ReadShaderSource",
                      "label=" + label + ", stage=" +
                      std::to_string(static_cast<int>(stage)));
    }

    if (source.empty()) {
        throw std::runtime_error("Shader source is empty");
    }

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);

    shaderc_shader_kind kind = ShadercKindFromStage(stage);
    auto result = compiler.CompileGlslToSpv(source, kind, label.c_str(), options);
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::string error = result.GetErrorMessage();
        if (logger) {
            logger->Error("GuiRenderer shader compilation failed: " + label + "\n" + error);
        }
        throw std::runtime_error("Shader compilation failed: " + label + "\n" + error);
    }

    std::vector<uint32_t> spirv(result.cbegin(), result.cend());
    std::vector<uint8_t> buffer(spirv.size() * sizeof(uint32_t));
    if (!buffer.empty()) {
        std::memcpy(buffer.data(), spirv.data(), buffer.size());
    }
    if (logger) {
        logger->Trace("GuiRenderer", "ReadShaderSource",
                      "compiledBytes=" + std::to_string(buffer.size()));
    }
    return buffer;
}

const char* kGuiVertexSource = R"(
#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProj;
    // Extended fields for PBR/atmospherics (ignored by basic shaders)
    mat4 view;
    mat4 proj;
    mat4 lightViewProj;
    vec3 cameraPos;
    float time;
    // Atmospherics parameters
    float ambientStrength;
    float fogDensity;
    float fogStart;
    float fogEnd;
    vec3 fogColor;
    float gamma;
    float exposure;
    int enableShadows;
    int enableFog;
} pushConstants;

void main() {
    fragColor = inColor;
    vec4 worldPos = pushConstants.model * vec4(inPos, 1.0);
    gl_Position = pushConstants.viewProj * worldPos;
}
)";

const char* kGuiFragmentSource = R"(
#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = fragColor;
}
)";

} // namespace

GuiRenderer::GuiRenderer(VkDevice device, VkPhysicalDevice physicalDevice, VkFormat swapchainFormat,
                         VkRenderPass renderPass, const std::filesystem::path& scriptDirectory,
                         const GuiFontConfig& fontConfig,
                         std::shared_ptr<IBufferService> bufferService,
                         std::shared_ptr<ILogger> logger)
    : device_(device),
      physicalDevice_(physicalDevice),
      swapchainFormat_(swapchainFormat),
      renderPass_(renderPass),
      scriptDirectory_(scriptDirectory),
      fontConfig_(fontConfig),
      bufferService_(std::move(bufferService)),
      logger_(std::move(logger)) {
}

GuiRenderer::~GuiRenderer() {
    CleanupBuffers();
    CleanupPipeline();
    if (freetypeReady_) {
        FT_Face face = reinterpret_cast<FT_Face>(ftFace_);
        FT_Library library = reinterpret_cast<FT_Library>(ftLibrary_);
        if (face) {
            FT_Done_Face(face);
        }
        if (library) {
            FT_Done_FreeType(library);
        }
    }
}

bool GuiRenderer::IsReady() const {
    return pipeline_ != VK_NULL_HANDLE && !vertices_.empty();
}

void GuiRenderer::Prepare(const std::vector<GuiCommand>& commands, uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) {
        return;
    }

    viewportWidth_ = width;
    viewportHeight_ = height;

    // Create pipeline if needed
    if (pipeline_ == VK_NULL_HANDLE) {
        CreatePipeline(renderPass_, {width, height});
    }

    // Generate geometry from GUI commands
    GenerateGuiGeometry(commands, width, height);

    // Create/update vertex and index buffers
    if (!vertices_.empty() && !indices_.empty()) {
        CreateVertexAndIndexBuffers(vertices_.size(), indices_.size());
    }
}

void GuiRenderer::RenderToSwapchain(VkCommandBuffer commandBuffer, VkRenderPass renderPass) {
    if (!IsReady() || vertices_.empty() || indices_.empty()) {
        return;
    }

    // Bind pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

    // Bind vertex and index buffers
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer_, &offset);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer_, 0, VK_INDEX_TYPE_UINT32);

    // Push identity matrices for 2D GUI rendering (no 3D transformation)
    // The GUI coordinates are already in NDC space, so we use identity matrices
    struct GuiPushConstants {
        float model[16];      // Identity matrix
        float viewProj[16];   // Identity matrix
    } pushConstants{};

    // Initialize as identity matrices
    pushConstants.model[0] = 1.0f;
    pushConstants.model[5] = 1.0f;
    pushConstants.model[10] = 1.0f;
    pushConstants.model[15] = 1.0f;

    pushConstants.viewProj[0] = 1.0f;
    pushConstants.viewProj[5] = 1.0f;
    pushConstants.viewProj[10] = 1.0f;
    pushConstants.viewProj[15] = 1.0f;

    vkCmdPushConstants(commandBuffer, pipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(GuiPushConstants), &pushConstants);

    // Draw
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices_.size()), 1, 0, 0, 0);
}

void GuiRenderer::Resize(uint32_t width, uint32_t height, VkFormat format, VkRenderPass renderPass) {
    if (width == viewportWidth_ && height == viewportHeight_ &&
        format == swapchainFormat_ && renderPass == renderPass_) {
        return;
    }
    UpdateFormat(format);
    viewportWidth_ = width;
    viewportHeight_ = height;
    renderPass_ = renderPass;

    // Recreate pipeline for new viewport size
    if (pipeline_ != VK_NULL_HANDLE) {
        CleanupPipeline();
        CreatePipeline(renderPass_, {width, height});
    }
}

bool GuiRenderer::EnsureFreeTypeReady() {
    if (freetypeReady_ || !fontConfig_.useFreeType) {
        return freetypeReady_;
    }

    FT_Library library = nullptr;
    if (FT_Init_FreeType(&library) != 0) {
        if (logger_) {
            logger_->Warn("GuiRenderer: FreeType initialization failed");
        }
        return false;
    }

    std::filesystem::path fontPath = ResolveFontPath();
    if (fontPath.empty()) {
        if (logger_) {
            logger_->Warn("GuiRenderer: FreeType font path not set or not found");
        }
        FT_Done_FreeType(library);
        return false;
    }

    FT_Face face = nullptr;
    if (FT_New_Face(library, fontPath.string().c_str(), 0, &face) != 0) {
        if (logger_) {
            logger_->Warn("GuiRenderer: Failed to load FreeType font at " + fontPath.string());
        }
        FT_Done_FreeType(library);
        return false;
    }

    ftLibrary_ = library;
    ftFace_ = face;
    freetypeReady_ = true;
    return true;
}

std::filesystem::path GuiRenderer::ResolveFontPath() const {
    if (!fontConfig_.fontPath.empty()) {
        std::filesystem::path candidate = fontConfig_.fontPath;
        if (candidate.is_absolute()) {
            return candidate;
        }
        if (!scriptDirectory_.empty()) {
            auto projectRoot = scriptDirectory_.parent_path();
            if (!projectRoot.empty()) {
                return std::filesystem::weakly_canonical(projectRoot / candidate);
            }
            return std::filesystem::weakly_canonical(scriptDirectory_ / candidate);
        }
    }

    if (!scriptDirectory_.empty()) {
        auto fallback = scriptDirectory_ / "assets" / "fonts" / "Roboto-Regular.ttf";
        if (std::filesystem::exists(fallback)) {
            return std::filesystem::weakly_canonical(fallback);
        }
    }
    return {};
}

const GuiRenderer::GlyphBitmap* GuiRenderer::LoadGlyph(char c, int pixelSize) {
    if (!EnsureFreeTypeReady()) {
        return nullptr;
    }
    if (pixelSize <= 0) {
        pixelSize = 1;
    }
    uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(pixelSize)) << 32) |
                   static_cast<uint8_t>(c);
    auto it = glyphCache_.find(key);
    if (it != glyphCache_.end()) {
        return &it->second;
    }

    FT_Face face = reinterpret_cast<FT_Face>(ftFace_);
    if (!face) {
        return nullptr;
    }
    if (currentFontSize_ != pixelSize) {
        FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(pixelSize));
        currentFontSize_ = pixelSize;
    }

    if (FT_Load_Char(face, static_cast<FT_ULong>(static_cast<unsigned char>(c)), FT_LOAD_RENDER) != 0) {
        return nullptr;
    }

    const FT_GlyphSlot slot = face->glyph;
    GlyphBitmap glyph;
    glyph.width = static_cast<int>(slot->bitmap.width);
    glyph.height = static_cast<int>(slot->bitmap.rows);
    glyph.pitch = static_cast<int>(slot->bitmap.pitch);
    glyph.bearingX = slot->bitmap_left;
    glyph.bearingY = slot->bitmap_top;
    glyph.advance = static_cast<int>(slot->advance.x >> 6);
    int pitch = glyph.pitch;
    if (pitch < 0) {
        pitch = -pitch;
    }
    glyph.pixels.assign(slot->bitmap.buffer, slot->bitmap.buffer + pitch * glyph.height);

    auto inserted = glyphCache_.emplace(key, std::move(glyph));
    return &inserted.first->second;
}

void GuiRenderer::RenderFreeTypeText(const GuiCommand& cmd,
                                     const GuiCommand::RectData& activeClip,
                                     const GuiCommand::RectData& bounds) {
    FT_Face face = reinterpret_cast<FT_Face>(ftFace_);
    if (!face) {
        return;
    }
    int pixelSize = static_cast<int>(std::max(1.0f, cmd.fontSize));
    if (currentFontSize_ != pixelSize) {
        FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(pixelSize));
        currentFontSize_ = pixelSize;
    }

    float ascender = face->size ? static_cast<float>(face->size->metrics.ascender) / 64.0f : cmd.fontSize;
    float lineHeight = face->size ? static_cast<float>(face->size->metrics.height) / 64.0f : cmd.fontSize;

    float textWidth = 0.0f;
    for (char c : cmd.text) {
        const GlyphBitmap* glyph = LoadGlyph(c, pixelSize);
        if (glyph) {
            textWidth += static_cast<float>(glyph->advance);
        }
    }

    float startX = bounds.x;
    float startY = bounds.y;
    if (cmd.alignX == "center") {
        startX += (bounds.width - textWidth) * 0.5f;
    } else if (cmd.alignX == "right") {
        startX += bounds.width - textWidth;
    }
    if (cmd.alignY == "center") {
        startY += (bounds.height - lineHeight) * 0.5f;
    } else if (cmd.alignY == "bottom") {
        startY += bounds.height - lineHeight;
    }

    float penX = startX;
    float baseline = startY + ascender;

    for (char c : cmd.text) {
        const GlyphBitmap* glyph = LoadGlyph(c, pixelSize);
        if (!glyph || glyph->pixels.empty()) {
            penX += static_cast<float>(glyph ? glyph->advance : pixelSize / 2);
            continue;
        }

        float glyphX = penX + static_cast<float>(glyph->bearingX);
        float glyphY = baseline - static_cast<float>(glyph->bearingY);

        int pitch = glyph->pitch < 0 ? -glyph->pitch : glyph->pitch;
        for (int row = 0; row < glyph->height; ++row) {
            for (int col = 0; col < glyph->width; ++col) {
                uint8_t alpha = glyph->pixels[row * pitch + col];
                if (alpha == 0) {
                    continue;
                }
                float a = cmd.color.a * (static_cast<float>(alpha) / 255.0f);
                GuiColor color{cmd.color.r, cmd.color.g, cmd.color.b, a};
                GuiCommand::RectData pixelRect{
                    glyphX + static_cast<float>(col),
                    glyphY + static_cast<float>(row),
                    1.0f,
                    1.0f
                };
                AddQuad(pixelRect, color, activeClip);
            }
        }
        penX += static_cast<float>(glyph->advance);
    }
}

void GuiRenderer::AddQuad(const GuiCommand::RectData& rect,
                          const GuiColor& color,
                          const GuiCommand::RectData& clipRect) {
    GuiCommand::RectData clipped = IntersectRect(rect, clipRect);
    if (!RectHasArea(clipped)) {
        return;
    }

    auto toNDC = [this](float x, float y) -> std::pair<float, float> {
        float width = viewportWidth_ == 0 ? 1.0f : static_cast<float>(viewportWidth_);
        float height = viewportHeight_ == 0 ? 1.0f : static_cast<float>(viewportHeight_);
        return {
            (x / width) * 2.0f - 1.0f,
            (y / height) * 2.0f - 1.0f
        };
    };

    auto [x1, y1] = toNDC(clipped.x, clipped.y);
    auto [x2, y2] = toNDC(clipped.x + clipped.width, clipped.y + clipped.height);

    size_t baseIndex = vertices_.size();
    vertices_.push_back({x1, y1, 0.0f, color.r, color.g, color.b, color.a});
    vertices_.push_back({x2, y1, 0.0f, color.r, color.g, color.b, color.a});
    vertices_.push_back({x2, y2, 0.0f, color.r, color.g, color.b, color.a});
    vertices_.push_back({x1, y2, 0.0f, color.r, color.g, color.b, color.a});

    indices_.push_back(static_cast<uint32_t>(baseIndex + 0));
    indices_.push_back(static_cast<uint32_t>(baseIndex + 1));
    indices_.push_back(static_cast<uint32_t>(baseIndex + 2));
    indices_.push_back(static_cast<uint32_t>(baseIndex + 0));
    indices_.push_back(static_cast<uint32_t>(baseIndex + 2));
    indices_.push_back(static_cast<uint32_t>(baseIndex + 3));
}

void GuiRenderer::GenerateGuiGeometry(const std::vector<GuiCommand>& commands, uint32_t width, uint32_t height) {
    vertices_.clear();
    indices_.clear();

    // Convert screen coordinates to NDC (-1 to 1)
    auto toNDC = [width, height](float x, float y) -> std::pair<float, float> {
        return {
            (x / static_cast<float>(width)) * 2.0f - 1.0f,
            (y / static_cast<float>(height)) * 2.0f - 1.0f
        };
    };

    std::vector<GuiCommand::RectData> clipStack;
    clipStack.push_back({0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)});

    auto addClippedPolygon = [&](const std::vector<ClipPoint>& polygon,
                                 const GuiColor& color,
                                 const GuiCommand::RectData& clipRect) {
        std::vector<ClipPoint> clipped = ClipPolygonToRect(polygon, clipRect);
        if (clipped.size() < 3) {
            return;
        }
        uint32_t baseIndex = static_cast<uint32_t>(vertices_.size());
        for (const auto& point : clipped) {
            auto [x, y] = toNDC(point.x, point.y);
            vertices_.push_back({x, y, 0.0f, color.r, color.g, color.b, color.a});
        }
        for (size_t i = 1; i + 1 < clipped.size(); ++i) {
            indices_.push_back(baseIndex);
            indices_.push_back(baseIndex + static_cast<uint32_t>(i));
            indices_.push_back(baseIndex + static_cast<uint32_t>(i + 1));
        }
    };

    for (const auto& cmd : commands) {
        if (cmd.type == GuiCommand::Type::ClipPush) {
            GuiCommand::RectData nextClip = IntersectRect(clipStack.back(), cmd.rect);
            clipStack.push_back(nextClip);
            continue;
        }
        if (cmd.type == GuiCommand::Type::ClipPop) {
            if (clipStack.size() > 1) {
                clipStack.pop_back();
            }
            continue;
        }

        const GuiCommand::RectData& activeClip = clipStack.back();
        if (!RectHasArea(activeClip)) {
            continue;
        }

        if (cmd.type == GuiCommand::Type::Rect) {
            AddQuad(cmd.rect, cmd.color, activeClip);
            if (cmd.borderWidth > 0.0f && cmd.borderColor.a > 0.0f) {
                float border = std::min(cmd.borderWidth, std::min(cmd.rect.width, cmd.rect.height));
                if (border > 0.0f) {
                    float innerHeight = std::max(0.0f, cmd.rect.height - border * 2.0f);
                    GuiCommand::RectData top{cmd.rect.x, cmd.rect.y, cmd.rect.width, border};
                    GuiCommand::RectData bottom{cmd.rect.x, cmd.rect.y + cmd.rect.height - border,
                                                cmd.rect.width, border};
                    GuiCommand::RectData left{cmd.rect.x, cmd.rect.y + border, border, innerHeight};
                    GuiCommand::RectData right{cmd.rect.x + cmd.rect.width - border,
                                               cmd.rect.y + border, border, innerHeight};
                    AddQuad(top, cmd.borderColor, activeClip);
                    AddQuad(bottom, cmd.borderColor, activeClip);
                    AddQuad(left, cmd.borderColor, activeClip);
                    AddQuad(right, cmd.borderColor, activeClip);
                }
            }
        } else if (cmd.type == GuiCommand::Type::Text) {
            if (cmd.text.empty()) {
                continue;
            }

            // Font metrics (8x8 bitmap font)
            const float charWidth = cmd.fontSize * 0.6f;  // Slightly wider for better appearance
            const float charHeight = cmd.fontSize;
            const float charSpacing = charWidth * 0.05f;  // Tighter spacing

            // Calculate text width for alignment
            float textWidth = cmd.text.size() * (charWidth + charSpacing);

            // Calculate text position based on alignment
            // Text commands use bounds field, not rect field
            GuiCommand::RectData textBounds = cmd.bounds;
            if (!cmd.hasBounds) {
                textBounds = {
                    cmd.rect.x,
                    cmd.rect.y,
                    cmd.fontSize * static_cast<float>(std::max<size_t>(1, cmd.text.size())),
                    cmd.fontSize
                };
            }

            float startX = textBounds.x;
            float startY = textBounds.y;

            if (cmd.alignX == "center") {
                startX += (textBounds.width - textWidth) * 0.5f;
            } else if (cmd.alignX == "right") {
                startX += textBounds.width - textWidth;
            }

            if (cmd.alignY == "center") {
                startY += (textBounds.height - charHeight) * 0.5f;
            } else if (cmd.alignY == "bottom") {
                startY += textBounds.height - charHeight;
            }

            GuiCommand::RectData textClip = activeClip;
            if (cmd.hasClipRect) {
                textClip = IntersectRect(textClip, cmd.clipRect);
            }
            if (!RectHasArea(textClip)) {
                continue;
            }

            if (fontConfig_.useFreeType && EnsureFreeTypeReady()) {
                RenderFreeTypeText(cmd, textClip, textBounds);
            } else {
                // Render text using 8x8 bitmap font
                float x = startX;
                for (char c : cmd.text) {
                    if (c >= 32 && c < 127) {
                        const uint8_t* glyph = font8x8_basic[static_cast<unsigned char>(c)];
                        for (int row = 0; row < 8; ++row) {
                            uint8_t rowData = glyph[row];
                            for (int col = 0; col < 8; ++col) {
                                if (rowData & (1 << col)) {
                                    const float pixelScale = 1.15f;
                                    float pixelWidth = (charWidth / 8.0f) * pixelScale;
                                    float pixelHeight = (charHeight / 8.0f) * pixelScale;
                                    float px = x + col * (charWidth / 8.0f) - (pixelWidth - charWidth / 8.0f) * 0.5f;
                                    float py = startY + row * (charHeight / 8.0f) - (pixelHeight - charHeight / 8.0f) * 0.5f;
                                    GuiCommand::RectData pixelRect{px, py, pixelWidth, pixelHeight};
                                    AddQuad(pixelRect, cmd.color, textClip);
                                }
                            }
                        }
                    }
                    x += charWidth + charSpacing;
                }
            }
        } else if (cmd.type == GuiCommand::Type::Svg) {
            if (cmd.svgPath.empty()) {
                continue;
            }
            const ParsedSvg* svg = LoadSvg(cmd.svgPath);
            if (!svg || svg->circles.empty() || svg->viewWidth <= 0.0f || svg->viewHeight <= 0.0f) {
                continue;
            }

            GuiCommand::RectData clippedTarget = IntersectRect(cmd.rect, activeClip);
            if (!RectHasArea(clippedTarget)) {
                continue;
            }

            float scaleX = clippedTarget.width / svg->viewWidth;
            float scaleY = clippedTarget.height / svg->viewHeight;
            float scale = std::min(scaleX, scaleY);

            for (const auto& circle : svg->circles) {
                float cx = clippedTarget.x + circle.cx * scaleX;
                float cy = clippedTarget.y + circle.cy * scaleY;
                float radius = circle.r * scale;
                if (radius <= 0.0f) {
                    continue;
                }

                GuiColor color = circle.color;
                if (cmd.svgTint.a > 0.0f) {
                    color.r *= cmd.svgTint.r;
                    color.g *= cmd.svgTint.g;
                    color.b *= cmd.svgTint.b;
                    color.a *= cmd.svgTint.a;
                }

                int segments = std::max(12, static_cast<int>(radius * 0.25f));
                const float twoPi = 6.283185307179586f;
                ClipPoint center{cx, cy};

                for (int i = 0; i < segments; ++i) {
                    float angle0 = twoPi * static_cast<float>(i) / static_cast<float>(segments);
                    float angle1 = twoPi * static_cast<float>(i + 1) / static_cast<float>(segments);
                    ClipPoint p0{cx + std::cos(angle0) * radius, cy + std::sin(angle0) * radius};
                    ClipPoint p1{cx + std::cos(angle1) * radius, cy + std::sin(angle1) * radius};
                    addClippedPolygon({center, p0, p1}, color, activeClip);
                }
            }
        }
    }
}

const std::vector<uint8_t>& GuiRenderer::LoadShaderBytes(const std::filesystem::path& path,
                                                         VkShaderStageFlagBits stage) {
    const std::string key = path.string();
    auto cached = shaderSpirvCache_.find(key);
    if (cached != shaderSpirvCache_.end()) {
        if (logger_) {
            logger_->Trace("GuiRenderer", "LoadShaderBytes",
                           "cacheHit=true, path=" + key +
                           ", bytes=" + std::to_string(cached->second.size()));
        }
        return cached->second;
    }

    std::vector<uint8_t> shaderBytes = ReadShaderFile(path, stage, logger_.get());
    auto inserted = shaderSpirvCache_.emplace(key, std::move(shaderBytes));
    if (logger_) {
        logger_->Trace("GuiRenderer", "LoadShaderBytes",
                       "cacheHit=false, path=" + key +
                       ", bytes=" + std::to_string(inserted.first->second.size()));
    }
    return inserted.first->second;
}

const std::vector<uint8_t>& GuiRenderer::LoadShaderBytes(const std::string& cacheKey,
                                                         const std::string& source,
                                                         VkShaderStageFlagBits stage) {
    auto cached = shaderSpirvCache_.find(cacheKey);
    if (cached != shaderSpirvCache_.end()) {
        if (logger_) {
            logger_->Trace("GuiRenderer", "LoadShaderBytes",
                           "cacheHit=true, key=" + cacheKey +
                           ", bytes=" + std::to_string(cached->second.size()));
        }
        return cached->second;
    }

    std::vector<uint8_t> shaderBytes = ReadShaderSource(source, stage, cacheKey, logger_.get());
    auto inserted = shaderSpirvCache_.emplace(cacheKey, std::move(shaderBytes));
    if (logger_) {
        logger_->Trace("GuiRenderer", "LoadShaderBytes",
                       "cacheHit=false, key=" + cacheKey +
                       ", bytes=" + std::to_string(inserted.first->second.size()));
    }
    return inserted.first->second;
}

void GuiRenderer::CreatePipeline(VkRenderPass renderPass, VkExtent2D extent) {
    // Load shader modules
    const std::string vertexLabel = "inline:gui_2d.vert";
    const std::string fragmentLabel = "inline:gui_2d.frag";
    if (logger_) {
        logger_->Trace("GuiRenderer", "CreatePipeline",
                       "renderPassIsNull=" + std::string(renderPass == VK_NULL_HANDLE ? "true" : "false") +
                       ", extent=" + std::to_string(extent.width) + "x" + std::to_string(extent.height) +
                       ", vertexShader=" + vertexLabel +
                       ", fragmentShader=" + fragmentLabel);
    }

    const auto& vertShaderCode = LoadShaderBytes(vertexLabel, kGuiVertexSource,
                                                 VK_SHADER_STAGE_VERTEX_BIT);
    const auto& fragShaderCode = LoadShaderBytes(fragmentLabel, kGuiFragmentSource,
                                                 VK_SHADER_STAGE_FRAGMENT_BIT);

    VkShaderModuleCreateInfo vertModuleInfo{};
    vertModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertModuleInfo.codeSize = vertShaderCode.size();
    vertModuleInfo.pCode = reinterpret_cast<const uint32_t*>(vertShaderCode.data());

    if (vkCreateShaderModule(device_, &vertModuleInfo, nullptr, &vertShaderModule_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create vertex shader module for GUI");
    }

    VkShaderModuleCreateInfo fragModuleInfo{};
    fragModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragModuleInfo.codeSize = fragShaderCode.size();
    fragModuleInfo.pCode = reinterpret_cast<const uint32_t*>(fragShaderCode.data());

    if (vkCreateShaderModule(device_, &fragModuleInfo, nullptr, &fragShaderModule_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create fragment shader module for GUI");
    }

    // Shader stages
    VkPipelineShaderStageCreateInfo vertStageInfo{};
    vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module = vertShaderModule_;
    vertStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragStageInfo{};
    fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module = fragShaderModule_;
    fragStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};

    // Vertex input - GuiVertex format
    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(GuiVertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 2> attributeDescs{};
    // Position (vec3)
    attributeDescs[0].binding = 0;
    attributeDescs[0].location = 0;
    attributeDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescs[0].offset = offsetof(GuiVertex, x);

    // Color (vec4)
    attributeDescs[1].binding = 0;
    attributeDescs[1].location = 1;
    attributeDescs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescs[1].offset = offsetof(GuiVertex, r);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescs.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescs.data();

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport and scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Rasterization
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;  // No culling for 2D GUI
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling - enable sample shading for smoother text
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_TRUE;  // Enable for smoother rendering
    multisampling.minSampleShading = 0.5f;  // Shade at least 50% of samples
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // **CRITICAL: Alpha blending for transparency**
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;  // Enable blending!
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Depth stencil - disable depth test for 2D GUI overlay
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // Push constants for transformation matrices
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 32;  // 2 mat4s

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create GUI pipeline layout");
    }

    // Create graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout_;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create GUI graphics pipeline");
    }
}

void GuiRenderer::CreateVertexAndIndexBuffers(size_t vertexCount, size_t indexCount) {
    // Clean up old buffers
    CleanupBuffers();

    if (vertexCount == 0 || indexCount == 0) {
        return;
    }

    // Create vertex buffer
    VkDeviceSize vertexBufferSize = sizeof(GuiVertex) * vertexCount;
    bufferService_->CreateBuffer(
        vertexBufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vertexBuffer_,
        vertexMemory_
    );

    // Upload vertex data
    void* vertexData;
    vkMapMemory(device_, vertexMemory_, 0, vertexBufferSize, 0, &vertexData);
    std::memcpy(vertexData, vertices_.data(), vertexBufferSize);
    vkUnmapMemory(device_, vertexMemory_);

    // Create index buffer
    VkDeviceSize indexBufferSize = sizeof(uint32_t) * indexCount;
    bufferService_->CreateBuffer(
        indexBufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        indexBuffer_,
        indexMemory_
    );

    // Upload index data
    void* indexData;
    vkMapMemory(device_, indexMemory_, 0, indexBufferSize, 0, &indexData);
    std::memcpy(indexData, indices_.data(), indexBufferSize);
    vkUnmapMemory(device_, indexMemory_);
}

void GuiRenderer::CleanupPipeline() {
    if (pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }
    if (pipelineLayout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
        pipelineLayout_ = VK_NULL_HANDLE;
    }
    if (vertShaderModule_ != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device_, vertShaderModule_, nullptr);
        vertShaderModule_ = VK_NULL_HANDLE;
    }
    if (fragShaderModule_ != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device_, fragShaderModule_, nullptr);
        fragShaderModule_ = VK_NULL_HANDLE;
    }
}

void GuiRenderer::CleanupBuffers() {
    if (vertexBuffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, vertexBuffer_, nullptr);
        vertexBuffer_ = VK_NULL_HANDLE;
    }
    if (vertexMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, vertexMemory_, nullptr);
        vertexMemory_ = VK_NULL_HANDLE;
    }
    if (indexBuffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, indexBuffer_, nullptr);
        indexBuffer_ = VK_NULL_HANDLE;
    }
    if (indexMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, indexMemory_, nullptr);
        indexMemory_ = VK_NULL_HANDLE;
    }
}

void GuiRenderer::UpdateFormat(VkFormat format) {
    if (swapchainFormat_ == format) {
        return;
    }
    swapchainFormat_ = format;
}

const ParsedSvg* GuiRenderer::LoadSvg(const std::string& relativePath) {
    auto it = svgCache_.find(relativePath);
    if (it != svgCache_.end()) {
        return &it->second;
    }
    std::filesystem::path path = scriptDirectory_ / relativePath;
    try {
        ParsedSvg parsed = ParseSvgFile(path);
        auto inserted = svgCache_.emplace(relativePath, std::move(parsed));
        return &inserted.first->second;
    } catch (...) {
        return nullptr;
    }
}

} // namespace sdl3cpp::services::impl
