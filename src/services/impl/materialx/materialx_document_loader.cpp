#include "materialx_document_loader.hpp"
#include "materialx_search_path_builder.hpp"

#include <MaterialXFormat/Util.h>
#include <MaterialXFormat/XmlIo.h>

namespace sdl3cpp::services::impl {

MaterialX::DocumentPtr MaterialXDocumentLoader::Load(const std::filesystem::path& documentPath,
                                                     const MaterialXConfig& config,
                                                     const std::filesystem::path& scriptDirectory) const {
    MaterialX::DocumentPtr document = MaterialX::createDocument();
    MaterialXSearchPathBuilder builder;
    auto searchPath = builder.Build(config, scriptDirectory);
    MaterialX::readFromXmlFile(document, MaterialX::FilePath(documentPath.string()), searchPath);

    if (!config.libraryFolders.empty()) {
        MaterialX::DocumentPtr stdLib = MaterialX::createDocument();
        MaterialX::FilePathVec folders;
        folders.reserve(config.libraryFolders.size());
        for (const auto& folder : config.libraryFolders) {
            folders.emplace_back(folder);
        }
        MaterialX::loadLibraries(folders, searchPath, stdLib);
        document->importLibrary(stdLib);
    }

    return document;
}

}  // namespace sdl3cpp::services::impl
