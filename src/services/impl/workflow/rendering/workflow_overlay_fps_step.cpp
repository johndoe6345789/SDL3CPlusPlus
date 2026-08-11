#include "services/interfaces/workflow/rendering/workflow_overlay_fps_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_timer.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowOverlayFpsStep::WorkflowOverlayFpsStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

WorkflowOverlayFpsStep::~WorkflowOverlayFpsStep() {
    if (renderer_) { SDL_DestroyRenderer(renderer_); renderer_ = nullptr; }
    if (surface_)  { SDL_DestroySurface(surface_);   surface_  = nullptr; }
    if (device_) {
        if (sampler_)  SDL_ReleaseGPUSampler(device_, sampler_);
        if (vtx_buf_)  SDL_ReleaseGPUBuffer(device_, vtx_buf_);
        if (transfer_) SDL_ReleaseGPUTransferBuffer(device_, transfer_);
        if (tex_)      SDL_ReleaseGPUTexture(device_, tex_);
        if (pipeline_) SDL_ReleaseGPUGraphicsPipeline(device_, pipeline_);
    }
}

std::string WorkflowOverlayFpsStep::GetPluginId() const {
    return "overlay.fps";
}

void WorkflowOverlayFpsStep::TryInit(SDL_GPUDevice* device, SDL_Window* window) {
    if (disabled_) return;
    device_ = device;

    const char* driver = SDL_GetGPUDeviceDriver(device);
    const std::string driverName = driver ? driver : "";
    if (driverName != "vulkan") {
        if (logger_) logger_->Warn("overlay.fps: SPIR-V overlay only supported on Vulkan, disabled for " + driverName);
        disabled_ = true;
        return;
    }

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
        if (logger_) logger_->Warn("overlay.fps: shaders not found — FPS display disabled");
        return;
    }

    SDL_GPUShaderCreateInfo vs_info = {};
    vs_info.code       = vert_spv.data();
    vs_info.code_size  = vert_spv.size();
    vs_info.entrypoint = "main";
    vs_info.format     = SDL_GPU_SHADERFORMAT_SPIRV;
    vs_info.stage      = SDL_GPU_SHADERSTAGE_VERTEX;

    SDL_GPUShaderCreateInfo fs_info = {};
    fs_info.code         = frag_spv.data();
    fs_info.code_size    = frag_spv.size();
    fs_info.entrypoint   = "main";
    fs_info.format       = SDL_GPU_SHADERFORMAT_SPIRV;
    fs_info.stage        = SDL_GPU_SHADERSTAGE_FRAGMENT;
    fs_info.num_samplers = 1;

    SDL_GPUShader* vs = SDL_CreateGPUShader(device, &vs_info);
    SDL_GPUShader* fs = SDL_CreateGPUShader(device, &fs_info);
    if (!vs || !fs) {
        if (vs) SDL_ReleaseGPUShader(device, vs);
        if (fs) SDL_ReleaseGPUShader(device, fs);
        return;
    }

    SDL_GPUVertexBufferDescription vbuf_desc = {};
    vbuf_desc.slot       = 0;
    vbuf_desc.pitch      = sizeof(float) * 5;
    vbuf_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    SDL_GPUVertexAttribute attrs[2] = {};
    attrs[0] = { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0 };
    attrs[1] = { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, sizeof(float) * 3 };

    SDL_GPUVertexInputState vis = {};
    vis.vertex_buffer_descriptions = &vbuf_desc;
    vis.num_vertex_buffers         = 1;
    vis.vertex_attributes          = attrs;
    vis.num_vertex_attributes      = 2;

    SDL_GPUTextureFormat sc_fmt = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
    if (window) sc_fmt = SDL_GetGPUSwapchainTextureFormat(device, window);

    SDL_GPUColorTargetDescription ctd = {};
    ctd.format                            = sc_fmt;
    ctd.blend_state.enable_blend          = true;
    ctd.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    ctd.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    ctd.blend_state.color_blend_op        = SDL_GPU_BLENDOP_ADD;
    ctd.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    ctd.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    ctd.blend_state.alpha_blend_op        = SDL_GPU_BLENDOP_ADD;

    SDL_GPUGraphicsPipelineCreateInfo pci = {};
    pci.vertex_shader                          = vs;
    pci.fragment_shader                        = fs;
    pci.vertex_input_state                     = vis;
    pci.primitive_type                         = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pci.rasterizer_state.fill_mode             = SDL_GPU_FILLMODE_FILL;
    pci.rasterizer_state.cull_mode             = SDL_GPU_CULLMODE_NONE;
    pci.rasterizer_state.front_face            = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    pci.depth_stencil_state.enable_depth_test  = false;
    pci.depth_stencil_state.enable_depth_write = false;
    pci.target_info.num_color_targets          = 1;
    pci.target_info.color_target_descriptions  = &ctd;
    pci.target_info.has_depth_stencil_target   = false;

    pipeline_ = SDL_CreateGPUGraphicsPipeline(device, &pci);
    SDL_ReleaseGPUShader(device, vs);
    SDL_ReleaseGPUShader(device, fs);
    if (!pipeline_) return;

    SDL_GPUTextureCreateInfo tci = {};
    tci.type                 = SDL_GPU_TEXTURETYPE_2D;
    tci.format               = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    tci.usage                = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    tci.width                = static_cast<Uint32>(kW);
    tci.height               = static_cast<Uint32>(kH);
    tci.layer_count_or_depth = 1;
    tci.num_levels           = 1;
    tex_ = SDL_CreateGPUTexture(device, &tci);
    if (!tex_) { SDL_ReleaseGPUGraphicsPipeline(device, pipeline_); pipeline_ = nullptr; return; }

    SDL_GPUTransferBufferCreateInfo tbci = {};
    tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbci.size  = static_cast<Uint32>(kW * kH * 4);
    transfer_ = SDL_CreateGPUTransferBuffer(device, &tbci);
    if (!transfer_) { SDL_ReleaseGPUTexture(device, tex_); tex_ = nullptr; return; }

    SDL_GPUBufferCreateInfo bci = {};
    bci.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    bci.size  = 6u * 5u * static_cast<Uint32>(sizeof(float));
    vtx_buf_ = SDL_CreateGPUBuffer(device, &bci);
    if (!vtx_buf_) { SDL_ReleaseGPUTransferBuffer(device, transfer_); transfer_ = nullptr; return; }

    SDL_GPUSamplerCreateInfo sci = {};
    sci.min_filter    = SDL_GPU_FILTER_NEAREST;
    sci.mag_filter    = SDL_GPU_FILTER_NEAREST;
    sci.mipmap_mode   = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sci.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sci.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_ = SDL_CreateGPUSampler(device, &sci);
    if (!sampler_) { SDL_ReleaseGPUBuffer(device, vtx_buf_); vtx_buf_ = nullptr; return; }

    surface_ = SDL_CreateSurface(kW, kH, SDL_PIXELFORMAT_RGBA32);
    if (!surface_) { SDL_ReleaseGPUSampler(device, sampler_); sampler_ = nullptr; return; }

    renderer_ = SDL_CreateSoftwareRenderer(surface_);
    if (!renderer_) {
        SDL_DestroySurface(surface_); surface_ = nullptr;
        SDL_ReleaseGPUSampler(device, sampler_); sampler_ = nullptr;
        return;
    }

    ready_ = true;
    if (logger_) logger_->Info("overlay.fps: FPS overlay initialised (" + std::string(SDL_GetGPUDeviceDriver(device)) + ")");
}

