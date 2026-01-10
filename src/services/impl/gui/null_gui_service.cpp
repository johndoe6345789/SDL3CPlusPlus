#include "null_gui_service.hpp"

#include "services/interfaces/gui_types.hpp"

#include <string>

namespace sdl3cpp::services::impl {

NullGuiService::NullGuiService(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("NullGuiService", "NullGuiService");
    }
}

void NullGuiService::PrepareFrame(const std::vector<GuiCommand>& commands,
                                  uint32_t width,
                                  uint32_t height) {
    if (logger_) {
        logger_->Trace("NullGuiService", "PrepareFrame",
                       "commands.size=" + std::to_string(commands.size()) +
                       ", width=" + std::to_string(width) +
                       ", height=" + std::to_string(height));
    }
}

}  // namespace sdl3cpp::services::impl
