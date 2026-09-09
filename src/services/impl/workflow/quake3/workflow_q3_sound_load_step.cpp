#include "services/interfaces/workflow/quake3/workflow_q3_sound_load_step.hpp"
#include "services/interfaces/workflow/quake3/q3_sound_bank.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>
#include <zip.h>

#include <fstream>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {

std::vector<uint8_t> ReadEntry(zip_t* archive, const std::string& name) {
    zip_stat_t stat;
    if (zip_stat(archive, name.c_str(), 0, &stat) != 0) return {};
    zip_file_t* file = zip_fopen(archive, name.c_str(), 0);
    if (!file) return {};
    std::vector<uint8_t> bytes(stat.size);
    zip_fread(file, bytes.data(), stat.size);
    zip_fclose(file);
    return bytes;
}

}  // namespace

WorkflowQ3SoundLoadStep::WorkflowQ3SoundLoadStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3SoundLoadStep::GetPluginId() const {
    return "q3.sound.load";
}

void WorkflowQ3SoundLoadStep::Execute(const WorkflowStepDefinition& step,
                                      WorkflowContext& context) {
    if (context.Contains("q3.sound.bank")) return;

    WorkflowStepParameterResolver params;
    const auto* pathParam = params.FindParameter(step, "config_path");
    const std::string configPath =
        pathParam ? pathParam->stringValue
                  : "packages/quake3/config/sounds.json";

    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        if (logger_) logger_->Warn("q3.sound.load: no " + configPath);
        return;
    }
    nlohmann::json config;
    configFile >> config;

    const auto bspConfig =
        context.Get<nlohmann::json>("bsp_config", nlohmann::json{});
    const std::string pk3 = bspConfig.value("pk3_path", std::string());
    int err = 0;
    zip_t* archive = pk3.empty()
                         ? nullptr
                         : zip_open(pk3.c_str(), ZIP_RDONLY, &err);
    if (!archive) {
        if (logger_) logger_->Warn("q3.sound.load: cannot open " + pk3);
        return;
    }

    auto bank = std::make_shared<q3::SoundBank>();
    for (const auto& entry : config.value("sounds", nlohmann::json::array())) {
        const auto name = entry.get<std::string>();
        const auto bytes = ReadEntry(archive, name);
        q3::Sound sound;
        if (q3::DecodeWav(bytes.data(), bytes.size(), sound)) {
            (*bank)[name] = std::move(sound);
        } else if (logger_) {
            logger_->Warn("q3.sound.load: could not decode " + name);
        }
    }
    zip_close(archive);

    context.Set("q3.sound.bank", bank);
    if (logger_) {
        logger_->Info("q3.sound.load: " +
                      std::to_string(q3::CountPlayable(*bank)) +
                      " sounds loaded from pk3");
    }
}

}  // namespace sdl3cpp::services::impl
