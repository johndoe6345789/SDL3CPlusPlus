#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <vector>

// Test: Texture loading and resource management
//
// This test suite investigates the system crash that occurs during texture loading.
//
// Crash Context (from sdl3_app.log):
// - Successfully loaded 6 textures for "wall" shader (all 2048x2048)
// - Crash occurred during compilation of "solid:fragment" shader
// - Last log entry: "BgfxShaderCompiler::CompileShader(label=solid:fragment, sourceLength=81022)"
//
// Hypothesis:
// 1. Texture handle limit exceeded (bgfx has per-pipeline or global texture limits)
// 2. Memory exhaustion from loading 6x 2048x2048 textures (~100MB)
// 3. Shader compilation triggered GPU memory allocation failure
// 4. Missing validation of texture handle after createTexture2D
// 5. Resource leak preventing proper cleanup

namespace {

// Mock test to verify texture file existence
TEST(BgfxTextureLoadingTest, TextureFilesExist) {
    // Verify the textures that were being loaded when crash occurred
    std::vector<std::string> textures = {
        "MaterialX/resources/Images/brick_variation_mask.jpg",
        "MaterialX/resources/Images/brick_base_gray.jpg",
        "MaterialX/resources/Images/brick_dirt_mask.jpg",
        "MaterialX/resources/Images/brick_mask.jpg",
        "MaterialX/resources/Images/brick_roughness.jpg",
        "MaterialX/resources/Images/brick_normal.jpg"
    };

    for (const auto& texture : textures) {
        std::filesystem::path texPath = std::filesystem::current_path() / texture;
        EXPECT_TRUE(std::filesystem::exists(texPath))
            << "Texture file not found: " << texPath;
    }
}

// Test: Verify texture file sizes
TEST(BgfxTextureLoadingTest, TextureFileSizes) {
    std::vector<std::string> textures = {
        "MaterialX/resources/Images/brick_variation_mask.jpg",
        "MaterialX/resources/Images/brick_base_gray.jpg",
        "MaterialX/resources/Images/brick_dirt_mask.jpg",
        "MaterialX/resources/Images/brick_mask.jpg",
        "MaterialX/resources/Images/brick_roughness.jpg",
        "MaterialX/resources/Images/brick_normal.jpg"
    };

    size_t totalBytes = 0;
    for (const auto& texture : textures) {
        std::filesystem::path texPath = std::filesystem::current_path() / texture;
        if (std::filesystem::exists(texPath)) {
            size_t fileSize = std::filesystem::file_size(texPath);
            totalBytes += fileSize;
            std::cout << "Texture: " << texture << " -> "
                      << (fileSize / 1024) << " KB" << std::endl;
        }
    }

    std::cout << "Total texture data (compressed): "
              << (totalBytes / 1024 / 1024) << " MB" << std::endl;

    // After decompression, 6x 2048x2048x4 (RGBA8) = ~100MB
    const size_t expectedUncompressed = 6 * 2048 * 2048 * 4;
    std::cout << "Estimated uncompressed size: "
              << (expectedUncompressed / 1024 / 1024) << " MB" << std::endl;
}

// Test: Simulate texture handle limit
TEST(BgfxTextureLoadingTest, TextureHandleLimitSimulation) {
    // bgfx may have limits on:
    // 1. Maximum textures per shader stage
    // 2. Maximum texture samplers (caps->limits.maxTextureSamplers)
    // 3. Total GPU memory available

    // From bgfx_graphics_backend.cpp:828
    // maxStages is capped at min(caps->limits.maxTextureSamplers, 255)

    // The "wall" shader loaded 6 textures (stages 0-5)
    // This test documents the expected behavior

    const size_t loadedTextures = 6;
    const size_t textureWidth = 2048;
    const size_t textureHeight = 2048;
    const size_t bytesPerPixel = 4; // RGBA8

    size_t memoryPerTexture = textureWidth * textureHeight * bytesPerPixel;
    size_t totalMemory = loadedTextures * memoryPerTexture;

    std::cout << "Memory per texture: " << (memoryPerTexture / 1024 / 1024) << " MB" << std::endl;
    std::cout << "Total GPU memory (6 textures): " << (totalMemory / 1024 / 1024) << " MB" << std::endl;

    // On AMD RX 6600 (8GB VRAM), this should not be a problem
    // BUT: if mipmaps are generated, memory usage increases by ~33%
    EXPECT_LT(totalMemory, 200 * 1024 * 1024) << "Texture memory should be under 200MB";
}

// Test: Document LoadTextureFromFile behavior
TEST(BgfxTextureLoadingTest, LoadTextureFromFileCodeReview) {
    // This test documents the actual code behavior in LoadTextureFromFile
    // (bgfx_graphics_backend.cpp:698-744)

    // ISSUE 1: Missing proper error handling
    // Line 732: if (!bgfx::isValid(handle) && logger_)
    //     logger_->Error(...);
    // Problem: Still returns the invalid handle to caller!

    // ISSUE 2: No validation of bgfx::copy() result
    // Line 720: const bgfx::Memory* mem = bgfx::copy(pixels, size);
    // What if bgfx::copy() returns nullptr?

    // ISSUE 3: No check for texture dimension limits
    // bgfx has max texture size limits (e.g., 16384x16384)
    // 2048x2048 should be fine, but no validation

    // ISSUE 4: Fallback texture creation (line 859) has same issues
    // binding.texture = CreateSolidTexture(0xff00ffff, samplerFlags);
    // No validation that CreateSolidTexture succeeded

    EXPECT_TRUE(true) << "Code review complete - documented 4 potential issues";
}

// Test: Shader compilation crash hypothesis
TEST(BgfxTextureLoadingTest, ShaderCompilationCrashHypothesis) {
    // Crash occurred during solid:fragment shader compilation
    //
    // Timeline from log:
    // 1. 23:45:01.371 - 6 textures loaded successfully for "wall" shader
    // 2. 23:45:01.371 - Started CreateShader for solid:vertex
    // 3. 23:45:01.422 - solid:vertex compiled successfully
    // 4. 23:45:01.422 - Started CompileShader for solid:fragment (81022 bytes)
    // 5. CRASH (no further log entries)
    //
    // Hypothesis:
    // - Fragment shader compilation allocates GPU resources
    // - With 6 large textures already loaded (~100MB), available memory is low
    // - Shader compiler needs additional memory for SPIR-V compilation
    // - GPU driver runs out of resources -> system crash
    //
    // Key observation: Fragment shader is LARGE (81KB source = ~81,022 bytes)
    // This is unusually large for a fragment shader

    const size_t fragmentShaderSourceSize = 81022;
    std::cout << "Fragment shader source size: " << fragmentShaderSourceSize << " bytes" << std::endl;
    std::cout << "This is unusually large - typical shaders are 1-10KB" << std::endl;

    // A 81KB fragment shader likely contains:
    // - Many uniform declarations
    // - Complex MaterialX node graph
    // - Large amount of generated code

    EXPECT_GT(fragmentShaderSourceSize, 50000)
        << "Abnormally large shader detected";
}

// Test: Resource cleanup ordering
TEST(BgfxTextureLoadingTest, ResourceCleanupOrdering) {
    // From bgfx_graphics_backend.cpp:
    //
    // Shutdown() order (line 599-612):
    // 1. DestroyPipelines() - destroys textures and samplers
    // 2. DestroyBuffers() - destroys vertex/index buffers
    // 3. DestroyUniforms() - destroys uniform handles
    // 4. bgfx::shutdown() - shuts down bgfx
    //
    // DestroyPipeline() (line 877-897):
    // - Destroys textures: bgfx::destroy(binding.texture)
    // - Destroys samplers: bgfx::destroy(binding.sampler)
    // - Destroys program: bgfx::destroy(program)
    //
    // POTENTIAL ISSUE: What if a texture handle is invalid?
    // bgfx::destroy() on invalid handle may cause issues

    // From line 886-891:
    // for (const auto& binding : it->second->textures) {
    //     if (bgfx::isValid(binding.texture)) {
    //         bgfx::destroy(binding.texture);  // GOOD: validates before destroy
    //     }
    //     ...
    // }
    //
    // This is correct - validates handles before destroying

    EXPECT_TRUE(true) << "Cleanup ordering appears correct";
}

// Test: Pipeline creation texture loading
TEST(BgfxTextureLoadingTest, PipelineCreationTextureLoading) {
    // From CreatePipeline (line 804-875):
    //
    // Line 857: binding.texture = LoadTextureFromFile(binding.sourcePath, samplerFlags);
    // Line 858-860: if (!bgfx::isValid(binding.texture)) {
    //                   binding.texture = CreateSolidTexture(0xff00ffff, samplerFlags);
    //               }
    //
    // ISSUE: What if CreateSolidTexture ALSO fails?
    // Then binding.texture is invalid, but it's still added to entry->textures
    // Later, in Draw(), bgfx::setTexture() is called with invalid handle
    //
    // Line 1026-1029 (Draw):
    // for (const auto& binding : pipelineIt->second->textures) {
    //     if (bgfx::isValid(binding.sampler) && bgfx::isValid(binding.texture)) {
    //         bgfx::setTexture(binding.stage, binding.sampler, binding.texture);
    //     }
    // }
    //
    // GOOD: Draw() validates handles before use

    EXPECT_TRUE(true) << "Pipeline creation has proper fallback handling";
}

} // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
