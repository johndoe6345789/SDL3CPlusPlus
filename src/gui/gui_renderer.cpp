
#include "gui/gui_renderer.hpp"

#include "app/vulkan_api.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../third_party/font8x8_basic.h"

namespace script = sdl3cpp::script;
namespace vulkan = sdl3cpp::app::vulkan;

namespace sdl3cpp::gui {
namespace {

using ParsedSvg = sdl3cpp::gui::ParsedSvg;
using SvgCircle = sdl3cpp::gui::SvgCircle;

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

script::GuiColor ParseColorString(const std::string& text, const script::GuiColor& fallback) {
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

script::CubeScript::GuiCommand::RectData IntersectRect(const script::CubeScript::GuiCommand::RectData& a,
                                                     const script::CubeScript::GuiCommand::RectData& b) {
    script::CubeScript::GuiCommand::RectData result;
    result.x = std::max(a.x, b.x);
    result.y = std::max(a.y, b.y);
    float right = std::min(a.x + a.width, b.x + b.width);
    float bottom = std::min(a.y + a.height, b.y + b.height);
    result.width = std::max(0.0f, right - result.x);
    result.height = std::max(0.0f, bottom - result.y);
    return result;
}

int ClampToRange(int value, int minimum, int maximum) {
    return std::min(std::max(value, minimum), maximum);
}

} // namespace

class GuiRenderer::Canvas {
public:
    using RectData = script::CubeScript::GuiCommand::RectData;

    void Resize(uint32_t width, uint32_t height) {
        width_ = width;
        height_ = height;
        pixels_.assign(static_cast<size_t>(width_) * static_cast<size_t>(height_) * 4, 0);
        clipStack_.clear();
        clipStack_.push_back({0.0f, 0.0f, static_cast<float>(width_), static_cast<float>(height_)});
    }

    void Clear() {
        std::fill(pixels_.begin(), pixels_.end(), 0);
        clipStack_.clear();
        clipStack_.push_back({0.0f, 0.0f, static_cast<float>(width_), static_cast<float>(height_)});
    }

    void PushClip(const RectData& rect) {
        clipStack_.push_back(rect);
    }

    void PopClip() {
        if (clipStack_.size() > 1) {
            clipStack_.pop_back();
        }
    }

    void FillRect(const RectData& rect, const script::GuiColor& fillColor,
                  const script::GuiColor& borderColor, float borderWidth) {
        DrawFilledRect(rect, fillColor);
        if (borderWidth > 0.0f && borderColor.a > 0.0f) {
            DrawFilledRect({rect.x, rect.y, rect.width, borderWidth}, borderColor);
            DrawFilledRect({rect.x, rect.y + rect.height - borderWidth, rect.width, borderWidth}, borderColor);
            DrawFilledRect({rect.x, rect.y + borderWidth, borderWidth, rect.height - borderWidth * 2.0f}, borderColor);
            DrawFilledRect({rect.x + rect.width - borderWidth, rect.y + borderWidth, borderWidth,
                            rect.height - borderWidth * 2.0f}, borderColor);
        }
    }

    void DrawText(const std::string& text, const script::GuiColor& color, const RectData& bounds,
                  const std::string& alignX, const std::string& alignY, float fontSize) {
        if (text.empty() || width_ == 0 || height_ == 0) {
            return;
        }
        float scale = std::max(1.0f, fontSize / 8.0f);
        float glyphWidth = 8.0f * scale;
        float glyphHeight = 8.0f * scale;
        float textWidth = glyphWidth * static_cast<float>(text.size());
        float x = bounds.x;
        if (alignX == "center") {
            x += (bounds.width - textWidth) * 0.5f;
        } else if (alignX == "right") {
            x += bounds.width - textWidth;
        }
        float y = bounds.y;
        if (alignY == "center") {
            y += (bounds.height - glyphHeight) * 0.5f;
        } else if (alignY == "bottom") {
            y += bounds.height - glyphHeight;
        }
        for (size_t i = 0; i < text.size(); ++i) {
            unsigned char code = static_cast<unsigned char>(text[i]);
            if (code >= 128) {
                continue;
            }
            float glyphX = x + glyphWidth * static_cast<float>(i);
            for (int row = 0; row < 8; ++row) {
                uint8_t pattern = static_cast<uint8_t>(font8x8_basic[code][row]);
                for (int col = 0; col < 8; ++col) {
                    if ((pattern & (1 << col)) == 0) {
                        continue;
                    }
                    RectData pixelRect{
                        glyphX + static_cast<float>(col) * scale,
                        y + static_cast<float>(row) * scale,
                        scale,
                        scale,
                    };
                    DrawFilledRect(pixelRect, color);
                }
            }
        }
    }

