#pragma once

#include <nlohmann/json.hpp>
#include <bgfx/bgfx.h>
#include <bx/math.h>
#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>
#include <iostream>
#include <fstream>

using json = nlohmann::json;

// ============================================================================
// WORKFLOW CONTEXT - Shared state between workflow steps
// ============================================================================
class WorkflowContext {
public:
    std::unordered_map<std::string, std::string> stringData;
    std::unordered_map<std::string, float> floatData;
    std::unordered_map<std::string, int> intData;
    std::unordered_map<std::string, void*> ptrData;

    SDL_Window* window = nullptr;
    bgfx::VertexBufferHandle vbh = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle ibh = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
    bgfx::VertexLayoutHandle layoutHandle = BGFX_INVALID_HANDLE;

    float view[16] = {};
    float proj[16] = {};
    float time = 0.0f;
    uint32_t frameCount = 0;
    bool running = true;
};

// ============================================================================
// WORKFLOW STEP INTERFACE
// ============================================================================
class IWorkflowStep {
public:
    virtual ~IWorkflowStep() = default;
    virtual void Execute(const json& nodeConfig, const json& variables, WorkflowContext& context) = 0;
};

// ============================================================================
// STEP IMPLEMENTATIONS
// ============================================================================

class GraphicsInitStep : public IWorkflowStep {
    void Execute(const json& nodeConfig, const json& variables, WorkflowContext& context) override {
        auto params = nodeConfig["parameters"];

        uint32_t width = params["window_width"].is_string()
            ? variables[extractVar(params["window_width"])]["value"].get<uint32_t>()
            : params["window_width"].get<uint32_t>();
        uint32_t height = params["window_height"].is_string()
            ? variables[extractVar(params["window_height"])]["value"].get<uint32_t>()
            : params["window_height"].get<uint32_t>();
        std::string title = params["window_title"].get<std::string>();

        std::cout << "[WORKFLOW] graphics.init: Creating window " << width << "x" << height << std::endl;

        if (!SDL_Init(SDL_INIT_VIDEO)) {
            throw std::runtime_error("SDL_Init failed");
        }

        context.window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_RESIZABLE);
        if (!context.window) {
            throw std::runtime_error("SDL_CreateWindow failed");
        }

        bgfx::PlatformData pd{};
#if defined(__APPLE__)
        SDL_PropertiesID props = SDL_GetWindowProperties(context.window);
        pd.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
#elif defined(__linux__)
        SDL_PropertiesID props = SDL_GetWindowProperties(context.window);
        pd.ndt = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
        Sint64 x11Window = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
        pd.nwh = reinterpret_cast<void*>(static_cast<uintptr_t>(x11Window));
#elif defined(_WIN32)
        SDL_PropertiesID props = SDL_GetWindowProperties(context.window);
        pd.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#endif

        bgfx::setPlatformData(pd);
        bgfx::renderFrame();

        bgfx::Init init;
        init.platformData = pd;
        init.resolution.width = width;
        init.resolution.height = height;
        init.resolution.reset = BGFX_RESET_VSYNC;
        init.debug = true;
        init.profile = true;

        if (!bgfx::init(init)) {
            throw std::runtime_error("bgfx::init failed");
        }

        std::cout << "[WORKFLOW] ✓ Graphics initialized" << std::endl;
    }

private:
    std::string extractVar(const std::string& varStr) {
        // Extract variable name from "${variables.xxx}"
        size_t start = varStr.find("variables.") + 10;
        size_t end = varStr.find("}", start);
        return varStr.substr(start, end - start);
    }
};

