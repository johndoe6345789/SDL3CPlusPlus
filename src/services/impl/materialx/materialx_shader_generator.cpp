#include "materialx_shader_generator.hpp"
#include "services/impl/shader/shader_pipeline_validator.hpp"
#include "../../../core/vertex.hpp"

#include <MaterialXCore/Document.h>
#include <MaterialXFormat/File.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXFormat/XmlIo.h>
#include <MaterialXGenGlsl/VkShaderGenerator.h>
#include <MaterialXGenShader/GenContext.h>
#include <MaterialXGenShader/Shader.h>
#include <MaterialXGenShader/Util.h>
#include <MaterialXGenShader/HwShaderGenerator.h>
#include <MaterialXRender/Util.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <optional>
#include <set>
#include <unordered_set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <tuple>
#include <vector>

namespace sdl3cpp::services::impl {
namespace mx = MaterialX;

namespace {

std::optional<std::string> FindVertexDataBlock(const std::string& source) {
    const std::string blockName = "VertexData";
    const std::string instanceToken = "vd;";
    size_t searchPos = 0;
    while (true) {
        size_t blockPos = source.find(blockName, searchPos);
        if (blockPos == std::string::npos) {
            return std::nullopt;
        }
        size_t lineStart = source.rfind('\n', blockPos);
        if (lineStart == std::string::npos) {
            lineStart = 0;
        } else {
            ++lineStart;
        }
        size_t lineEnd = source.find('\n', blockPos);
        if (lineEnd == std::string::npos) {
            lineEnd = source.size();
        }
        std::string_view header(source.data() + lineStart, lineEnd - lineStart);
        if (header.find("layout") == std::string_view::npos) {
            searchPos = blockPos + blockName.size();
            continue;
        }
        size_t instancePos = source.find(instanceToken, blockPos);
        if (instancePos == std::string::npos) {
            searchPos = blockPos + blockName.size();
            continue;
        }
        size_t blockEnd = source.find('\n', instancePos);
        if (blockEnd == std::string::npos) {
            blockEnd = source.size();
        }
        return source.substr(lineStart, blockEnd - lineStart);
    }
}

bool UsesVertexDataInstance(const std::string& source) {
    return source.find("vd.") != std::string::npos;
}

std::string ToVertexOutputBlock(std::string block) {
    const std::string inToken = " in VertexData";
    const std::string outToken = " out VertexData";
    size_t tokenPos = block.find(inToken);
    if (tokenPos != std::string::npos) {
        block.replace(tokenPos, inToken.size(), outToken);
        return block;
    }
    tokenPos = block.find("in VertexData");
    if (tokenPos != std::string::npos) {
        block.replace(tokenPos, std::string("in VertexData").size(), "out VertexData");
    }
    return block;
}

void InsertAfterVersion(std::string& source, const std::string& block) {
    size_t lineEnd = source.find('\n');
    if (lineEnd == std::string::npos) {
        source.append("\n");
        source.append(block);
        source.append("\n");
        return;
    }
    ++lineEnd;
    source.insert(lineEnd, block + "\n");
}

std::vector<std::string> CollectMaterialXTokens(const std::string& source) {
    std::vector<std::string> tokens;
    std::unordered_set<std::string> seen;
    size_t pos = 0;
    while ((pos = source.find('$', pos)) != std::string::npos) {
        size_t start = pos;
        size_t end = pos + 1;
        while (end < source.size() &&
               std::isalnum(static_cast<unsigned char>(source[end]))) {
            ++end;
        }
        std::string token = end > start + 1 ? source.substr(start, end - start) : "$";
        if (seen.insert(token).second) {
            tokens.emplace_back(token);
        }
        pos = end;
    }
    return tokens;
}

std::string JoinTokens(const std::vector<std::string>& tokens, size_t limit) {
    const size_t count = std::min(tokens.size(), limit);
    std::string result;
    for (size_t i = 0; i < count; ++i) {
        if (i > 0) {
            result += ", ";
        }
        result += tokens[i];
    }
    return result;
}

void AddFallbackTokenSubstitutions(mx::StringMap& substitutions,
                                   const std::vector<std::string>& tokensInSource,
                                   const std::string& stageLabel,
                                   const std::shared_ptr<ILogger>& logger) {
    constexpr const char* kTexSamplerSignatureToken = "$texSamplerSignature";
    constexpr const char* kTexSamplerSampler2DToken = "$texSamplerSampler2D";
    constexpr const char* kClosureDataConstructorToken = "$closureDataConstructor";
    constexpr const char* kTexSamplerParameterName = "tex_sampler";
    constexpr const char* kTexSamplerSignatureReplacement = "sampler2D tex_sampler";
    constexpr const char* kClosureDataConstructorReplacement =
        "ClosureData(closureType, L, V, N, P, occlusion)";
    constexpr size_t kTokenLimit = 8;

    const auto tokenPresent = [&](const char* token) {
        return std::find(tokensInSource.begin(), tokensInSource.end(), token) != tokensInSource.end();
    };

    std::vector<std::string> addedTokens;
    const auto addIfMissing = [&](const char* token, const char* replacement) {
        if (!tokenPresent(token)) {
            return;
        }
        if (substitutions.find(token) != substitutions.end()) {
            return;
        }
        substitutions[token] = replacement;
        addedTokens.emplace_back(token);
    };

    addIfMissing(kTexSamplerSignatureToken, kTexSamplerSignatureReplacement);
    addIfMissing(kTexSamplerSampler2DToken, kTexSamplerParameterName);
    addIfMissing(kClosureDataConstructorToken, kClosureDataConstructorReplacement);

    if (logger && !addedTokens.empty()) {
        logger->Trace("MaterialXShaderGenerator", "Generate",
                      "tokenSubstitutionFallback stage=" + stageLabel,
                      "addedTokens=" + JoinTokens(addedTokens, kTokenLimit));
    }
}

template <typename T, typename = void>
struct HasHwAiryFresnelIterations : std::false_type {};

template <typename T>
struct HasHwAiryFresnelIterations<T, std::void_t<decltype(std::declval<T>().hwAiryFresnelIterations)>>
    : std::true_type {};

template <typename Options>
unsigned int ResolveAiryFresnelIterationsFromOptions(const Options& options,
                                                     unsigned int defaultIterations,
                                                     bool& fromOptions,
                                                     const std::shared_ptr<ILogger>& logger) {
    if constexpr (HasHwAiryFresnelIterations<Options>::value) {
        fromOptions = true;
        if (logger) {
            logger->Trace("MaterialXShaderGenerator", "Generate",
                          "airyFresnelIterationsOption=hwAiryFresnelIterations");
        }
        return options.hwAiryFresnelIterations;
    }

    if (logger) {
        logger->Trace("MaterialXShaderGenerator", "Generate",
                      "airyFresnelIterationsOption=unavailable");
    }
    return defaultIterations;
}

void ApplyTokenSubstitutions(const mx::ShaderGenerator& generator,
                             std::string& source,
                             const std::string& stageLabel,
                             unsigned int airyIterations,
                             const std::shared_ptr<ILogger>& logger) {
    auto tokensBefore = CollectMaterialXTokens(source);
    mx::StringMap substitutions = generator.getTokenSubstitutions();
    AddFallbackTokenSubstitutions(substitutions, tokensBefore, stageLabel, logger);
    mx::tokenSubstitution(substitutions, source);
    auto tokensAfter = CollectMaterialXTokens(source);

    if (logger && (!tokensBefore.empty() || !tokensAfter.empty())) {
        logger->Trace("MaterialXShaderGenerator", "Generate",
                      "tokenSubstitution stage=" + stageLabel +
                          ", tokensBefore=" + std::to_string(tokensBefore.size()) +
                          ", tokensAfter=" + std::to_string(tokensAfter.size()));
    }
    if (logger && !tokensAfter.empty()) {
        constexpr size_t kTokenLimit = 8;
        std::string message = "unresolvedTokens=" + JoinTokens(tokensAfter, kTokenLimit);
        if (tokensAfter.size() > kTokenLimit) {
            message += ", total=" + std::to_string(tokensAfter.size());
        }
        logger->Trace("MaterialXShaderGenerator", "Generate",
                      "tokenSubstitution stage=" + stageLabel,
                      message);
        logger->Error("MaterialX token substitution left unresolved tokens for " +
                      stageLabel + ": " + JoinTokens(tokensAfter, kTokenLimit));
    }

    // Add missing constants if not present.
    if (source.find("AIRY_FRESNEL_ITERATIONS") != std::string::npos &&
        source.find("#define AIRY_FRESNEL_ITERATIONS") == std::string::npos) {
        InsertAfterVersion(source, "#define AIRY_FRESNEL_ITERATIONS " + std::to_string(airyIterations));
        if (logger) {
            logger->Trace("MaterialXShaderGenerator", "Generate",
                          "tokenSubstitution stage=" + stageLabel,
                          "insertedDefine=AIRY_FRESNEL_ITERATIONS");
        }
    }
}

unsigned int ResolveAiryFresnelIterations(const mx::GenContext& context,
                                          const std::shared_ptr<ILogger>& logger) {
    constexpr unsigned int kDefaultAiryFresnelIterations = 4;
    bool fromOptions = false;
    const auto& options = context.getOptions();
    unsigned int iterations = ResolveAiryFresnelIterationsFromOptions(options,
                                                                      kDefaultAiryFresnelIterations,
                                                                      fromOptions,
                                                                      logger);
    if (logger) {
        logger->Trace("MaterialXShaderGenerator", "Generate",
                      "airyFresnelIterations=" + std::to_string(iterations) +
                          ", source=" + std::string(fromOptions ? "options" : "default"));
    }
    return iterations;
}

bool ReplaceFirstOccurrence(std::string& source, const std::string& before, const std::string& after) {
    size_t pos = source.find(before);
    if (pos == std::string::npos) {
        return false;
    }
    source.replace(pos, before.size(), after);
    return true;
}

std::string ConvertIndividualOutputsToBlock(const std::string& source,
                                            const std::shared_ptr<ILogger>& logger) {
    // Find individual output declarations like:
    // layout (location = N) out vec3 varname;
    // And convert them to a VertexData block

    std::vector<std::tuple<int, std::string, std::string>> outputs;  // location, type, name
    const std::string layoutToken = "layout (location =";
    const std::string layoutTokenCompact = "layout(location =";
    size_t searchPos = 0;
    size_t firstOutputStart = std::string::npos;
    size_t lastOutputEnd = 0;
    
    while (true) {
        size_t layoutPos = source.find(layoutToken, searchPos);
        size_t compactPos = source.find(layoutTokenCompact, searchPos);
        size_t tokenLength = 0;
        if (compactPos != std::string::npos &&
            (layoutPos == std::string::npos || compactPos < layoutPos)) {
            layoutPos = compactPos;
            tokenLength = layoutTokenCompact.size();
        } else {
            tokenLength = layoutToken.size();
        }
        if (layoutPos == std::string::npos) {
            break;
        }
        
        // Check if this line contains "out" (to confirm it's an output)
        size_t lineEnd = source.find('\n', layoutPos);
        if (lineEnd == std::string::npos) lineEnd = source.size();
        std::string line = source.substr(layoutPos, lineEnd - layoutPos);
        
        if (line.find(" out ") == std::string::npos ||
            line.find("VertexData") != std::string::npos) {
            searchPos = lineEnd;
            continue;
        }
        
        // Extract location number
        size_t locStart = layoutPos + tokenLength;  // after "layout (location ="
        while (locStart < source.size() && std::isspace(source[locStart])) ++locStart;
        size_t locEnd = locStart;
        while (locEnd < source.size() && std::isdigit(source[locEnd])) ++locEnd;
        if (locStart == locEnd) {
            searchPos = lineEnd;
            continue;
        }
        int location = std::stoi(source.substr(locStart, locEnd - locStart));
        
        // Find "out"
        size_t outPos = line.find(" out ");
        if (outPos == std::string::npos) {
            searchPos = lineEnd;
            continue;
        }
        outPos += layoutPos;  // Make absolute
        
        // Skip "out " and whitespace
        size_t typeStart = outPos + 5;  // after " out "
        while (typeStart < source.size() && std::isspace(source[typeStart])) ++typeStart;
        
        // Extract type
        size_t typeEnd = typeStart;
        while (typeEnd < source.size() && !std::isspace(source[typeEnd]) && source[typeEnd] != ';') ++typeEnd;
        std::string type = source.substr(typeStart, typeEnd - typeStart);
        
        // Extract variable name
        size_t nameStart = typeEnd;
        while (nameStart < source.size() && std::isspace(source[nameStart])) ++nameStart;
        size_t nameEnd = nameStart;
        while (nameEnd < source.size() && !std::isspace(source[nameEnd]) && source[nameEnd] != ';') ++nameEnd;
        std::string name = source.substr(nameStart, nameEnd - nameStart);
        
        if (name.empty() || type.empty()) {
            searchPos = lineEnd;
            continue;
        }
        
        outputs.push_back({location, type, name});
        
        // Track the range to replace
        if (firstOutputStart == std::string::npos) {
            firstOutputStart = layoutPos;
        }
        lastOutputEnd = lineEnd + 1;  // Include the newline
        
        searchPos = lastOutputEnd;
    }
    
    if (outputs.empty()) {
        return source;
    }
    
    std::sort(outputs.begin(), outputs.end(),
              [](const auto& left, const auto& right) {
                  return std::get<0>(left) < std::get<0>(right);
              });

    // Build the VertexData block while preserving explicit locations.
    std::string block = "out VertexData\n{\n";
    for (const auto& [loc, type, name] : outputs) {
        block += "    layout (location = " + std::to_string(loc) + ") " +
            type + " " + name + ";\n";
    }
    block += "} vd;\n\n";
    
    // Replace the individual outputs with the block
    std::string result = source.substr(0, firstOutputStart);
    result += block;
    result += source.substr(lastOutputEnd);
    
    if (logger) {
        logger->Trace("MaterialXShaderGenerator", "Generate",
                      "vertexOutputsConverted=" + std::to_string(outputs.size()));
    }
    return result;
}

std::string RemapVertexShaderInputLocations(const std::string& source,
                                             const std::shared_ptr<ILogger>& logger) {
    // Remap vertex shader input locations to match bgfx vertex layout order
    // WITHOUT creating an input block (which is not allowed in vertex shaders)

    std::map<std::string, int> bgfxLocationMap;
    bgfxLocationMap["i_position"] = 0;
    bgfxLocationMap["i_normal"] = 1;
    bgfxLocationMap["i_tangent"] = 2;
    bgfxLocationMap["i_texcoord_0"] = 3;
    bgfxLocationMap["i_texcoord_1"] = 4;
    bgfxLocationMap["i_color0"] = 5;

    std::string result = source;
    const std::string layoutToken = "layout (location =";
    const std::string layoutTokenCompact = "layout(location =";

    // Find all input declarations and remap their locations
    size_t searchPos = 0;
    while (true) {
        size_t layoutPos = result.find(layoutToken, searchPos);
        size_t compactPos = result.find(layoutTokenCompact, searchPos);
        size_t tokenLength = 0;

        if (compactPos != std::string::npos &&
            (layoutPos == std::string::npos || compactPos < layoutPos)) {
            layoutPos = compactPos;
            tokenLength = layoutTokenCompact.size();
        } else {
            tokenLength = layoutToken.size();
        }

        if (layoutPos == std::string::npos) {
            break;
        }

        // Check if this line contains " in " (vertex input)
        size_t lineEnd = result.find('\n', layoutPos);
        if (lineEnd == std::string::npos) lineEnd = result.size();
        std::string line = result.substr(layoutPos, lineEnd - layoutPos);

        if (line.find(" in ") == std::string::npos) {
            searchPos = lineEnd;
            continue;
        }

        // Extract variable name
        size_t inPos = line.find(" in ");
        size_t typeStart = inPos + 4;
        while (typeStart < line.size() && std::isspace(line[typeStart])) ++typeStart;
        size_t typeEnd = typeStart;
        while (typeEnd < line.size() && !std::isspace(line[typeEnd])) ++typeEnd;
        size_t nameStart = typeEnd;
        while (nameStart < line.size() && std::isspace(line[nameStart])) ++nameStart;
        size_t nameEnd = nameStart;
        while (nameEnd < line.size() && !std::isspace(line[nameEnd]) && line[nameEnd] != ';') ++nameEnd;
        std::string name = line.substr(nameStart, nameEnd - nameStart);

        // Check if we need to remap this attribute
        if (bgfxLocationMap.count(name) > 0) {
            int newLoc = bgfxLocationMap[name];

            // Find the old location number
            size_t locStart = layoutPos + tokenLength;
            while (locStart < result.size() && std::isspace(result[locStart])) ++locStart;
            size_t locEnd = locStart;
            while (locEnd < result.size() && std::isdigit(result[locEnd])) ++locEnd;

            std::string oldLocStr = result.substr(locStart, locEnd - locStart);
            std::string newLocStr = std::to_string(newLoc);

            // Replace the location number
            result.replace(locStart, locEnd - locStart, newLocStr);

            if (logger) {
                logger->Trace("MaterialXShaderGenerator", "RemapVertexShaderInputLocations",
                              "Remapped " + name + ": location " + oldLocStr + " -> " + newLocStr);
            }

            // Adjust searchPos for the length change
            searchPos = locStart + newLocStr.size();
        } else {
            searchPos = lineEnd;
        }
    }

    return result;
}

std::string ConvertIndividualInputsToBlock(const std::string& source,
                                           const std::shared_ptr<ILogger>& logger) {
    // Find individual input declarations like:
    // layout (location = N) in vec3 varname;
    // And convert them to a VertexData block

    std::vector<std::tuple<int, std::string, std::string>> inputs;  // location, type, name
    const std::string layoutToken = "layout (location =";
    const std::string layoutTokenCompact = "layout(location =";
    size_t searchPos = 0;
    size_t firstInputStart = std::string::npos;
    size_t lastInputEnd = 0;
    
    while (true) {
        size_t layoutPos = source.find(layoutToken, searchPos);
        size_t compactPos = source.find(layoutTokenCompact, searchPos);
        size_t tokenLength = 0;
        if (compactPos != std::string::npos &&
            (layoutPos == std::string::npos || compactPos < layoutPos)) {
            layoutPos = compactPos;
            tokenLength = layoutTokenCompact.size();
        } else {
            tokenLength = layoutToken.size();
        }
        if (layoutPos == std::string::npos) {
            break;
        }
        
        // Check if this line contains "in" (to confirm it's an input)
        size_t lineEnd = source.find('\n', layoutPos);
        if (lineEnd == std::string::npos) lineEnd = source.size();
        std::string line = source.substr(layoutPos, lineEnd - layoutPos);
        
        if (line.find(" in ") == std::string::npos ||
            line.find("VertexData") != std::string::npos) {
            searchPos = lineEnd;
            continue;
        }
        
        // Extract location number
        size_t locStart = layoutPos + tokenLength;  // after "layout (location ="
        while (locStart < source.size() && std::isspace(source[locStart])) ++locStart;
        size_t locEnd = locStart;
        while (locEnd < source.size() && std::isdigit(source[locEnd])) ++locEnd;
        if (locStart == locEnd) {
            searchPos = lineEnd;
            continue;
        }
        int location = std::stoi(source.substr(locStart, locEnd - locStart));
        
        // Find "in"
        size_t inPos = line.find(" in ");
        if (inPos == std::string::npos) {
            searchPos = lineEnd;
            continue;
        }
        inPos += layoutPos;  // Make absolute
        
        // Skip "in " and whitespace
        size_t typeStart = inPos + 4;  // after " in "
        while (typeStart < source.size() && std::isspace(source[typeStart])) ++typeStart;
        
        // Extract type
        size_t typeEnd = typeStart;
        while (typeEnd < source.size() && !std::isspace(source[typeEnd]) && source[typeEnd] != ';') ++typeEnd;
        std::string type = source.substr(typeStart, typeEnd - typeStart);
        
        // Extract variable name
        size_t nameStart = typeEnd;
        while (nameStart < source.size() && std::isspace(source[nameStart])) ++nameStart;
        size_t nameEnd = nameStart;
        while (nameEnd < source.size() && !std::isspace(source[nameEnd]) && source[nameEnd] != ';') ++nameEnd;
        std::string name = source.substr(nameStart, nameEnd - nameStart);
        
        if (name.empty() || type.empty()) {
            searchPos = lineEnd;
            continue;
        }
        
        inputs.push_back({location, type, name});
        
        // Track the range to replace
        if (firstInputStart == std::string::npos) {
            firstInputStart = layoutPos;
        }
        lastInputEnd = lineEnd + 1;  // Include the newline
        
        searchPos = lastInputEnd;
    }
    
    if (inputs.empty()) {
        return source;
    }
    
    // Remap locations to match bgfx VertexLayout order:
    // location=0: position (vec3)
    // location=1: normal (vec3)
    // location=2: texcoord (vec2)
    // location=3: color (vec3)
    // Note: tangent is NOT in our Vertex struct, so it gets assigned to location 4+
    std::map<std::string, int> nameToLocation;
    std::set<int> usedLocations;
    int nextAvailableLocation = 4;  // Start assigning non-standard attributes from location 4
    
    for (const auto& [loc, type, name] : inputs) {
        int newLoc = loc;  // default: keep original
        
        if (name.find("position") != std::string::npos) {
            newLoc = 0;
        } else if (name.find("normal") != std::string::npos) {
            newLoc = 1;
        } else if (name.find("texcoord") != std::string::npos) {
            newLoc = 2;
        } else if (name.find("color") != std::string::npos) {
            newLoc = 3;
        } else if (name.find("tangent") != std::string::npos) {
            // Tangent is not in our Vertex struct, assign to unused location
            newLoc = nextAvailableLocation++;
            if (logger) {
                logger->Trace("MaterialXShaderGenerator", "ConvertIndividualInputsToBlock",
                               "Tangent attribute found but not in Vertex struct, assigning location " + 
                               std::to_string(newLoc) + " (original was " + std::to_string(loc) + ")");
            }
        }
        
        // Check for location conflicts and assign a new location if needed
        while (usedLocations.count(newLoc) > 0) {
            newLoc = nextAvailableLocation++;
            if (logger) {
                logger->Trace("MaterialXShaderGenerator", "ConvertIndividualInputsToBlock",
                               "Location conflict for " + name + ", assigning location " + 
                               std::to_string(newLoc));
            }
        }
        
        if (logger && newLoc != loc) {
            logger->Trace("MaterialXShaderGenerator", "ConvertIndividualInputsToBlock",
                           "Remapping " + name + ": location " + std::to_string(loc) + 
                           " -> " + std::to_string(newLoc));
        }
        
        nameToLocation[name] = newLoc;
        usedLocations.insert(newLoc);
    }
    
    // Sort by remapped location
    std::vector<std::tuple<int, std::string, std::string>> remapped;
    for (const auto& [loc, type, name] : inputs) {
        remapped.push_back({nameToLocation[name], type, name});
    }
    std::sort(remapped.begin(), remapped.end(),
              [](const auto& left, const auto& right) {
                  return std::get<0>(left) < std::get<0>(right);
              });

    // Build the VertexData block with locations matching bgfx vertex layout order.
    // bgfx assigns locations sequentially: Position=0, Normal=1, Tangent=2, TexCoord0=3
    std::map<std::string, int> bgfxLocationMap;
    bgfxLocationMap["i_position"] = 0;
    bgfxLocationMap["i_normal"] = 1;
    bgfxLocationMap["i_tangent"] = 2;
    bgfxLocationMap["i_texcoord_0"] = 3;
    bgfxLocationMap["i_texcoord_1"] = 4;
    bgfxLocationMap["i_color0"] = 5;

    std::vector<std::tuple<int, std::string, std::string>> bgfxRemapped;
    for (const auto& [loc, type, name] : remapped) {
        int bgfxLoc = loc;  // default to original
        if (bgfxLocationMap.count(name) > 0) {
            bgfxLoc = bgfxLocationMap[name];
        }
        bgfxRemapped.push_back({bgfxLoc, type, name});
    }

    // Sort by bgfx location
    std::sort(bgfxRemapped.begin(), bgfxRemapped.end(),
              [](const auto& left, const auto& right) {
                  return std::get<0>(left) < std::get<0>(right);
              });

    std::string block = "in VertexData\n{\n";
    for (const auto& [loc, type, name] : bgfxRemapped) {
        block += "    layout (location = " + std::to_string(loc) + ") " +
            type + " " + name + ";\n";
        if (logger) {
            logger->Trace("MaterialXShaderGenerator", "ConvertIndividualInputsToBlock",
                          "Input " + name + " assigned to location " + std::to_string(loc));
        }
    }
    block += "} vd;\n\n";
    
    // Replace the individual inputs with the block
    std::string result = source.substr(0, firstInputStart);
    result += block;
    result += source.substr(lastInputEnd);
    
    if (logger) {
        logger->Trace("MaterialXShaderGenerator", "Generate",
                      "fragmentInputsConverted=" + std::to_string(inputs.size()));
    }
    return result;
}

std::vector<std::filesystem::path> BuildTextureSearchRoots(
    const std::filesystem::path& documentPath,
    const std::filesystem::path& libraryPath,
    const std::filesystem::path& scriptDirectory) {
    std::vector<std::filesystem::path> roots;
    auto addRoot = [&](const std::filesystem::path& root) {
        if (root.empty()) {
            return;
        }
        std::error_code ec;
        auto canonical = std::filesystem::weakly_canonical(root, ec);
        if (ec) {
            canonical = root;
        }
        roots.push_back(canonical);
    };

    if (!documentPath.empty()) {
        addRoot(documentPath.parent_path());
    }
    if (!libraryPath.empty()) {
        addRoot(libraryPath);
        addRoot(libraryPath.parent_path());
    }
    if (!scriptDirectory.empty()) {
        addRoot(scriptDirectory);
        addRoot(scriptDirectory.parent_path());
    }
    addRoot(std::filesystem::current_path());

    std::unordered_set<std::string> seen;
    std::vector<std::filesystem::path> unique;
    for (const auto& root : roots) {
        auto key = root.string();
        if (seen.insert(key).second) {
            unique.push_back(root);
        }
    }
    return unique;
}

std::filesystem::path ResolveTexturePath(const std::string& filename,
                                         const std::filesystem::path& documentPath,
                                         const std::vector<std::filesystem::path>& searchRoots,
                                         const std::shared_ptr<ILogger>& logger) {
    if (filename.empty()) {
        return {};
    }

    std::filesystem::path candidate(filename);
    auto tryPath = [&](const std::filesystem::path& path) -> std::filesystem::path {
        std::error_code ec;
        if (std::filesystem::exists(path, ec)) {
            return std::filesystem::weakly_canonical(path, ec);
        }
        return {};
    };

    if (candidate.is_absolute()) {
        auto resolved = tryPath(candidate);
        if (!resolved.empty()) {
            return resolved;
        }
    }

    if (!documentPath.empty()) {
        auto resolved = tryPath(documentPath.parent_path() / candidate);
        if (!resolved.empty()) {
            return resolved;
        }
    }

    for (const auto& root : searchRoots) {
        auto resolved = tryPath(root / candidate);
        if (!resolved.empty()) {
            return resolved;
        }
    }

    if (!candidate.has_parent_path()) {
        for (const auto& root : searchRoots) {
            std::error_code ec;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(root, ec)) {
                if (ec) {
                    break;
                }
                if (!entry.is_regular_file(ec)) {
                    continue;
                }
                if (entry.path().filename() == candidate) {
                    return entry.path();
                }
            }
        }
    }

