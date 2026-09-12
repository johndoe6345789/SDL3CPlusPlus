#include "services/interfaces/workflow/gta5/stream/gta5_hitch_report.hpp"

#include <cstdio>
#include <string>

namespace sdl3cpp::services::impl {

void ReportGta5Hitch(const std::shared_ptr<ILogger>& logger,
                     const Gta5FrameCost& cost, double frameMs) {
    if (!logger) return;
    const double known = cost.load + cost.evict + cost.cull + cost.draw;
    char line[256];
    std::snprintf(line, sizeof(line),
                  "gta5.hitch: %.1f ms: load %.1f (finish %.1f, spawn %.1f "
                  "x%d, adopt %.1f), evict %.1f, cull %.1f, draw %.1f, "
                  "other %.1f",
                  frameMs, cost.load, cost.finish, cost.spawn, cost.spawned,
                  cost.adopt, cost.evict, cost.cull, cost.draw,
                  frameMs - known);
    logger->Info(cost.phases.empty() ? std::string(line)
                                     : std::string(line) + " | " +
                                           cost.phases);
}

}  // namespace sdl3cpp::services::impl
