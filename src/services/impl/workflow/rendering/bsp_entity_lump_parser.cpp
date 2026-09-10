#include "services/interfaces/workflow/rendering/bsp_entity_lump_parser.hpp"

#include <cctype>
#include <cstdio>

namespace sdl3cpp::services::impl {
namespace {

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

}  // namespace

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

bool ParseBspVec3(const std::string& text, float& x, float& y, float& z) {
    return std::sscanf(text.c_str(), "%f %f %f", &x, &y, &z) == 3;
}

std::vector<BspModel> ReadBspModels(const std::vector<uint8_t>& bspData,
                                    const BspLump& modelLump) {
    std::vector<BspModel> models;
    const bool inBounds =
        modelLump.length >= 0 && modelLump.offset >= 0 &&
        static_cast<size_t>(modelLump.offset + modelLump.length) <=
            bspData.size();
    if (!inBounds) {
        return models;
    }
    const auto* modelData =
        reinterpret_cast<const BspModel*>(bspData.data() + modelLump.offset);
    const size_t modelCount =
        static_cast<size_t>(modelLump.length) / sizeof(BspModel);
    models.assign(modelData, modelData + modelCount);
    return models;
}

}  // namespace sdl3cpp::services::impl
