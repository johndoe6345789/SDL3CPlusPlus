#pragma once

#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief The md3 prefix a pickup's world model is loaded under.
 *
 * Quake draws an item as the model named by its bg_itemlist entry
 * (world_model[0]), not as a coloured marker. Weapons reuse the model
 * already loaded for the viewmodel; everything else is loaded under its
 * own classname. Empty when the item has no model loaded, in which case
 * there is nothing to draw.
 */
std::string Q3ItemModelPrefix(const std::string& classname);

/// Whether the item spins at Quake's faster rate: cg_ents.c CG_Item
/// turns health twice as quickly as everything else.
bool Q3ItemSpinsFast(const std::string& classname);

}  // namespace sdl3cpp::services::impl