void WorkflowOverlayFpsStep::Render(SDL_GPUCommandBuffer* cmd,
                                     SDL_GPUTexture* swapchain_tex,
                                     SDL_GPUDevice* device,
                                     uint32_t frame_w, uint32_t frame_h) {
    // FPS measurement — exponential moving average
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

    // Rasterise FPS text onto SDL surface
    char buf[32];
    std::snprintf(buf, sizeof(buf), "FPS: %.0f", fps_smooth_);

    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 0);
    SDL_RenderClear(renderer_);
    SDL_SetRenderDrawColor(renderer_, 255, 220, 50, 255);
    SDL_RenderDebugText(renderer_, 5.0f, 2.0f, buf);
    SDL_RenderPresent(renderer_);  // flush batch to surface_->pixels

    // Upload surface pixels to GPU texture
    void* mapped = SDL_MapGPUTransferBuffer(device, transfer_, false);
    if (!mapped) return;
    std::memcpy(mapped, surface_->pixels, static_cast<size_t>(kW * kH * 4));
    SDL_UnmapGPUTransferBuffer(device, transfer_);

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);
    if (copy_pass) {
        SDL_GPUTextureTransferInfo src = {};
        src.transfer_buffer = transfer_;
        src.pixels_per_row  = static_cast<Uint32>(kW);
        src.rows_per_layer  = static_cast<Uint32>(kH);

        SDL_GPUTextureRegion dst = {};
        dst.texture = tex_;
        dst.w = static_cast<Uint32>(kW);
        dst.h = static_cast<Uint32>(kH);
        dst.d = 1;

        SDL_UploadToGPUTexture(copy_pass, &src, &dst, false);
        SDL_EndGPUCopyPass(copy_pass);
    }

    // Upload vertex buffer once (quad in top-right corner)
    if (!vbuf_uploaded_) {
        const float vw = static_cast<float>(frame_w > 0 ? frame_w : 1280);
        const float vh = static_cast<float>(frame_h > 0 ? frame_h : 960);

        const float px_l = vw - static_cast<float>(kW) - 10.0f;
        const float px_r = vw - 10.0f;
        const float py_t = 10.0f;
        const float py_b = 10.0f + static_cast<float>(kH);

        // Pixel → NDC (y=0 is top → NDC y=+1)
        const float x0 = px_l / vw * 2.0f - 1.0f;
        const float x1 = px_r / vw * 2.0f - 1.0f;
        const float y1 = 1.0f - py_t / vh * 2.0f;
        const float y0 = 1.0f - py_b / vh * 2.0f;

        const float verts[6][5] = {
            { x0, y1, 0.0f,  0.0f, 0.0f },
            { x1, y1, 0.0f,  1.0f, 0.0f },
            { x1, y0, 0.0f,  1.0f, 1.0f },
            { x0, y1, 0.0f,  0.0f, 0.0f },
            { x1, y0, 0.0f,  1.0f, 1.0f },
            { x0, y0, 0.0f,  0.0f, 1.0f },
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
                SDL_GPUBufferRegion bregion = { vtx_buf_, 0, vbuf_size };
                SDL_UploadToGPUBuffer(vcp, &btinfo, &bregion, false);
                SDL_EndGPUCopyPass(vcp);
            }
            SDL_ReleaseGPUTransferBuffer(device, vtb);
        }
        vbuf_uploaded_ = true;
    }

    // Overlay render pass — LOADOP_LOAD preserves existing scene
    SDL_GPUColorTargetInfo ov_target = {};
    ov_target.texture  = swapchain_tex;
    ov_target.load_op  = SDL_GPU_LOADOP_LOAD;
    ov_target.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* ov_pass = SDL_BeginGPURenderPass(cmd, &ov_target, 1, nullptr);
    if (!ov_pass) return;

    SDL_BindGPUGraphicsPipeline(ov_pass, pipeline_);

    SDL_GPUBufferBinding vb = { vtx_buf_, 0 };
    SDL_BindGPUVertexBuffers(ov_pass, 0, &vb, 1);

    SDL_GPUTextureSamplerBinding tsb = { tex_, sampler_ };
    SDL_BindGPUFragmentSamplers(ov_pass, 0, &tsb, 1);

    SDL_DrawGPUPrimitives(ov_pass, 6, 1, 0, 0);
    SDL_EndGPURenderPass(ov_pass);
    // Command buffer NOT submitted here — postfx.composite handles final submit
}

void WorkflowOverlayFpsStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    if (context.GetBool("frame_skip", false)) return;

    auto* cmd          = context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* swapchain    = context.Get<SDL_GPUTexture*>("gpu_swapchain_texture", nullptr);
    auto* device       = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);

    if (!cmd || !swapchain || !device) return;

    if (!ready_) {
        auto* window = context.Get<SDL_Window*>("sdl_window", nullptr);
        TryInit(device, window);
    }

    // Expose init status for debug.screenshot
    context.Set<bool>("overlay_fps_ready", ready_);
    context.Set<SDL_Surface*>("overlay_fps_surface", surface_);

    if (!ready_) return;

    const auto fw = context.Get<uint32_t>("frame_width",  1280u);
    const auto fh = context.Get<uint32_t>("frame_height", 960u);
    Render(cmd, swapchain, device, fw, fh);
}

}  // namespace sdl3cpp::services::impl
