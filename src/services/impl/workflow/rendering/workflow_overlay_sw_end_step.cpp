#include "services/interfaces/workflow/rendering/workflow_overlay_sw_end_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include <SDL3/SDL.h>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

namespace {
std::vector<uint8_t> LoadBin(const char* path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open()) return {};
    auto sz = f.tellg(); std::vector<uint8_t> d(sz);
    f.seekg(0); f.read((char*)d.data(), sz); return d;
}
} // namespace

WorkflowOverlaySwEndStep::WorkflowOverlaySwEndStep(std::shared_ptr<ILogger> l)
    : logger_(std::move(l)) {}

WorkflowOverlaySwEndStep::~WorkflowOverlaySwEndStep() {
    if (device_) {
        if (head_sampler_) SDL_ReleaseGPUSampler(device_, head_sampler_);
        if (head_vtx_buf_) SDL_ReleaseGPUBuffer(device_, head_vtx_buf_);
        if (sampler_)  SDL_ReleaseGPUSampler(device_, sampler_);
        if (vtx_buf_)  SDL_ReleaseGPUBuffer(device_, vtx_buf_);
        if (transfer_) SDL_ReleaseGPUTransferBuffer(device_, transfer_);
        if (tex_)      SDL_ReleaseGPUTexture(device_, tex_);
        if (pipeline_) SDL_ReleaseGPUGraphicsPipeline(device_, pipeline_);
    }
}

std::string WorkflowOverlaySwEndStep::GetPluginId() const { return "overlay.sw.end"; }

