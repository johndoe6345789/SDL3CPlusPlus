#pragma once

/// Phase functions for the sdl3_app entry point (src/main.cpp). Splitting
/// them out keeps main() itself to a short sequence of calls in the
/// original startup order: parse args, validate packages, build logging
/// and workflow infrastructure, then load and run the default workflow.

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/workflow_executor.hpp"
#include "services/interfaces/workflow/workflow_step_registry.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_definition.hpp"
#include "services/interfaces/workflow_registrar.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace sdl3cpp::services::app {

/** @brief Parsed command-line configuration for the sdl3_app entry point. */
struct CliOptions {
    std::string gamePackage = "standalone_cubes";
    std::string bootstrapPackage = "bootstrap_mac";
    std::filesystem::path projectRoot = std::filesystem::current_path();
    bool traceEnabled = false;
};

/**
 * @brief Parse argv into outOptions.
 * @return false on an unknown/malformed argument; a usage or error message
 *         has already been written to stderr and the caller should exit
 *         with failure.
 */
bool ParseCliArgs(int argc, char** argv, CliOptions& outOptions);

/**
 * @brief Verify that packages/<name>/package.json exists and, if not,
 * print the packages of `expectedType` that do exist to stderr.
 *
 * A mistyped package name used to fall through silently: the shader
 * backend stayed on the macOS default and the run died later with an
 * opaque vkCreateShaderModule error. Failing here instead, and listing
 * only the packages valid for the flag that was wrong, catches that
 * class of mistake immediately.
 */
bool RequirePackage(const std::filesystem::path& projectRoot,
                     const std::string& kind, const std::string& expectedType,
                     const std::string& name);

/// @brief Build and configure the application logger (file output, trace
/// level if requested, console output disabled).
std::shared_ptr<ILogger> CreateAppLogger(
    const std::filesystem::path& projectRoot, bool traceEnabled);

/// @brief The workflow execution infrastructure created for one run.
struct WorkflowRuntime {
    std::shared_ptr<impl::WorkflowStepRegistry> registry;
    std::unique_ptr<impl::WorkflowRegistrar> registrar;
    std::shared_ptr<impl::WorkflowExecutor> executor;
};

/// @brief Create the registry/registrar/executor, register all base,
/// application-lifecycle, and executor-dependent steps.
WorkflowRuntime BuildWorkflowRuntime(std::shared_ptr<ILogger> logger);

/// @brief Read packages/<gamePackage>/package.json's "defaultWorkflow"
/// field, falling back to "workflows/main.json" when absent or unreadable.
std::string LoadDefaultWorkflowPath(const std::filesystem::path& projectRoot,
                                     const std::string& gamePackage,
                                     const std::shared_ptr<ILogger>& logger);

/// @brief Determine the shader source subdirectory ("msl" or "spirv") from
/// the bootstrap package's configured renderer; "msl" (Mac) is the default.
std::string DetermineShaderBackend(const std::filesystem::path& projectRoot,
                                    const std::string& bootstrapPackage);

/// @brief Populate outContext from the workflow's variable defaults,
/// rewriting Metal shader paths (shaders/msl/*.metal) to their SPIR-V
/// equivalents (shaders/spirv/*.spv) when the backend isn't Metal.
void PopulateContextFromWorkflowVariables(
    const WorkflowDefinition& workflow, bool rewriteShaders,
    const std::shared_ptr<ILogger>& logger, WorkflowContext& outContext);

}  // namespace sdl3cpp::services::app
