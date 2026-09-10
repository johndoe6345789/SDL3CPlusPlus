#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief One-time parse of `bsp.entities` into q3.triggers.check's trigger
 * list and destination index, if not already done this map load.
 *
 * Builds `q3.trigger_list` (trigger_push/trigger_teleport/trigger_hurt
 * entities, each with classname/origin/target/dmg) and
 * `q3.trigger_dest_index` (targetname -> origin, from target_position and
 * misc_teleporter_dest entities), then sets `q3.triggers_loaded` so this
 * only runs once. A no-op if `q3.triggers_loaded` is already true.
 */
void LoadQ3TriggersIfNeeded(WorkflowContext& context,
                            const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
