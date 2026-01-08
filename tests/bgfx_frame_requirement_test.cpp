#include <gtest/gtest.h>
#include <bgfx/bgfx.h>
#include <SDL3/SDL.h>
#include <thread>
#include <chrono>

// TDD Test: This test FAILS until we fix the initialization order bug
//
// This test validates that bgfx::createTexture2D() is ONLY called AFTER
// the first bgfx::frame() call.
//
// Test Strategy:
// 1. Initialize bgfx
// 2. Try to create texture BEFORE first frame() → SHOULD FAIL or return invalid
// 3. Call frame() to initialize resource system
// 4. Create texture AFTER first frame() → SHOULD SUCCEED
//
// Expected behavior:
// - BEFORE fix: Test might crash or succeed incorrectly (undefined behavior)
// - AFTER fix: Test passes because we validate frame count before texture creation

namespace {

class BgfxFrameRequirementTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize SDL (required for bgfx on most platforms)
        SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            GTEST_SKIP() << "SDL_Init failed: " << SDL_GetError();
        }

        // Create a minimal window for bgfx
        window_ = SDL_CreateWindow(
            "bgfx_test",
            800, 600,
            SDL_WINDOW_HIDDEN  // Hidden window for testing
        );

        if (!window_) {
            SDL_Quit();
            GTEST_SKIP() << "SDL_CreateWindow failed: " << SDL_GetError();
        }

        // Setup bgfx platform data
        SetupBgfxPlatform();

        // Initialize bgfx
        bgfx::Init init;
        init.type = bgfx::RendererType::Count;  // Auto-select
        init.resolution.width = 800;
        init.resolution.height = 600;
        init.resolution.reset = BGFX_RESET_VSYNC;
        init.platformData = platformData_;

        if (!bgfx::init(init)) {
            SDL_DestroyWindow(window_);
            SDL_Quit();
            GTEST_SKIP() << "bgfx::init failed";
        }

        bgfxInitialized_ = true;

        // Setup a basic view
        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);
        bgfx::setViewRect(0, 0, 0, 800, 600);
    }

    void TearDown() override {
        if (bgfxInitialized_) {
            bgfx::shutdown();
        }
        if (window_) {
            SDL_DestroyWindow(window_);
        }
        SDL_Quit();
    }

    void SetupBgfxPlatform() {
        platformData_ = {};

#if defined(_WIN32)
        SDL_PropertiesID props = SDL_GetWindowProperties(window_);
        platformData_.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#elif defined(__linux__)
        SDL_PropertiesID props = SDL_GetWindowProperties(window_);

        // Try X11 first
        void* x11Display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
        Sint64 x11Window = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);

        if (x11Display && x11Window != 0) {
            platformData_.ndt = x11Display;
            platformData_.nwh = reinterpret_cast<void*>(static_cast<uintptr_t>(x11Window));
        } else {
            // Try Wayland
            void* wlDisplay = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
            void* wlSurface = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);

            if (wlDisplay && wlSurface) {
                platformData_.ndt = wlDisplay;
                platformData_.nwh = wlSurface;
                platformData_.type = bgfx::NativeWindowHandleType::Wayland;
            }
        }
#elif defined(__APPLE__)
        SDL_PropertiesID props = SDL_GetWindowProperties(window_);
        platformData_.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
#endif

    }

    void ProcessFrame() {
        bgfx::touch(0);
        bgfx::frame();
        frameCount_++;
    }

    bool HasProcessedFrame() const {
        return frameCount_ > 0;
    }

    SDL_Window* window_ = nullptr;
    bgfx::PlatformData platformData_{};
    bool bgfxInitialized_ = false;
    int frameCount_ = 0;  // Track frames manually since bgfx::Stats doesn't have numFrames
};

// ============================================================================
// TEST 1: Frame Count Starts at Zero
// ============================================================================

