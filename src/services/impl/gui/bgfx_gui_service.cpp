#include "bgfx_gui_service.hpp"
#include "services/impl/graphics/bgfx_shader_compiler.hpp"

#include "services/interfaces/config_types.hpp"
#include "services/interfaces/gui_types.hpp"

#include <bgfx/bgfx.h>
#include <bx/math.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <lunasvg/lunasvg.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <numeric>
#include <optional>

namespace sdl3cpp::services::impl {

namespace {

constexpr uint64_t kGuiSamplerFlags = BGFX_SAMPLER_U_CLAMP |
                                      BGFX_SAMPLER_V_CLAMP;

const char* RendererTypeName(bgfx::RendererType::Enum type) {
    switch (type) {
        case bgfx::RendererType::Vulkan:
            return "Vulkan";
        case bgfx::RendererType::OpenGL:
            return "OpenGL";
        case bgfx::RendererType::OpenGLES:
            return "OpenGLES";
        case bgfx::RendererType::Direct3D11:
            return "Direct3D11";
        case bgfx::RendererType::Direct3D12:
            return "Direct3D12";
        case bgfx::RendererType::Metal:
            return "Metal";
        case bgfx::RendererType::Noop:
            return "Noop";
        case bgfx::RendererType::Count:
            return "Auto";
        default:
            return "Unknown";
    }
}

const char* kGuiVertexSource = R"(
#version 450

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec4 a_color0;
layout (location = 2) in vec2 a_texcoord0;

out VertexData
{
    layout (location = 0) vec4 color;
    layout (location = 1) vec2 texcoord;
} vd;

layout (binding = 0) uniform UniformBuffer {
    mat4 u_modelViewProj;
};

void main()
{
    vd.color = a_color0;
    vd.texcoord = a_texcoord0;
    gl_Position = u_modelViewProj * vec4(a_position, 1.0);
}
)";

const char* kGuiFragmentSource = R"(
#version 450

in VertexData
{
    layout (location = 0) vec4 color;
    layout (location = 1) vec2 texcoord;
} vd;

layout (location = 0) out vec4 out_color;

layout (binding = 1) uniform sampler2D s_tex;

void main()
{
    out_color = vd.color * texture(s_tex, vd.texcoord);
}
)";

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

float Clamp01(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

}  // namespace

struct BgfxGuiService::FreeTypeState {
    FT_Library library = nullptr;
    FT_Face face = nullptr;
    std::filesystem::path fontPath;
    bool ready = false;
};

BgfxGuiService::BgfxGuiService(std::shared_ptr<IConfigService> configService,
                               std::shared_ptr<ILogger> logger,
                               std::shared_ptr<sdl3cpp::services::IPipelineCompilerService> pipelineCompiler)
    : configService_(std::move(configService)),
      logger_(std::move(logger)),
      pipelineCompiler_(std::move(pipelineCompiler)),
      materialxGenerator_(logger_),
      freeType_(std::make_unique<FreeTypeState>()) {
    if (logger_) {
        logger_->Trace("BgfxGuiService", "BgfxGuiService",
                       "configService=" + std::string(configService_ ? "set" : "null") +
                       ", pipelineCompiler=" + std::string(pipelineCompiler_ ? "set" : "null"));
    }
    if (configService_) {
        const auto& budgets = configService_->GetRenderBudgetConfig();
        maxTextCacheEntries_ = budgets.guiTextCacheEntries;
        maxSvgCacheEntries_ = budgets.guiSvgCacheEntries;
        maxTextureDim_ = budgets.maxTextureDim;
        if (logger_) {
            logger_->Trace("BgfxGuiService", "BgfxGuiService",
                           "guiTextCacheEntries=" + std::to_string(maxTextCacheEntries_) +
                           ", guiSvgCacheEntries=" + std::to_string(maxSvgCacheEntries_) +
                           ", maxTextureDim=" + std::to_string(maxTextureDim_));
        }
    }
}

BgfxGuiService::~BgfxGuiService() {
    if (initialized_) {
        Shutdown();
    }
}

void BgfxGuiService::PrepareFrame(const std::vector<GuiCommand>& commands,
                                  uint32_t width,
                                  uint32_t height) {
    if (!initialized_) {
        InitializeResources();
    }

    if (!bgfx::isValid(program_) || !bgfx::isValid(whiteTexture_)) {
        if (logger_ && !loggedMissingResources_) {
            logger_->Warn("BgfxGuiService::PrepareFrame: GUI resources not initialized");
        }
        loggedMissingResources_ = true;
        return;
    }
    if (loggedMissingResources_ && logger_) {
        logger_->Trace("BgfxGuiService", "PrepareFrame", "GUI resources recovered");
    }
    loggedMissingResources_ = false;

    ApplyGuiView(width, height);
    scissorStack_.clear();
    ++frameIndex_;

    for (const auto& command : commands) {
        switch (command.type) {
            case GuiCommand::Type::ClipPush: {
                ScissorRect incoming{command.rect.x, command.rect.y, command.rect.width, command.rect.height};
                auto current = CurrentScissor();
                if (current) {
                    auto merged = IntersectScissor(*current, incoming);
                    if (merged) {
                        scissorStack_.push_back(*merged);
                    } else {
                        scissorStack_.push_back(ScissorRect{0.0f, 0.0f, 0.0f, 0.0f});
                    }
                } else {
                    scissorStack_.push_back(incoming);
                }
                break;
            }
            case GuiCommand::Type::ClipPop: {
                if (!scissorStack_.empty()) {
                    scissorStack_.pop_back();
                }
                break;
            }
            case GuiCommand::Type::Rect: {
                SubmitRect(command, BuildScissor(std::nullopt));
                break;
            }
            case GuiCommand::Type::Text: {
                std::optional<ScissorRect> scoped;
                if (command.hasClipRect) {
                    scoped = ScissorRect{command.clipRect.x, command.clipRect.y,
                                         command.clipRect.width, command.clipRect.height};
                }
                SubmitText(command, BuildScissor(scoped));
                break;
            }
            case GuiCommand::Type::Svg: {
                SubmitSvg(command, BuildScissor(std::nullopt));
                break;
            }
            default:
                break;
        }
    }

    PruneTextCache();
    PruneSvgCache();
}