    void DrawSvg(const ParsedSvg& svg, const RectData& target, const script::GuiColor& tint) {
        if (svg.circles.empty() || svg.viewWidth <= 0.0f || svg.viewHeight <= 0.0f || width_ == 0 ||
            height_ == 0) {
            return;
        }
        RectData clipped = ClipRect(target);
        if (clipped.width <= 0.0f || clipped.height <= 0.0f) {
            return;
        }
        float scaleX = clipped.width / svg.viewWidth;
        float scaleY = clipped.height / svg.viewHeight;
        float scale = std::min(scaleX, scaleY);
        for (const auto& circle : svg.circles) {
            float cx = clipped.x + circle.cx * scaleX;
            float cy = clipped.y + circle.cy * scaleY;
            float radius = circle.r * scale;
            script::GuiColor color = circle.color;
            if (tint.a > 0.0f) {
                color.r *= tint.r;
                color.g *= tint.g;
                color.b *= tint.b;
                color.a *= tint.a;
            }
            int yStart = ClampToRange(static_cast<int>(std::floor(cy - radius)), 0, static_cast<int>(height_));
            int yEnd = ClampToRange(static_cast<int>(std::ceil(cy + radius)), 0, static_cast<int>(height_));
            for (int row = yStart; row < yEnd; ++row) {
                float dy = (static_cast<float>(row) + 0.5f) - cy;
                float horizontalSpan = radius * radius - dy * dy;
                if (horizontalSpan <= 0.0f) {
                    continue;
                }
                float span = std::sqrt(horizontalSpan);
                RectData slice{
                    cx - span,
                    static_cast<float>(row),
                    2.0f * span,
                    1.0f,
                };
                DrawFilledRect(slice, color);
            }
        }
    }

    const std::vector<uint8_t>& Pixels() const {
        return pixels_;
    }

private:
    RectData ClipRect(const RectData& rect) const {
        RectData clipped = rect;
        for (const auto& entry : clipStack_) {
            clipped = IntersectRect(clipped, entry);
        }
        return clipped;
    }

    void DrawFilledRect(const RectData& rect, const script::GuiColor& color) {
        if (rect.width <= 0.0f || rect.height <= 0.0f) {
            return;
        }
        RectData clipped = ClipRect(rect);
        if (clipped.width <= 0.0f || clipped.height <= 0.0f) {
            return;
        }
        int startX = ClampToRange(static_cast<int>(std::floor(clipped.x)), 0, static_cast<int>(width_));
        int startY = ClampToRange(static_cast<int>(std::floor(clipped.y)), 0, static_cast<int>(height_));
        int endX = ClampToRange(static_cast<int>(std::ceil(clipped.x + clipped.width)), 0, static_cast<int>(width_));
        int endY = ClampToRange(static_cast<int>(std::ceil(clipped.y + clipped.height)), 0, static_cast<int>(height_));
        for (int y = startY; y < endY; ++y) {
            for (int x = startX; x < endX; ++x) {
                BlendPixel(x, y, color);
            }
        }
    }