class GeometryCreateCubeStep : public IWorkflowStep {
    void Execute(const json& nodeConfig, const json& variables, WorkflowContext& context) override {
        std::cout << "[WORKFLOW] geometry.create_cube: Creating 8-vertex cube mesh" << std::endl;

        struct PosColorVertex {
            float x, y, z;
            uint32_t abgr;
        };

        auto params = nodeConfig["parameters"];
        auto colorValues = params["vertex_colors"];

        std::vector<uint32_t> colors;
        for (const auto& colorStr : variables[extractVar(colorValues)]["value"]) {
            colors.push_back(std::stoul(colorStr.get<std::string>(), nullptr, 0));
        }

        static const PosColorVertex s_cubeVertices[] = {
            {-1.0f,  1.0f,  1.0f, 0},
            { 1.0f,  1.0f,  1.0f, 0},
            {-1.0f, -1.0f,  1.0f, 0},
            { 1.0f, -1.0f,  1.0f, 0},
            {-1.0f,  1.0f, -1.0f, 0},
            { 1.0f,  1.0f, -1.0f, 0},
            {-1.0f, -1.0f, -1.0f, 0},
            { 1.0f, -1.0f, -1.0f, 0},
        };

        static const uint16_t s_cubeIndices[] = {
            0, 1, 2,  2, 1, 3,
            4, 6, 5,  5, 6, 7,
            0, 2, 4,  4, 2, 6,
            1, 5, 3,  5, 7, 3,
            0, 4, 1,  4, 5, 1,
            2, 3, 6,  6, 3, 7,
        };

        bgfx::VertexLayout layout;
        layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();

        const bgfx::Memory* vbMem = bgfx::copy(s_cubeVertices, sizeof(s_cubeVertices));
        context.vbh = bgfx::createVertexBuffer(vbMem, layout);

        const bgfx::Memory* ibMem = bgfx::copy(s_cubeIndices, sizeof(s_cubeIndices));
        context.ibh = bgfx::createIndexBuffer(ibMem);

        std::cout << "[WORKFLOW] ✓ Cube geometry created" << std::endl;
    }

private:
    std::string extractVar(const std::string& varStr) {
        size_t start = varStr.find("variables.") + 10;
        size_t end = varStr.find("}", start);
        return varStr.substr(start, end - start);
    }
};

class ShaderLoadBinaryStep : public IWorkflowStep {
    void Execute(const json& nodeConfig, const json& variables, WorkflowContext& context) override {
        auto params = nodeConfig["parameters"];
        std::string vsFilename = params["vertex_shader_path"].get<std::string>();
        std::string fsFilename = params["fragment_shader_path"].get<std::string>();

        std::cout << "[WORKFLOW] shader.load_binary: Loading " << vsFilename << " and " << fsFilename << std::endl;

        const bgfx::Memory* vsMem = loadShaderWithSearchPaths(vsFilename);
        const bgfx::Memory* fsMem = loadShaderWithSearchPaths(fsFilename);

        if (!vsMem || !fsMem) {
            throw std::runtime_error("Failed to load one or more shaders");
        }

        bgfx::ShaderHandle vsh = bgfx::createShader(vsMem);
        bgfx::ShaderHandle fsh = bgfx::createShader(fsMem);
        context.program = bgfx::createProgram(vsh, fsh, true);

        if (!bgfx::isValid(context.program)) {
            throw std::runtime_error("Failed to create shader program");
        }

        std::cout << "[WORKFLOW] ✓ Shaders loaded and program created" << std::endl;
    }

private:
    const bgfx::Memory* loadShaderWithSearchPaths(const std::string& filename) {
        // Search in multiple locations
        std::vector<std::string> searchPaths = {
            "",  // Current directory
            "./",
            "../",
            "build/",
            "/Users/rmac/Documents/metabuilder/gameengine/experiment/",
        };

        for (const auto& basePath : searchPaths) {
            std::string fullPath = basePath + filename;
            std::ifstream file(fullPath, std::ios::binary | std::ios::ate);

            if (file.is_open()) {
                std::streamsize size = file.tellg();
                file.seekg(0, std::ios::beg);

                const bgfx::Memory* mem = bgfx::alloc(uint32_t(size + 1));
                if (file.read((char*)mem->data, size)) {
                    mem->data[mem->size - 1] = '\0';
                    std::cout << "   Loaded shader from: " << fullPath << std::endl;
                    return mem;
                }
            }
        }

        std::cerr << "Failed to find shader file: " << filename << std::endl;
        std::cerr << "Searched paths:" << std::endl;
        for (const auto& path : searchPaths) {
            std::cerr << "  - " << (path.empty() ? "." : path) << std::endl;
        }
        return nullptr;
    }
};