void BgfxGuiService::Shutdown() noexcept {
    if (logger_) {
        logger_->Trace("BgfxGuiService", "Shutdown");
    }

    for (auto& [key, entry] : textCache_) {
        if (bgfx::isValid(entry.texture)) {
            bgfx::destroy(entry.texture);
        }
    }
    textCache_.clear();

    for (auto& [key, entry] : svgCache_) {
        if (bgfx::isValid(entry.texture)) {
            bgfx::destroy(entry.texture);
        }
    }
    svgCache_.clear();

    if (bgfx::isValid(whiteTexture_)) {
        bgfx::destroy(whiteTexture_);
        whiteTexture_ = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(program_)) {
        bgfx::destroy(program_);
        program_ = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(sampler_)) {
        bgfx::destroy(sampler_);
        sampler_ = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(modelViewProjUniform_)) {
        bgfx::destroy(modelViewProjUniform_);
        modelViewProjUniform_ = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(worldMatrixUniform_)) {
        bgfx::destroy(worldMatrixUniform_);
        worldMatrixUniform_ = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(viewProjMatrixUniform_)) {
        bgfx::destroy(viewProjMatrixUniform_);
        viewProjMatrixUniform_ = BGFX_INVALID_HANDLE;
    }

    if (freeType_) {
        if (freeType_->face) {
            FT_Done_Face(freeType_->face);
            freeType_->face = nullptr;
        }
        if (freeType_->library) {
            FT_Done_FreeType(freeType_->library);
            freeType_->library = nullptr;
        }
        freeType_->ready = false;
    }

    initialized_ = false;
}

bool BgfxGuiService::IsProgramReady() const {
    return bgfx::isValid(program_);
}

bool BgfxGuiService::IsWhiteTextureReady() const {
    return bgfx::isValid(whiteTexture_);
}

void BgfxGuiService::InitializeResources() {
    if (initialized_) {
        return;
    }

    if (logger_) {
        logger_->Trace("BgfxGuiService", "InitializeResources", "Creating GUI shader uniforms");
    }

    layout_.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();

    sampler_ = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);
    
    const char* vertexSource = kGuiVertexSource;
    const char* fragmentSource = kGuiFragmentSource;
    guiVertexSourceOverride_.clear();
    guiFragmentSourceOverride_.clear();
    bool usingMaterialX = false;

    if (configService_) {
        const auto& materialConfig = configService_->GetMaterialXConfig();
        if (materialConfig.enabled && materialConfig.shaderKey == "gui") {
            try {
                ShaderPaths generated = materialxGenerator_.Generate(materialConfig, {});
                if (!generated.vertexSource.empty() && !generated.fragmentSource.empty()) {
                    guiVertexSourceOverride_ = std::move(generated.vertexSource);
                    guiFragmentSourceOverride_ = std::move(generated.fragmentSource);
                    vertexSource = guiVertexSourceOverride_.c_str();
                    fragmentSource = guiFragmentSourceOverride_.c_str();
                    usingMaterialX = true;
                    if (logger_) {
                        logger_->Trace("BgfxGuiService", "InitializeResources",
                                       "Using MaterialX GUI shaders");
                    }
                } else if (logger_) {
                    logger_->Warn("BgfxGuiService::InitializeResources: MaterialX GUI shaders were empty; falling back");
                }
            } catch (const std::exception& ex) {
                if (logger_) {
                    logger_->Warn("BgfxGuiService::InitializeResources: MaterialX GUI shader generation failed: " +
                                  std::string(ex.what()));
                }
            }
        }
    }

    usesMaterialXShaders_ = usingMaterialX;
    usesPredefinedModelViewProj_ = false;
    modelViewProjUniform_ = BGFX_INVALID_HANDLE;

    // Create uniforms matching the shader we're using
    if (usesMaterialXShaders_) {
        // MaterialX shaders use separate world and viewProjection matrices
        worldMatrixUniform_ = bgfx::createUniform("u_worldMatrix", bgfx::UniformType::Mat4);
        viewProjMatrixUniform_ = bgfx::createUniform("u_viewProjectionMatrix", bgfx::UniformType::Mat4);
        if (logger_) {
            logger_->Trace("BgfxGuiService", "InitializeResources",
                           "MaterialX uniforms: world=" + std::to_string(bgfx::isValid(worldMatrixUniform_)) +
                           ", viewProj=" + std::to_string(bgfx::isValid(viewProjMatrixUniform_)) +
                           ", sampler=" + std::to_string(bgfx::isValid(sampler_)));
        }
    } else {
        modelViewProjUniform_ = BGFX_INVALID_HANDLE;
        usesPredefinedModelViewProj_ = false;
    }

    if (logger_) {
        logger_->Trace("BgfxGuiService", "InitializeResources",
                       "GUI shader mode=" + std::string(usesMaterialXShaders_ ? "materialx" : "builtin"));
    }

    program_ = CreateProgram(vertexSource, fragmentSource);

    const uint32_t whitePixel = 0xffffffff;
    whiteTexture_ = CreateTexture(reinterpret_cast<const uint8_t*>(&whitePixel), 1, 1, kGuiSamplerFlags);
    
    if (logger_) {
        logger_->Trace("BgfxGuiService", "InitializeResources",
                       "Resources created: program=" + std::to_string(bgfx::isValid(program_)) +
                       ", whiteTexture=" + std::to_string(bgfx::isValid(whiteTexture_)));
    }

    if (!bgfx::isValid(program_) && logger_) {
        logger_->Error("BgfxGuiService::InitializeResources: Failed to create GUI shader program");
    }

    EnsureFontReady();
    initialized_ = true;
}