    void BlendPixel(int x, int y, const script::GuiColor& color) {
        size_t index = (static_cast<size_t>(y) * width_ + static_cast<size_t>(x)) * 4;
        auto clampByte = [](float value) -> uint8_t {
            return static_cast<uint8_t>(std::clamp(value, 0.0f, 1.0f) * 255.0f);
        };
        float destR = pixels_[index] / 255.0f;
        float destG = pixels_[index + 1] / 255.0f;
        float destB = pixels_[index + 2] / 255.0f;
        float destA = pixels_[index + 3] / 255.0f;
        float srcA = std::clamp(color.a, 0.0f, 1.0f);
        float invSrc = 1.0f - srcA;
        float outR = color.r * srcA + destR * invSrc;
        float outG = color.g * srcA + destG * invSrc;
        float outB = color.b * srcA + destB * invSrc;
        float outA = srcA + destA * invSrc;
        pixels_[index] = clampByte(outR);
        pixels_[index + 1] = clampByte(outG);
        pixels_[index + 2] = clampByte(outB);
        pixels_[index + 3] = clampByte(outA);
    }

    uint32_t width_ = 0;
    uint32_t height_ = 0;
    std::vector<uint8_t> pixels_;
    std::vector<RectData> clipStack_;
};

GuiRenderer::GuiRenderer(VkDevice device, VkPhysicalDevice physicalDevice, VkFormat swapchainFormat,
                         const std::filesystem::path& scriptDirectory)
    : device_(device),
      physicalDevice_(physicalDevice),
      swapchainFormat_(swapchainFormat),
      scriptDirectory_(scriptDirectory),
      canvas_(std::make_unique<Canvas>()) {}

    GuiRenderer::~GuiRenderer() {
        DestroyStagingBuffer();
    }

    bool GuiRenderer::IsReady() const {
        return canvasWidth_ > 0 && canvasHeight_ > 0 && stagingBuffer_ != VK_NULL_HANDLE;
    }

    void GuiRenderer::Prepare(const std::vector<script::CubeScript::GuiCommand>& commands, uint32_t width,
                              uint32_t height) {
        if (width == 0 || height == 0 || !canvas_) {
            return;
        }
        EnsureCanvas(width, height);
        canvas_->Clear();
        for (const auto& command : commands) {
            switch (command.type) {
                case script::CubeScript::GuiCommand::Type::Rect:
                    canvas_->FillRect(command.rect, command.color, command.borderColor, command.borderWidth);
                    break;
                case script::CubeScript::GuiCommand::Type::Text: {
                    if (command.hasClipRect) {
                        canvas_->PushClip(command.clipRect);
                    }
                    if (command.hasBounds) {
                        canvas_->DrawText(command.text, command.color, command.bounds, command.alignX,
                                          command.alignY, command.fontSize);
                    } else {
                        script::CubeScript::GuiCommand::RectData fallback{
                            command.rect.x, command.rect.y,
                            command.fontSize * static_cast<float>(std::max<size_t>(1, command.text.size())), command.fontSize};
                        canvas_->DrawText(command.text, command.color, fallback, command.alignX,
                                          command.alignY, command.fontSize);
                    }
                    if (command.hasClipRect) {
                        canvas_->PopClip();
                    }
                    break;
                }
                case script::CubeScript::GuiCommand::Type::ClipPush:
                    canvas_->PushClip(command.rect);
                    break;
                case script::CubeScript::GuiCommand::Type::ClipPop:
                    canvas_->PopClip();
                    break;
                case script::CubeScript::GuiCommand::Type::Svg:
                    if (command.svgPath.empty()) {
                        break;
                    }
                    if (const ParsedSvg* svg = LoadSvg(command.svgPath)) {
                        canvas_->DrawSvg(*svg, command.rect, command.svgTint);
                    }
                    break;
            }
        }
        UpdateStagingBuffer();
    }

