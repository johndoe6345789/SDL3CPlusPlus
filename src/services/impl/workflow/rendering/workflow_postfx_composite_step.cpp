#include "services/interfaces/workflow/rendering/workflow_postfx_composite_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_timer.h>
#include <fstream>
#include <stdexcept>
#include <cstdio>

namespace sdl3cpp::services::impl {

WorkflowPostfxCompositeStep::WorkflowPostfxCompositeStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

WorkflowPostfxCompositeStep::~WorkflowPostfxCompositeStep() {
    if (fps_renderer_)    { SDL_DestroyRenderer(fps_renderer_);  fps_renderer_ = nullptr; }
    if (fps_surface_)     { SDL_DestroySurface(fps_surface_);    fps_surface_  = nullptr; }
    if (overlay_device_) {
        if (overlay_sampler_)  SDL_ReleaseGPUSampler(overlay_device_, overlay_sampler_);
        if (overlay_vtx_buf_)  SDL_ReleaseGPUBuffer(overlay_device_, overlay_vtx_buf_);
        if (overlay_transfer_) SDL_ReleaseGPUTransferBuffer(overlay_device_, overlay_transfer_);
        if (overlay_tex_)      SDL_ReleaseGPUTexture(overlay_device_, overlay_tex_);
        if (overlay_pipeline_) SDL_ReleaseGPUGraphicsPipeline(overlay_device_, overlay_pipeline_);
    }
}

std::string WorkflowPostfxCompositeStep::GetPluginId() const {
    return "postfx.composite";
}

// ---------------------------------------------------------------------------
// Overlay initialisation — called once on the first frame where a GPU device
// is available.  Silent no-op if shaders aren't found (debug feature).
// ---------------------------------------------------------------------------
void WorkflowPostfxCompositeStep::TryInitOverlay(SDL_GPUDevice* device,
                                                  WorkflowContext& context) {
    overlay_device_ = device;

    // Load overlay SPIR-V shaders from disk
    auto loadSpv = [](const char* path) -> std::vector<uint8_t> {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f.is_open()) return {};
        auto sz = f.tellg();
        std::vector<uint8_t> buf(static_cast<size_t>(sz));
        f.seekg(0);
        f.read(reinterpret_cast<char*>(buf.data()), sz);
        return buf;
    };

    auto vert_spv = loadSpv("packages/quake3/shaders/spirv/overlay.vert.spv");
    auto frag_spv = loadSpv("packages/quake3/shaders/spirv/overlay.frag.spv");
    if (vert_spv.empty() || frag_spv.empty()) {
        if (logger_) logger_->Warn("postfx.composite: overlay shaders not found — FPS display disabled");
        return;
    }

    // Only SPIRV supported (D3D12/Metal paths would need DXIL/MSL variants)
    SDL_GPUShaderFormat fmt = SDL_GPU_SHADERFORMAT_SPIRV;
    const char* drv = SDL_GetGPUDeviceDriver(device);
    if (drv && std::string(drv) != "vulkan") {
        if (logger_) logger_->Warn("postfx.composite: overlay only supported on Vulkan — FPS display disabled");
        return;
    }

    // Vertex shader — no uniforms, no samplers
    SDL_GPUShaderCreateInfo vs_info = {};
    vs_info.code = vert_spv.data(); vs_info.code_size = vert_spv.size();
    vs_info.entrypoint = "main"; vs_info.format = fmt;
    vs_info.stage = SDL_GPU_SHADERSTAGE_VERTEX;

    // Fragment shader — 1 sampler (set=2, binding=0)
    SDL_GPUShaderCreateInfo fs_info = {};
    fs_info.code = frag_spv.data(); fs_info.code_size = frag_spv.size();
    fs_info.entrypoint = "main"; fs_info.format = fmt;
    fs_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    fs_info.num_samplers = 1;

    SDL_GPUShader* vs = SDL_CreateGPUShader(device, &vs_info);
    SDL_GPUShader* fs = SDL_CreateGPUShader(device, &fs_info);
    if (!vs || !fs) {
        if (vs) SDL_ReleaseGPUShader(device, vs);
        if (fs) SDL_ReleaseGPUShader(device, fs);
        return;
    }