void BgfxGuiService::EnsureFontReady() {
    if (!freeType_ || freeType_->ready) {
        return;
    }

    GuiFontConfig fontConfig{};
    if (configService_) {
        fontConfig = configService_->GetGuiFontConfig();
    }

    if (!fontConfig.useFreeType) {
        if (logger_) {
            logger_->Warn("BgfxGuiService::EnsureFontReady: use_freetype disabled; GUI text disabled");
        }
        return;
    }

    defaultFontSize_ = fontConfig.fontSize > 0.0f
        ? static_cast<int>(std::lround(fontConfig.fontSize))
        : defaultFontSize_;

    std::filesystem::path fontPath = fontConfig.fontPath;
    if (fontPath.empty()) {
        fontPath = ResolveDefaultFontPath();
    }
    fontPath = ResolvePath(fontPath);

    if (fontPath.empty() || !std::filesystem::exists(fontPath)) {
        if (logger_) {
            logger_->Warn("BgfxGuiService::EnsureFontReady: font path missing; GUI text disabled");
        }
        return;
    }

    if (FT_Init_FreeType(&freeType_->library) != 0) {
        if (logger_) {
            logger_->Error("BgfxGuiService::EnsureFontReady: FreeType init failed");
        }
        return;
    }

    if (FT_New_Face(freeType_->library, fontPath.string().c_str(), 0, &freeType_->face) != 0) {
        if (logger_) {
            logger_->Error("BgfxGuiService::EnsureFontReady: Failed to load font " + fontPath.string());
        }
        FT_Done_FreeType(freeType_->library);
        freeType_->library = nullptr;
        return;
    }

    freeType_->fontPath = fontPath;
    freeType_->ready = true;

    if (logger_) {
        logger_->Trace("BgfxGuiService", "EnsureFontReady",
                       "fontPath=" + fontPath.string() +
                       ", defaultSize=" + std::to_string(defaultFontSize_));
    }
}

void BgfxGuiService::ApplyGuiView(uint32_t width, uint32_t height) {
    const uint32_t previousWidth = frameWidth_;
    const uint32_t previousHeight = frameHeight_;
    frameWidth_ = width;
    frameHeight_ = height;

    float view[16];
    float proj[16];
    bx::mtxIdentity(view);
    const bool homogeneousDepth = bgfx::getCaps() && bgfx::getCaps()->homogeneousDepth;
    bx::mtxOrtho(proj,
                 0.0f,
                 static_cast<float>(width),
                 static_cast<float>(height),
                 0.0f,
                 0.0f,
                 100.0f,
                 0.0f,
                 homogeneousDepth);
    std::copy(std::begin(proj), std::end(proj), viewProjection_.begin());

    if (logger_ && (previousWidth != width || previousHeight != height)) {
        logger_->Trace("BgfxGuiService", "ApplyGuiView",
                       "viewport=" + std::to_string(width) + "x" + std::to_string(height));
        logger_->Trace("BgfxGuiService", "ApplyGuiView",
                       "projection[0-3]=[" + std::to_string(proj[0]) + "," +
                       std::to_string(proj[1]) + "," +
                       std::to_string(proj[2]) + "," +
                       std::to_string(proj[3]) + "]");
        logger_->Trace("BgfxGuiService", "ApplyGuiView",
                       "projection[12-15]=[" + std::to_string(proj[12]) + "," +
                       std::to_string(proj[13]) + "," +
                       std::to_string(proj[14]) + "," +
                       std::to_string(proj[15]) + "]");
    }

    bgfx::setViewTransform(viewId_, view, proj);
    bgfx::setViewRect(viewId_, 0, 0,
                      static_cast<uint16_t>(std::min<uint32_t>(width, 0xffff)),
                      static_cast<uint16_t>(std::min<uint32_t>(height, 0xffff)));
    bgfx::touch(viewId_);
}

