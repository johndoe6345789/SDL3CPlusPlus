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

#include <optional>
#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {
namespace mx = MaterialX;

namespace {

bool HasVertexDataBlock(const std::string& source) {
    return source.find("VertexData") != std::string::npos
        && source.find("vd;") != std::string::npos;
}

std::optional<std::string> ExtractVertexDataBlock(const std::string& source) {
    const std::string marker = "VertexData";
    const std::string instance = "vd;";
    size_t blockPos = source.find(marker);
    if (blockPos == std::string::npos) {
        return std::nullopt;
    }
    size_t instancePos = source.find(instance, blockPos);
    if (instancePos == std::string::npos) {
        return std::nullopt;
    }
    size_t lineStart = source.rfind('\n', blockPos);
    if (lineStart == std::string::npos) {
        lineStart = 0;
    } else {
        ++lineStart;
    }
    size_t lineEnd = source.find('\n', instancePos);
    if (lineEnd == std::string::npos) {
        lineEnd = source.size();
    }
    return source.substr(lineStart, lineEnd - lineStart);
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

    const bool vertexHasBlock = HasVertexDataBlock(paths.vertexSource);
    const bool fragmentHasBlock = HasVertexDataBlock(paths.fragmentSource);
    if (!vertexHasBlock && fragmentHasBlock) {
        auto fragmentBlock = ExtractVertexDataBlock(paths.fragmentSource);
        if (fragmentBlock) {
            std::string vertexBlock = ToVertexOutputBlock(*fragmentBlock);
            InsertAfterVersion(paths.vertexSource, vertexBlock);
            if (logger_) {
                logger_->Trace("MaterialXShaderGenerator", "Generate",
                               "vertexDataBlock=inserted");
            }
        } else if (logger_) {
            logger_->Trace("MaterialXShaderGenerator", "Generate",
                           "vertexDataBlock=missing");
        }
    } else if (logger_) {
        logger_->Trace("MaterialXShaderGenerator", "Generate",
                       "vertexDataBlock=" + std::string(vertexHasBlock ? "present" : "absent") +
                       ", fragmentVertexDataBlock=" + std::string(fragmentHasBlock ? "present" : "absent"));
    }
    return paths;
}

}  // namespace sdl3cpp::services::impl