    // Vertex format: float3 pos + float2 uv = 20 bytes per vertex
    SDL_GPUVertexBufferDescription vbuf = {};
    vbuf.slot = 0;
    vbuf.pitch = sizeof(float) * 5;
    vbuf.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    SDL_GPUVertexAttribute attrs[2] = {};
    attrs[0] = { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0 };
    attrs[1] = { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, sizeof(float) * 3 };

    SDL_GPUVertexInputState vis = {};
    vis.vertex_buffer_descriptions = &vbuf;
    vis.num_vertex_buffers = 1;
    vis.vertex_attributes = attrs;
    vis.num_vertex_attributes = 2;

    // Query swapchain format from window for the blend target
    SDL_GPUTextureFormat sc_fmt = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
    auto* window = context.Get<SDL_Window*>("sdl_window", nullptr);
    if (window) sc_fmt = SDL_GetGPUSwapchainTextureFormat(device, window);

    SDL_GPUColorTargetDescription ctd = {};
    ctd.format = sc_fmt;
    ctd.blend_state.enable_blend             = true;
    ctd.blend_state.src_color_blendfactor    = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    ctd.blend_state.dst_color_blendfactor    = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    ctd.blend_state.color_blend_op           = SDL_GPU_BLENDOP_ADD;
    ctd.blend_state.src_alpha_blendfactor    = SDL_GPU_BLENDFACTOR_ONE;
    ctd.blend_state.dst_alpha_blendfactor    = SDL_GPU_BLENDFACTOR_ZERO;
    ctd.blend_state.alpha_blend_op           = SDL_GPU_BLENDOP_ADD;

    SDL_GPUGraphicsPipelineCreateInfo pci = {};
    pci.vertex_shader   = vs;
    pci.fragment_shader = fs;
    pci.vertex_input_state = vis;
    pci.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pci.rasterizer_state.fill_mode  = SDL_GPU_FILLMODE_FILL;
    pci.rasterizer_state.cull_mode  = SDL_GPU_CULLMODE_NONE;
    pci.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    pci.depth_stencil_state.enable_depth_test  = false;
    pci.depth_stencil_state.enable_depth_write = false;
    pci.target_info.num_color_targets          = 1;
    pci.target_info.color_target_descriptions  = &ctd;
    pci.target_info.has_depth_stencil_target   = false;

    overlay_pipeline_ = SDL_CreateGPUGraphicsPipeline(device, &pci);
    SDL_ReleaseGPUShader(device, vs);
    SDL_ReleaseGPUShader(device, fs);
    if (!overlay_pipeline_) return;

    // Font texture (RGBA8 — matches SDL_PIXELFORMAT_RGBA32 memory layout)
    SDL_GPUTextureCreateInfo tci = {};
    tci.type              = SDL_GPU_TEXTURETYPE_2D;
    tci.format            = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    tci.usage             = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    tci.width             = kOvW;
    tci.height            = kOvH;
    tci.layer_count_or_depth = 1;
    tci.num_levels        = 1;
    overlay_tex_ = SDL_CreateGPUTexture(device, &tci);
    if (!overlay_tex_) { SDL_ReleaseGPUGraphicsPipeline(device, overlay_pipeline_); overlay_pipeline_ = nullptr; return; }

    // Transfer buffer for CPU→GPU texture upload each frame
    SDL_GPUTransferBufferCreateInfo tbci = {};
    tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbci.size  = static_cast<Uint32>(kOvW * kOvH * 4);
    overlay_transfer_ = SDL_CreateGPUTransferBuffer(device, &tbci);
    if (!overlay_transfer_) { SDL_ReleaseGPUTexture(device, overlay_tex_); overlay_tex_ = nullptr; return; }

    // Vertex buffer: 6 vertices × 5 floats = 120 bytes (static quad, uploaded once)
    SDL_GPUBufferCreateInfo bci = {};
    bci.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    bci.size  = 6u * 5u * static_cast<Uint32>(sizeof(float));
    overlay_vtx_buf_ = SDL_CreateGPUBuffer(device, &bci);
    if (!overlay_vtx_buf_) { SDL_ReleaseGPUTransferBuffer(device, overlay_transfer_); overlay_transfer_ = nullptr; return; }