class CameraSetupStep : public IWorkflowStep {
    void Execute(const json& nodeConfig, const json& variables, WorkflowContext& context) override {
        auto params = nodeConfig["parameters"];

        float distance = getFloat(params["camera_distance"], variables);
        float fov = getFloat(params["camera_fov"], variables);
        float width = getFloat(variables["window_width"], variables);
        float height = getFloat(variables["window_height"], variables);
        float aspectRatio = width / height;

        std::cout << "[WORKFLOW] camera.setup: FOV=" << fov << " distance=" << distance
                  << " aspect=" << aspectRatio << std::endl;

        bx::mtxLookAt(context.view,
            bx::Vec3(0.0f, 0.0f, -distance),
            bx::Vec3(0.0f, 0.0f, 0.0f),
            bx::Vec3(0.0f, 1.0f, 0.0f)
        );

        bx::mtxProj(context.proj, fov, aspectRatio, 0.1f, 100.0f,
                    bgfx::getCaps()->homogeneousDepth);

        std::cout << "[WORKFLOW] ✓ Camera setup complete" << std::endl;
    }

private:
    float getFloat(const json& val, const json& variables) {
        if (val.is_string()) {
            std::string varStr = val.get<std::string>();
            size_t start = varStr.find("variables.") + 10;
            size_t end = varStr.find("}", start);
            std::string varName = varStr.substr(start, end - start);
            return variables[varName]["value"].get<float>();
        }
        return val.get<float>();
    }
};

class RenderCubeGridStep : public IWorkflowStep {
    void Execute(const json& nodeConfig, const json& variables, WorkflowContext& context) override {
        auto params = nodeConfig["parameters"];

        uint32_t gridWidth = getInt(params["grid_width"], variables);
        uint32_t gridHeight = getInt(params["grid_height"], variables);
        float spacing = getFloat(params["grid_spacing"], variables);
        float startX = getFloat(params["grid_start_x"], variables);
        float startY = getFloat(params["grid_start_y"], variables);
        float rotOffsetX = getFloat(params["rotation_offset_x"], variables);
        float rotOffsetY = getFloat(params["rotation_offset_y"], variables);
        uint32_t numFrames = getInt(params["num_frames"], variables);

        std::cout << "[WORKFLOW] render.cube_grid: " << gridWidth << "x" << gridHeight
                  << " grid, " << numFrames << " frames" << std::endl;

        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
        bgfx::setViewRect(0, 0, 0, 1280, 720);

        bool running = true;
        SDL_Event event;

        for (uint32_t frame = 0; frame < numFrames && running; ++frame) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT ||
                    (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
                    running = false;
                }
            }

            context.time += 1.0f / 60.0f;
            bgfx::setViewTransform(0, context.view, context.proj);

            for (uint32_t yy = 0; yy < gridHeight; ++yy) {
                for (uint32_t xx = 0; xx < gridWidth; ++xx) {
                    float mtx[16];
                    bx::mtxRotateXY(mtx, context.time + xx * rotOffsetX,
                                   context.time + yy * rotOffsetY);
                    mtx[12] = startX + float(xx) * spacing;
                    mtx[13] = startY + float(yy) * spacing;
                    mtx[14] = 0.0f;

                    bgfx::setTransform(mtx);
                    bgfx::setVertexBuffer(0, context.vbh);
                    bgfx::setIndexBuffer(context.ibh);
                    bgfx::setState(BGFX_STATE_DEFAULT);
                    bgfx::submit(0, context.program);
                }
            }

            bgfx::frame();
            context.frameCount++;

