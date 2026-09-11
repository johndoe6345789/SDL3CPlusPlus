#include "services/interfaces/workflow/gta5/gta5_settings.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

bool KeepGta5Settings(Gta5Settings& settings, float health, float armour,
                      const Gta5Inventory& inventory) {
    settings.health = health;
    settings.armour = armour;
    const bool owns = std::any_of(inventory.clip.begin(),
                                  inventory.clip.end(),
                                  [](int c) { return c >= 0; });
    if (owns) settings.inventory = inventory;
    return SaveGta5Settings(settings);
}

}  // namespace sdl3cpp::services::impl