    // Linear sampler for the font texture
    SDL_GPUSamplerCreateInfo sci = {};
    sci.min_filter  = SDL_GPU_FILTER_NEAREST;
    sci.mag_filter  = SDL_GPU_FILTER_NEAREST;
    sci.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sci.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sci.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    overlay_sampler_ = SDL_CreateGPUSampler(device, &sci);
    if (!overlay_sampler_) { SDL_ReleaseGPUBuffer(device, overlay_vtx_buf_); overlay_vtx_buf_ = nullptr; return; }

    // SDL software renderer for rasterising the FPS string
    fps_surface_ = SDL_CreateSurface(kOvW, kOvH, SDL_PIXELFORMAT_RGBA32);
    if (!fps_surface_) { SDL_ReleaseGPUSampler(device, overlay_sampler_); overlay_sampler_ = nullptr; return; }

    fps_renderer_ = SDL_CreateSoftwareRenderer(fps_surface_);
    if (!fps_renderer_) {
        SDL_DestroySurface(fps_surface_); fps_surface_ = nullptr;
        SDL_ReleaseGPUSampler(device, overlay_sampler_); overlay_sampler_ = nullptr;
        return;
    }

    overlay_ready_ = true;
    if (logger_) logger_->Info("postfx.composite: FPS overlay initialised");
}

