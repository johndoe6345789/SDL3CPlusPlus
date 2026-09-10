#include "services/interfaces/workflow/quake3/workflow_q3_md3_read_step.hpp"
#include "services/interfaces/workflow/quake3/q3_md3_format.hpp"
#include "services/interfaces/workflow/quake3/q3_pk3_reader.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {
namespace {

bool NamesSkinFile(const std::string& skin) {
    return skin.size() > 5 && skin.compare(skin.size() - 5, 5, ".skin") == 0;
}

}  // namespace

WorkflowQ3Md3ReadStep::WorkflowQ3Md3ReadStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3Md3ReadStep::GetPluginId() const {
    return "q3.md3.read";
}

void WorkflowQ3Md3ReadStep::Execute(const WorkflowStepDefinition& step,
                                    WorkflowContext& context) {
    const Q3Md3StepParameters params = ReadQ3Md3StepParameters(step);
    const std::string key            = Q3Md3SourceKey(params.prefix);

    auto existing = sources_.find(params.prefix);
    if (existing != sources_.end()) {
        context.Set<Q3Md3Source*>(key, &existing->second);
        return;  // Already read — keep the load idempotent.
    }

    const auto bspConfig =
        context.Get<nlohmann::json>("bsp_config", nlohmann::json{});
    const std::string pk3 = bspConfig.value("pk3_path", std::string(""));
    auto* device          = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (pk3.empty() || !device || params.path.empty()) {
        if (logger_) {
            logger_->Warn("q3.md3.read[" + params.prefix +
                          "]: pk3/device/path missing");
        }
        return;
    }

    Q3Md3Source source;
    source.prefix  = params.prefix;
    source.path    = params.path;
    source.skin    = params.skin;
    source.anim    = params.anim;
    source.pk3Path = pk3;
    source.bytes   = q3::ReadPk3Entry(pk3, params.path);

    if (source.bytes.size() < sizeof(q3::Md3Header)) {
        if (logger_) {
            logger_->Warn("q3.md3.read[" + params.prefix + "]: cannot read " +
                          params.path);
        }
        return;
    }

    // A .skin file names the texture per surface and is authoritative for
    // player models, whose MD3 shader names are usually blank.
    if (NamesSkinFile(params.skin)) {
        const auto skinBytes = q3::ReadPk3Entry(pk3, params.skin);
        if (!skinBytes.empty()) {
            source.skinMap = q3::ParseSkinFile(
                std::string(reinterpret_cast<const char*>(skinBytes.data()),
                            skinBytes.size()));
        }
    }

    auto inserted = sources_.emplace(params.prefix, std::move(source));
    context.Set<Q3Md3Source*>(key, &inserted.first->second);
}

}  // namespace sdl3cpp::services::impl
