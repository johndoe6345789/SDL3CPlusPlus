#include "mesh_service.hpp"
#include <utility>

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <filesystem>
#include <limits>
#include <lua.hpp>
#include <system_error>
#include <zip.h>

namespace sdl3cpp::services::impl {
namespace {
constexpr unsigned int kAssimpLoadFlags =
    aiProcess_Triangulate |
    aiProcess_JoinIdenticalVertices |
    aiProcess_PreTransformVertices |
    aiProcess_GenNormals;

struct ZipArchiveDeleter {
    void operator()(zip_t* archive) const {
        if (archive) {
            zip_close(archive);
        }
    }
};

struct ZipFileDeleter {
    void operator()(zip_file_t* file) const {
        if (file) {
            zip_fclose(file);
        }
    }
};

std::string BuildZipErrorMessage(int errorCode) {
    zip_error_t zipError;
    zip_error_init_with_code(&zipError, errorCode);
    std::string message = zip_error_strerror(&zipError);
    zip_error_fini(&zipError);
    return message;
}

std::string BuildZipArchiveErrorMessage(zip_t* archive) {
    if (!archive) {
        return "unknown zip archive error";
    }
    zip_error_t* zipError = zip_get_error(archive);
    if (!zipError) {
        return "unknown zip archive error";
    }
    return zip_error_strerror(zipError);
}

std::string GetExtensionHint(const std::string& entryPath, const std::string& fallback) {
    std::filesystem::path entry(entryPath);
    std::string ext = entry.extension().string();
    if (!ext.empty() && ext.front() == '.') {
        ext.erase(ext.begin());
    }
    if (!ext.empty()) {
        return ext;
    }
    return fallback;
}

aiColor3D ResolveMaterialColor(const aiScene* scene, const aiMesh* mesh) {
    aiColor3D defaultColor(0.6f, 0.8f, 1.0f);
    if (!scene || !mesh) {
        return defaultColor;
    }
    if (mesh->mMaterialIndex < scene->mNumMaterials) {
        const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        aiColor4D diffuse;
        if (material && material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS) {
            return aiColor3D(diffuse.r, diffuse.g, diffuse.b);
        }
    }
    return defaultColor;
}

bool AppendMeshPayload(const aiScene* scene,
                       const aiMesh* mesh,
                       MeshPayload& outPayload,
                       std::string& outError,
                       size_t& outIndicesAdded) {
    outIndicesAdded = 0;
    if (!mesh || !mesh->mNumVertices) {
        outError = "Mesh contains no vertices";
        return false;
    }

    size_t vertexOffset = outPayload.positions.size();
    if (vertexOffset > std::numeric_limits<uint32_t>::max()) {
        outError = "Mesh vertex count exceeds uint32_t index range";
        return false;
    }

    aiColor3D materialColor = ResolveMaterialColor(scene, mesh);

    size_t positionsStart = outPayload.positions.size();
    size_t normalsStart = outPayload.normals.size();
    size_t colorsStart = outPayload.colors.size();
    size_t indicesStart = outPayload.indices.size();

    outPayload.positions.reserve(positionsStart + mesh->mNumVertices);
    outPayload.normals.reserve(normalsStart + mesh->mNumVertices);
    outPayload.colors.reserve(colorsStart + mesh->mNumVertices);
    outPayload.indices.reserve(indicesStart + mesh->mNumFaces * 3);

    for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
        const aiVector3D& vertex = mesh->mVertices[i];
        outPayload.positions.push_back({vertex.x, vertex.y, vertex.z});

        aiVector3D normal(0.0f, 0.0f, 1.0f);
        if (mesh->HasNormals()) {
            normal = mesh->mNormals[i];
        }
        outPayload.normals.push_back({normal.x, normal.y, normal.z});

        aiColor3D color = materialColor;
        if (mesh->HasVertexColors(0) && mesh->mColors[0]) {
            const aiColor4D& vertexColor = mesh->mColors[0][i];
            color = aiColor3D(vertexColor.r, vertexColor.g, vertexColor.b);
        }
        outPayload.colors.push_back({color.r, color.g, color.b});
    }

    for (unsigned faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
        const aiFace& face = mesh->mFaces[faceIndex];
        if (face.mNumIndices != 3) {
            continue;
        }
        outPayload.indices.push_back(static_cast<uint32_t>(face.mIndices[0]) +
                                     static_cast<uint32_t>(vertexOffset));
        outPayload.indices.push_back(static_cast<uint32_t>(face.mIndices[1]) +
                                     static_cast<uint32_t>(vertexOffset));
        outPayload.indices.push_back(static_cast<uint32_t>(face.mIndices[2]) +
                                     static_cast<uint32_t>(vertexOffset));
    }

