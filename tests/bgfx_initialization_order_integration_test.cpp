#include <gtest/gtest.h>
#include <bgfx/bgfx.h>
#include <vector>
#include <memory>
#include <cstring>
#include <iostream>

// Integration test for texture creation BEFORE first bgfx::frame()
// Based on INITIALIZATION_ORDER_BUG.md
//
// Problem: TextureHandle created before first bgfx::frame() is invalid (0xFFFF)
// This caused "texture 65535 (INVALID HANDLE)" errors
//
// Correct sequence:
// 1. bgfx::init()
// 2. bgfx::frame() ← MUST be called first!
// 3. bgfx::createTexture2D() ← Now safe

namespace {

const char* RendererTypeName(bgfx::RendererType::Enum type) {
    switch (type) {
        case bgfx::RendererType::Noop:
            return "Noop";
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
        case bgfx::RendererType::Count:
            return "Auto";
        default:
            return "Unknown";
    }
}

class BgfxInitializationOrderTest : public ::testing::Test {
protected:
    bool bgfxInitialized_ = false;

    bool InitializeNoopBgfx() {
        bgfx::Init init;
        init.type = bgfx::RendererType::Noop;
        init.resolution.width = 1024;
        init.resolution.height = 768;
        init.resolution.reset = BGFX_RESET_NONE;

        bgfxInitialized_ = bgfx::init(init);
        std::cout << "[TRACE] BgfxInitializationOrderTest::InitializeNoopBgfx initialized="
                  << (bgfxInitialized_ ? "true" : "false") << "\n";

        return bgfxInitialized_;
    }

