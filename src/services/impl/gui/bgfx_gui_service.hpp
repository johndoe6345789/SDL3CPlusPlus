#pragma once

#include "services/interfaces/i_config_service.hpp"
#include "services/interfaces/i_gui_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_pipeline_compiler_service.hpp"
#include "../../../di/lifecycle.hpp"

#include <bgfx/bgfx.h>

#include "services/impl/materialx/materialx_shader_generator.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::services::impl {

class BgfxGuiService final : public IGuiService,
                             public di::IShutdownable {
public:
    BgfxGuiService(std::shared_ptr<IConfigService> configService,
                   std::shared_ptr<ILogger> logger,
                   std::shared_ptr<sdl3cpp::services::IPipelineCompilerService> pipelineCompiler = nullptr);
    ~BgfxGuiService() override;

    void PrepareFrame(const std::vector<GuiCommand>& commands,
                      uint32_t width,
                      uint32_t height) override;

    void Shutdown() noexcept override;

    bool IsProgramReady() const;
    bool IsWhiteTextureReady() const;

private:
    friend void AddTextCacheEntryForTest(BgfxGuiService& service,
                                         const std::string& text,
                                         int fontSize,
                                         uint64_t lastUsedFrame);
    friend void AddSvgCacheEntryForTest(BgfxGuiService& service,
                                        const std::string& path,
                                        int width,
                                        int height,
                                        uint64_t lastUsedFrame);
    friend size_t GetTextCacheSizeForTest(const BgfxGuiService& service);
    friend size_t GetSvgCacheSizeForTest(const BgfxGuiService& service);
    friend void PruneTextCacheForTest(BgfxGuiService& service);
    friend void PruneSvgCacheForTest(BgfxGuiService& service);

    struct GuiVertex {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;
        float u = 0.0f;
        float v = 0.0f;
    };

    struct ScissorRect {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
    };

    struct TextKey {
        std::string text;
        int fontSize = 0;

        bool operator==(const TextKey& other) const {
            return fontSize == other.fontSize && text == other.text;
        }
    };

    struct TextKeyHash {
        size_t operator()(const TextKey& key) const {
            return std::hash<std::string>{}(key.text) ^ (static_cast<size_t>(key.fontSize) << 1);
        }
    };

    struct TextTexture {
        bgfx::TextureHandle texture = BGFX_INVALID_HANDLE;
        int width = 0;
        int height = 0;
        int baseline = 0;
        int fontSize = 0;
        uint64_t lastUsedFrame = 0;
    };

    struct SvgKey {
        std::string path;
        int width = 0;
        int height = 0;

        bool operator==(const SvgKey& other) const {
            return width == other.width && height == other.height && path == other.path;
        }
    };

    struct SvgKeyHash {
        size_t operator()(const SvgKey& key) const {
            size_t seed = std::hash<std::string>{}(key.path);
            seed ^= (static_cast<size_t>(key.width) << 1);
            seed ^= (static_cast<size_t>(key.height) << 2);
            return seed;
        }
    };

    struct SvgTexture {
        bgfx::TextureHandle texture = BGFX_INVALID_HANDLE;
        int width = 0;
        int height = 0;
        uint64_t lastUsedFrame = 0;
    };

    struct FreeTypeState;

    void InitializeResources();
    void EnsureFontReady();
    void ApplyGuiView(uint32_t width, uint32_t height);

    std::filesystem::path ResolvePath(const std::filesystem::path& path) const;
    std::filesystem::path ResolveDefaultFontPath() const;

    std::optional<ScissorRect> IntersectScissor(const ScissorRect& a, const ScissorRect& b) const;
    std::optional<ScissorRect> CurrentScissor() const;
    std::optional<ScissorRect> BuildScissor(const std::optional<ScissorRect>& scoped) const;
    void SetScissor(const std::optional<ScissorRect>& scissor) const;

    void SubmitRect(const GuiCommand& command, const std::optional<ScissorRect>& scissor);
    void SubmitText(const GuiCommand& command, const std::optional<ScissorRect>& scissor);
    void SubmitSvg(const GuiCommand& command, const std::optional<ScissorRect>& scissor);

    void SubmitQuad(const GuiVertex& v0,
                    const GuiVertex& v1,
                    const GuiVertex& v2,
                    const GuiVertex& v3,
                    bgfx::TextureHandle texture,
                    const std::optional<ScissorRect>& scissor);

    const TextTexture* GetTextTexture(const std::string& text, int fontSize);
    const SvgTexture* GetSvgTexture(const std::string& path, int width, int height);
    void PruneTextCache();
    void PruneSvgCache();

    bgfx::TextureHandle CreateTexture(const uint8_t* rgba,
                                      uint32_t width,
                                      uint32_t height,
                                      uint64_t flags) const;
    bgfx::ProgramHandle CreateProgram(const char* vertexSource, const char* fragmentSource);
    bgfx::ShaderHandle CreateShader(const std::string& label, const std::string& source, bool isVertex) const;
    void ResolveGuiMatrixUniform(bgfx::ShaderHandle shader);

    std::shared_ptr<IConfigService> configService_;
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<sdl3cpp::services::IPipelineCompilerService> pipelineCompiler_;
    MaterialXShaderGenerator materialxGenerator_;

    std::unique_ptr<FreeTypeState> freeType_;
    std::unordered_map<TextKey, TextTexture, TextKeyHash> textCache_;
    std::unordered_map<SvgKey, SvgTexture, SvgKeyHash> svgCache_;

    bgfx::ProgramHandle program_ = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle sampler_ = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle modelViewProjUniform_ = BGFX_INVALID_HANDLE;  // For built-in shader
    bgfx::UniformHandle worldMatrixUniform_ = BGFX_INVALID_HANDLE;   // For MaterialX shaders
    bgfx::UniformHandle viewProjMatrixUniform_ = BGFX_INVALID_HANDLE; // For MaterialX shaders
    bgfx::TextureHandle whiteTexture_ = BGFX_INVALID_HANDLE;
    bgfx::VertexLayout layout_;
    std::string guiVertexSourceOverride_;
    std::string guiFragmentSourceOverride_;

    std::vector<ScissorRect> scissorStack_;
    std::array<float, 16> viewProjection_{};
    int defaultFontSize_ = 16;
    uint32_t frameWidth_ = 0;
    uint32_t frameHeight_ = 0;
    uint16_t viewId_ = 1;
    bool initialized_ = false;
    bool loggedMissingResources_ = false;
    bool usesMaterialXShaders_ = false;
    bool usesPredefinedModelViewProj_ = false;
    uint64_t frameIndex_ = 0;
    size_t maxTextCacheEntries_ = 256;
    size_t maxSvgCacheEntries_ = 64;
    uint32_t maxTextureDim_ = 0;
};

}  // namespace sdl3cpp::services::impl
