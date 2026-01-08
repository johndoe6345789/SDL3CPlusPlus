#pragma once

#include "gui_types.hpp"
#include <vector>

namespace sdl3cpp::services {

/**
 * @brief Script-facing GUI command service interface.
 */
class IGuiScriptService {
public:
    virtual ~IGuiScriptService() = default;

    virtual std::vector<GuiCommand> LoadGuiCommands() = 0;
    virtual void UpdateGuiInput(const GuiInputSnapshot& input) = 0;
    virtual bool HasGuiCommands() const = 0;
};

}  // namespace sdl3cpp::services