std::filesystem::path BgfxGuiService::ResolvePath(const std::filesystem::path& path) const {
    if (path.empty() || path.is_absolute()) {
        return path;
    }

    std::vector<std::filesystem::path> roots;
    if (configService_) {
        auto projectRoot = configService_->GetProjectRoot();
        if (!projectRoot.empty()) {
            roots.push_back(projectRoot / "scripts");
            roots.push_back(projectRoot);
        }
    }
    roots.push_back(std::filesystem::current_path());

    for (const auto& root : roots) {
        auto candidate = root / path;
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return path;
}

std::filesystem::path BgfxGuiService::ResolveDefaultFontPath() const {
    std::vector<std::filesystem::path> candidates;
    if (configService_) {
        auto projectRoot = configService_->GetProjectRoot();
        if (!projectRoot.empty()) {
            candidates.push_back(projectRoot / "assets" / "fonts" / "Roboto-Regular.ttf");
            candidates.push_back(projectRoot / "scripts" / "assets" / "fonts" / "Roboto-Regular.ttf");
        }
    }
    candidates.push_back(std::filesystem::current_path() / "scripts" / "assets" / "fonts" / "Roboto-Regular.ttf");

    for (const auto& candidate : candidates) {
        if (!candidate.empty() && std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return {};
}

std::optional<BgfxGuiService::ScissorRect> BgfxGuiService::IntersectScissor(const ScissorRect& a,
                                                                            const ScissorRect& b) const {
    float x0 = std::max(a.x, b.x);
    float y0 = std::max(a.y, b.y);
    float x1 = std::min(a.x + a.width, b.x + b.width);
    float y1 = std::min(a.y + a.height, b.y + b.height);
    if (x1 <= x0 || y1 <= y0) {
        return std::nullopt;
    }
    return ScissorRect{x0, y0, x1 - x0, y1 - y0};
}

std::optional<BgfxGuiService::ScissorRect> BgfxGuiService::CurrentScissor() const {
    if (scissorStack_.empty()) {
        return std::nullopt;
    }
    return scissorStack_.back();
}

std::optional<BgfxGuiService::ScissorRect> BgfxGuiService::BuildScissor(
    const std::optional<ScissorRect>& scoped) const {
    auto current = CurrentScissor();
    if (current && scoped) {
        return IntersectScissor(*current, *scoped);
    }
    if (current) {
        return current;
    }
    return scoped;
}

void BgfxGuiService::SetScissor(const std::optional<ScissorRect>& scissor) const {
    if (!scissor || scissor->width <= 0.0f || scissor->height <= 0.0f) {
        bgfx::setScissor(0, 0, 0, 0);
        return;
    }

    uint16_t x = static_cast<uint16_t>(std::clamp(scissor->x, 0.0f, static_cast<float>(frameWidth_)));
    uint16_t y = static_cast<uint16_t>(std::clamp(scissor->y, 0.0f, static_cast<float>(frameHeight_)));
    uint16_t w = static_cast<uint16_t>(std::clamp(scissor->width, 0.0f, static_cast<float>(frameWidth_ - x)));
    uint16_t h = static_cast<uint16_t>(std::clamp(scissor->height, 0.0f, static_cast<float>(frameHeight_ - y)));
    bgfx::setScissor(x, y, w, h);
}

void BgfxGuiService::SubmitRect(const GuiCommand& command, const std::optional<ScissorRect>& scissor) {
    GuiColor color = command.color;
    color.r = Clamp01(color.r);
    color.g = Clamp01(color.g);
    color.b = Clamp01(color.b);
    color.a = Clamp01(color.a);
    if (color.a > 0.0f) {
        GuiVertex v0{command.rect.x, command.rect.y, 0.0f,
                     Clamp01(color.r), Clamp01(color.g), Clamp01(color.b), Clamp01(color.a), 0.0f, 0.0f};
        GuiVertex v1{command.rect.x + command.rect.width, command.rect.y, 0.0f,
                     Clamp01(color.r), Clamp01(color.g), Clamp01(color.b), Clamp01(color.a), 1.0f, 0.0f};
        GuiVertex v2{command.rect.x + command.rect.width, command.rect.y + command.rect.height, 0.0f,
                     Clamp01(color.r), Clamp01(color.g), Clamp01(color.b), Clamp01(color.a), 1.0f, 1.0f};
        GuiVertex v3{command.rect.x, command.rect.y + command.rect.height, 0.0f,
                     Clamp01(color.r), Clamp01(color.g), Clamp01(color.b), Clamp01(color.a), 0.0f, 1.0f};
        SubmitQuad(v0, v1, v2, v3, whiteTexture_, scissor);
    }

    if (command.borderWidth <= 0.0f) {
        return;
    }

    GuiColor border = command.borderColor;
    if (border.a <= 0.0f) {
        return;
    }
    border.r = Clamp01(border.r);
    border.g = Clamp01(border.g);
    border.b = Clamp01(border.b);
    border.a = Clamp01(border.a);

    float bw = std::min(command.borderWidth, std::min(command.rect.width, command.rect.height));
    float x = command.rect.x;
    float y = command.rect.y;
    float w = command.rect.width;
    float h = command.rect.height;

    GuiVertex vt0{x, y, 0.0f, border.r, border.g, border.b, border.a, 0.0f, 0.0f};
    GuiVertex vt1{x + w, y, 0.0f, border.r, border.g, border.b, border.a, 1.0f, 0.0f};
    GuiVertex vt2{x + w, y + bw, 0.0f, border.r, border.g, border.b, border.a, 1.0f, 1.0f};
    GuiVertex vt3{x, y + bw, 0.0f, border.r, border.g, border.b, border.a, 0.0f, 1.0f};
    SubmitQuad(vt0, vt1, vt2, vt3, whiteTexture_, scissor);

    GuiVertex vb0{x, y + h - bw, 0.0f, border.r, border.g, border.b, border.a, 0.0f, 0.0f};
    GuiVertex vb1{x + w, y + h - bw, 0.0f, border.r, border.g, border.b, border.a, 1.0f, 0.0f};
    GuiVertex vb2{x + w, y + h, 0.0f, border.r, border.g, border.b, border.a, 1.0f, 1.0f};
    GuiVertex vb3{x, y + h, 0.0f, border.r, border.g, border.b, border.a, 0.0f, 1.0f};
    SubmitQuad(vb0, vb1, vb2, vb3, whiteTexture_, scissor);

    GuiVertex vl0{x, y + bw, 0.0f, border.r, border.g, border.b, border.a, 0.0f, 0.0f};
    GuiVertex vl1{x + bw, y + bw, 0.0f, border.r, border.g, border.b, border.a, 1.0f, 0.0f};
    GuiVertex vl2{x + bw, y + h - bw, 0.0f, border.r, border.g, border.b, border.a, 1.0f, 1.0f};
    GuiVertex vl3{x, y + h - bw, 0.0f, border.r, border.g, border.b, border.a, 0.0f, 1.0f};
    SubmitQuad(vl0, vl1, vl2, vl3, whiteTexture_, scissor);

    GuiVertex vr0{x + w - bw, y + bw, 0.0f, border.r, border.g, border.b, border.a, 0.0f, 0.0f};
    GuiVertex vr1{x + w, y + bw, 0.0f, border.r, border.g, border.b, border.a, 1.0f, 0.0f};
    GuiVertex vr2{x + w, y + h - bw, 0.0f, border.r, border.g, border.b, border.a, 1.0f, 1.0f};
    GuiVertex vr3{x + w - bw, y + h - bw, 0.0f, border.r, border.g, border.b, border.a, 0.0f, 1.0f};
    SubmitQuad(vr0, vr1, vr2, vr3, whiteTexture_, scissor);
}

void BgfxGuiService::SubmitText(const GuiCommand& command, const std::optional<ScissorRect>& scissor) {
    if (command.text.empty()) {
        return;
    }

    int fontSize = command.fontSize > 0.0f
        ? static_cast<int>(std::lround(command.fontSize))
        : defaultFontSize_;
    fontSize = std::max(8, fontSize);

    const TextTexture* texture = GetTextTexture(command.text, fontSize);
    if (!texture || !bgfx::isValid(texture->texture) || texture->width == 0 || texture->height == 0) {
        return;
    }

    std::string alignX = ToLower(command.alignX);
    std::string alignY = ToLower(command.alignY);

    float x = command.rect.x;
    float y = command.rect.y;
    float width = static_cast<float>(texture->width);
    float height = static_cast<float>(texture->height);

    if (alignX == "center") {
        x -= width * 0.5f;
    } else if (alignX == "right") {
        x -= width;
    }

    if (alignY == "center") {
        y -= height * 0.5f;
    } else if (alignY == "bottom") {
        y -= height;
    }

    GuiColor color = command.color;
    GuiVertex v0{x, y, 0.0f, color.r, color.g, color.b, color.a, 0.0f, 0.0f};
    GuiVertex v1{x + width, y, 0.0f, color.r, color.g, color.b, color.a, 1.0f, 0.0f};
    GuiVertex v2{x + width, y + height, 0.0f, color.r, color.g, color.b, color.a, 1.0f, 1.0f};
    GuiVertex v3{x, y + height, 0.0f, color.r, color.g, color.b, color.a, 0.0f, 1.0f};
    SubmitQuad(v0, v1, v2, v3, texture->texture, scissor);
}

void BgfxGuiService::SubmitSvg(const GuiCommand& command, const std::optional<ScissorRect>& scissor) {
    if (command.svgPath.empty()) {
        return;
    }

    int width = static_cast<int>(std::lround(command.rect.width));
    int height = static_cast<int>(std::lround(command.rect.height));
    if (width <= 0 || height <= 0) {
        return;
    }

    const SvgTexture* texture = GetSvgTexture(command.svgPath, width, height);
    if (!texture || !bgfx::isValid(texture->texture)) {
        return;
    }

    GuiColor color = command.svgTint;
    color.r = Clamp01(color.r);
    color.g = Clamp01(color.g);
    color.b = Clamp01(color.b);
    color.a = Clamp01(color.a);
    GuiVertex v0{command.rect.x, command.rect.y, 0.0f, color.r, color.g, color.b, color.a, 0.0f, 0.0f};
    GuiVertex v1{command.rect.x + command.rect.width, command.rect.y, 0.0f, color.r, color.g, color.b, color.a, 1.0f, 0.0f};
    GuiVertex v2{command.rect.x + command.rect.width, command.rect.y + command.rect.height, 0.0f, color.r, color.g, color.b, color.a, 1.0f, 1.0f};
    GuiVertex v3{command.rect.x, command.rect.y + command.rect.height, 0.0f, color.r, color.g, color.b, color.a, 0.0f, 1.0f};
    SubmitQuad(v0, v1, v2, v3, texture->texture, scissor);
}

void BgfxGuiService::SubmitQuad(const GuiVertex& v0,
                                const GuiVertex& v1,
                                const GuiVertex& v2,
                                const GuiVertex& v3,
                                bgfx::TextureHandle texture,
                                const std::optional<ScissorRect>& scissor) {
    if (!bgfx::isValid(program_) || !bgfx::isValid(texture)) {
        return;
    }
    if (scissor && (scissor->width <= 0.0f || scissor->height <= 0.0f)) {
        return;
    }

    if (bgfx::getAvailTransientVertexBuffer(4, layout_) < 4 ||
        bgfx::getAvailTransientIndexBuffer(6) < 6) {
        if (logger_) {
            logger_->Trace("BgfxGuiService", "SubmitQuad", "Transient buffer exhausted");
        }
        return;
    }

    bgfx::TransientVertexBuffer tvb{};
    bgfx::TransientIndexBuffer tib{};
    bgfx::allocTransientVertexBuffer(&tvb, 4, layout_);
    bgfx::allocTransientIndexBuffer(&tib, 6);

    auto* vertices = reinterpret_cast<GuiVertex*>(tvb.data);
    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    vertices[3] = v3;

    auto* indices = reinterpret_cast<uint16_t*>(tib.data);
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 0;
    indices[4] = 2;
    indices[5] = 3;

    float identity[16];
    bx::mtxIdentity(identity);

    if (logger_) {
        logger_->Trace("BgfxGuiService", "SubmitQuad",
                       "vertex[0]: pos=[" + std::to_string(v0.x) + "," + std::to_string(v0.y) + "," + std::to_string(v0.z) +
                       "], color=[" + std::to_string(v0.r) + "," + std::to_string(v0.g) + "," + std::to_string(v0.b) + "," + std::to_string(v0.a) +
                       "], uv=[" + std::to_string(v0.u) + "," + std::to_string(v0.v) + "]");
        logger_->Trace("BgfxGuiService", "SubmitQuad",
                       "uniforms: mode=" + std::string(usesMaterialXShaders_ ? "materialx" : "builtin") +
                       ", sampler=" + std::to_string(bgfx::isValid(sampler_)) +
                       ", program=" + std::to_string(bgfx::isValid(program_)) +
                       ", texture=" + std::to_string(bgfx::isValid(texture)) +
                       ", viewId=" + std::to_string(viewId_));
        logger_->Trace("BgfxGuiService", "SubmitQuad",
                       "viewProjection[0-3]=[" + std::to_string(viewProjection_[0]) + "," +
                       std::to_string(viewProjection_[1]) + "," +
                       std::to_string(viewProjection_[2]) + "," +
                       std::to_string(viewProjection_[3]) + "]");
    }

    if (!bgfx::isValid(sampler_)) {
        if (logger_) {
            logger_->Error("BgfxGuiService::SubmitQuad: Sampler uniform not initialized");
        }
        return;
    }

    SetScissor(scissor);
    bgfx::setTransform(identity);
    
    // Use appropriate uniforms based on shader type
    if (usesMaterialXShaders_) {
        if (!bgfx::isValid(worldMatrixUniform_) || !bgfx::isValid(viewProjMatrixUniform_)) {
            if (logger_) {
                logger_->Error("BgfxGuiService::SubmitQuad: MaterialX uniforms not initialized");
            }
            return;
        }
        // MaterialX shader: separate matrices
        bgfx::setUniform(worldMatrixUniform_, identity);
        bgfx::setUniform(viewProjMatrixUniform_, viewProjection_.data());
    } else if (!usesPredefinedModelViewProj_) {
        if (!bgfx::isValid(modelViewProjUniform_)) {
            if (logger_) {
                logger_->Error("BgfxGuiService::SubmitQuad: GUI modelViewProj uniform not initialized");
            }
            return;
        }
        bgfx::setUniform(modelViewProjUniform_, viewProjection_.data());
    }
    bgfx::setTexture(0, sampler_, texture);
    bgfx::setVertexBuffer(0, &tvb, 0, 4);
    bgfx::setIndexBuffer(&tib, 0, 6);
    bgfx::setState(BGFX_STATE_WRITE_RGB |
                   BGFX_STATE_WRITE_A |
                   BGFX_STATE_BLEND_ALPHA |
                   BGFX_STATE_MSAA);
    bgfx::submit(viewId_, program_);
}

const BgfxGuiService::TextTexture* BgfxGuiService::GetTextTexture(const std::string& text, int fontSize) {
    if (text.empty()) {
        return nullptr;
    }
    if (fontSize <= 0) {
        return nullptr;
    }

    EnsureFontReady();
    if (!freeType_ || !freeType_->ready || !freeType_->face) {
        return nullptr;
    }

    TextKey key{text, fontSize};
    auto it = textCache_.find(key);
    if (it != textCache_.end()) {
        it->second.lastUsedFrame = frameIndex_;
        return &it->second;
    }

    FT_Face face = freeType_->face;
    const FT_UInt fontSizePixels = static_cast<FT_UInt>(fontSize);
    if (FT_Set_Pixel_Sizes(face, 0, fontSizePixels) != 0) {
        return nullptr;
    }

    int ascent = static_cast<int>(face->size->metrics.ascender >> 6);
    int descent = static_cast<int>(face->size->metrics.descender >> 6);
    int height = ascent - descent;
    int width = 0;

    for (char ch : text) {
        const FT_ULong codepoint = static_cast<FT_ULong>(static_cast<unsigned char>(ch));
        if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER) != 0) {
            continue;
        }
        const int advance = static_cast<int>(face->glyph->advance.x >> 6);
        width += advance;
    }

    if (width <= 0 || height <= 0) {
        return nullptr;
    }

    std::vector<uint8_t> pixels(static_cast<size_t>(width * height * 4), 0);
    int penX = 0;
    for (char ch : text) {
        const FT_ULong codepoint = static_cast<FT_ULong>(static_cast<unsigned char>(ch));
        if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER) != 0) {
            continue;
        }

        FT_GlyphSlot glyph = face->glyph;
        FT_Bitmap& bitmap = glyph->bitmap;
        int pitch = bitmap.pitch;
        if (pitch < 0) {
            pitch = -pitch;
        }

        int x0 = penX + glyph->bitmap_left;
        int y0 = ascent - glyph->bitmap_top;

        for (int row = 0; row < static_cast<int>(bitmap.rows); ++row) {
            int y = y0 + row;
            if (y < 0 || y >= height) {
                continue;
            }
            for (int col = 0; col < static_cast<int>(bitmap.width); ++col) {
                int x = x0 + col;
                if (x < 0 || x >= width) {
                    continue;
                }
                uint8_t alpha = bitmap.buffer[row * pitch + col];
                size_t idx = static_cast<size_t>((y * width + x) * 4);
                pixels[idx + 0] = 255;
                pixels[idx + 1] = 255;
                pixels[idx + 2] = 255;
                pixels[idx + 3] = alpha;
            }
        }

        penX += static_cast<int>(glyph->advance.x >> 6);
    }

    TextTexture entry{};
    entry.texture = CreateTexture(pixels.data(),
                                  static_cast<uint32_t>(width),
                                  static_cast<uint32_t>(height),
                                  kGuiSamplerFlags);
    entry.width = width;
    entry.height = height;
    entry.baseline = ascent;
    entry.fontSize = fontSize;
    entry.lastUsedFrame = frameIndex_;

    auto [insertIt, inserted] = textCache_.emplace(std::move(key), entry);
    if (!inserted) {
        return nullptr;
    }
    return &insertIt->second;
}

