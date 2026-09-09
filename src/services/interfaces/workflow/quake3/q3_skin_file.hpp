#pragma once

#include <string>
#include <unordered_map>

namespace sdl3cpp::q3 {

/// Surface name to texture path, as read from a Quake .skin file.
using SkinMap = std::unordered_map<std::string, std::string>;

/**
 * @brief Parse a Quake .skin file.
 *
 * Each line is "surface,texture". A surface with no texture (the tag
 * entries, "tag_torso,") is skipped. The .skin file is authoritative for
 * player models: the shader names inside the MD3 itself are frequently
 * absent or wrong, which leaves models untextured if they are trusted.
 *
 * Lookups are case-insensitive on the surface name, and texture paths
 * are lowercased, because the shipped files disagree with themselves
 * about capitalisation ("models/players/Keel/keel.tga" refers to a file
 * stored as "models/players/keel/keel.tga") and pk3 lookups are exact.
 */
SkinMap ParseSkinFile(const std::string& text);

/// Texture for a surface, or empty when the skin does not name one.
std::string SkinTextureFor(const SkinMap& skin, const std::string& surface);

}  // namespace sdl3cpp::q3
