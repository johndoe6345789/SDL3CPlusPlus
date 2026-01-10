#include "mesh_service.hpp"
#include <utility>

#include <assimp/DefaultIOSystem.h>
#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
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

std::string NormalizeExtension(std::string extension) {
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    return extension;
}

class ArchiveMapAwareIOSystem : public Assimp::DefaultIOSystem {
public:
    bool Exists(const char* pFile) const override {
        const std::string trimmed = TrimArchiveSuffix(pFile);
        return Assimp::DefaultIOSystem::Exists(trimmed.c_str());
    }

    Assimp::IOStream* Open(const char* pFile, const char* pMode = "rb") override {
        const std::string trimmed = TrimArchiveSuffix(pFile);
        return Assimp::DefaultIOSystem::Open(trimmed.c_str(), pMode);
    }

private:
    static std::string TrimArchiveSuffix(const char* path) {
        if (!path) {
            return std::string();
        }
        std::string trimmed(path);
        const std::string::size_type comma = trimmed.find(',');
        if (comma != std::string::npos) {
            trimmed.erase(comma);
        }
        return trimmed;
    }
};

bool ReadArchiveEntry(zip_t* archive,
                      const std::string& entryPath,
                      const zip_stat_t& entryStat,
                      std::vector<uint8_t>& buffer,
                      std::string& outError) {
    if (!archive) {
        outError = "Archive handle is null";
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
        zip_fopen(archive, entryPath.c_str(), ZIP_FL_ENC_GUESS));
    if (!file) {
        outError = "Failed to open archive entry: " + BuildZipArchiveErrorMessage(archive);
        return false;
    }

    size_t entrySize = static_cast<size_t>(entryStat.size);
    buffer.assign(entrySize, 0);
    zip_int64_t totalRead = 0;
    while (static_cast<size_t>(totalRead) < entrySize) {
        zip_int64_t bytesRead = zip_fread(file.get(),
                                          buffer.data() + totalRead,
                                          entrySize - static_cast<size_t>(totalRead));
        if (bytesRead < 0) {
            outError = "Failed to read archive entry: " + BuildZipArchiveErrorMessage(archive);
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

    return true;
}

#pragma pack(push, 1)
struct BspHeader {
    char id[4];
    int32_t version;
};

struct BspLump {
    int32_t offset;
    int32_t length;
};

struct BspVertex {
    float position[3];
    float texCoord[2];
    float lightmapCoord[2];
    float normal[3];
    uint8_t color[4];
};

struct BspFace {
    int32_t textureId;
    int32_t effect;
    int32_t type;
    int32_t vertexIndex;
    int32_t numVertices;
    int32_t meshVertIndex;
    int32_t numMeshVerts;
    int32_t lightmapId;
    int32_t lightmapCorner[2];
    int32_t lightmapSize[2];
    float lightmapPos[3];
    float lightmapVecs[2][3];
    float normal[3];
    int32_t patchSize[2];
};
#pragma pack(pop)

static_assert(sizeof(BspHeader) == 8, "Unexpected BSP header size");
static_assert(sizeof(BspLump) == 8, "Unexpected BSP lump size");
static_assert(sizeof(BspVertex) == 44, "Unexpected BSP vertex size");
static_assert(sizeof(BspFace) == 104, "Unexpected BSP face size");

bool BuildPayloadFromBspBuffer(const std::vector<uint8_t>& buffer,
                               MeshPayload& outPayload,
                               std::string& outError,
                               const std::shared_ptr<ILogger>& logger) {
    constexpr int32_t kBspExpectedVersion = 46;
    constexpr size_t kBspLumpCount = 17;
    constexpr size_t kBspVerticesLump = 10;
    constexpr size_t kBspMeshVertsLump = 11;
    constexpr size_t kBspFacesLump = 13;
    constexpr int32_t kBspFacePolygon = 1;
    constexpr int32_t kBspFaceMesh = 3;

    if (buffer.size() < sizeof(BspHeader) + kBspLumpCount * sizeof(BspLump)) {
        outError = "BSP buffer is too small to contain header and lumps";
        return false;
    }

    BspHeader header{};
    std::memcpy(&header, buffer.data(), sizeof(BspHeader));
    if (std::string(header.id, sizeof(header.id)) != "IBSP") {
        outError = "BSP header mismatch: expected IBSP";
        return false;
    }
    if (header.version != kBspExpectedVersion) {
        outError = "Unsupported BSP version: " + std::to_string(header.version);
        return false;
    }

    std::array<BspLump, kBspLumpCount> lumps{};
    std::memcpy(lumps.data(), buffer.data() + sizeof(BspHeader),
                kBspLumpCount * sizeof(BspLump));

    auto validateLump = [&](size_t index,
                            size_t elementSize,
                            size_t& outCount,
                            const std::string& label) -> bool {
        const BspLump& lump = lumps[index];
        if (lump.offset < 0 || lump.length < 0) {
            outError = "BSP " + label + " lump has negative bounds";
            return false;
        }
        size_t offset = static_cast<size_t>(lump.offset);
        size_t length = static_cast<size_t>(lump.length);
        if (offset + length > buffer.size()) {
            outError = "BSP " + label + " lump exceeds buffer size";
            return false;
        }
        if (length % elementSize != 0) {
            outError = "BSP " + label + " lump size is not aligned";
            return false;
        }
        outCount = length / elementSize;
        return true;
    };

    size_t vertexCount = 0;
    size_t meshVertCount = 0;
    size_t faceCount = 0;
    if (!validateLump(kBspVerticesLump, sizeof(BspVertex), vertexCount, "vertex")) {
        return false;
    }
    if (!validateLump(kBspMeshVertsLump, sizeof(int32_t), meshVertCount, "meshvert")) {
        return false;
    }
    if (!validateLump(kBspFacesLump, sizeof(BspFace), faceCount, "face")) {
        return false;
    }
    if (vertexCount == 0) {
        outError = "BSP contains no vertices";
        return false;
    }
    if (meshVertCount == 0 || faceCount == 0) {
        outError = "BSP contains no face indices";
        return false;
    }

    const uint8_t* vertexData = buffer.data() + lumps[kBspVerticesLump].offset;
    const uint8_t* meshVertData = buffer.data() + lumps[kBspMeshVertsLump].offset;
    const uint8_t* faceData = buffer.data() + lumps[kBspFacesLump].offset;

    std::vector<BspVertex> vertices(vertexCount);
    std::memcpy(vertices.data(), vertexData, vertexCount * sizeof(BspVertex));

    std::vector<int32_t> meshVerts(meshVertCount);
    std::memcpy(meshVerts.data(), meshVertData, meshVertCount * sizeof(int32_t));

    outPayload.positions.resize(vertexCount);
    outPayload.normals.resize(vertexCount);
    outPayload.tangents.resize(vertexCount);
    outPayload.colors.resize(vertexCount);
    outPayload.texcoords.resize(vertexCount);
    outPayload.indices.clear();

    for (size_t i = 0; i < vertexCount; ++i) {
        const BspVertex& vertex = vertices[i];
        outPayload.positions[i] = {vertex.position[0], vertex.position[1], vertex.position[2]};
        outPayload.normals[i] = {vertex.normal[0], vertex.normal[1], vertex.normal[2]};

        // Compute a simple tangent perpendicular to the normal
        float nx = vertex.normal[0];
        float ny = vertex.normal[1];
        float nz = vertex.normal[2];
        // Find a vector not parallel to the normal
        float tx, ty, tz;
        if (std::fabs(nx) < 0.9f) {
            tx = 1.0f; ty = 0.0f; tz = 0.0f;
        } else {
            tx = 0.0f; ty = 1.0f; tz = 0.0f;
        }
        // Cross product: tangent = arbitrary × normal
        float cx = ty * nz - tz * ny;
        float cy = tz * nx - tx * nz;
        float cz = tx * ny - ty * nx;
        // Normalize
        float len = std::sqrt(cx*cx + cy*cy + cz*cz);
        if (len > 0.0001f) {
            cx /= len; cy /= len; cz /= len;
        }
        outPayload.tangents[i] = {cx, cy, cz};

        outPayload.colors[i] = {
            static_cast<float>(vertex.color[0]) / 255.0f,
            static_cast<float>(vertex.color[1]) / 255.0f,
            static_cast<float>(vertex.color[2]) / 255.0f
        };
        outPayload.texcoords[i] = {vertex.texCoord[0], vertex.texCoord[1]};
    }

    size_t trianglesBuilt = 0;
    size_t trianglesSkipped = 0;
    for (size_t faceIndex = 0; faceIndex < faceCount; ++faceIndex) {
        BspFace face{};
        std::memcpy(&face, faceData + faceIndex * sizeof(BspFace), sizeof(BspFace));
        if (face.type != kBspFacePolygon && face.type != kBspFaceMesh) {
            continue;
        }
        if (face.numMeshVerts < 3) {
            continue;
        }

        for (int32_t i = 0; i + 2 < face.numMeshVerts; i += 3) {
            int32_t meshIndex = face.meshVertIndex + i;
            if (meshIndex < 0 || static_cast<size_t>(meshIndex + 2) >= meshVerts.size()) {
                ++trianglesSkipped;
                continue;
            }
            int32_t index0 = face.vertexIndex + meshVerts[static_cast<size_t>(meshIndex)];
            int32_t index1 = face.vertexIndex + meshVerts[static_cast<size_t>(meshIndex + 1)];
            int32_t index2 = face.vertexIndex + meshVerts[static_cast<size_t>(meshIndex + 2)];
            if (index0 < 0 || index1 < 0 || index2 < 0) {
                ++trianglesSkipped;
                continue;
            }
            if (static_cast<size_t>(index0) >= vertexCount ||
                static_cast<size_t>(index1) >= vertexCount ||
                static_cast<size_t>(index2) >= vertexCount) {
                ++trianglesSkipped;
                continue;
            }

            outPayload.indices.push_back(static_cast<uint32_t>(index0));
            outPayload.indices.push_back(static_cast<uint32_t>(index1));
            outPayload.indices.push_back(static_cast<uint32_t>(index2));
            ++trianglesBuilt;
        }
    }

    if (logger) {
        logger->Trace("MeshService", "BuildPayloadFromBspBuffer",
                      "vertexCount=" + std::to_string(vertexCount) +
                      ", meshVertCount=" + std::to_string(meshVertCount) +
                      ", faceCount=" + std::to_string(faceCount) +
                      ", trianglesBuilt=" + std::to_string(trianglesBuilt) +
                      ", texcoordCount=" + std::to_string(outPayload.texcoords.size()) +
                      ", trianglesSkipped=" + std::to_string(trianglesSkipped));
    }

    if (outPayload.indices.empty()) {
        outError = "BSP contains no triangle faces";
        return false;
    }
    if (outPayload.positions.size() > std::numeric_limits<uint16_t>::max()) {
        outError = "Mesh vertex count exceeds uint16_t index range: " +
                   std::to_string(outPayload.positions.size());
        return false;
    }

    return true;
}

bool WriteArchiveEntryToTempFile(const std::vector<uint8_t>& buffer,
                                 const std::string& entryPath,
                                 const std::string& extensionHint,
                                 std::filesystem::path& outPath,
                                 std::string& outError) {
    std::error_code tempError;
    std::filesystem::path tempDirectory = std::filesystem::temp_directory_path(tempError);
    if (tempError) {
        outError = "Failed to resolve temp directory: " + tempError.message();
        return false;
    }

    std::filesystem::path entryName(entryPath);
    std::string baseName = entryName.stem().string();
    if (baseName.empty()) {
        baseName = "archive_entry";
    }

    auto timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::string fileName = "sdl3cpp_" + baseName + "_" + std::to_string(timestampMs);
    if (!extensionHint.empty()) {
        fileName += "." + extensionHint;
    }

    outPath = tempDirectory / fileName;
    std::ofstream output(outPath, std::ios::binary | std::ios::trunc);
    if (!output) {
        outError = "Failed to create temp file: " + outPath.string();
        return false;
    }
    output.write(reinterpret_cast<const char*>(buffer.data()),
                 static_cast<std::streamsize>(buffer.size()));
    if (!output) {
        outError = "Failed to write temp file: " + outPath.string();
        return false;
    }
    return true;
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

struct MeshBounds {
    aiVector3D min;
    aiVector3D max;
};

MeshBounds ComputeMeshBounds(const aiMesh* mesh) {
    MeshBounds bounds{};
    bounds.min = aiVector3D(std::numeric_limits<float>::max());
    bounds.max = aiVector3D(std::numeric_limits<float>::lowest());
    if (!mesh || !mesh->mNumVertices) {
        return bounds;
    }
    for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
        const aiVector3D& v = mesh->mVertices[i];
        bounds.min.x = std::min(bounds.min.x, v.x);
        bounds.min.y = std::min(bounds.min.y, v.y);
        bounds.min.z = std::min(bounds.min.z, v.z);
        bounds.max.x = std::max(bounds.max.x, v.x);
        bounds.max.y = std::max(bounds.max.y, v.y);
        bounds.max.z = std::max(bounds.max.z, v.z);
    }
    return bounds;
}

float NormalizeCoord(float value, float minValue, float maxValue) {
    float range = maxValue - minValue;
    if (range == 0.0f) {
        return 0.0f;
    }
    return (value - minValue) / range;
}

std::array<float, 2> ComputeFallbackTexcoord(const aiVector3D& position,
                                             const aiVector3D& normal,
                                             const MeshBounds& bounds) {
    float ax = std::fabs(normal.x);
    float ay = std::fabs(normal.y);
    float az = std::fabs(normal.z);
    if (ax >= ay && ax >= az) {
        return {NormalizeCoord(position.z, bounds.min.z, bounds.max.z),
                NormalizeCoord(position.y, bounds.min.y, bounds.max.y)};
    }
    if (ay >= ax && ay >= az) {
        return {NormalizeCoord(position.x, bounds.min.x, bounds.max.x),
                NormalizeCoord(position.z, bounds.min.z, bounds.max.z)};
    }
    return {NormalizeCoord(position.x, bounds.min.x, bounds.max.x),
            NormalizeCoord(position.y, bounds.min.y, bounds.max.y)};
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
    const MeshBounds bounds = ComputeMeshBounds(mesh);
    const bool hasTexcoords = mesh->HasTextureCoords(0) && mesh->mTextureCoords[0];

    size_t positionsStart = outPayload.positions.size();
    size_t normalsStart = outPayload.normals.size();
    size_t tangentsStart = outPayload.tangents.size();
    size_t colorsStart = outPayload.colors.size();
    size_t texcoordsStart = outPayload.texcoords.size();
    size_t indicesStart = outPayload.indices.size();

    outPayload.positions.reserve(positionsStart + mesh->mNumVertices);
    outPayload.normals.reserve(normalsStart + mesh->mNumVertices);
    outPayload.tangents.reserve(tangentsStart + mesh->mNumVertices);
    outPayload.colors.reserve(colorsStart + mesh->mNumVertices);
    outPayload.texcoords.reserve(texcoordsStart + mesh->mNumVertices);
    outPayload.indices.reserve(indicesStart + mesh->mNumFaces * 3);

    for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
        const aiVector3D& vertex = mesh->mVertices[i];
        outPayload.positions.push_back({vertex.x, vertex.y, vertex.z});

        aiVector3D normal(0.0f, 0.0f, 1.0f);
        if (mesh->HasNormals()) {
            normal = mesh->mNormals[i];
        }
        outPayload.normals.push_back({normal.x, normal.y, normal.z});

        // Compute tangent
        aiVector3D tangent(1.0f, 0.0f, 0.0f);
        if (mesh->HasTangentsAndBitangents()) {
            tangent = mesh->mTangents[i];
        } else {
            // Generate a tangent perpendicular to the normal
            aiVector3D arbitrary(1.0f, 0.0f, 0.0f);
            if (std::fabs(normal.x) > 0.9f) {
                arbitrary = aiVector3D(0.0f, 1.0f, 0.0f);
            }
            tangent = (arbitrary ^ normal);  // Cross product
            tangent.Normalize();
        }
        outPayload.tangents.push_back({tangent.x, tangent.y, tangent.z});

        aiColor3D color = materialColor;
        if (mesh->HasVertexColors(0) && mesh->mColors[0]) {
            const aiColor4D& vertexColor = mesh->mColors[0][i];
            color = aiColor3D(vertexColor.r, vertexColor.g, vertexColor.b);
        }
        outPayload.colors.push_back({color.r, color.g, color.b});

        if (hasTexcoords) {
            const aiVector3D& uv = mesh->mTextureCoords[0][i];
            outPayload.texcoords.push_back({uv.x, uv.y});
        } else {
            outPayload.texcoords.push_back(ComputeFallbackTexcoord(vertex, normal, bounds));
        }
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
        outPayload.tangents.resize(tangentsStart);
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

    std::string extensionHint = GetExtensionHint(entryPath, "bsp");
    std::string normalizedExtension = NormalizeExtension(extensionHint);
    auto buildPayload = [&](const aiScene* loadedScene, std::string& error) -> bool {
        if (!BuildPayloadFromScene(loadedScene, true, outPayload, error, logger_)) {
            return false;
        }
        if (outPayload.positions.size() > std::numeric_limits<uint16_t>::max()) {
            error = "Mesh vertex count exceeds uint16_t index range: " +
                    std::to_string(outPayload.positions.size());
            return false;
        }
        return true;
    };

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

    if (normalizedExtension == "bsp") {
        if (entryStat.size == 0) {
            outError = "Archive entry is empty: " + entryPath;
            return false;
        }
        if (logger_) {
            logger_->Trace("MeshService", "LoadFromArchive",
                           "Detected BSP entry; using archive importer. archive=" +
                           resolvedArchive.string() + ", entry=" + entryPath);
            logger_->Trace("MeshService", "LoadFromArchive",
                           "BSP entry size=" + std::to_string(entryStat.size));
        }

        std::string importName = resolvedArchive.string() + "," + entryPath;
        Assimp::Importer importer;
        importer.SetIOHandler(new ArchiveMapAwareIOSystem());
        const aiScene* scene = importer.ReadFile(importName, kAssimpLoadFlags);
        if (scene && logger_) {
            unsigned int rootMeshes = 0;
            unsigned int rootChildren = 0;
            if (scene->mRootNode) {
                rootMeshes = scene->mRootNode->mNumMeshes;
                rootChildren = scene->mRootNode->mNumChildren;
            }
            logger_->Trace("MeshService", "LoadFromArchive",
                           "BSP scene stats: meshes=" + std::to_string(scene->mNumMeshes) +
                           ", materials=" + std::to_string(scene->mNumMaterials) +
                           ", textures=" + std::to_string(scene->mNumTextures) +
                           ", rootMeshes=" + std::to_string(rootMeshes) +
                           ", rootChildren=" + std::to_string(rootChildren));
        }
        std::string assimpError;
        if (!scene) {
            assimpError = importer.GetErrorString() ? importer.GetErrorString()
                                                    : "Assimp failed to load BSP from archive";
        } else if (buildPayload(scene, assimpError)) {
            return true;
        }

        if (logger_) {
            logger_->Trace("MeshService", "LoadFromArchive",
                           "Assimp BSP import did not yield meshes, falling back to BSP parser: " +
                           assimpError);
        }

        std::vector<uint8_t> buffer;
        std::string readError;
        if (!ReadArchiveEntry(archive.get(), entryPath, entryStat, buffer, readError)) {
            outError = "Failed to read BSP entry for fallback parser: " + readError;
            return false;
        }
        if (!BuildPayloadFromBspBuffer(buffer, outPayload, outError, logger_)) {
            return false;
        }
        return true;
    }

    std::vector<uint8_t> buffer;
    if (!ReadArchiveEntry(archive.get(), entryPath, entryStat, buffer, outError)) {
        return false;
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFileFromMemory(
        buffer.data(),
        buffer.size(),
        kAssimpLoadFlags,
        extensionHint.c_str());
    if (!scene) {
        std::string memoryError = importer.GetErrorString() ? importer.GetErrorString()
                                                            : "Assimp failed to load archive entry";
        if (logger_) {
            logger_->Trace("MeshService", "LoadFromArchive",
                           "ReadFileFromMemory failed: " + memoryError +
                           ", entryPath=" + entryPath +
                           ", extensionHint=" + extensionHint);
        }

        std::filesystem::path tempPath;
        std::string tempError;
        if (!WriteArchiveEntryToTempFile(buffer, entryPath, extensionHint, tempPath, tempError)) {
            outError = "Assimp failed to load archive entry from memory: " + memoryError +
                       "; " + tempError;
            return false;
        }
        if (logger_) {
            logger_->Trace("MeshService", "LoadFromArchive",
                           "Falling back to temp file: " + tempPath.string());
        }

        Assimp::Importer fallbackImporter;
        const aiScene* fallbackScene = fallbackImporter.ReadFile(tempPath.string(), kAssimpLoadFlags);

        std::error_code removeError;
        std::filesystem::remove(tempPath, removeError);
        if (removeError && logger_) {
            logger_->Trace("MeshService", "LoadFromArchive",
                           "Failed to remove temp file: " + tempPath.string() +
                           ", error=" + removeError.message());
        }

        if (!fallbackScene) {
            std::string fallbackError = fallbackImporter.GetErrorString() ? fallbackImporter.GetErrorString()
                                                                          : "Assimp failed to load temp file";
            outError = "Assimp failed to load archive entry from memory: " + memoryError +
                       "; fallback to temp file failed: " + fallbackError;
            return false;
        }

        if (!buildPayload(fallbackScene, outError)) {
            return false;
        }

        return true;
    }

    if (!buildPayload(scene, outError)) {
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
        resolved = configService_->GetProjectRoot() / resolved;
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

}  // namespace sdl3cpp::services::impl
