#pragma once

#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// A menu the HUD draws (gta5.menu, while gta5.menu.open): a title bar,
/// its items with one selected, and the controls at the foot.
struct Gta5Menu {
    std::string title;
    std::vector<std::string> items;
    int selected{0};
    std::string hint;
};

/// The weapon wheel, while it is held open (gta5.wheel.*).
struct Gta5Wheel {
    std::vector<std::string> items;
    int selected{0};
    bool open{false};
};

}  // namespace sdl3cpp::services::impl