const BgfxGuiService::SvgTexture* BgfxGuiService::GetSvgTexture(const std::string& path,
                                                                int width,
                                                                int height) {
    if (path.empty()) {
        return nullptr;
    }

    SvgKey key{ResolvePath(path).string(), width, height};
    auto it = svgCache_.find(key);
    if (it != svgCache_.end()) {
        it->second.lastUsedFrame = frameIndex_;
        return &it->second;
    }

    auto document = lunasvg::Document::loadFromFile(key.path);
    if (!document) {
        if (logger_) {
            logger_->Warn("BgfxGuiService::GetSvgTexture: Failed to load " + key.path);
        }
        return nullptr;
    }

    auto bitmap = document->renderToBitmap(width, height);
    if (!bitmap.valid()) {
        return nullptr;
    }

    const uint8_t* data = bitmap.data();
    const uint32_t w = static_cast<uint32_t>(bitmap.width());
    const uint32_t h = static_cast<uint32_t>(bitmap.height());
    if (!data || w == 0 || h == 0) {
        return nullptr;
    }

    std::vector<uint8_t> rgba(static_cast<size_t>(w * h * 4), 0);
    for (uint32_t i = 0; i < w * h; ++i) {
        const uint8_t b = data[i * 4 + 0];
        const uint8_t g = data[i * 4 + 1];
        const uint8_t r = data[i * 4 + 2];
        const uint8_t a = data[i * 4 + 3];
        if (a > 0) {
            rgba[i * 4 + 0] = static_cast<uint8_t>(std::min(255, (static_cast<int>(r) * 255) / a));
            rgba[i * 4 + 1] = static_cast<uint8_t>(std::min(255, (static_cast<int>(g) * 255) / a));
            rgba[i * 4 + 2] = static_cast<uint8_t>(std::min(255, (static_cast<int>(b) * 255) / a));
            rgba[i * 4 + 3] = a;
        } else {
            rgba[i * 4 + 0] = 0;
            rgba[i * 4 + 1] = 0;
            rgba[i * 4 + 2] = 0;
            rgba[i * 4 + 3] = 0;
        }
    }

    SvgTexture entry{};
    entry.texture = CreateTexture(rgba.data(), w, h, kGuiSamplerFlags);
    entry.width = static_cast<int>(w);
    entry.height = static_cast<int>(h);
    entry.lastUsedFrame = frameIndex_;

    auto [insertIt, inserted] = svgCache_.emplace(std::move(key), entry);
    if (!inserted) {
        return nullptr;
    }
    return &insertIt->second;
}

