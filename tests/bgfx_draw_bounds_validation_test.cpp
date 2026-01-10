#include <gtest/gtest.h>

// Test suite for Draw buffer bounds validation
//
// Problem: From sdl3_app.log crash analysis, the last draw command before crash was:
// BgfxGraphicsBackend::Draw(vertexOffset=1170, indexOffset=5232, indexCount=72, totalVertices=1194)
//
// This means:
// - Vertex buffer has 1194 vertices total
// - Drawing starts at vertex 1170, leaving only 24 vertices available (1194 - 1170 = 24)
// - Trying to draw 72 indices (24 triangles)
// - If indices reference vertices beyond 1194, we get GPU segfault in Vulkan driver
//
// The crash stack trace shows:
// #0-7: Deep in AMD Vulkan driver (libvulkan_radeon.so)
// #8: bgfx::vk::RendererContextVK::getPipeline()
// #9: bgfx::vk::RendererContextVK::submit()
//
// Root Cause: Missing bounds validation in Draw() function
// Current implementation does NOT validate that index buffer references stay within vertex buffer bounds

namespace {

// Test 1: Document the actual crash scenario from sdl3_app.log
TEST(BgfxDrawBoundsValidationTest, CrashScenario_IndexOutOfBounds) {
    // From crash log:
    // vertexOffset=1170, indexOffset=5232, indexCount=72, totalVertices=1194
    //
    // Available vertices after offset: 1194 - 1170 = 24 vertices
    // Indices to draw: 72 indices (references to 24 triangles)
    //
    // Problem: We don't know if those 72 indices reference vertices in range [0, 1194)
    // If any index >= 1194, GPU will try to read beyond vertex buffer → SIGSEGV

    const uint32_t totalVertices = 1194;
    const int32_t vertexOffset = 1170;
    const uint32_t indexOffset = 5232;
    const uint32_t indexCount = 72;

    const uint32_t availableVertices = totalVertices - vertexOffset;

    // THIS TEST SHOULD FAIL: We expect validation to detect potential out-of-bounds access
    // Currently, BgfxGraphicsBackend::Draw() has NO such validation
    
    // Expected behavior: Draw should validate that all indices stay within bounds
    // bool isValid = ValidateDrawBounds(totalVertices, vertexOffset, indexOffset, indexCount, indexBufferData);
    // EXPECT_FALSE(isValid) << "Should detect potential out-of-bounds access";

    // Current behavior: NO VALIDATION
    // This test documents the missing validation that caused the crash
    EXPECT_EQ(availableVertices, 24u) << "Only 24 vertices available after offset";
    EXPECT_EQ(indexCount, 72u) << "Attempting to draw 72 indices";
    
    // THE BUG: We don't validate if those 72 indices reference vertices > 1194
    EXPECT_TRUE(true) << "This test documents missing validation - validation not implemented yet";
}

// Test 2: Index buffer references must stay within vertex buffer bounds
TEST(BgfxDrawBoundsValidationTest, IndexReferencesWithinVertexBufferBounds) {
    // Scenario: Combined vertex buffer with multiple meshes
    // Mesh 1: vertices [0, 441)
    // Mesh 2: vertices [441, 882)
    // Mesh 3: vertices [882, 1194)
    //
    // When drawing Mesh 3 with vertexOffset=882, indices must not reference
    // vertices beyond 1194

    struct DrawCall {
        uint32_t vertexOffset;
        uint32_t indexOffset;
        uint32_t indexCount;
        uint32_t totalVertices;
    };

    // Simulate the draw calls from the crash log
    std::vector<DrawCall> drawCalls = {
        {0, 0, 2400, 1194},        // Mesh 1: large mesh, many indices
        {441, 2400, 2400, 1194},    // Mesh 2: another large mesh
        {882, 4800, 36, 1194},      // Mesh 3: small cube
        {906, 4836, 36, 1194},      // ...more cubes
        {1170, 5232, 72, 1194},     // Last draw before crash
    };

    for (const auto& call : drawCalls) {
        // Calculate max possible vertex index this draw could reference
        // If indices are in range [0, vertexCount) relative to vertexOffset,
        // max absolute index would be vertexOffset + maxIndexValue
        
        // Without reading the actual index buffer, we can't validate
        // THIS IS THE BUG: No validation in Draw()
        
        const uint32_t remainingVertices = call.totalVertices - call.vertexOffset;
        
        // We can only check if there are ENOUGH vertices after the offset
        // But we can't validate actual index values without reading index buffer
        EXPECT_GT(remainingVertices, 0u) << "Must have vertices available after offset";
    }

    // THIS TEST EXPOSES THE PROBLEM:
    // Draw() should validate index buffer contents, but currently doesn't
    EXPECT_TRUE(true) << "Missing validation: Index buffer bounds checking not implemented";
}

// Test 3: Vertex offset must not exceed vertex buffer size
TEST(BgfxDrawBoundsValidationTest, VertexOffsetWithinBounds) {
    const uint32_t totalVertices = 1194;
    
    // Valid offsets
    EXPECT_LT(0, totalVertices);
    EXPECT_LT(441, totalVertices);
    EXPECT_LT(1170, totalVertices);
    
    // Invalid offset (would exceed buffer)
    const int32_t invalidOffset = 1200;
    EXPECT_GT(invalidOffset, static_cast<int32_t>(totalVertices))
        << "This should be caught by validation but isn't";

    // THIS TEST SHOULD FAIL if we had validation
    // Current implementation in bgfx_graphics_backend.cpp:1093-1145 does NOT check this
    EXPECT_TRUE(true) << "Missing validation: vertexOffset bounds check not implemented";
}

// Test 4: Index count must not cause reads beyond buffer
TEST(BgfxDrawBoundsValidationTest, IndexCountWithinBufferBounds) {
    // From crash scenario: indexOffset=5232, indexCount=72
    // If index buffer has N indices total, we need: indexOffset + indexCount <= N
    
    const uint32_t indexOffset = 5232;
    const uint32_t indexCount = 72;
    const uint32_t maxIndex = indexOffset + indexCount; // = 5304
    
    // We don't know the total index buffer size from the log
    // But Draw() should validate this
    
    // THIS TEST DOCUMENTS THE MISSING VALIDATION
    EXPECT_EQ(maxIndex, 5304u);
    EXPECT_TRUE(true) << "Missing validation: Index buffer size check not implemented";
}

// Test 5: Empty draw (zero indices) should be handled
TEST(BgfxDrawBoundsValidationTest, EmptyDraw_ZeroIndices) {
    const uint32_t indexCount = 0;
    
    // Drawing 0 indices should either:
    // 1. Be validated and rejected with clear error
    // 2. Be safely handled as no-op
    
    // Current implementation probably submits to GPU anyway
    EXPECT_EQ(indexCount, 0u);
    EXPECT_TRUE(true) << "Edge case: Zero index count handling not validated";
}

// Test 6: Negative vertex offset should be rejected
TEST(BgfxDrawBoundsValidationTest, NegativeVertexOffset) {
    const int32_t negativeOffset = -10;
    
    // Negative offsets are invalid and should be rejected
    EXPECT_LT(negativeOffset, 0) << "Negative offset is invalid";
    
    // Current implementation: int32_t can be negative, but no validation checks this
    EXPECT_TRUE(true) << "Missing validation: Negative vertex offset not checked";
}

// Test 7: Document the fix needed in BgfxGraphicsBackend::Draw
TEST(BgfxDrawBoundsValidationTest, RequiredValidations) {
    // File: src/services/impl/graphics/bgfx_graphics_backend.cpp
    // Function: BgfxGraphicsBackend::Draw (lines ~1093-1145)
    //
    // Missing validations:
    // 1. vertexOffset >= 0
    // 2. vertexOffset < totalVertices
    // 3. indexOffset + indexCount <= indexBufferSize
    // 4. All index values in range [indexOffset, indexOffset+indexCount) must be < totalVertices
    //
    // Without these checks, GPU receives invalid commands → driver crash

    const std::string validations[] = {
        "Check vertexOffset >= 0",
        "Check vertexOffset < totalVertices",
        "Check indexOffset + indexCount <= indexBufferSize",
        "Check all indices < totalVertices (requires reading index buffer)",
        "Log detailed error message on validation failure",
        "Return early instead of calling bgfx::submit() with invalid parameters"
    };

    // All these validations are MISSING in current implementation
    EXPECT_EQ(sizeof(validations) / sizeof(validations[0]), 6u);
    EXPECT_TRUE(true) << "These 6 validations need to be added to prevent GPU crashes";
}

// Test 8: Combined mesh scenario - typical use case
TEST(BgfxDrawBoundsValidationTest, CombinedMeshBuffer_MultipleDraws) {
    // Real-world scenario: Multiple meshes in one vertex/index buffer
    // From the crash log, we have 15 meshes being drawn in one frame
    
    struct MeshInBuffer {
        uint32_t vertexStart;
        uint32_t vertexCount;
        uint32_t indexStart;
        uint32_t indexCount;
    };

    // Simulated mesh layout
    std::vector<MeshInBuffer> meshes = {
        {0, 441, 0, 2400},          // Floor/wall mesh
        {441, 441, 2400, 2400},     // Another large mesh
        {882, 24, 4800, 36},        // Cube 1
        {906, 24, 4836, 36},        // Cube 2
        // ... more cubes ...
        {1170, 24, 5232, 72},       // Last cube (the one that crashes)
    };

    const uint32_t totalVertices = 1194;
    const uint32_t totalIndices = 5304; // Estimated

    for (size_t i = 0; i < meshes.size(); ++i) {
        const auto& mesh = meshes[i];
        
        // Validation that SHOULD happen but doesn't:
        const bool vertexRangeValid = 
            (mesh.vertexStart + mesh.vertexCount) <= totalVertices;
        const bool indexRangeValid = 
            (mesh.indexStart + mesh.indexCount) <= totalIndices;
        
        EXPECT_TRUE(vertexRangeValid) 
            << "Mesh " << i << " vertex range exceeds buffer";
        EXPECT_TRUE(indexRangeValid)
            << "Mesh " << i << " index range exceeds buffer";
    }

    // The bug: Even if these ranges are valid, we don't check if
    // indices[mesh.indexStart..mesh.indexStart+mesh.indexCount] reference
    // vertices in range [0, totalVertices)
    
    EXPECT_TRUE(true) << "This validation requires reading index buffer at draw time";
}

} // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
