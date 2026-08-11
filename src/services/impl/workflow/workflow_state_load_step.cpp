#include "services/interfaces/workflow/workflow_state_load_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowStateLoadStep::WorkflowStateLoadStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowStateLoadStep::GetPluginId() const {
    return "state.load";
}

void WorkflowStateLoadStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;

    // Get input file path - can come from parameter or context input
    std::string inputFile;
    const auto& inputs = step.inputs;
    if (inputs.find("input_file") != inputs.end()) {
        // Try to get from context first
        const auto* inputFilePath = context.TryGet<std::string>(inputs.at("input_file"));
        if (inputFilePath) {
            inputFile = *inputFilePath;
        }
    }

    if (inputFile.empty()) {
        inputFile = resolver.GetOptionalParameterValue(step, "input_file", "");
    }

    if (inputFile.empty()) {
        throw std::runtime_error("state.load missing parameter 'input_file' or input connection");
    }

    // Verify file exists
    if (!std::filesystem::exists(inputFile)) {
        throw std::runtime_error("state.load file not found: " + inputFile);
    }

    // Read and parse JSON file
    std::ifstream file(inputFile);
    if (!file.is_open()) {
        throw std::runtime_error("state.load failed to open file: " + inputFile);
    }

    nlohmann::json loadedState;
    try {
        file >> loadedState;
    } catch (const std::exception& e) {
        throw std::runtime_error("state.load failed to parse JSON: " + std::string(e.what()));
    }
    file.close();

    // Set output state
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "state");
    context.Set(outputKey, loadedState);

    if (logger_) {
        logger_->Trace("WorkflowStateLoadStep", "Execute",
                       "file=" + inputFile,
                       "Loaded game state from file");
    }
}

}  // namespace sdl3cpp::services::impl