    outIndicesAdded = outPayload.indices.size() - indicesStart;
    if (outIndicesAdded == 0) {
        outPayload.positions.resize(positionsStart);
        outPayload.normals.resize(normalsStart);
        outPayload.colors.resize(colorsStart);
        outPayload.indices.resize(indicesStart);
        outError = "Mesh contains no triangle faces";
        return false;
    }

    return true;
}

bool BuildPayloadFromScene(const aiScene* scene,
                           bool combineMeshes,
                           MeshPayload& outPayload,
                           std::string& outError,
                           const std::shared_ptr<ILogger>& logger) {
    if (!scene) {
        outError = "Assimp scene is null";
        return false;
    }
    if (scene->mNumMeshes == 0) {
        outError = "Scene contains no meshes";
        return false;
    }

    outPayload.positions.clear();
    outPayload.normals.clear();
    outPayload.colors.clear();
    outPayload.indices.clear();

    if (!combineMeshes) {
        size_t indicesAdded = 0;
        if (!AppendMeshPayload(scene, scene->mMeshes[0], outPayload, outError, indicesAdded)) {
            return false;
        }
        return true;
    }

    size_t totalIndicesAdded = 0;
    for (unsigned meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        const aiMesh* mesh = scene->mMeshes[meshIndex];
        std::string meshError;
        size_t indicesAdded = 0;
        if (!AppendMeshPayload(scene, mesh, outPayload, meshError, indicesAdded)) {
            if (logger) {
                logger->Trace("MeshService", "BuildPayloadFromScene",
                              "Skipping mesh " + std::to_string(meshIndex) + ": " + meshError);
            }
            continue;
        }
        totalIndicesAdded += indicesAdded;
    }

    if (totalIndicesAdded == 0) {
        outError = "Scene contains no triangle faces";
        return false;
    }

    return true;
}
}  // namespace

MeshService::MeshService(std::shared_ptr<IConfigService> configService,
                         std::shared_ptr<ILogger> logger)
    : configService_(std::move(configService)),
      logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("MeshService", "MeshService",
                       "configService=" + std::string(configService_ ? "set" : "null"));
    }
}