void WorkflowOverlaySwEndStep::TryInit(
        SDL_GPUDevice* device, SDL_Window* window,
        const std::string& vertPath, const std::string& fragPath) {
    device_ = device;
    const char* driver = SDL_GetGPUDeviceDriver(device);
    const std::string drv = driver ? driver : "";
    SDL_GPUShaderFormat fmt = SDL_GPU_SHADERFORMAT_INVALID;
    auto vert = LoadBin(vertPath.c_str());
    auto frag = LoadBin(fragPath.c_str());
    const char* entry = (drv == "metal") ? "main0" : "main";
    if (drv == "metal") fmt = SDL_GPU_SHADERFORMAT_MSL;
    else if (drv == "vulkan") fmt = SDL_GPU_SHADERFORMAT_SPIRV;
    else { disabled_ = true; return; }
    if (vert.empty() || frag.empty()) { disabled_ = true; return; }

    SDL_GPUShaderCreateInfo vsi{}, fsi{};
    vsi.code=vert.data(); vsi.code_size=vert.size(); vsi.entrypoint=entry;
    vsi.format=fmt; vsi.stage=SDL_GPU_SHADERSTAGE_VERTEX;
    fsi.code=frag.data(); fsi.code_size=frag.size(); fsi.entrypoint=entry;
    fsi.format=fmt; fsi.stage=SDL_GPU_SHADERSTAGE_FRAGMENT; fsi.num_samplers=1;
    auto* vs = SDL_CreateGPUShader(device, &vsi);
    auto* fs = SDL_CreateGPUShader(device, &fsi);
    if (!vs || !fs) {
        if (vs) SDL_ReleaseGPUShader(device, vs);
        if (fs) SDL_ReleaseGPUShader(device, fs);
        disabled_ = true; return;
    }
    SDL_GPUVertexBufferDescription vbd{}; vbd.slot=0; vbd.pitch=sizeof(float)*5;
    vbd.input_rate=SDL_GPU_VERTEXINPUTRATE_VERTEX;
    SDL_GPUVertexAttribute attrs[2]{};
    attrs[0]={0,0,SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,0};
    attrs[1]={1,0,SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,sizeof(float)*3};
    SDL_GPUVertexInputState vis{}; vis.vertex_buffer_descriptions=&vbd; vis.num_vertex_buffers=1;
    vis.vertex_attributes=attrs; vis.num_vertex_attributes=2;
    SDL_GPUColorTargetDescription ctd{};
    ctd.format = window ? SDL_GetGPUSwapchainTextureFormat(device,window)
                        : SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
    ctd.blend_state.enable_blend=true;
    ctd.blend_state.src_color_blendfactor=SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    ctd.blend_state.dst_color_blendfactor=SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    ctd.blend_state.color_blend_op=SDL_GPU_BLENDOP_ADD;
    ctd.blend_state.src_alpha_blendfactor=SDL_GPU_BLENDFACTOR_ONE;
    ctd.blend_state.dst_alpha_blendfactor=SDL_GPU_BLENDFACTOR_ZERO;
    ctd.blend_state.alpha_blend_op=SDL_GPU_BLENDOP_ADD;
    SDL_GPUGraphicsPipelineCreateInfo pci{};
    pci.vertex_shader=vs; pci.fragment_shader=fs; pci.vertex_input_state=vis;
    pci.primitive_type=SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pci.rasterizer_state.fill_mode=SDL_GPU_FILLMODE_FILL;
    pci.rasterizer_state.cull_mode=SDL_GPU_CULLMODE_NONE;
    pci.rasterizer_state.front_face=SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    pci.depth_stencil_state.enable_depth_test=false;
    pci.depth_stencil_state.enable_depth_write=false;
    pci.target_info.num_color_targets=1;
    pci.target_info.color_target_descriptions=&ctd;
    pipeline_=SDL_CreateGPUGraphicsPipeline(device,&pci);
    SDL_ReleaseGPUShader(device,vs); SDL_ReleaseGPUShader(device,fs);
    if (!pipeline_) { disabled_=true; return; }

    SDL_GPUTextureCreateInfo tci{}; tci.type=SDL_GPU_TEXTURETYPE_2D;
    tci.format=SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    tci.width=kW; tci.height=kH; tci.layer_count_or_depth=1; tci.num_levels=1;
    tci.usage=SDL_GPU_TEXTUREUSAGE_SAMPLER;
    tex_=SDL_CreateGPUTexture(device,&tci);
    SDL_GPUTransferBufferCreateInfo tbci{}; tbci.usage=SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbci.size=kW*kH*4; transfer_=SDL_CreateGPUTransferBuffer(device,&tbci);
    SDL_GPUBufferCreateInfo bci{}; bci.usage=SDL_GPU_BUFFERUSAGE_VERTEX;
    bci.size=6u*5u*(uint32_t)sizeof(float); vtx_buf_=SDL_CreateGPUBuffer(device,&bci);
    SDL_GPUSamplerCreateInfo sci{}; sci.min_filter=SDL_GPU_FILTER_NEAREST;
    sci.mag_filter=SDL_GPU_FILTER_NEAREST; sci.mipmap_mode=SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sci.address_mode_u=SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sci.address_mode_v=SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_=SDL_CreateGPUSampler(device,&sci);
    ready_ = tex_ && transfer_ && vtx_buf_ && sampler_ && pipeline_;
}

