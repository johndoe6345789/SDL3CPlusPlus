#pragma once
#include <cstddef>
#include <cstdint>
#ifdef __cplusplus
extern "C" {
#endif

// Compile GLSL source provided in-memory to SPIR-V-like bgfx binary
// Returns 0 on success, non-zero on failure.
int shaderc_compile_from_memory(const char* source, size_t source_len, const char* profile,
                                uint8_t** out_data, size_t* out_size, char** out_error);

// Variant that accepts an explicit output target language. Valid targets include:
// "spirv" (SPIR-V), "glsl" (GLSL), "msl" (Metal), "hlsl" (HLSL).
int shaderc_compile_from_memory_with_target(const char* source, size_t source_len, const char* profile,
                                            const char* target, uint8_t** out_data, size_t* out_size, char** out_error);

// Free buffers returned by shaderc_compile_from_memory
void shaderc_free_buffer(uint8_t* data);
void shaderc_free_error(char* err);

#ifdef __cplusplus
}
#endif
