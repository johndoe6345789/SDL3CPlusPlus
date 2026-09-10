#include "services/interfaces/workflow/workflow_generic_steps/string_template.hpp"

#include <regex>
#include <stdexcept>
#include <unordered_map>

namespace sdl3cpp::services::impl {

std::string AnyToString(const std::any& value) {
    if (auto* s = std::any_cast<std::string>(&value)) {
        return *s;
    }
    if (auto* d = std::any_cast<double>(&value)) {
        return std::to_string(*d);
    }
    if (auto* i = std::any_cast<int>(&value)) {
        return std::to_string(*i);
    }
    if (auto* b = std::any_cast<bool>(&value)) {
        return *b ? "true" : "false";
    }
    return "";
}

std::string InterpolateTemplate(WorkflowContext& context,
                                const std::string& templateContent,
                                const std::string& valuesKey) {
    std::regex placeholder_regex(R"(\{([^}]+)\})");
    std::smatch match;
    std::string::const_iterator search_start(templateContent.cbegin());

    std::string formatted;
    while (std::regex_search(search_start, templateContent.cend(), match,
                             placeholder_regex)) {
        formatted.append(match.prefix().first, match.prefix().second);

        std::string placeholder_name = match[1].str();

        std::string replacement;
        const auto* value = context.TryGetAny(placeholder_name);
        if (value) {
            replacement = AnyToString(*value);
        } else if (!valuesKey.empty()) {
            const auto* valuesMap =
                context.TryGet<std::unordered_map<std::string, std::string>>(
                    valuesKey);
            if (valuesMap) {
                auto mapIt = valuesMap->find(placeholder_name);
                if (mapIt != valuesMap->end()) {
                    replacement = mapIt->second;
                } else {
                    throw std::runtime_error("string.format: placeholder '{" +
                                             placeholder_name +
                                             "}' not found in values map");
                }
            } else {
                throw std::runtime_error("string.format: placeholder '{" +
                                         placeholder_name + "}' not found");
            }
        } else {
            throw std::runtime_error("string.format: placeholder '{" +
                                     placeholder_name + "}' not found");
        }

        formatted.append(replacement);
        search_start = match.suffix().first;
    }
    formatted.append(search_start, templateContent.cend());
    return formatted;
}

}  // namespace sdl3cpp::services::impl
