#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief A Stunts .RES archive, keyed by its four-character tags.
 *
 * The layout is a 32-bit total size, a 16-bit entry count, one
 * four-character tag per entry and then one 32-bit offset per entry,
 * each offset measured from the end of that header. Entries are not
 * stored in tag order, so each blob ends where the next offset begins.
 */
using StuntsResources = std::map<std::string, std::vector<std::uint8_t>>;

/// Splits an already-decoded .RES image into its tagged resources.
StuntsResources ParseStuntsResources(const std::vector<std::uint8_t>& raw);

}  // namespace sdl3cpp::services::impl