// ---------------------------------------------------------------------------
// Per-frame overlay render — called after the composite pass ends but before
// the command buffer is submitted.  Uses a second render pass with LOADOP_LOAD
// so the composite output is preserved underneath the text quad.
// ---------------------------------------------------------------------------
void WorkflowPostfxCompositeStep::RenderOverlay(SDL_GPUCommandBuffer* cmd,
                                                 SDL_GPUTexture* swapchain_tex,
                                                 SDL_GPUDevice* device,
                                                 WorkflowContext& context) {
    // --- FPS measurement (exponential moving average) ---
    const uint64_t now_ns = SDL_GetTicksNS();
    if (fps_last_ns_ != 0) {
        const float raw_dt = static_cast<float>(
            static_cast<double>(now_ns - fps_last_ns_) / 1e9);
        if (raw_dt > 0.0f) {
            const float raw_fps = 1.0f / raw_dt;
            fps_smooth_ = fps_smooth_ * 0.9f + raw_fps * 0.1f;
        }
    }
    fps_last_ns_ = now_ns;

    // --- Rasterise FPS text onto SDL surface ---
    char buf[32];
    std::snprintf(buf, sizeof(buf), "FPS: %.0f", fps_smooth_);

    SDL_ClearSurface(fps_surface_, 0.0f, 0.0f, 0.0f, 0.0f);
    SDL_SetRenderDrawColor(fps_renderer_, 255, 220, 50, 255);
    SDL_RenderDebugText(fps_renderer_, 5.0f, 2.0f, buf);

    // --- Upload surface pixels to GPU overlay texture ---
    void* mapped = SDL_MapGPUTransferBuffer(device, overlay_transfer_, false);
    if (!mapped) return;
    std::memcpy(mapped, fps_surface_->pixels,
                static_cast<size_t>(kOvW * kOvH * 4));
    SDL_UnmapGPUTransferBuffer(device, overlay_transfer_);

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);
    if (copy_pass) {
        SDL_GPUTextureTransferInfo src = {};
        src.transfer_buffer = overlay_transfer_;
        src.pixels_per_row  = static_cast<Uint32>(kOvW);
        src.rows_per_layer  = static_cast<Uint32>(kOvH);

        SDL_GPUTextureRegion dst = {};
        dst.texture = overlay_tex_;
        dst.w = static_cast<Uint32>(kOvW);
        dst.h = static_cast<Uint32>(kOvH);
        dst.d = 1;

        SDL_UploadToGPUTexture(copy_pass, &src, &dst, false);
        SDL_EndGPUCopyPass(copy_pass);
    }

    // --- Upload vertex buffer (once — positions don't change) ---
    if (!vbuf_uploaded_) {
        // Read viewport dimensions for NDC calculation
        const float vw = static_cast<float>(
            context.Get<int>("viewport_width", 1280));
        const float vh = static_cast<float>(
            context.Get<int>("viewport_height", 960));

        // Top-right corner: 10px margin from edges
        const float px_l = vw - static_cast<float>(kOvW) - 10.0f;
        const float px_r = vw - 10.0f;
        const float py_t = 10.0f;
        const float py_b = 10.0f + static_cast<float>(kOvH);

        // Convert to NDC (y flipped: pixel y=0 is top → NDC y=+1)
        const float x0 = px_l / vw * 2.0f - 1.0f;
        const float x1 = px_r / vw * 2.0f - 1.0f;
        const float y1 = 1.0f - py_t / vh * 2.0f;
        const float y0 = 1.0f - py_b / vh * 2.0f;

        // Two triangles: {x,y,z,u,v}
        const float verts[6][5] = {
            { x0, y1, 0.0f,  0.0f, 0.0f },  // top-left
            { x1, y1, 0.0f,  1.0f, 0.0f },  // top-right
            { x1, y0, 0.0f,  1.0f, 1.0f },  // bottom-right
            { x0, y1, 0.0f,  0.0f, 0.0f },  // top-left
            { x1, y0, 0.0f,  1.0f, 1.0f },  // bottom-right
            { x0, y0, 0.0f,  0.0f, 1.0f },  // bottom-left
        };

        const Uint32 vbuf_size = 6u * 5u * static_cast<Uint32>(sizeof(float));
        SDL_GPUTransferBufferCreateInfo vtbci = {};
        vtbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        vtbci.size  = vbuf_size;
        SDL_GPUTransferBuffer* vtb = SDL_CreateGPUTransferBuffer(device, &vtbci);
        if (vtb) {
            void* vptr = SDL_MapGPUTransferBuffer(device, vtb, false);
            if (vptr) {
                std::memcpy(vptr, verts, vbuf_size);
                SDL_UnmapGPUTransferBuffer(device, vtb);
            }
            SDL_GPUCopyPass* vcp = SDL_BeginGPUCopyPass(cmd);
            if (vcp) {
                SDL_GPUTransferBufferLocation btinfo = { vtb, 0 };
                SDL_GPUBufferRegion bregion = { overlay_vtx_buf_, 0, vbuf_size };
                SDL_UploadToGPUBuffer(vcp, &btinfo, &bregion, false);
                SDL_EndGPUCopyPass(vcp);
            }
            SDL_ReleaseGPUTransferBuffer(device, vtb);
        }
        vbuf_uploaded_ = true;
    }

    // --- Second render pass: LOADOP_LOAD preserves the composite output ---
    SDL_GPUColorTargetInfo ov_target = {};
    ov_target.texture  = swapchain_tex;
    ov_target.load_op  = SDL_GPU_LOADOP_LOAD;
    ov_target.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* ov_pass = SDL_BeginGPURenderPass(cmd, &ov_target, 1, nullptr);
    if (!ov_pass) return;

    SDL_BindGPUGraphicsPipeline(ov_pass, overlay_pipeline_);

    SDL_GPUBufferBinding vb = { overlay_vtx_buf_, 0 };
    SDL_BindGPUVertexBuffers(ov_pass, 0, &vb, 1);

    SDL_GPUTextureSamplerBinding tsb = { overlay_tex_, overlay_sampler_ };
    SDL_BindGPUFragmentSamplers(ov_pass, 0, &tsb, 1);

    SDL_DrawGPUPrimitives(ov_pass, 6, 1, 0, 0);
    SDL_EndGPURenderPass(ov_pass);
}