// ---------------------------------------------------------------------------
// Blit the 3D head portrait GPU texture into the face-rect position.
// Uses the same pipeline as the overlay quad but with a small dynamic quad.
// ---------------------------------------------------------------------------
void WorkflowOverlaySwEndStep::BlitHeadPortrait(
        SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* swapchain,
        SDL_GPUTexture* headTex, WorkflowContext& context) {
    if (!pipeline_ || !headTex) return;

    // Lazily create a nearest-neighbour sampler for the head portrait
    if (!head_sampler_) {
        SDL_GPUSamplerCreateInfo sci{};
        sci.min_filter  = SDL_GPU_FILTER_LINEAR;
        sci.mag_filter  = SDL_GPU_FILTER_LINEAR;
        sci.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        sci.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        sci.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        head_sampler_ = SDL_CreateGPUSampler(device_, &sci);
        if (!head_sampler_) return;
    }

    // Build a 6-vertex quad from the face rect stored in context by q3.hud_head_render.
    // Positions are in overlay NDC: x ∈ [-1,1], y ∈ [-1,1] (Y-up, same as the fullscreen quad).
    const float fx = context.Get<float>("hud.face_rect_x", 354.f);
    const float fy = context.Get<float>("hud.face_rect_y", 322.f);
    const float fw = context.Get<float>("hud.face_rect_w",  32.f);
    const float fh = context.Get<float>("hud.face_rect_h",  32.f);

    // Pixel → NDC using the fixed overlay resolution (640×360)
    const float x0 =  2.f * fx        / kW - 1.f;
    const float x1 =  2.f * (fx + fw) / kW - 1.f;
    const float y1 =  1.f - 2.f * fy        / kH;  // top edge (larger y in NDC)
    const float y0 =  1.f - 2.f * (fy + fh) / kH;  // bottom edge

    const float verts[6][5] = {
        {x0, y1, 0.f, 0.f, 0.f},
        {x1, y1, 0.f, 1.f, 0.f},
        {x1, y0, 0.f, 1.f, 1.f},
        {x0, y1, 0.f, 0.f, 0.f},
        {x1, y0, 0.f, 1.f, 1.f},
        {x0, y0, 0.f, 0.f, 1.f},
    };
    const uint32_t sz = sizeof(verts);

    // Lazily create / reallocate head vertex buffer (created once — rect is stable)
    if (!head_vtx_buf_) {
        SDL_GPUBufferCreateInfo bci{};
        bci.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        bci.size  = sz;
        head_vtx_buf_ = SDL_CreateGPUBuffer(device_, &bci);
        if (!head_vtx_buf_) return;
    }

    // Upload quad vertices via a transient transfer buffer
    SDL_GPUTransferBufferCreateInfo tbci{};
    tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbci.size  = sz;
    auto* tmp = SDL_CreateGPUTransferBuffer(device_, &tbci);
    if (!tmp) return;
    void* ptr = SDL_MapGPUTransferBuffer(device_, tmp, false);
    if (ptr) { std::memcpy(ptr, verts, sz); SDL_UnmapGPUTransferBuffer(device_, tmp); }
    auto* cp = SDL_BeginGPUCopyPass(cmd);
    if (cp) {
        SDL_GPUTransferBufferLocation src{tmp, 0};
        SDL_GPUBufferRegion dst{head_vtx_buf_, 0, sz};
        SDL_UploadToGPUBuffer(cp, &src, &dst, false);
        SDL_EndGPUCopyPass(cp);
    }
    SDL_ReleaseGPUTransferBuffer(device_, tmp);

    // Render pass on swapchain (LOADOP_LOAD preserves previous content)
    SDL_GPUColorTargetInfo target{};
    target.texture  = swapchain;
    target.load_op  = SDL_GPU_LOADOP_LOAD;
    target.store_op = SDL_GPU_STOREOP_STORE;
    auto* pass = SDL_BeginGPURenderPass(cmd, &target, 1, nullptr);
    if (!pass) return;

    SDL_BindGPUGraphicsPipeline(pass, pipeline_);
    SDL_GPUBufferBinding vb{head_vtx_buf_, 0};
    SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
    SDL_GPUTextureSamplerBinding ts{headTex, head_sampler_};
    SDL_BindGPUFragmentSamplers(pass, 0, &ts, 1);
    SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
}