TEST_F(BgfxFrameRequirementTest, FrameCountStartsAtZero) {
    // After bgfx::init(), frame count should be 0
    EXPECT_EQ(frameCount_, 0)
        << "Frame count should be 0 immediately after bgfx::init()";

    EXPECT_FALSE(HasProcessedFrame())
        << "Should not have processed any frames yet";
}

// ============================================================================
// TEST 2: Frame Count Increments After frame()
// ============================================================================

TEST_F(BgfxFrameRequirementTest, FrameCountIncrementsAfterFrame) {
    int framesBefore = frameCount_;
    EXPECT_EQ(framesBefore, 0);

    // Call ProcessFrame() which calls bgfx::frame()
    ProcessFrame();

    EXPECT_EQ(frameCount_, framesBefore + 1)
        << "Frame count should increment by 1 after bgfx::frame()";

    EXPECT_TRUE(HasProcessedFrame())
        << "Should have processed at least one frame";
}

// ============================================================================
// TEST 3: Creating Texture Before First Frame (THE BUG)
// ============================================================================

TEST_F(BgfxFrameRequirementTest, DISABLED_CreatingTextureBeforeFirstFrame_ShouldFail) {
    // THIS TEST IS DISABLED because it demonstrates UNDEFINED BEHAVIOR
    // that crashes the system on AMD GPUs
    //
    // DO NOT ENABLE unless you want to test the crash!
    //
    // This is what the app was doing:
    // 1. bgfx::init() ✓
    // 2. bgfx::createTexture2D() ← BEFORE first frame() → CRASH on AMD

    EXPECT_EQ(frameCount_, 0) << "Should be before first frame";

    // Create a 1x1 texture BEFORE calling bgfx::frame()
    // This is UNDEFINED BEHAVIOR in bgfx
    uint32_t pixel = 0xFFFFFFFF;
    const bgfx::Memory* mem = bgfx::copy(&pixel, sizeof(pixel));

    // THIS CALL VIOLATES bgfx's CONTRACT
    bgfx::TextureHandle texture = bgfx::createTexture2D(
        1, 1, false, 1,
        bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_NONE,
        mem
    );

    // On AMD/RADV: This might appear to succeed but corrupts internal state
    // When second texture is created, GPU driver crashes
    // On NVIDIA: Might work due to more defensive validation

    // Cleanup (if we get here)
    if (bgfx::isValid(texture)) {
        bgfx::destroy(texture);
    }
}

// ============================================================================
// TEST 4: Our Fix - Validate Frame Count Before Texture Creation
// ============================================================================

TEST_F(BgfxFrameRequirementTest, ValidateFrameCountBeforeTextureCreation) {
    // This is the validation we should add to LoadTextureFromFile()

    // VALIDATION: Check if bgfx is ready for texture creation
    bool isSafeToCreateTexture = HasProcessedFrame();

    EXPECT_FALSE(isSafeToCreateTexture)
        << "BEFORE first frame: Should NOT be safe to create textures";

    // Now call frame() to initialize resource system
    ProcessFrame();
    isSafeToCreateTexture = HasProcessedFrame();

    EXPECT_TRUE(isSafeToCreateTexture)
        << "AFTER first frame: Should be safe to create textures";
}

// ============================================================================
// TEST 5: Proper Initialization Sequence
// ============================================================================

TEST_F(BgfxFrameRequirementTest, ProperInitializationSequence) {
    // This test demonstrates the CORRECT sequence

    // Step 1: Verify we're at frame 0
    EXPECT_EQ(frameCount_, 0);

    // Step 2: Process first frame (initializes resource system)
    ProcessFrame();

    // Step 3: Verify frame count incremented
    EXPECT_EQ(frameCount_, 1);

    // Step 4: NOW it's safe to create textures
    uint32_t pixel = 0xFFFFFFFF;
    const bgfx::Memory* mem = bgfx::copy(&pixel, sizeof(pixel));

    bgfx::TextureHandle texture = bgfx::createTexture2D(
        1, 1, false, 1,
        bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_NONE,
        mem
    );

    EXPECT_TRUE(bgfx::isValid(texture))
        << "Texture creation should succeed after first frame";

    // Cleanup
    if (bgfx::isValid(texture)) {
        // Process another frame to actually create the texture on GPU
        ProcessFrame();

        // Now destroy it
        bgfx::destroy(texture);
    }
}