    if (logger) {
        logger->Trace("MaterialXShaderGenerator", "Generate",
                      "texturePathResolutionFailed file=" + filename);
    }
    return {};
}

std::vector<ShaderPaths::TextureBinding> CollectTextureBindings(
    const mx::Shader& shader,
    const std::filesystem::path& documentPath,
    const std::filesystem::path& libraryPath,
    const std::filesystem::path& scriptDirectory,
    const std::shared_ptr<ILogger>& logger) {
    std::vector<ShaderPaths::TextureBinding> bindings;
    const mx::ShaderStage& pixelStage = shader.getStage(mx::Stage::PIXEL);
    const auto& uniformBlocks = pixelStage.getUniformBlocks();
    if (uniformBlocks.empty()) {
        return bindings;
    }

    const auto searchRoots = BuildTextureSearchRoots(documentPath, libraryPath, scriptDirectory);
    std::unordered_set<std::string> seenUniforms;

    for (const auto& entry : uniformBlocks) {
        const auto& block = entry.second;
        if (!block) {
            continue;
        }
        for (size_t i = 0; i < block->size(); ++i) {
            const mx::ShaderPort* port = (*block)[i];
            if (!port) {
                continue;
            }
            const auto& type = port->getType();
            if (type.getName() != "filename") {
                continue;
            }
            mx::ValuePtr value = port->getValue();
            if (!value) {
                continue;
            }
            std::string file = value->getValueString();
            if (file.empty()) {
                continue;
            }
            std::string uniformName = port->getVariable();
            if (uniformName.empty()) {
                uniformName = port->getName();
            }
            if (uniformName.empty()) {
                continue;
            }
            if (!seenUniforms.insert(uniformName).second) {
                continue;
            }
            auto resolved = ResolveTexturePath(file, documentPath, searchRoots, logger);
            if (resolved.empty()) {
                continue;
            }
            ShaderPaths::TextureBinding binding;
            binding.uniformName = uniformName;
            binding.path = resolved.string();
            bindings.push_back(std::move(binding));
            if (logger) {
                logger->Trace("MaterialXShaderGenerator", "Generate",
                              "textureBinding uniform=" + uniformName +
                                  ", file=" + file +
                                  ", resolved=" + resolved.string());
            }
        }
    }

    return bindings;
}

}  // namespace

