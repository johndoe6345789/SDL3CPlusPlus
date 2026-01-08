#pragma once

#include "config_ir_types.hpp"
#include <string>

namespace sdl3cpp::services {

/**
 * @brief Compiles JSON configuration into validated IR structures.
 */
class IConfigCompilerService {
public:
    virtual ~IConfigCompilerService() = default;

    /**
     * @brief Compile config JSON into IR.
     *
     * @param configJson JSON payload
     * @return Compilation result (diagnostics included)
     */
    virtual ConfigCompilerResult Compile(const std::string& configJson) = 0;

    /**
     * @brief Get the last compilation result.
     */
    virtual const ConfigCompilerResult& GetLastResult() const = 0;
};

}  // namespace sdl3cpp::services
