#include "materialx_shader_generator.hpp"

#include <MaterialXCore/Document.h>
#include <MaterialXFormat/File.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXFormat/XmlIo.h>
#include <MaterialXGenGlsl/VkShaderGenerator.h>
#include <MaterialXGenShader/GenContext.h>
#include <MaterialXGenShader/Shader.h>
#include <MaterialXGenShader/Util.h>
#include <MaterialXRender/Util.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <optional>
#include <unordered_set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
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

template <typename T>
constexpr bool HasHwAiryFresnelIterations = requires(const T& options) {
    options.hwAiryFresnelIterations;
};

void ApplyTokenSubstitutions(const mx::ShaderGenerator& generator,
                             std::string& source,
                             const std::string& stageLabel,
                             unsigned int airyIterations,
                             const std::shared_ptr<ILogger>& logger) {
    auto tokensBefore = CollectMaterialXTokens(source);
    mx::tokenSubstitution(generator.getTokenSubstitutions(), source);
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
    unsigned int iterations = kDefaultAiryFresnelIterations;
    bool fromOptions = false;
    using OptionsType = std::remove_reference_t<decltype(context.getOptions())>;
    constexpr bool kHasHwAiryFresnelIterations = HasHwAiryFresnelIterations<OptionsType>;
    if constexpr (kHasHwAiryFresnelIterations) {
        iterations = context.getOptions().hwAiryFresnelIterations;
        fromOptions = true;
    } else if (logger) {
        logger->Trace("MaterialXShaderGenerator", "Generate",
                      "airyFresnelIterationsOption=unavailable");
    }
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

std::string ConvertIndividualOutputsToBlock(const std::string& source) {
    // Find individual output declarations like:
    // layout (location = N) out vec3 varname;
    // And convert them to a VertexData block

    std::vector<std::tuple<int, std::string, std::string>> outputs;  // location, type, name
    size_t searchPos = 0;
    size_t firstOutputStart = std::string::npos;
    size_t lastOutputEnd = 0;
    
    while (true) {
        size_t layoutPos = source.find("layout (location =", searchPos);
        if (layoutPos == std::string::npos) break;
        
        // Check if this line contains "out" (to confirm it's an output)
        size_t lineEnd = source.find('\n', layoutPos);
        if (lineEnd == std::string::npos) lineEnd = source.size();
        std::string line = source.substr(layoutPos, lineEnd - layoutPos);
        
        if (line.find(" out ") == std::string::npos) {
            searchPos = lineEnd;
            continue;
        }
        
        // Extract location number
        size_t locStart = layoutPos + 18;  // after "layout (location ="
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
    
    // Build the VertexData block
    std::string block = "layout (location = 0) out VertexData\n{\n";
    for (const auto& [loc, type, name] : outputs) {
        block += "    " + type + " " + name + ";\n";
    }
    block += "} vd;\n\n";
    
    // Replace the individual outputs with the block
    std::string result = source.substr(0, firstOutputStart);
    result += block;
    result += source.substr(lastOutputEnd);
    
    return result;
}

std::string ConvertIndividualInputsToBlock(const std::string& source) {
    // Find individual input declarations like:
    // layout (location = N) in vec3 varname;
    // And convert them to a VertexData block

    std::vector<std::tuple<int, std::string, std::string>> inputs;  // location, type, name
    size_t searchPos = 0;
    size_t firstInputStart = std::string::npos;
    size_t lastInputEnd = 0;
    
    while (true) {
        size_t layoutPos = source.find("layout (location =", searchPos);
        if (layoutPos == std::string::npos) break;
        
        // Check if this line contains "in" (to confirm it's an input)
        size_t lineEnd = source.find('\n', layoutPos);
        if (lineEnd == std::string::npos) lineEnd = source.size();
        std::string line = source.substr(layoutPos, lineEnd - layoutPos);
        
        // Skip lines with "in vec3 i_" (vertex inputs)
        if (line.find(" in ") == std::string::npos || line.find(" in vec3 i_") != std::string::npos) {
            searchPos = lineEnd;
            continue;
        }
        
        // Extract location number
        size_t locStart = layoutPos + 18;  // after "layout (location ="
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
    
    // Build the VertexData block
    std::string block = "layout (location = 0) in VertexData\n{\n";
    for (const auto& [loc, type, name] : inputs) {
        block += "    " + type + " " + name + ";\n";
    }
    block += "} vd;\n\n";
    
    // Replace the individual inputs with the block
    std::string result = source.substr(0, firstInputStart);
    result += block;
    result += source.substr(lastInputEnd);
    
    return result;
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

        std::filesystem::path documentPath = ResolvePath(config.documentPath, scriptDirectory);
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

    // Fix vertex shader outputs: convert individual layout outputs to VertexData block
    // MaterialX VkShaderGenerator incorrectly emits individual out variables instead of
    // a VertexData struct block, which causes compilation errors when the shader code
    // references vd.normalWorld etc. We convert them here as a workaround.
    paths.vertexSource = ConvertIndividualOutputsToBlock(paths.vertexSource);
    
    // Fix fragment shader inputs: convert individual layout inputs to VertexData block
    paths.fragmentSource = ConvertIndividualInputsToBlock(paths.fragmentSource);
    
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
    return paths;
}

}  // namespace sdl3cpp::services::impl
