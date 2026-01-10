#include "services/impl/graphics/gxm_graphics_backend.hpp"
#include <iostream>
#include <memory>
#include <array>
#include <vector>

namespace {
constexpr std::array<float, 16> kIdentityMatrix = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
};

void Assert(bool condition, const std::string& message, int& failures) {
    if (!condition) {
        std::cerr << "test failure: " << message << '\n';
        ++failures;
    }
}
} // namespace

int main() {
    int failures = 0;
    std::cout << "Testing GXM Graphics Backend (stub implementation)\n";

    try {
        // Test backend creation
        auto backend = std::make_unique<sdl3cpp::services::impl::GxmGraphicsBackend>();
        Assert(backend != nullptr, "backend creation failed", failures);

        // Test initialization (this will use stub implementations)
        sdl3cpp::services::GraphicsConfig config;
        // GraphicsConfig doesn't have width/height/vsync in this implementation
        // config.width = 960;
        // config.height = 544;  // Vita resolution
        // config.vsync = true;

        backend->Initialize(nullptr, config);
        std::cout << "GXM backend initialized successfully\n";

        // Test device creation
        auto device = backend->CreateDevice();
        Assert(device != nullptr, "device creation failed", failures);
        std::cout << "GXM device created successfully\n";

        // Test pipeline creation
        sdl3cpp::services::ShaderPaths shaderPaths;
        shaderPaths.vertex = "inline:gui_2d.vert";
        shaderPaths.fragment = "inline:gui_2d.frag";
        shaderPaths.vertexSource = R"(
#version 450
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 0) out vec4 fragColor;
layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProj;
    mat4 view;
    mat4 proj;
    mat4 lightViewProj;
    vec3 cameraPos;
    float time;
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
        shaderPaths.fragmentSource = R"(
#version 450
layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;
void main() {
    outColor = fragColor;
}
)";
        auto pipeline = backend->CreatePipeline(device, "test_shader", shaderPaths);
        Assert(pipeline != nullptr, "pipeline creation failed", failures);
        std::cout << "GXM pipeline created successfully\n";

        // Test buffer creation
        std::vector<uint8_t> vertexData(36 * sizeof(float));  // Simple cube vertices
        auto vertexBuffer = backend->CreateVertexBuffer(device, vertexData);
        Assert(vertexBuffer != nullptr, "vertex buffer creation failed", failures);
        std::cout << "GXM vertex buffer created successfully\n";

        std::vector<uint8_t> indexData(36 * sizeof(uint16_t));  // Cube indices
        auto indexBuffer = backend->CreateIndexBuffer(device, indexData);
        Assert(indexBuffer != nullptr, "index buffer creation failed", failures);
        std::cout << "GXM index buffer created successfully\n";

        // Test frame operations
        bool frameStarted = backend->BeginFrame(device);
        Assert(frameStarted, "begin frame failed", failures);
        std::cout << "GXM frame started successfully\n";

        // Test view-projection setting
        backend->SetViewProjection(kIdentityMatrix);
        std::cout << "GXM view-projection set successfully\n";

        // Test draw call
        backend->Draw(device, pipeline, vertexBuffer, indexBuffer, 36, kIdentityMatrix);
        std::cout << "GXM draw call executed successfully\n";

        // Test end frame
        bool frameEnded = backend->EndFrame(device);
        Assert(frameEnded, "end frame failed", failures);
        std::cout << "GXM frame ended successfully\n";

        // Cleanup
        backend->DestroyBuffer(device, indexBuffer);
        backend->DestroyBuffer(device, vertexBuffer);
        backend->DestroyPipeline(device, pipeline);
        backend->DestroyDevice(device);
        backend->Shutdown();

        std::cout << "GXM backend cleanup completed\n";

    } catch (const std::exception& ex) {
        std::cerr << "exception during GXM backend tests: " << ex.what() << '\n';
        return 1;
    }

    if (failures == 0) {
        std::cout << "gxm_backend_tests: PASSED\n";
    } else {
        std::cerr << "gxm_backend_tests: FAILED (" << failures << " errors)\n";
    }

    return failures;
}
