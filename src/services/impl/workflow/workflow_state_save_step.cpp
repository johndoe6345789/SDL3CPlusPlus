#include "services/interfaces/workflow/workflow_state_save_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowStateSaveStep::WorkflowStateSaveStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowStateSaveStep::GetPluginId() const {
    return "state.save";
}

void WorkflowStateSaveStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string stateKey = resolver.GetRequiredInputKey(step, "state");
    const std::string outputFileParam = resolver.GetOptionalParameterValue(step, "output_file", "");

    const auto* statePtr = context.TryGet<nlohmann::json>(stateKey);
    if (!statePtr) {
        throw std::runtime_error("state.save missing input '" + stateKey + "'");
    }

    if (outputFileParam.empty()) {
        throw std::runtime_error("state.save missing parameter 'output_file'");
    }

    const nlohmann::json& state = *statePtr;

    // Create directory if needed
    std::filesystem::path filePath(outputFileParam);
    std::filesystem::create_directories(filePath.parent_path());

    // Backup existing file if requested
    const std::string backupParam = resolver.GetOptionalParameterValue(step, "backup_existing", "false");
    if (backupParam == "true" && std::filesystem::exists(filePath)) {
        std::string backupPath = outputFileParam + ".backup";
        std::filesystem::copy_file(filePath, backupPath, std::filesystem::copy_options::overwrite_existing);
    }

    // Write state to file
    std::ofstream file(outputFileParam);
    if (!file.is_open()) {
        throw std::runtime_error("state.save failed to open file: " + outputFileParam);
    }

    const std::string prettyParam = resolver.GetOptionalParameterValue(step, "pretty_print", "false");
    if (prettyParam == "true") {
        file << state.dump(2);  // Pretty print with 2-space indent
    } else {
        file << state.dump();   // Compact format
    }

    file.close();

    // Set output keys if specified
    const auto& outputs = step.outputs;
    if (outputs.find("success") != outputs.end()) {
        context.Set(outputs.at("success"), true);
    }
    if (outputs.find("file_path") != outputs.end()) {
        context.Set(outputs.at("file_path"), outputFileParam);
    }

    if (logger_) {
        logger_->Trace("WorkflowStateSaveStep", "Execute",
                       "file=" + outputFileParam,
                       "Saved game state to file");
    }
}

}  // namespace sdl3cpp::services::impl