// ---------------------------------------------------------------------------
// Execute
// ---------------------------------------------------------------------------
void WorkflowPostfxCompositeStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    if (context.GetBool("frame_skip", false)) return;

    auto* cmd          = context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline     = context.Get<SDL_GPUGraphicsPipeline*>("postfx_composite_pipeline", nullptr);
    auto* hdr_texture  = context.Get<SDL_GPUTexture*>("postfx_hdr_texture", nullptr);
    auto* sampler      = context.Get<SDL_GPUSampler*>("postfx_linear_sampler", nullptr);
    auto* swapchain_tex = context.Get<SDL_GPUTexture*>("postfx_swapchain_texture", nullptr);
    auto* device       = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);

    if (!cmd || !pipeline || !hdr_texture || !sampler || !swapchain_tex) {
        if (logger_) logger_->Warn("postfx.composite: Missing required resources");
        if (cmd) {
            // GPU screenshot: before submitting, blit swapchain to a download buffer
            // so the screenshot captures the full 3D scene (not just the CPU overlay).
            auto* gswap = context.Get<SDL_GPUTexture*>("gpu_swapchain_texture", nullptr);
            const auto* ssPath = context.TryGet<std::string>("screenshot_output_path");
            if (device && gswap && ssPath && !ssPath->empty()) {
                const uint32_t sw = context.Get<uint32_t>("frame_width",  1);
                const uint32_t sh = context.Get<uint32_t>("frame_height", 1);
                SDL_GPUTransferBufferCreateInfo tbci = {};
                tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
                tbci.size  = sw * sh * 4;
                auto* tbuf = SDL_CreateGPUTransferBuffer(device, &tbci);
                if (tbuf) {
                    auto* copy = SDL_BeginGPUCopyPass(cmd);
                    if (copy) {
                        SDL_GPUTextureRegion src = {};
                        src.texture = gswap;
                        src.w = sw; src.h = sh; src.d = 1;
                        SDL_GPUTextureTransferInfo dst = {};
                        dst.transfer_buffer  = tbuf;
                        dst.pixels_per_row   = sw;
                        dst.rows_per_layer   = sh;
                        SDL_DownloadFromGPUTexture(copy, &src, &dst);
                        SDL_EndGPUCopyPass(copy);
                    }
                    SDL_SubmitGPUCommandBuffer(cmd);
                    SDL_WaitForGPUIdle(device);
                    void* mapped = SDL_MapGPUTransferBuffer(device, tbuf, false);
                    if (mapped) {
                        SDL_Surface* surf = SDL_CreateSurfaceFrom(
                            (int)sw, (int)sh, SDL_PIXELFORMAT_ABGR8888,
                            mapped, (int)(sw * 4));
                        if (surf) {
                            SDL_SaveBMP(surf, ssPath->c_str());
                            SDL_DestroySurface(surf);
                            if (logger_) logger_->Info("postfx.composite: GPU screenshot saved to " + *ssPath);
                        }
                        SDL_UnmapGPUTransferBuffer(device, tbuf);
                    }
                    SDL_ReleaseGPUTransferBuffer(device, tbuf);
                    context.Set<std::string>("screenshot_output_path", std::string(""));
                    context.Remove("gpu_command_buffer");
                    return;
                }
            }
            SDL_SubmitGPUCommandBuffer(cmd);
            context.Remove("gpu_command_buffer");
        }
        return;
    }

    // Lazy-init FPS overlay (first frame only)
    if (!overlay_ready_ && device) {
        TryInitOverlay(device, context);
    }

    // --- Composite pass: HDR → swapchain ---
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture  = swapchain_tex;
    colorTarget.load_op  = SDL_GPU_LOADOP_DONT_CARE;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorTarget, 1, nullptr);
    if (!pass) {
        SDL_SubmitGPUCommandBuffer(cmd);
        context.Remove("gpu_command_buffer");
        return;
    }

    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    auto* ssao_texture  = context.Get<SDL_GPUTexture*>("postfx_ssao_texture", nullptr);
    auto* bloom_texture = context.Get<SDL_GPUTexture*>("postfx_bloom_result_texture", nullptr);

    SDL_GPUTextureSamplerBinding bindings[3] = {};
    bindings[0].texture = hdr_texture;                                      bindings[0].sampler = sampler;
    bindings[1].texture = ssao_texture  ? ssao_texture  : hdr_texture;     bindings[1].sampler = sampler;
    bindings[2].texture = bloom_texture ? bloom_texture : hdr_texture;     bindings[2].sampler = sampler;
    SDL_BindGPUFragmentSamplers(pass, 0, bindings, 3);

    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(pass);

    // --- FPS overlay: second pass on swapchain with LOADOP_LOAD ---
    if (overlay_ready_ && device) {
        RenderOverlay(cmd, swapchain_tex, device, context);
    }

    // Submit and clean up
    SDL_SubmitGPUCommandBuffer(cmd);
    context.Remove("gpu_command_buffer");
    context.Remove("postfx_swapchain_texture");

    auto frameNum = context.Get<uint32_t>("frame_number", 0u);
    context.Set<uint32_t>("frame_number", frameNum + 1);
}

}  // namespace sdl3cpp::services::impl