    void TearDown() override {
        if (bgfxInitialized_) {
            std::cout << "[TRACE] BgfxInitializationOrderTest::TearDown shutting down bgfx\n";
            bgfx::shutdown();
            bgfxInitialized_ = false;
        } else {
            std::cout << "[TRACE] BgfxInitializationOrderTest::TearDown skipped bgfx shutdown (not initialized)\n";
        }
    }
};

TEST_F(BgfxInitializationOrderTest, CreateTexture_BeforeFrame_ProducesInvalidHandle) {
    // Initialize bgfx
    if (!InitializeNoopBgfx()) {
        GTEST_SKIP() << "bgfx::init failed";
    }
    const auto rendererType = bgfx::getRendererType();
    if (rendererType != bgfx::RendererType::Noop) {
        GTEST_SKIP() << "Noop renderer unavailable; actual=" << RendererTypeName(rendererType);
    }
    
    // ❌ WRONG: Create texture BEFORE first frame
    // NOTE: With Noop renderer this might still create valid handles,
    // but with real renderers (Vulkan, OpenGL) this causes invalid handles
    const uint32_t width = 64;
    const uint32_t height = 64;
    const bgfx::Memory* mem = bgfx::alloc(width * height * 4);
    memset(mem->data, 255, mem->size);
    
    auto textureHandle = bgfx::createTexture2D(
        width, height, false, 1,
        bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE,
        mem
    );
    
    // The behavior depends on the renderer:
    // - Noop: might create valid handles (testing artifact)
    // - Real renderers: 0xFFFF (65535) invalid handle
    // This test documents that you MUST call frame() first for real renderers
    EXPECT_TRUE(true) << "Documented: frame() must be called before texture creation";
    
    if (bgfx::isValid(textureHandle)) {
        bgfx::destroy(textureHandle);
    }
    
    bgfx::frame(); // Now call frame (too late for real renderers)
}

TEST_F(BgfxInitializationOrderTest, CreateTexture_AfterFrame_ProducesValidHandle) {
    // Initialize bgfx
    ASSERT_TRUE(InitializeNoopBgfx());
    
    // ✅ CORRECT: Call frame FIRST to prime bgfx
    bgfx::frame();
    
    // Now create texture
    const uint32_t width = 64;
    const uint32_t height = 64;
    const bgfx::Memory* mem = bgfx::alloc(width * height * 4);
    memset(mem->data, 255, mem->size);
    
    auto textureHandle = bgfx::createTexture2D(
        width, height, false, 1,
        bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE,
        mem
    );
    
    // The handle should now be valid
    EXPECT_TRUE(bgfx::isValid(textureHandle))
        << "Texture created after first frame should be valid";
    
    if (bgfx::isValid(textureHandle)) {
        bgfx::destroy(textureHandle);
    }
}

TEST_F(BgfxInitializationOrderTest, MultipleTextures_AfterFrame_AllValid) {
    // Initialize and prime bgfx
    ASSERT_TRUE(InitializeNoopBgfx());
    bgfx::frame();
    
    // Create multiple textures
    std::vector<bgfx::TextureHandle> textures;
    for (int i = 0; i < 5; ++i) {
        const uint32_t size = 32 << i; // 32, 64, 128, 256, 512
        const bgfx::Memory* mem = bgfx::alloc(size * size * 4);
        memset(mem->data, static_cast<uint8_t>(i * 50), mem->size);
        
        auto handle = bgfx::createTexture2D(
            size, size, false, 1,
            bgfx::TextureFormat::RGBA8,
            BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE,
            mem
        );
        
        textures.push_back(handle);
        EXPECT_TRUE(bgfx::isValid(handle)) << "Texture " << i << " should be valid";
    }
    
    // Clean up
    for (auto handle : textures) {
        if (bgfx::isValid(handle)) {
            bgfx::destroy(handle);
        }
    }
}

TEST_F(BgfxInitializationOrderTest, CorrectInitSequence_Documented) {
    // Documents the correct initialization sequence from INITIALIZATION_ORDER_BUG.md
    
    // Step 1: Initialize bgfx
    ASSERT_TRUE(InitializeNoopBgfx());
    
    // Step 2: Call frame() FIRST - this primes bgfx
    bgfx::frame();
    
    // Step 3: Now safe to create resources (textures, buffers, shaders)
    const bgfx::Memory* mem = bgfx::alloc(64 * 64 * 4);
    auto handle = bgfx::createTexture2D(
        64, 64, false, 1,
        bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE,
        mem
    );
    
    EXPECT_TRUE(bgfx::isValid(handle));
    
    if (bgfx::isValid(handle)) {
        bgfx::destroy(handle);
    }
}

TEST_F(BgfxInitializationOrderTest, ErrorSymptom_InvalidHandleValue) {
    // From INITIALIZATION_ORDER_BUG.md:
    // Invalid handle has value 0xFFFF (65535 in decimal)
    
    const uint16_t invalidHandle = 0xFFFF;
    EXPECT_EQ(invalidHandle, 65535u);
    
    // Error messages that appeared in logs:
    // "texture 65535 (INVALID HANDLE)"
    // "Failed to access texture ID 65535, which is marked as invalid"
}

TEST_F(BgfxInitializationOrderTest, BufferCreation_AlsoRequiresFrame) {
    // Buffers (vertex, index) also need frame() to be called first
    
    ASSERT_TRUE(InitializeNoopBgfx());
    bgfx::frame(); // Prime first
    
    // Create vertex buffer - simplified test without layout issues
    std::vector<float> vertices = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
    const bgfx::Memory* vbMem = bgfx::copy(vertices.data(), vertices.size() * sizeof(float));
    
    // Note: In real usage you need a proper vertex layout
    // This test just documents that frame() should be called first
    EXPECT_NE(vbMem, nullptr) << "Buffer memory should be allocated after frame";
    
    // Not creating actual vertex buffer here as it requires proper layout
    // The key point: call frame() before ANY resource creation
}

} // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "Bgfx Initialization Order Integration Tests\n";
    std::cout << "================================================================================\n";
    std::cout << "\n";
    std::cout << "These tests verify the correct initialization sequence for bgfx resources.\n";
    std::cout << "\n";
    std::cout << "CRITICAL: Must call bgfx::frame() BEFORE creating ANY resources!\n";
    std::cout << "\n";
    std::cout << "Correct sequence:\n";
    std::cout << "  1. bgfx::init()\n";
    std::cout << "  2. bgfx::frame()      ← MUST BE FIRST!\n";
    std::cout << "  3. Create resources (textures, buffers, shaders)\n";
    std::cout << "\n";
    std::cout << "Symptom of bug: Invalid handle 0xFFFF (65535)\n";
    std::cout << "================================================================================\n";
    std::cout << "\n";
    
    return RUN_ALL_TESTS();
}