            if (context.frameCount % 300 == 0) {
                std::cout << "[WORKFLOW] Frame " << context.frameCount << std::endl;
            }
        }

        std::cout << "[WORKFLOW] ✓ Render complete: " << context.frameCount << " frames" << std::endl;
    }

private:
    float getFloat(const json& val, const json& variables) {
        if (val.is_string()) {
            std::string varStr = val.get<std::string>();
            size_t start = varStr.find("variables.") + 10;
            size_t end = varStr.find("}", start);
            std::string varName = varStr.substr(start, end - start);
            return variables[varName]["value"].get<float>();
        }
        return val.get<float>();
    }

    uint32_t getInt(const json& val, const json& variables) {
        if (val.is_string()) {
            std::string varStr = val.get<std::string>();
            size_t start = varStr.find("variables.") + 10;
            size_t end = varStr.find("}", start);
            std::string varName = varStr.substr(start, end - start);
            return variables[varName]["value"].get<uint32_t>();
        }
        return val.get<uint32_t>();
    }
};

class ExitAppStep : public IWorkflowStep {
    void Execute(const json& nodeConfig, const json& variables, WorkflowContext& context) override {
        auto params = nodeConfig["parameters"];
        std::string message = params["message"].get<std::string>();

        std::cout << "[WORKFLOW] system.exit: " << message << std::endl;

        bgfx::destroy(context.vbh);
        bgfx::destroy(context.ibh);
        bgfx::destroy(context.program);
        bgfx::shutdown();

        if (context.window) {
            SDL_DestroyWindow(context.window);
        }
        SDL_Quit();
    }
};

// ============================================================================
// WORKFLOW ENGINE
// ============================================================================
class WorkflowEngine {
public:
    WorkflowEngine() {
        registerStep("graphics.init", std::make_shared<GraphicsInitStep>());
        registerStep("geometry.create_cube", std::make_shared<GeometryCreateCubeStep>());
        registerStep("shader.load_binary", std::make_shared<ShaderLoadBinaryStep>());
        registerStep("camera.setup", std::make_shared<CameraSetupStep>());
        registerStep("render.cube_grid", std::make_shared<RenderCubeGridStep>());
        registerStep("system.exit", std::make_shared<ExitAppStep>());
    }

    void registerStep(const std::string& type, std::shared_ptr<IWorkflowStep> step) {
        steps_[type] = step;
    }

    void Execute(const std::string& workflowPath) {
        std::ifstream file(workflowPath);
        if (!file) {
            throw std::runtime_error("Failed to open workflow: " + workflowPath);
        }

        json workflow = json::parse(file);
        json variables = workflow["variables"];
        json nodes = workflow["nodes"];

        std::cout << "\n[WORKFLOW ENGINE] Starting: " << workflow["name"].get<std::string>() << std::endl;
        std::cout << "[WORKFLOW ENGINE] Nodes to execute: " << nodes.size() << std::endl;
        std::cout << "[WORKFLOW ENGINE] Variables loaded: " << variables.size() << std::endl << std::endl;

        WorkflowContext context;

        for (size_t i = 0; i < nodes.size(); ++i) {
            const auto& node = nodes[i];
            std::string type = node["type"].get<std::string>();
            std::string id = node["id"].get<std::string>();

            std::cout << "\n[WORKFLOW ENGINE] Step " << (i+1) << "/" << nodes.size()
                      << " - Executing: " << type << " (id=" << id << ")" << std::endl;

            auto it = steps_.find(type);
            if (it == steps_.end()) {
                throw std::runtime_error("Unknown step type: " + type);
            }

            try {
                it->second->Execute(node, variables, context);
            } catch (const std::exception& e) {
                std::cerr << "\n[WORKFLOW ERROR] Step failed: " << e.what() << std::endl;
                throw;
            }
        }

        std::cout << "\n[WORKFLOW ENGINE] ✓ Workflow execution complete!" << std::endl;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<IWorkflowStep>> steps_;
};