    void GuiRenderer::BlitToSwapchain(VkCommandBuffer commandBuffer, VkImage image) {
        if (!IsReady()) {
            return;
        }
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {canvasWidth_, canvasHeight_, 1};

        vkCmdCopyBufferToImage(commandBuffer, stagingBuffer_, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &region);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = 0;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &barrier);
    }

    void GuiRenderer::Resize(uint32_t width, uint32_t height, VkFormat format) {
        if (width == canvasWidth_ && height == canvasHeight_ && format == swapchainFormat_) {
            return;
        }
        UpdateFormat(format);
        EnsureCanvas(width, height);
    }

    void GuiRenderer::EnsureCanvas(uint32_t width, uint32_t height) {
        if (width == canvasWidth_ && height == canvasHeight_) {
            return;
        }
        canvasWidth_ = width;
        canvasHeight_ = height;
        if (canvas_) {
            canvas_->Resize(width, height);
        }
        size_t bufferSize = static_cast<size_t>(canvasWidth_) * canvasHeight_ * 4;
        CreateStagingBuffer(bufferSize);
    }

    void GuiRenderer::UpdateStagingBuffer() {
        if (!stagingMapped_ || !canvas_) {
            return;
        }
        const auto& pixels = canvas_->Pixels();
        size_t pixelCount = static_cast<size_t>(canvasWidth_) * canvasHeight_;
        uint8_t* dest = reinterpret_cast<uint8_t*>(stagingMapped_);
        for (size_t i = 0; i < pixelCount; ++i) {
            size_t offset = i * 4;
            uint8_t r = pixels[offset];
            uint8_t g = pixels[offset + 1];
            uint8_t b = pixels[offset + 2];
            uint8_t a = pixels[offset + 3];
            switch (swapchainFormat_) {
                case VK_FORMAT_B8G8R8A8_UNORM:
                case VK_FORMAT_B8G8R8A8_SRGB:
                    dest[offset] = b;
                    dest[offset + 1] = g;
                    dest[offset + 2] = r;
                    dest[offset + 3] = a;
                    break;
                case VK_FORMAT_R8G8B8A8_UNORM:
                case VK_FORMAT_R8G8B8A8_SRGB:
                default:
                    dest[offset] = r;
                    dest[offset + 1] = g;
                    dest[offset + 2] = b;
                    dest[offset + 3] = a;
                    break;
            }
        }
    }

    void GuiRenderer::CreateStagingBuffer(size_t size) {
        DestroyStagingBuffer();
        if (size == 0) {
            return;
        }
        vulkan::CreateBuffer(device_, physicalDevice_, static_cast<VkDeviceSize>(size),
                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             stagingBuffer_, stagingMemory_);
        stagingSize_ = size;
        vkMapMemory(device_, stagingMemory_, 0, stagingSize_, 0, &stagingMapped_);
    }

    void GuiRenderer::DestroyStagingBuffer() {
        if (stagingMapped_) {
            vkUnmapMemory(device_, stagingMemory_);
            stagingMapped_ = nullptr;
        }
        if (stagingBuffer_ != VK_NULL_HANDLE) {
            vkDestroyBuffer(device_, stagingBuffer_, nullptr);
            stagingBuffer_ = VK_NULL_HANDLE;
        }
        if (stagingMemory_ != VK_NULL_HANDLE) {
            vkFreeMemory(device_, stagingMemory_, nullptr);
            stagingMemory_ = VK_NULL_HANDLE;
        }
        stagingSize_ = 0;
    }

    void GuiRenderer::UpdateFormat(VkFormat format) {
        if (swapchainFormat_ == format) {
            return;
        }
        swapchainFormat_ = format;
        DestroyStagingBuffer();
        if (canvasWidth_ > 0 && canvasHeight_ > 0) {
            size_t bufferSize = static_cast<size_t>(canvasWidth_) * canvasHeight_ * 4;
            CreateStagingBuffer(bufferSize);
        }
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

} // namespace sdl3cpp::gui