void WorkflowOverlaySwEndStep::Execute(
        const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;
    if (!context.GetBool("overlay.ready", false)) return;

    auto* renderer  = context.Get<SDL_Renderer*>("overlay.renderer", nullptr);
    auto* surface   = context.Get<SDL_Surface*> ("overlay.surface",  nullptr);
    auto* cmd       = context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* swapchain = context.Get<SDL_GPUTexture*>("gpu_swapchain_texture", nullptr);
    auto* device    = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!surface || !cmd || !swapchain || !device) return;

    if (renderer) SDL_RenderPresent(renderer);

    if (!ready_ && !disabled_) {
        WorkflowStepParameterResolver params;
        auto getStr = [&](const char* k, const char* def) -> std::string {
            const auto* p = params.FindParameter(step, k);
            return (p && p->type == WorkflowParameterValue::Type::String)
                   ? p->stringValue : def;
        };
        const char* driver = SDL_GetGPUDeviceDriver(device);
        const std::string drv = driver ? driver : "";
        std::string vp, fp;
        if (drv == "metal") {
            vp = getStr("vert_shader_path_msl",
                        "packages/quake3/shaders/msl/overlay.vert.metal");
            fp = getStr("frag_shader_path_msl",
                        "packages/quake3/shaders/msl/overlay.frag.metal");
        } else {
            vp = getStr("vert_shader_path_spirv",
                        "packages/quake3/shaders/spirv/overlay.vert.spv");
            fp = getStr("frag_shader_path_spirv",
                        "packages/quake3/shaders/spirv/overlay.frag.spv");
        }
        TryInit(device, context.Get<SDL_Window*>("sdl_window", nullptr), vp, fp);
    }
    if (!ready_) return;

    // Upload surface pixels to GPU texture
    void* mapped = SDL_MapGPUTransferBuffer(device, transfer_, false);
    if (!mapped) return;
    std::memcpy(mapped, surface->pixels, kW * kH * 4);
    SDL_UnmapGPUTransferBuffer(device, transfer_);
    auto* copy = SDL_BeginGPUCopyPass(cmd);
    if (copy) {
        SDL_GPUTextureTransferInfo src{}; src.transfer_buffer=transfer_;
        src.pixels_per_row=kW; src.rows_per_layer=kH;
        SDL_GPUTextureRegion dst{}; dst.texture=tex_; dst.w=kW; dst.h=kH; dst.d=1;
        SDL_UploadToGPUTexture(copy, &src, &dst, false);
        SDL_EndGPUCopyPass(copy);
    }
    // Upload vertex buffer once
    if (!vbuf_uploaded_) {
        const float verts[6][5]={{-1,1,0,0,0},{1,1,0,1,0},{1,-1,0,1,1},
                                  {-1,1,0,0,0},{1,-1,0,1,1},{-1,-1,0,0,1}};
        const uint32_t sz=sizeof(verts);
        SDL_GPUTransferBufferCreateInfo tb{}; tb.usage=SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD; tb.size=sz;
        auto* tmp=SDL_CreateGPUTransferBuffer(device,&tb);
        if (tmp) {
            void* ptr=SDL_MapGPUTransferBuffer(device,tmp,false);
            if (ptr) { std::memcpy(ptr,verts,sz); SDL_UnmapGPUTransferBuffer(device,tmp); }
            auto* cp=SDL_BeginGPUCopyPass(cmd);
            if (cp) {
                SDL_GPUTransferBufferLocation s{tmp,0};
                SDL_GPUBufferRegion d{vtx_buf_,0,sz};
                SDL_UploadToGPUBuffer(cp,&s,&d,false);
                SDL_EndGPUCopyPass(cp);
            }
            SDL_ReleaseGPUTransferBuffer(device,tmp);
        }
        vbuf_uploaded_=true;
    }
    // Render full-screen quad
    SDL_GPUColorTargetInfo target{}; target.texture=swapchain;
    target.load_op=SDL_GPU_LOADOP_LOAD; target.store_op=SDL_GPU_STOREOP_STORE;
    auto* pass=SDL_BeginGPURenderPass(cmd,&target,1,nullptr);
    if (!pass) return;
    SDL_BindGPUGraphicsPipeline(pass,pipeline_);
    SDL_GPUBufferBinding vb{vtx_buf_,0}; SDL_BindGPUVertexBuffers(pass,0,&vb,1);
    SDL_GPUTextureSamplerBinding ts{tex_,sampler_}; SDL_BindGPUFragmentSamplers(pass,0,&ts,1);
    SDL_DrawGPUPrimitives(pass,6,1,0,0);
    SDL_EndGPURenderPass(pass);

    // Blit 3D head portrait (rendered by q3.hud_head_render) over the HUD face rect
    auto* headTex = context.Get<SDL_GPUTexture*>("overlay.head_gpu_tex", nullptr);
    if (headTex) BlitHeadPortrait(cmd, swapchain, headTex, context);

    // Screenshot
    const auto* ssPath = context.TryGet<std::string>("screenshot_output_path");
    if (ssPath && !ssPath->empty()) {
        if (SDL_SaveBMP(surface, ssPath->c_str()))
            context.Set<bool>("screenshot_saved", true);
        context.Set<std::string>("screenshot_output_path", std::string(""));
    }
}
} // namespace sdl3cpp::services::impl
