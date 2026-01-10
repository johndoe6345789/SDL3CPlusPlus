#pragma once

#include "services/interfaces/i_gui_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

class NullGuiService : public IGuiService {
public:
    explicit NullGuiService(std::shared_ptr<ILogger> logger);

    void PrepareFrame(const std::vector<GuiCommand>& commands,
                      uint32_t width,
                      uint32_t height) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
