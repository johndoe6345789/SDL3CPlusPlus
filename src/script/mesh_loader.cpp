#include "script/mesh_loader.hpp"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <lua.hpp>

#include <system_error>

namespace sdl3cpp::script {

bool MeshLoader::LoadFromFile(const std::filesystem::path& scriptDirectory,
                              const std::string& requestedPath,
                              MeshPayload& outPayload,
                              std::string& outError) {
    std::filesystem::path resolved(requestedPath);
    if (!resolved.is_absolute()) {
        resolved = scriptDirectory / resolved;
    }
    
    std::error_code ec;
    resolved = std::filesystem::weakly_canonical(resolved, ec);
    if (ec) {
        outError = "Failed to resolve mesh path: " + ec.message();
        return false;
    }
    
    if (!std::filesystem::exists(resolved)) {
        outError = "Mesh file not found: " + resolved.string();
        return false;
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        resolved.string(),
        aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices);
    
    if (!scene) {
        outError = importer.GetErrorString() ? importer.GetErrorString() : "Assimp failed to load mesh";
        return false;
    }
    
    if (scene->mNumMeshes == 0) {
        outError = "Scene contains no meshes";
        return false;
    }

    const aiMesh* mesh = scene->mMeshes[0];
    if (!mesh->mNumVertices) {
        outError = "Mesh contains no vertices";
        return false;
    }

    outPayload.positions.reserve(mesh->mNumVertices);
    outPayload.colors.reserve(mesh->mNumVertices);
    outPayload.indices.reserve(mesh->mNumFaces * 3);

    aiColor3D defaultColor(0.6f, 0.8f, 1.0f);
    aiColor3D materialColor = defaultColor;
    
    if (mesh->mMaterialIndex < scene->mNumMaterials) {
        const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        aiColor4D diffuse;
        if (material && material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS) {
            materialColor = aiColor3D(diffuse.r, diffuse.g, diffuse.b);
        }
    }

    for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
        const aiVector3D& vertex = mesh->mVertices[i];
        outPayload.positions.push_back({vertex.x, vertex.y, vertex.z});

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
        outPayload.indices.push_back(face.mIndices[0]);
        outPayload.indices.push_back(face.mIndices[1]);
        outPayload.indices.push_back(face.mIndices[2]);
    }

    if (outPayload.indices.empty()) {
        outError = "Mesh contains no triangle faces";
        return false;
    }
    
    return true;
}

int MeshLoader::PushMeshToLua(lua_State* L, const MeshPayload& payload) {
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

    return 1;
}

} // namespace sdl3cpp::script