bgfx::TextureHandle BgfxGuiService::CreateTexture(const uint8_t* rgba,
                                                  uint32_t width,
                                                  uint32_t height,
                                                  uint64_t flags) const {
    if (!rgba || width == 0 || height == 0) {
        return BGFX_INVALID_HANDLE;
    }
    uint32_t maxDim = maxTextureDim_;
    if (const bgfx::Caps* caps = bgfx::getCaps()) {
        if (caps->limits.maxTextureSize > 0) {
            maxDim = (maxDim == 0)
                ? caps->limits.maxTextureSize
                : std::min<uint32_t>(maxDim, caps->limits.maxTextureSize);
        }
    }
    if (maxDim > 0 && (width > maxDim || height > maxDim)) {
        if (logger_) {
            logger_->Error("BgfxGuiService::CreateTexture: texture size (" +
                          std::to_string(width) + "x" + std::to_string(height) +
                          ") exceeds max texture dim (" + std::to_string(maxDim) + ")");
        }
        return BGFX_INVALID_HANDLE;
    }
    const uint32_t size = width * height * 4;
    const bgfx::Memory* mem = bgfx::copy(rgba, size);
    return bgfx::createTexture2D(static_cast<uint16_t>(width),
                                 static_cast<uint16_t>(height),
                                 false,
                                 1,
                                 bgfx::TextureFormat::RGBA8,
                                 flags,
                                 mem);
}

