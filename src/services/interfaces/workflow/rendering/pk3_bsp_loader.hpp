#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <nlohmann/json.hpp>
#include <zip.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// Reads a string parameter, falling back to the context value named by
/// the step's same-named input, then to `def`. Used by bsp.load to let
/// "pk3_path"/"map_name" come from either a literal parameter or a
/// wired-in context key.
std::string GetStringParamOrInput(const WorkflowStepDefinition& step,
                                  WorkflowContext& context, const char* name,
                                  const std::string& def);

/// Lists every "maps/<name>.bsp" entry in `archive` as its bare <name>.
nlohmann::json ListPk3Maps(zip_t* archive);

/**
 * @brief Reads "maps/<mapName>.bsp" out of `archive` into memory.
 *
 * @param pk3Path Only used in the thrown error's message.
 * @throws std::runtime_error if the entry is missing or fails to open.
 */
std::shared_ptr<std::vector<uint8_t>> ReadBspFromPk3(
    zip_t* archive, const std::string& mapName, const std::string& pk3Path);

/// Validates the IBSP magic/version-46 header.
/// @throws std::runtime_error if `bspData` is too small or not a valid
///         Q3 BSP.
void ValidateBspHeader(const std::vector<uint8_t>& bspData);

}  // namespace sdl3cpp::services::impl
