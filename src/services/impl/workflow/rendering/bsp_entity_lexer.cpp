#include "services/interfaces/workflow/rendering/bsp_entity_lexer.hpp"

#include <cctype>

namespace sdl3cpp::services::impl {

void SkipWhitespace(const std::string& text, size_t& pos) {
    while (pos < text.size() &&
           std::isspace(static_cast<unsigned char>(text[pos]))) {
        ++pos;
    }
}

bool ReadQuotedToken(const std::string& text, size_t& pos, std::string& out) {
    SkipWhitespace(text, pos);
    if (pos >= text.size() || text[pos] != '"') {
        return false;
    }
    ++pos;

    out.clear();
    while (pos < text.size()) {
        const char c = text[pos++];
        if (c == '"') {
            return true;
        }
        if (c == '\\' && pos < text.size()) {
            out.push_back(text[pos++]);
        } else {
            out.push_back(c);
        }
    }
    return false;
}

}  // namespace sdl3cpp::services::impl
