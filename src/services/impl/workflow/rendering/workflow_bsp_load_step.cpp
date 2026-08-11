#include "services/interfaces/workflow/rendering/workflow_bsp_load_step.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <nlohmann/json.hpp>
#include <zip.h>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowBspLoadStep::WorkflowBspLoadStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBspLoadStep::GetPluginId() const {
    return "bsp.load";
}

void WorkflowBspLoadStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepParameterResolver params;

    auto getStr = [&](const char* name, const std::string& def) -> std::string {
        const auto* p = params.FindParameter(step, name);
        if (p && p->type == WorkflowParameterValue::Type::String) return p->stringValue;
        auto it = step.inputs.find(name);
        if (it != step.inputs.end()) {
            const auto* ctx = context.TryGet<std::string>(it->second);
            if (ctx) return *ctx;
        }
        return def;
    };
    auto getNum = [&](const char* name, float def) -> float {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::Number) ? static_cast<float>(p->numberValue) : def;
    };

    const std::string pk3_path = getStr("pk3_path", "");

    // Context key q3.pending_map (set by menu) takes precedence over the workflow parameter.
    // This allows in-process map switching without modifying the workflow JSON.
    std::string map_name;
    const auto* pendingMap = context.TryGet<std::string>("q3.pending_map");
    if (pendingMap && !pendingMap->empty()) {
        map_name = *pendingMap;
        context.Set<std::string>("q3.pending_map", "");
    } else {
        map_name = getStr("map_name", "q3dm7");
    }
    if (map_name.empty()) map_name = "q3dm7";
    const float scale = getNum("scale", 1.0f / 32.0f);

    if (pk3_path.empty()) {
        throw std::runtime_error("bsp.load: 'pk3_path' parameter required");
    }

    // Open pk3 (zip) and read BSP entry
    int zip_err = 0;
    zip_t* archive = zip_open(pk3_path.c_str(), ZIP_RDONLY, &zip_err);
    if (!archive) {
        throw std::runtime_error("bsp.load: Failed to open pk3: " + pk3_path);
    }

    nlohmann::json maps = nlohmann::json::array();
    const zip_int64_t entries = zip_get_num_entries(archive, 0);
    for (zip_uint64_t i = 0; i < static_cast<zip_uint64_t>(entries); ++i) {
        const char* name = zip_get_name(archive, i, 0);
        if (!name) continue;
        std::string entry(name);
        if (entry.rfind("maps/", 0) != 0 || entry.size() <= 9) continue;
        if (entry.substr(entry.size() - 4) != ".bsp") continue;
        maps.push_back(entry.substr(5, entry.size() - 9));
    }
    context.Set("q3.maps", maps);

    std::string bsp_entry = "maps/" + map_name + ".bsp";
    zip_stat_t st;
    if (zip_stat(archive, bsp_entry.c_str(), 0, &st) != 0) {
        zip_close(archive);
        throw std::runtime_error("bsp.load: Map '" + bsp_entry + "' not found in " + pk3_path);
    }

    auto bspData = std::make_shared<std::vector<uint8_t>>(st.size);
    zip_file_t* zf = zip_fopen(archive, bsp_entry.c_str(), 0);
    if (!zf) {
        zip_close(archive);
        throw std::runtime_error("bsp.load: Failed to open " + bsp_entry);
    }
    zip_fread(zf, bspData->data(), st.size);
    zip_fclose(zf);
    zip_close(archive);

    if (logger_) {
        logger_->Info("bsp.load: Read " + bsp_entry + " (" + std::to_string(bspData->size()) + " bytes)");
    }

    // Validate BSP header
    if (bspData->size() < sizeof(BspHeader) + sizeof(BspLump) * NUM_LUMPS) {
        throw std::runtime_error("bsp.load: BSP file too small");
    }

    auto* header = reinterpret_cast<const BspHeader*>(bspData->data());
    if (std::memcmp(header->magic, "IBSP", 4) != 0 || header->version != 46) {
        throw std::runtime_error("bsp.load: Not a valid Q3 BSP (magic/version mismatch)");
    }

    // Store raw BSP data and config for downstream steps
    context.Set("bsp_raw_data", bspData);
    context.Set("bsp_config", nlohmann::json{
        {"pk3_path", pk3_path},
        {"map_name", map_name},
        {"scale", scale}
    });

    if (logger_) {
        logger_->Info("bsp.load: '" + map_name + "' validated, " +
                     std::to_string(bspData->size()) + " bytes stored in context");
    }
}

}  // namespace sdl3cpp::services::impl
