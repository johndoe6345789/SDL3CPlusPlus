#include "services/interfaces/workflow/rendering/bsp_entity_lump_parser.hpp"
#include "services/interfaces/workflow/rendering/bsp_entity_lexer.hpp"

namespace sdl3cpp::services::impl {

std::vector<std::map<std::string, std::string>> ParseBspEntityLump(
    const std::string& entities) {
    std::vector<std::map<std::string, std::string>> parsed;
    size_t pos = 0;

    while (pos < entities.size()) {
        SkipWhitespace(entities, pos);
        if (pos >= entities.size()) {
            break;
        }
        if (entities[pos] != '{') {
            ++pos;
            continue;
        }

        ++pos;
        std::map<std::string, std::string> values;
        while (pos < entities.size()) {
            SkipWhitespace(entities, pos);
            if (pos >= entities.size()) {
                break;
            }
            if (entities[pos] == '}') {
                ++pos;
                break;
            }

            std::string key, value;
            if (!ReadQuotedToken(entities, pos, key) ||
                !ReadQuotedToken(entities, pos, value)) {
                break;
            }
            values[key] = value;
        }

        if (!values.empty()) {
            parsed.push_back(std::move(values));
        }
    }

    return parsed;
}

}  // namespace sdl3cpp::services::impl
