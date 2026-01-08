#pragma once
#include <string>
#include <vector>
#include <optional>

namespace sdl3cpp::services {
/**
 * @brief Interface for pipeline/shader compilation using bgfx_tools.
 */
class IPipelineCompilerService {
public:
    virtual ~IPipelineCompilerService() = default;
    /**
     * Compile a shader or pipeline using bgfx_tools.
     * @param inputPath Path to input shader source file.
     * @param outputPath Path to output compiled binary.
     * @param args Additional arguments for bgfx_tools (e.g., profile, macros).
     * @return true if compilation succeeded, false otherwise.
     */
    virtual bool Compile(const std::string& inputPath,
                        const std::string& outputPath,
                        const std::vector<std::string>& args = {}) = 0;
    /**
     * Get last error message (if any).
     */
    virtual std::optional<std::string> GetLastError() const = 0;
};
} // namespace sdl3cpp::services