bool MeshService::LoadFromFile(const std::string& requestedPath,
                               MeshPayload& outPayload,
                               std::string& outError) {
    if (logger_) {
        logger_->Trace("MeshService", "LoadFromFile",
                       "requestedPath=" + requestedPath);
    }
    std::filesystem::path resolved;
    if (!ResolvePath(requestedPath, resolved, outError)) {
        return false;
    }

    if (!std::filesystem::exists(resolved)) {
        outError = "Mesh file not found: " + resolved.string();
        return false;
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(resolved.string(), kAssimpLoadFlags);

    if (!scene) {
        outError = importer.GetErrorString() ? importer.GetErrorString() : "Assimp failed to load mesh";
        return false;
    }
    return BuildPayloadFromScene(scene, false, outPayload, outError, logger_);
}

bool MeshService::LoadFromArchive(const std::string& archivePath,
                                  const std::string& entryPath,
                                  MeshPayload& outPayload,
                                  std::string& outError) {
    if (logger_) {
        logger_->Trace("MeshService", "LoadFromArchive",
                       "archivePath=" + archivePath +
                       ", entryPath=" + entryPath);
    }

    std::filesystem::path resolvedArchive;
    if (!ResolvePath(archivePath, resolvedArchive, outError)) {
        return false;
    }
    if (!std::filesystem::exists(resolvedArchive)) {
        outError = "Archive file not found: " + resolvedArchive.string();
        return false;
    }
    if (entryPath.empty()) {
        outError = "Archive entry path is empty";
        return false;
    }

    int errorCode = 0;
    std::unique_ptr<zip_t, ZipArchiveDeleter> archive(
        zip_open(resolvedArchive.string().c_str(), ZIP_RDONLY, &errorCode));
    if (!archive) {
        outError = "Failed to open archive: " + BuildZipErrorMessage(errorCode);
        return false;
    }

    zip_stat_t entryStat;
    if (zip_stat(archive.get(), entryPath.c_str(), ZIP_FL_ENC_GUESS, &entryStat) != 0) {
        outError = "Archive entry not found: " + entryPath;
        return false;
    }
    if (entryStat.size == 0) {
        outError = "Archive entry is empty: " + entryPath;
        return false;
    }
    if (entryStat.size > std::numeric_limits<size_t>::max()) {
        outError = "Archive entry exceeds addressable size: " + entryPath;
        return false;
    }

    std::unique_ptr<zip_file_t, ZipFileDeleter> file(
        zip_fopen(archive.get(), entryPath.c_str(), ZIP_FL_ENC_GUESS));
    if (!file) {
        outError = "Failed to open archive entry: " + BuildZipArchiveErrorMessage(archive.get());
        return false;
    }

    size_t entrySize = static_cast<size_t>(entryStat.size);
    std::vector<uint8_t> buffer(entrySize);
    zip_int64_t totalRead = 0;
    while (static_cast<size_t>(totalRead) < entrySize) {
        zip_int64_t bytesRead = zip_fread(file.get(),
                                          buffer.data() + totalRead,
                                          entrySize - static_cast<size_t>(totalRead));
        if (bytesRead < 0) {
            outError = "Failed to read archive entry: " + BuildZipArchiveErrorMessage(archive.get());
            return false;
        }
        if (bytesRead == 0) {
            break;
        }
        totalRead += bytesRead;
    }
    if (static_cast<size_t>(totalRead) != entrySize) {
        outError = "Archive entry read incomplete: " + entryPath;
        return false;
    }

    std::string extensionHint = GetExtensionHint(entryPath, "bsp");
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFileFromMemory(
        buffer.data(),
        buffer.size(),
        kAssimpLoadFlags,
        extensionHint.c_str());
    if (!scene) {
        outError = importer.GetErrorString() ? importer.GetErrorString()
                                             : "Assimp failed to load archive entry";
        return false;
    }

    if (!BuildPayloadFromScene(scene, true, outPayload, outError, logger_)) {
        return false;
    }
    if (outPayload.positions.size() > std::numeric_limits<uint16_t>::max()) {
        outError = "Mesh vertex count exceeds uint16_t index range: " +
                   std::to_string(outPayload.positions.size());
        return false;
    }

    return true;
}

bool MeshService::ResolvePath(const std::string& requestedPath,
                              std::filesystem::path& resolvedPath,
                              std::string& outError) const {
    if (!configService_) {
        outError = "Config service not available";
        return false;
    }

    std::filesystem::path resolved(requestedPath);
    if (!resolved.is_absolute()) {
        resolved = configService_->GetScriptPath().parent_path() / resolved;
    }

    std::error_code ec;
    resolved = std::filesystem::weakly_canonical(resolved, ec);
    if (ec) {
        outError = "Failed to resolve path: " + ec.message();
        return false;
    }

    resolvedPath = std::move(resolved);
    return true;
}

void MeshService::PushMeshToLua(lua_State* L, const MeshPayload& payload) {
    if (logger_) {
        logger_->Trace("MeshService", "PushMeshToLua",
                       "positions.size=" + std::to_string(payload.positions.size()) +
                       ", normals.size=" + std::to_string(payload.normals.size()) +
                       ", colors.size=" + std::to_string(payload.colors.size()) +
                       ", indices.size=" + std::to_string(payload.indices.size()) +
                       ", luaStateIsNull=" + std::string(L ? "false" : "true"));
    }
    lua_newtable(L);

    lua_newtable(L);
    for (size_t vertexIndex = 0; vertexIndex < payload.positions.size(); ++vertexIndex) {
        lua_newtable(L);

        lua_newtable(L);
        for (int component = 0; component < 3; ++component) {
            lua_pushnumber(L, payload.positions[vertexIndex][component]);
            lua_rawseti(L, -2, component + 1);
        }
        lua_setfield(L, -2, "position");

        lua_newtable(L);
        std::array<float, 3> normal = {0.0f, 0.0f, 1.0f};
        if (vertexIndex < payload.normals.size()) {
            normal = payload.normals[vertexIndex];
        }
        for (int component = 0; component < 3; ++component) {
            lua_pushnumber(L, normal[component]);
            lua_rawseti(L, -2, component + 1);
        }
        lua_setfield(L, -2, "normal");

        lua_newtable(L);
        for (int component = 0; component < 3; ++component) {
            lua_pushnumber(L, payload.colors[vertexIndex][component]);
            lua_rawseti(L, -2, component + 1);
        }
        lua_setfield(L, -2, "color");

        lua_rawseti(L, -2, static_cast<int>(vertexIndex + 1));
    }
    lua_setfield(L, -2, "vertices");

    lua_newtable(L);
    for (size_t index = 0; index < payload.indices.size(); ++index) {
        lua_pushinteger(L, static_cast<lua_Integer>(payload.indices[index]) + 1);
        lua_rawseti(L, -2, static_cast<int>(index + 1));
    }
    lua_setfield(L, -2, "indices");
}

}  // namespace sdl3cpp::services::impl