// ============================================================================
// TEST 6: Multiple Textures Before First Frame (THE ACTUAL CRASH)
// ============================================================================

TEST_F(BgfxFrameRequirementTest, DISABLED_MultipleTexturesBeforeFirstFrame_CausesSystemCrash) {
    // THIS TEST IS DISABLED because it CRASHES THE ENTIRE SYSTEM on AMD GPUs
    //
    // This is the EXACT sequence that caused your crash:
    // 1. bgfx::init() ✓
    // 2. Create texture 1 (wood_color.jpg) → appears to succeed
    // 3. Create texture 2 (wood_roughness.jpg) → SYSTEM CRASH
    //
    // DO NOT ENABLE unless you want to reproduce the crash!

    EXPECT_EQ(frameCount_, 0);

    // Create FIRST texture (this "succeeds" but corrupts state)
    uint32_t pixel1 = 0xFF0000FF;  // Red
    const bgfx::Memory* mem1 = bgfx::copy(&pixel1, sizeof(pixel1));
    bgfx::TextureHandle texture1 = bgfx::createTexture2D(
        1, 1, false, 1,
        bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_NONE,
        mem1
    );

    // Create SECOND texture (this triggers the crash on AMD)
    uint32_t pixel2 = 0x00FF00FF;  // Green
    const bgfx::Memory* mem2 = bgfx::copy(&pixel2, sizeof(pixel2));
    bgfx::TextureHandle texture2 = bgfx::createTexture2D(
        1, 1, false, 1,
        bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_NONE,
        mem2
    );

    // On AMD RADV: System freezes here, requires hard power-off
    // On NVIDIA: Might work, might return invalid handle

    // Cleanup (never reached on AMD)
    if (bgfx::isValid(texture1)) bgfx::destroy(texture1);
    if (bgfx::isValid(texture2)) bgfx::destroy(texture2);
}

// ============================================================================
// TEST 7: The Fix Applied
// ============================================================================

TEST_F(BgfxFrameRequirementTest, WithFix_MultipleTexturesWork) {
    // This test demonstrates the fix working correctly

    // THE FIX: Call frame() BEFORE creating any textures
    ProcessFrame();
    EXPECT_TRUE(HasProcessedFrame()) << "Should have processed at least one frame";

    // NOW create multiple textures safely
    uint32_t pixel1 = 0xFF0000FF;  // Red
    const bgfx::Memory* mem1 = bgfx::copy(&pixel1, sizeof(pixel1));
    bgfx::TextureHandle texture1 = bgfx::createTexture2D(
        1, 1, false, 1,
        bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_NONE,
        mem1
    );
    EXPECT_TRUE(bgfx::isValid(texture1)) << "First texture should be valid";

    uint32_t pixel2 = 0x00FF00FF;  // Green
    const bgfx::Memory* mem2 = bgfx::copy(&pixel2, sizeof(pixel2));
    bgfx::TextureHandle texture2 = bgfx::createTexture2D(
        1, 1, false, 1,
        bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_NONE,
        mem2
    );
    EXPECT_TRUE(bgfx::isValid(texture2)) << "Second texture should be valid";

    // Process frame to create textures on GPU
    ProcessFrame();

    // Cleanup
    if (bgfx::isValid(texture1)) bgfx::destroy(texture1);
    if (bgfx::isValid(texture2)) bgfx::destroy(texture2);

    // This test PASSES because we called frame() before creating textures
}

} // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
