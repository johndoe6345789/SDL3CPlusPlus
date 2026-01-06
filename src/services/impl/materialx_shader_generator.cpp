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

#include <stdexcept>

namespace sdl3cpp::services::impl {
namespace mx = MaterialX;

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
    context.registerSourceCodeSearchPath(searchPath);

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
    return paths;
}

}  // namespace sdl3cpp::services::impl
