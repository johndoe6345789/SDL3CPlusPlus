#include "shaderc_mem.h"
#include "shaderc.h"
#include <bx/file.h>
#include <bx/allocator.h>
#include <bx/error.h>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>

namespace {

constexpr uint8_t kShaderBinVersion = 11;
constexpr uint32_t kChunkMagicVsh = BX_MAKEFOURCC('V', 'S', 'H', kShaderBinVersion);
constexpr uint32_t kChunkMagicFsh = BX_MAKEFOURCC('F', 'S', 'H', kShaderBinVersion);
constexpr uint32_t kChunkMagicCsh = BX_MAKEFOURCC('C', 'S', 'H', kShaderBinVersion);

// Writer that captures binary output into a std::vector<uint8_t>
class MemoryWriter : public bx::FileWriter
{
public:
    MemoryWriter() = default;
    virtual ~MemoryWriter() {}

    virtual int32_t write(const void* _data, int32_t _size, bx::Error*) override
    {
        const uint8_t* d = static_cast<const uint8_t*>(_data);
        m_buffer.insert(m_buffer.end(), d, d + _size);
        return _size;
    }

    const std::vector<uint8_t>& data() const { return m_buffer; }

private:
    std::vector<uint8_t> m_buffer;
};

uint32_t ChunkMagicForType(char shaderType) {
    switch (shaderType) {
        case 'v':
            return kChunkMagicVsh;
        case 'f':
            return kChunkMagicFsh;
        case 'c':
            return kChunkMagicCsh;
        default:
            return kChunkMagicFsh;
    }
}

void WriteShaderHeader(MemoryWriter& writer, char shaderType) {
    bx::ErrorAssert err;
    const uint32_t magic = ChunkMagicForType(shaderType);
    const uint32_t hash = 0;
    bx::write(&writer, magic, &err);
    bx::write(&writer, hash, &err);
    bx::write(&writer, hash, &err);
}

} // anonymous

int shaderc_compile_from_memory(const char* source, size_t source_len, const char* profile,
                                uint8_t** out_data, size_t* out_size, char** out_error) {
    if (!source || source_len == 0) {
        if (out_error) *out_error = strdup("empty source");
        return -1;
    }

    try {
        bgfx::Options opts;
        // Map profile string (e.g., "vertex"/"fragment") into shader type
        if (profile && std::strcmp(profile, "vertex") == 0) {
            opts.shaderType = 'v';
        } else if (profile && std::strcmp(profile, "fragment") == 0) {
            opts.shaderType = 'f';
        } else if (profile && std::strcmp(profile, "compute") == 0) {
            opts.shaderType = 'c';
        } else {
            // default to fragment
            opts.shaderType = 'f';
        }

        opts.profile = "glsl";
        // Use SPIR-V target by default (matches CLI's typical default for desktop)
        const uint32_t spirvVersion = 1010;

        MemoryWriter shaderWriter;
        MemoryWriter messageWriter;
        bx::WriterI* messageOut = &messageWriter;

        const std::string code(source, source_len);

        WriteShaderHeader(shaderWriter, opts.shaderType);
        bool ok = bgfx::compileSPIRVShader(opts, spirvVersion, code, &shaderWriter, messageOut);
        if (!ok) {
            // capture any message buffer
            const auto& msg = messageWriter.data();
            std::string s(msg.begin(), msg.end());
            if (out_error) *out_error = strdup(s.c_str());
            return -1;
        }

        const auto& out = shaderWriter.data();
        if (out.empty()) {
            if (out_error) *out_error = strdup("shaderc: empty output");
            return -1;
        }

        uint8_t* buf = (uint8_t*)malloc(out.size());
        if (!buf) {
            if (out_error) *out_error = strdup("out of memory");
            return -1;
        }
        memcpy(buf, out.data(), out.size());
        *out_data = buf;
        *out_size = out.size();
        if (out_error) *out_error = nullptr;
        return 0;
    } catch (const std::exception& e) {
        if (out_error) *out_error = strdup(e.what());
        return -1;
    }
}

void shaderc_free_buffer(uint8_t* data) { free(data); }
void shaderc_free_error(char* err) { free(err); }

int shaderc_compile_from_memory_with_target(const char* source, size_t source_len, const char* profile,
                                            const char* target, uint8_t** out_data, size_t* out_size, char** out_error) {
    if (!target) {
        // delegate to legacy function
        return shaderc_compile_from_memory(source, source_len, profile, out_data, out_size, out_error);
    }

    if (!source || source_len == 0) {
        if (out_error) *out_error = strdup("empty source");
        return -1;
    }

    try {
        bgfx::Options opts;
        if (profile && std::strcmp(profile, "vertex") == 0) {
            opts.shaderType = 'v';
        } else if (profile && std::strcmp(profile, "fragment") == 0) {
            opts.shaderType = 'f';
        } else if (profile && std::strcmp(profile, "compute") == 0) {
            opts.shaderType = 'c';
        } else {
            opts.shaderType = 'f';
        }

        if (0 == std::strcmp(target, "spirv")) {
            opts.profile = "glsl";
        }

        MemoryWriter shaderWriter;
        MemoryWriter messageWriter;
        bx::WriterI* messageOut = &messageWriter;
        const std::string code(source, source_len);

        bool ok = false;
        uint32_t version = 1010;
        // Dispatch based on target language (in-process supports SPIR-V/MSL/HLSL only).
        if (0 == std::strcmp(target, "spirv")) {
            WriteShaderHeader(shaderWriter, opts.shaderType);
            ok = bgfx::compileSPIRVShader(opts, version, code, &shaderWriter, messageOut);
        } else if (0 == std::strcmp(target, "msl") || 0 == std::strcmp(target, "metal")) {
            WriteShaderHeader(shaderWriter, opts.shaderType);
            ok = bgfx::compileMetalShader(opts, version, code, &shaderWriter, messageOut);
        } else if (0 == std::strcmp(target, "hlsl")) {
            WriteShaderHeader(shaderWriter, opts.shaderType);
            ok = bgfx::compileHLSLShader(opts, version, code, &shaderWriter, messageOut);
        } else {
            if (out_error) {
                *out_error = strdup("shaderc: target not supported by in-process compiler");
            }
            return -1;
        }

        if (!ok) {
            const auto& msg = messageWriter.data();
            std::string s(msg.begin(), msg.end());
            if (out_error) *out_error = strdup(s.c_str());
            return -1;
        }

        const auto& out = shaderWriter.data();
        if (out.empty()) {
            if (out_error) *out_error = strdup("shaderc: empty output");
            return -1;
        }

        uint8_t* buf = (uint8_t*)malloc(out.size());
        if (!buf) {
            if (out_error) *out_error = strdup("out of memory");
            return -1;
        }
        memcpy(buf, out.data(), out.size());
        *out_data = buf;
        *out_size = out.size();
        if (out_error) *out_error = nullptr;
        return 0;
    } catch (const std::exception& e) {
        if (out_error) *out_error = strdup(e.what());
        return -1;
    }
}
