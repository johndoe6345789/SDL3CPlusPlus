#pragma once

#include <cstddef>
#include <string>

namespace sdl3cpp::services::impl {

/// Advances `pos` past any run of whitespace in `text`.
void SkipWhitespace(const std::string& text, size_t& pos);

/**
 * @brief Reads one `"..."` quoted token starting at `pos`, advancing past
 * its closing quote.
 *
 * Backslash-escapes the next character verbatim (no escape-sequence
 * decoding, matching Quake's own entity-lump text format). Leading
 * whitespace before the opening quote is skipped.
 *
 * @return false (leaving `out` partially filled) if `text` runs out
 *         before an opening or closing quote is found.
 */
bool ReadQuotedToken(const std::string& text, size_t& pos, std::string& out);

}  // namespace sdl3cpp::services::impl
