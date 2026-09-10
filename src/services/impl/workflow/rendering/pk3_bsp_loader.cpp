#include "services/interfaces/workflow/rendering/pk3_bsp_loader.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <cstring>
#include <stdexcept>

namespace sdl3cpp::services::impl {

std::string GetStringParamOrInput(const WorkflowStepDefinition& step,
                                  WorkflowContext& context, const char* name,
                                  const std::string& def) {
    auto it = step.parameters.find(name);
    if (it != step.parameters.end() &&
        it->second.type == WorkflowParameterValue::Type::String) {
        return it->second.stringValue;
    }
    auto inputIt = step.inputs.find(name);
    if (inputIt != step.inputs.end()) {
        if (const auto* ctx =
                context.TryGet<std::string>(inputIt->second)) {
            return *ctx;
        }
    }
    return def;
}

nlohmann::json ListPk3Maps(zip_t* archive) {
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
    return maps;
}

std::shared_ptr<std::vector<uint8_t>> ReadBspFromPk3(
    zip_t* archive, const std::string& mapName, const std::string& pk3Path) {
    const std::string bsp_entry = "maps/" + mapName + ".bsp";
    zip_stat_t st;
    if (zip_stat(archive, bsp_entry.c_str(), 0, &st) != 0) {
        zip_close(archive);
        throw std::runtime_error("bsp.load: Map '" + bsp_entry +
                                 "' not found in " + pk3Path);
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
    return bspData;
}

void ValidateBspHeader(const std::vector<uint8_t>& bspData) {
    if (bspData.size() < sizeof(BspHeader) + sizeof(BspLump) * NUM_LUMPS) {
        throw std::runtime_error("bsp.load: BSP file too small");
    }

    auto* header = reinterpret_cast<const BspHeader*>(bspData.data());
    if (std::memcmp(header->magic, "IBSP", 4) != 0 ||
        header->version != 46) {
        throw std::runtime_error(
            "bsp.load: Not a valid Q3 BSP (magic/version mismatch)");
    }
}

}  // namespace sdl3cpp::services::impl