bgfx::ProgramHandle BgfxGuiService::CreateProgram(const char* vertexSource,
                                                  const char* fragmentSource) {
    if (!vertexSource || !fragmentSource) {
        if (logger_) {
            logger_->Error("BgfxGuiService::CreateProgram: null shader source");
        }
        return BGFX_INVALID_HANDLE;
    }

    bgfx::ShaderHandle vs = CreateShader("gui_vertex", vertexSource, true);
    bgfx::ShaderHandle fs = CreateShader("gui_fragment", fragmentSource, false);
    if (logger_) {
        logger_->Trace("BgfxGuiService", "CreateProgram",
                       "vs.idx=" + std::to_string(vs.idx) + ", vs.valid=" + std::to_string(bgfx::isValid(vs)) +
                       ", fs.idx=" + std::to_string(fs.idx) + ", fs.valid=" + std::to_string(bgfx::isValid(fs)));
    }
    if (!bgfx::isValid(vs) || !bgfx::isValid(fs)) {
        if (logger_) {
            logger_->Error("BgfxGuiService::CreateProgram: shader compilation failed (vs=" +
                          std::to_string(bgfx::isValid(vs)) + ", fs=" +
                          std::to_string(bgfx::isValid(fs)) + ")");
        }
        if (bgfx::isValid(vs)) {
            bgfx::destroy(vs);
        }
        if (bgfx::isValid(fs)) {
            bgfx::destroy(fs);
        }
        return BGFX_INVALID_HANDLE;
    }

    if (!usesMaterialXShaders_) {
        ResolveGuiMatrixUniform(vs);
    }
    
    bgfx::ProgramHandle program = bgfx::createProgram(vs, fs, true);
    if (!bgfx::isValid(program) && logger_) {
        logger_->Error("BgfxGuiService::CreateProgram: bgfx::createProgram failed to link shaders");
        logger_->Trace("BgfxGuiService", "CreateProgram",
                       "renderer=" + std::string(RendererTypeName(bgfx::getRendererType())) +
                       ", program.idx=" + std::to_string(program.idx));
    } else if (logger_) {
        logger_->Trace("BgfxGuiService", "CreateProgram", 
                       "GUI program created successfully, program.idx=" + std::to_string(program.idx));
    }
    return program;
}