MaterialXShaderGenerator::MaterialXShaderGenerator(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::filesystem::path MaterialXShaderGenerator::ResolvePath(
    const std::filesystem::path& path,
    const std::filesystem::path& scriptDirectory) const {
    if (path.empty()) {
        return {};
    }
    if (path.is_absolute()) {
        return path;
    }
    std::filesystem::path base = scriptDirectory;
    if (!base.empty()) {
        auto projectRoot = base.parent_path();
        if (!projectRoot.empty()) {
            return std::filesystem::weakly_canonical(projectRoot / path);
        }
    }
    return std::filesystem::weakly_canonical(path);
}

ShaderPaths MaterialXShaderGenerator::Generate(const MaterialXConfig& config,
                                               const std::filesystem::path& scriptDirectory) const {
    if (!config.enabled) {
        return {};
    }
    if (logger_) {
        logger_->Trace("MaterialXShaderGenerator", "Generate", "enabled=true");
    }

    mx::FileSearchPath searchPath;
    std::filesystem::path libraryPath = ResolvePath(config.libraryPath, scriptDirectory);
    if (libraryPath.empty() && !scriptDirectory.empty()) {
        auto fallback = scriptDirectory.parent_path() / "MaterialX" / "libraries";
        if (std::filesystem::exists(fallback)) {
            libraryPath = fallback;
        }
    }
    if (!libraryPath.empty()) {
        searchPath.append(mx::FilePath(libraryPath.string()));
    }
    mx::FileSearchPath sourceSearchPath = searchPath;
    if (!libraryPath.empty() && libraryPath.filename() == "libraries") {
        std::filesystem::path libraryRoot = libraryPath.parent_path();
        if (!libraryRoot.empty()) {
            sourceSearchPath.append(mx::FilePath(libraryRoot.string()));
        }
    }
    if (logger_) {
        logger_->Trace("MaterialXShaderGenerator", "Generate",
                       "libraryPath=" + libraryPath.string() +
                       ", libraryFolders=" + std::to_string(config.libraryFolders.size()));
    }

    mx::DocumentPtr stdLib = mx::createDocument();
    if (!config.libraryFolders.empty()) {
        mx::FilePathVec folders;
        for (const auto& folder : config.libraryFolders) {
            folders.emplace_back(folder);
        }
        mx::loadLibraries(folders, searchPath, stdLib);
    }

    mx::ShaderGeneratorPtr generator = mx::VkShaderGenerator::create();
    mx::GenContext context(generator);
    context.registerSourceCodeSearchPath(sourceSearchPath);

    mx::ShaderPtr shader;
    std::filesystem::path documentPath;
    if (config.useConstantColor) {
        mx::Color3 color(config.constantColor[0], config.constantColor[1], config.constantColor[2]);
        shader = mx::createConstantShader(context, stdLib, config.shaderKey, color);
        if (logger_) {
            logger_->Trace("MaterialXShaderGenerator", "Generate",
                           "usingConstantColor=true, shaderKey=" + config.shaderKey);
        }
    } else {
        if (config.documentPath.empty()) {
            throw std::runtime_error("MaterialX document path is required when use_constant_color is false");
        }

        documentPath = ResolvePath(config.documentPath, scriptDirectory);
        if (documentPath.empty()) {
            throw std::runtime_error("MaterialX document path could not be resolved");
        }
        if (logger_) {
            logger_->Trace("MaterialXShaderGenerator", "Generate",
                           "documentPath=" + documentPath.string() +
                           ", materialName=" + config.materialName);
        }

        mx::DocumentPtr document = mx::createDocument();
        mx::readFromXmlFile(document, mx::FilePath(documentPath.string()), searchPath);
        document->importLibrary(stdLib);

        mx::TypedElementPtr element;
        if (!config.materialName.empty()) {
            auto renderables = mx::findRenderableElements(document);
            for (const auto& candidate : renderables) {
                if (candidate && candidate->getName() == config.materialName) {
                    element = candidate;
                    break;
                }
            }
            if (!element) {
                mx::NodePtr node = document->getNode(config.materialName);
                if (node && (node->getCategory() == "surfacematerial"
                    || node->getType() == "surfaceshader")) {
                    element = node;
                }
            }
            if (!element) {
                mx::OutputPtr output = document->getOutput(config.materialName);
                if (output) {
                    element = output;
                }
            }
        }
        if (!element) {
            auto renderables = mx::findRenderableElements(document);
            if (!renderables.empty()) {
                element = renderables.front();
            }
        }
        if (!element) {
            throw std::runtime_error("MaterialX document has no renderable elements");
        }
        if (logger_) {
            logger_->Trace("MaterialXShaderGenerator", "Generate",
                           "selectedElement=" + element->getName() +
                           ", category=" + element->getCategory() +
                           ", type=" + element->getType());
        }

        shader = mx::createShader(config.shaderKey, context, element);
    }

    if (!shader) {
        throw std::runtime_error("MaterialX shader generation failed");
    }

    ShaderPaths paths;
    paths.vertexSource = shader->getSourceCode(mx::Stage::VERTEX);
    paths.fragmentSource = shader->getSourceCode(mx::Stage::PIXEL);
    paths.textures = CollectTextureBindings(*shader, documentPath, libraryPath, scriptDirectory, logger_);

    // Log raw vertex shader inputs to debug Vulkan vertex attribute location mismatch
    if (logger_) {
        logger_->Trace("MaterialXShaderGenerator", "Generate", "RAW_VERTEX_SHADER:\n" +
                       paths.vertexSource.substr(0, std::min(size_t(800), paths.vertexSource.size())));
    }

    // Fix vertex shader inputs: remap locations to match bgfx vertex layout order
    // Note: We DON'T create an input block for vertex shaders (GLSL doesn't allow it)
    paths.vertexSource = RemapVertexShaderInputLocations(paths.vertexSource, logger_);

    // Fix vertex shader outputs: convert individual layout outputs to VertexData block
    // MaterialX VkShaderGenerator incorrectly emits individual out variables instead of
    // a VertexData struct block, which causes compilation errors when the shader code
    // references vd.normalWorld etc. We convert them here as a workaround.
    paths.vertexSource = ConvertIndividualOutputsToBlock(paths.vertexSource, logger_);

    // Fix fragment shader inputs: convert individual layout inputs to VertexData block
    paths.fragmentSource = ConvertIndividualInputsToBlock(paths.fragmentSource, logger_);
    
    // Ensure any remaining MaterialX tokens are substituted using the generator's map.
    const unsigned int airyIterations = ResolveAiryFresnelIterations(context, logger_);
    ApplyTokenSubstitutions(context.getShaderGenerator(), paths.vertexSource, "vertex", airyIterations, logger_);
    ApplyTokenSubstitutions(context.getShaderGenerator(), paths.fragmentSource, "fragment", airyIterations, logger_);

    auto vertexBlock = FindVertexDataBlock(paths.vertexSource);
    auto fragmentBlock = FindVertexDataBlock(paths.fragmentSource);
    const bool vertexUsesInstance = UsesVertexDataInstance(paths.vertexSource);
    bool vertexHasBlock = vertexBlock.has_value();
    const bool fragmentHasBlock = fragmentBlock.has_value();

    if (vertexHasBlock) {
        std::string normalizedBlock = ToVertexOutputBlock(*vertexBlock);
        if (normalizedBlock != *vertexBlock) {
            if (ReplaceFirstOccurrence(paths.vertexSource, *vertexBlock, normalizedBlock)) {
                if (logger_) {
                    logger_->Trace("MaterialXShaderGenerator", "Generate",
                                   "vertexDataBlock=normalized");
                }
            }
        }
    } else if (fragmentHasBlock) {
        std::string vertexOutBlock = ToVertexOutputBlock(*fragmentBlock);
        InsertAfterVersion(paths.vertexSource, vertexOutBlock);
        vertexHasBlock = true;
        if (logger_) {
            logger_->Trace("MaterialXShaderGenerator", "Generate",
                           "vertexDataBlock=inserted");
        }
    } else if (logger_) {
        logger_->Trace("MaterialXShaderGenerator", "Generate",
                       "vertexDataBlock=missing, fragmentVertexDataBlock=missing");
    }

    if (logger_) {
        logger_->Trace("MaterialXShaderGenerator", "Generate",
                       "vertexDataBlock=" + std::string(vertexHasBlock ? "present" : "absent") +
                       ", fragmentVertexDataBlock=" + std::string(fragmentHasBlock ? "present" : "absent") +
                       ", vertexUsesVertexData=" + std::string(vertexUsesInstance ? "true" : "false"));
    }

    // ===  MEGA-STRICT SHADER PIPELINE VALIDATION ===
    // Validate the shader pipeline BEFORE returning to prevent GPU driver crashes
    ShaderPipelineValidator validator(logger_);

    // Define expected vertex layout (must match bgfx_graphics_backend.cpp)
    std::vector<ShaderPipelineValidator::AttributeInfo> expectedLayout = {
        ShaderPipelineValidator::AttributeInfo(0, "vec3", "Position", 12),
        ShaderPipelineValidator::AttributeInfo(1, "vec3", "Normal", 12),
        ShaderPipelineValidator::AttributeInfo(2, "vec3", "Tangent", 12),
        ShaderPipelineValidator::AttributeInfo(3, "vec2", "TexCoord0", 8),
        ShaderPipelineValidator::AttributeInfo(4, "vec3", "Color0", 12),
    };

    // Validate the complete pipeline
    std::string pipelineName = config.documentPath.filename().string();
    auto result = validator.ValidatePipeline(
        paths.vertexSource,
        paths.fragmentSource,
        expectedLayout,
        sizeof(core::Vertex),  // Actual vertex struct size
        pipelineName
    );

    // Log the validation result
    validator.LogValidationResult(result, "MaterialX Pipeline: " + pipelineName);

    // CRITICAL: If validation fails, throw an exception to prevent GPU driver crash
    if (!result.passed) {
        std::string errorMsg = "Shader pipeline validation failed for '" + pipelineName + "':\n";
        for (const auto& error : result.errors) {
            errorMsg += "  - " + error + "\n";
        }
        throw std::runtime_error(errorMsg);
    }

    return paths;
}

}  // namespace sdl3cpp::services::impl