bgfx::ShaderHandle BgfxGuiService::CreateShader(const std::string& label,
                                                const std::string& source,
                                                bool isVertex) const {
    BgfxShaderCompiler compiler(logger_, pipelineCompiler_);
    
    std::vector<BgfxShaderUniform> uniforms;
    std::vector<bgfx::Attrib::Enum> attributes;
    
    if (isVertex) {
        uniforms.push_back(BgfxShaderUniform{
            "u_modelViewProj",
            bgfx::UniformType::Mat4,
            1,    // num
            0,    // regIndex
            4,    // regCount (mat4 = 4 vec4s)
            0,    // texComponent
            0,    // texDimension
            0     // texFormat
        });
        attributes = {bgfx::Attrib::Position, bgfx::Attrib::Color0, bgfx::Attrib::TexCoord0};
    } else {
        uniforms.push_back(BgfxShaderUniform{
            "s_tex",
            bgfx::UniformType::Sampler,
            1,    // num
            0,    // regIndex
            1,    // regCount
            0,    // texComponent
            0,    // texDimension
            0     // texFormat
        });
    }
    
    return compiler.CompileShader(label, source, isVertex, uniforms, attributes);
}

void BgfxGuiService::ResolveGuiMatrixUniform(bgfx::ShaderHandle shader) {
    modelViewProjUniform_ = BGFX_INVALID_HANDLE;
    usesPredefinedModelViewProj_ = false;

    if (!bgfx::isValid(shader)) {
        usesPredefinedModelViewProj_ = true;
        return;
    }

    const uint16_t uniformCount = bgfx::getShaderUniforms(shader, nullptr, 0);
    std::string modelViewProjName = "predefined";
    std::string uniformNames;

    if (uniformCount > 0) {
        std::vector<bgfx::UniformHandle> uniforms(uniformCount);
        bgfx::getShaderUniforms(shader, uniforms.data(), uniformCount);

        for (const auto& uniform : uniforms) {
            bgfx::UniformInfo info{};
            bgfx::getUniformInfo(uniform, info);
            if (!uniformNames.empty()) {
                uniformNames += ", ";
            }
            uniformNames += info.name;
            if (std::strcmp(info.name, "u_modelViewProj") == 0 ||
                std::strcmp(info.name, "UniformBuffer.u_modelViewProj") == 0) {
                modelViewProjUniform_ = uniform;
                modelViewProjName = info.name;
                break;
            }
        }
    }

    if (!bgfx::isValid(modelViewProjUniform_)) {
        usesPredefinedModelViewProj_ = true;
    }

    if (logger_) {
        logger_->Trace("BgfxGuiService", "ResolveGuiMatrixUniform",
                       "uniformCount=" + std::to_string(uniformCount) +
                       ", modelViewProj=" + modelViewProjName +
                       (uniformNames.empty() ? "" : ", uniforms=[" + uniformNames + "]"));
        if (!uniformNames.empty() && modelViewProjName == "predefined") {
            logger_->Warn("BgfxGuiService::ResolveGuiMatrixUniform: u_modelViewProj not found; uniforms=[" +
                          uniformNames + "]");
        }
    }
}

void BgfxGuiService::PruneTextCache() {
    if (textCache_.size() <= maxTextCacheEntries_) {
        return;
    }
    while (textCache_.size() > maxTextCacheEntries_) {
        auto oldest = std::min_element(textCache_.begin(), textCache_.end(),
                                       [](const auto& left, const auto& right) {
                                           return left.second.lastUsedFrame < right.second.lastUsedFrame;
                                       });
        if (oldest == textCache_.end()) {
            break;
        }
        if (bgfx::isValid(oldest->second.texture)) {
            bgfx::destroy(oldest->second.texture);
        }
        textCache_.erase(oldest);
    }
}

void BgfxGuiService::PruneSvgCache() {
    if (svgCache_.size() <= maxSvgCacheEntries_) {
        return;
    }
    while (svgCache_.size() > maxSvgCacheEntries_) {
        auto oldest = std::min_element(svgCache_.begin(), svgCache_.end(),
                                       [](const auto& left, const auto& right) {
                                           return left.second.lastUsedFrame < right.second.lastUsedFrame;
                                       });
        if (oldest == svgCache_.end()) {
            break;
        }
        if (bgfx::isValid(oldest->second.texture)) {
            bgfx::destroy(oldest->second.texture);
        }
        svgCache_.erase(oldest);
    }
}

}  // namespace sdl3cpp::services::impl
