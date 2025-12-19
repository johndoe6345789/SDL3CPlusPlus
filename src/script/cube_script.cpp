#include "script/cube_script.hpp"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace sdl3cpp::script {

struct PhysicsBridge {
    struct BodyRecord {
        std::unique_ptr<btCollisionShape> shape;
        std::unique_ptr<btMotionState> motionState;
        std::unique_ptr<btRigidBody> body;
    };

    PhysicsBridge();
    ~PhysicsBridge();

    bool addBoxRigidBody(const std::string& name,
                         const btVector3& halfExtents,
                         float mass,
                         const btTransform& transform,
                         std::string& error);
    int stepSimulation(float deltaTime);
    bool getRigidBodyTransform(const std::string& name,
                               btTransform& outTransform,
                               std::string& error) const;

private:
    std::unique_ptr<btDefaultCollisionConfiguration> collisionConfig_;
    std::unique_ptr<btCollisionDispatcher> dispatcher_;
    std::unique_ptr<btBroadphaseInterface> broadphase_;
    std::unique_ptr<btSequentialImpulseConstraintSolver> solver_;
    std::unique_ptr<btDiscreteDynamicsWorld> world_;
    std::unordered_map<std::string, BodyRecord> bodies_;
};

PhysicsBridge::PhysicsBridge()
    : collisionConfig_(std::make_unique<btDefaultCollisionConfiguration>()),
      dispatcher_(std::make_unique<btCollisionDispatcher>(collisionConfig_.get())),
      broadphase_(std::make_unique<btDbvtBroadphase>()),
      solver_(std::make_unique<btSequentialImpulseConstraintSolver>()),
      world_(std::make_unique<btDiscreteDynamicsWorld>(
          dispatcher_.get(),
          broadphase_.get(),
          solver_.get(),
          collisionConfig_.get())) {
    world_->setGravity(btVector3(0.0f, -9.81f, 0.0f));
}

PhysicsBridge::~PhysicsBridge() {
    if (world_) {
        for (auto& [name, entry] : bodies_) {
            if (entry.body) {
                world_->removeRigidBody(entry.body.get());
            }
        }
    }
}

bool PhysicsBridge::addBoxRigidBody(const std::string& name,
                                    const btVector3& halfExtents,
                                    float mass,
                                    const btTransform& transform,
                                    std::string& error) {
    if (name.empty()) {
        error = "Rigid body name must not be empty";
        return false;
    }
    if (!world_) {
        error = "Physics world is not initialized";
        return false;
    }
    if (bodies_.count(name)) {
        error = "Rigid body already exists: " + name;
        return false;
    }
    auto shape = std::make_unique<btBoxShape>(halfExtents);
    btVector3 inertia(0.0f, 0.0f, 0.0f);
    if (mass > 0.0f) {
        shape->calculateLocalInertia(mass, inertia);
    }
    auto motionState = std::make_unique<btDefaultMotionState>(transform);
    btRigidBody::btRigidBodyConstructionInfo constructionInfo(
        mass,
        motionState.get(),
        shape.get(),
        inertia);
    auto body = std::make_unique<btRigidBody>(constructionInfo);
    world_->addRigidBody(body.get());
    bodies_.emplace(name, BodyRecord{
        std::move(shape),
        std::move(motionState),
        std::move(body),
    });
    return true;
}

int PhysicsBridge::stepSimulation(float deltaTime) {
    if (!world_) {
        return 0;
    }
    return static_cast<int>(world_->stepSimulation(deltaTime, 10, 1.0f / 60.0f));
}

bool PhysicsBridge::getRigidBodyTransform(const std::string& name,
                                          btTransform& outTransform,
                                          std::string& error) const {
    auto it = bodies_.find(name);
    if (it == bodies_.end()) {
        error = "Rigid body not found: " + name;
        return false;
    }
    if (!it->second.motionState) {
        error = "Rigid body motion state is missing";
        return false;
    }
    it->second.motionState->getWorldTransform(outTransform);
    return true;
}

namespace detail {

std::array<float, 3> ReadVector3(lua_State* L, int index) {
    std::array<float, 3> result{};
    int absIndex = lua_absindex(L, index);
    size_t len = lua_rawlen(L, absIndex);
    if (len != 3) {
        throw std::runtime_error("Expected vector with 3 components");
    }
    for (size_t i = 1; i <= 3; ++i) {
        lua_rawgeti(L, absIndex, static_cast<int>(i));
        if (!lua_isnumber(L, -1)) {
            lua_pop(L, 1);
            throw std::runtime_error("Vector component is not a number");
        }
        result[i - 1] = static_cast<float>(lua_tonumber(L, -1));
        lua_pop(L, 1);
    }
    return result;
}

std::array<float, 16> ReadMatrix(lua_State* L, int index) {
    std::array<float, 16> result{};
    int absIndex = lua_absindex(L, index);
    size_t len = lua_rawlen(L, absIndex);
    if (len != 16) {
        throw std::runtime_error("Expected 4x4 matrix with 16 components");
    }
    for (size_t i = 1; i <= 16; ++i) {
        lua_rawgeti(L, absIndex, static_cast<int>(i));
        if (!lua_isnumber(L, -1)) {
            lua_pop(L, 1);
            throw std::runtime_error("Matrix component is not a number");
        }
        result[i - 1] = static_cast<float>(lua_tonumber(L, -1));
        lua_pop(L, 1);
    }
    return result;
}

std::array<float, 4> ReadQuaternion(lua_State* L, int index) {
    std::array<float, 4> result{};
    int absIndex = lua_absindex(L, index);
    size_t len = lua_rawlen(L, absIndex);
    if (len != 4) {
        throw std::runtime_error("Expected quaternion with 4 components");
    }
    for (size_t i = 1; i <= 4; ++i) {
        lua_rawgeti(L, absIndex, static_cast<int>(i));
        if (!lua_isnumber(L, -1)) {
            lua_pop(L, 1);
            throw std::runtime_error("Quaternion component is not a number");
        }
        result[i - 1] = static_cast<float>(lua_tonumber(L, -1));
        lua_pop(L, 1);
    }
    return result;
}

} // namespace detail

namespace {

struct MeshPayload {
    std::vector<std::array<float, 3>> positions;
    std::vector<std::array<float, 3>> colors;
    std::vector<uint32_t> indices;
};

bool TryLoadMeshPayload(const CubeScript* script,
                        const std::string& requestedPath,
                        MeshPayload& payload,
                        std::string& error) {
    std::filesystem::path resolved(requestedPath);
    if (!resolved.is_absolute()) {
        resolved = script->GetScriptDirectory() / resolved;
    }
    std::error_code ec;
    resolved = std::filesystem::weakly_canonical(resolved, ec);
    if (ec) {
        error = "Failed to resolve mesh path: " + ec.message();
        return false;
    }
    if (!std::filesystem::exists(resolved)) {
        error = "Mesh file not found: " + resolved.string();
        return false;
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        resolved.string(),
        aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices);
    if (!scene) {
        error = importer.GetErrorString() ? importer.GetErrorString() : "Assimp failed to load mesh";
        return false;
    }
    if (scene->mNumMeshes == 0) {
        error = "Scene contains no meshes";
        return false;
    }

    const aiMesh* mesh = scene->mMeshes[0];
    if (!mesh->mNumVertices) {
        error = "Mesh contains no vertices";
        return false;
    }

    payload.positions.reserve(mesh->mNumVertices);
    payload.colors.reserve(mesh->mNumVertices);
    payload.indices.reserve(mesh->mNumFaces * 3);

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
        payload.positions.push_back({vertex.x, vertex.y, vertex.z});

        aiColor3D color = materialColor;
        if (mesh->HasVertexColors(0) && mesh->mColors[0]) {
            const aiColor4D& vertexColor = mesh->mColors[0][i];
            color = aiColor3D(vertexColor.r, vertexColor.g, vertexColor.b);
        }
        payload.colors.push_back({color.r, color.g, color.b});
    }

    for (unsigned faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
        const aiFace& face = mesh->mFaces[faceIndex];
        if (face.mNumIndices != 3) {
            continue;
        }
        payload.indices.push_back(face.mIndices[0]);
        payload.indices.push_back(face.mIndices[1]);
        payload.indices.push_back(face.mIndices[2]);
    }

    if (payload.indices.empty()) {
        error = "Mesh contains no triangle faces";
        return false;
    }
    return true;
}

glm::vec3 ToVec3(const std::array<float, 3>& value) {
    return glm::vec3(value[0], value[1], value[2]);
}

glm::quat ToQuat(const std::array<float, 4>& value) {
    // Lua exposes {x, y, z, w}
    return glm::quat(value[3], value[0], value[1], value[2]);
}

void PushMatrix(lua_State* L, const glm::mat4& matrix) {
    lua_newtable(L);
    const float* ptr = glm::value_ptr(matrix);
    for (int i = 0; i < 16; ++i) {
        lua_pushnumber(L, ptr[i]);
        lua_rawseti(L, -2, i + 1);
    }
}

int PushMeshToLua(lua_State* L, const MeshPayload& payload) {
    lua_newtable(L); // mesh

    lua_newtable(L); // vertices table
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

    lua_newtable(L); // indices table
    for (size_t index = 0; index < payload.indices.size(); ++index) {
        lua_pushinteger(L, static_cast<lua_Integer>(payload.indices[index]) + 1);
        lua_rawseti(L, -2, static_cast<int>(index + 1));
    }
    lua_setfield(L, -2, "indices");

    return 1;
}

int LuaLoadMeshFromFile(lua_State* L) {
    auto* script = static_cast<CubeScript*>(lua_touserdata(L, lua_upvalueindex(1)));
    const char* path = luaL_checkstring(L, 1);
    MeshPayload payload;
    std::string error;
    if (!TryLoadMeshPayload(script, path, payload, error)) {
        lua_pushnil(L);
        lua_pushstring(L, error.c_str());
        return 2;
    }
    PushMeshToLua(L, payload);
    lua_pushnil(L);
    return 2;
}

int LuaPhysicsCreateBox(lua_State* L) {
    auto* script = static_cast<CubeScript*>(lua_touserdata(L, lua_upvalueindex(1)));
    const char* name = luaL_checkstring(L, 1);
    if (!lua_istable(L, 2) || !lua_istable(L, 4) || !lua_istable(L, 5)) {
        luaL_error(L, "physics_create_box expects vector tables for half extents, origin, and rotation");
    }
    std::array<float, 3> halfExtents = detail::ReadVector3(L, 2);
    float mass = static_cast<float>(luaL_checknumber(L, 3));
    std::array<float, 3> origin = detail::ReadVector3(L, 4);
    std::array<float, 4> rotation = detail::ReadQuaternion(L, 5);

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(origin[0], origin[1], origin[2]));
    transform.setRotation(btQuaternion(rotation[0], rotation[1], rotation[2], rotation[3]));

    std::string error;
    if (!script->GetPhysicsBridge().addBoxRigidBody(
            name,
            btVector3(halfExtents[0], halfExtents[1], halfExtents[2]),
            mass,
            transform,
            error)) {
        lua_pushnil(L);
        lua_pushstring(L, error.c_str());
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

int LuaPhysicsStepSimulation(lua_State* L) {
    auto* script = static_cast<CubeScript*>(lua_touserdata(L, lua_upvalueindex(1)));
    float deltaTime = static_cast<float>(luaL_checknumber(L, 1));
    int steps = script->GetPhysicsBridge().stepSimulation(deltaTime);
    lua_pushinteger(L, steps);
    return 1;
}

int LuaPhysicsGetTransform(lua_State* L) {
    auto* script = static_cast<CubeScript*>(lua_touserdata(L, lua_upvalueindex(1)));
    const char* name = luaL_checkstring(L, 1);
    btTransform transform;
    std::string error;
    if (!script->GetPhysicsBridge().getRigidBodyTransform(name, transform, error)) {
        lua_pushnil(L);
        lua_pushstring(L, error.c_str());
        return 2;
    }

    lua_newtable(L);
    lua_newtable(L);
    const btVector3& origin = transform.getOrigin();
    lua_pushnumber(L, origin.x());
    lua_rawseti(L, -2, 1);
    lua_pushnumber(L, origin.y());
    lua_rawseti(L, -2, 2);
    lua_pushnumber(L, origin.z());
    lua_rawseti(L, -2, 3);
    lua_setfield(L, -2, "position");

    lua_newtable(L);
    const btQuaternion& orientation = transform.getRotation();
    lua_pushnumber(L, orientation.x());
    lua_rawseti(L, -2, 1);
    lua_pushnumber(L, orientation.y());
    lua_rawseti(L, -2, 2);
    lua_pushnumber(L, orientation.z());
    lua_rawseti(L, -2, 3);
    lua_pushnumber(L, orientation.w());
    lua_rawseti(L, -2, 4);
    lua_setfield(L, -2, "rotation");

    return 1;
}

int LuaGlmMatrixFromTransform(lua_State* L) {
    std::array<float, 3> translation = detail::ReadVector3(L, 1);
    std::array<float, 4> rotation = detail::ReadQuaternion(L, 2);
    glm::vec3 pos = ToVec3(translation);
    glm::quat quat = ToQuat(rotation);
    glm::mat4 matrix = glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(quat);
    PushMatrix(L, matrix);
    return 1;
}

std::array<float, 16> IdentityMatrix() {
    return {1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f};
}

} // namespace

CubeScript::CubeScript(const std::filesystem::path& scriptPath, bool debugEnabled)
    : L_(luaL_newstate()),
      scriptDirectory_(scriptPath.parent_path()),
      debugEnabled_(debugEnabled),
      physicsBridge_(std::make_unique<PhysicsBridge>()) {
    if (!L_) {
        throw std::runtime_error("Failed to create Lua state");
    }
    luaL_openlibs(L_);
    lua_pushlightuserdata(L_, this);
    lua_pushcclosure(L_, &LuaLoadMeshFromFile, 1);
    lua_setglobal(L_, "load_mesh_from_file");
    lua_pushlightuserdata(L_, this);
    lua_pushcclosure(L_, &LuaPhysicsCreateBox, 1);
    lua_setglobal(L_, "physics_create_box");
    lua_pushlightuserdata(L_, this);
    lua_pushcclosure(L_, &LuaPhysicsStepSimulation, 1);
    lua_setglobal(L_, "physics_step_simulation");
    lua_pushlightuserdata(L_, this);
    lua_pushcclosure(L_, &LuaPhysicsGetTransform, 1);
    lua_setglobal(L_, "physics_get_transform");
    lua_pushlightuserdata(L_, this);
    lua_pushcclosure(L_, &LuaGlmMatrixFromTransform, 1);
    lua_setglobal(L_, "glm_matrix_from_transform");
    lua_pushboolean(L_, debugEnabled_);
    lua_setglobal(L_, "lua_debug");
    auto scriptDir = scriptPath.parent_path();
    if (!scriptDir.empty()) {
        lua_getglobal(L_, "package");
        if (lua_istable(L_, -1)) {
            lua_getfield(L_, -1, "path");
            const char* currentPath = lua_tostring(L_, -1);
            std::string newPath = scriptDir.string() + "/?.lua;";
            if (currentPath) {
                newPath += currentPath;
            }
            lua_pop(L_, 1);
            lua_pushstring(L_, newPath.c_str());
            lua_setfield(L_, -2, "path");
        }
        lua_pop(L_, 1);
    }
    if (luaL_dofile(L_, scriptPath.string().c_str()) != LUA_OK) {
        std::string message = LuaErrorMessage(L_);
        lua_pop(L_, 1);
        lua_close(L_);
        L_ = nullptr;
        throw std::runtime_error("Failed to load Lua script: " + message);
    }

    lua_getglobal(L_, "gui_input");
    if (!lua_isnil(L_, -1)) {
        guiInputRef_ = luaL_ref(L_, LUA_REGISTRYINDEX);
    } else {
        lua_pop(L_, 1);
    }

    lua_getglobal(L_, "get_gui_commands");
    if (lua_isfunction(L_, -1)) {
        guiCommandsFnRef_ = luaL_ref(L_, LUA_REGISTRYINDEX);
    } else {
        lua_pop(L_, 1);
    }
}

CubeScript::~CubeScript() {
    if (L_) {
        if (guiInputRef_ != LUA_REFNIL) {
            luaL_unref(L_, LUA_REGISTRYINDEX, guiInputRef_);
        }
        if (guiCommandsFnRef_ != LUA_REFNIL) {
            luaL_unref(L_, LUA_REGISTRYINDEX, guiCommandsFnRef_);
        }
        lua_close(L_);
    }
}

std::vector<CubeScript::SceneObject> CubeScript::LoadSceneObjects() {
    lua_getglobal(L_, "get_scene_objects");
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("Lua function 'get_scene_objects' is missing");
    }
    if (lua_pcall(L_, 0, 1, 0) != LUA_OK) {
        std::string message = LuaErrorMessage(L_);
        lua_pop(L_, 1);
        throw std::runtime_error("Lua get_scene_objects failed: " + message);
    }
    if (!lua_istable(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("'get_scene_objects' did not return a table");
    }

    size_t count = lua_rawlen(L_, -1);
    std::vector<SceneObject> objects;
    objects.reserve(count);

    for (size_t i = 1; i <= count; ++i) {
        lua_rawgeti(L_, -1, static_cast<int>(i));
        if (!lua_istable(L_, -1)) {
            lua_pop(L_, 1);
            throw std::runtime_error("Scene object at index " + std::to_string(i) + " is not a table");
        }

        SceneObject object;
        lua_getfield(L_, -1, "vertices");
        object.vertices = ReadVertexArray(L_, -1);
        lua_pop(L_, 1);
        if (object.vertices.empty()) {
            lua_pop(L_, 1);
            throw std::runtime_error("Scene object " + std::to_string(i) + " must supply at least one vertex");
        }

        lua_getfield(L_, -1, "indices");
        object.indices = ReadIndexArray(L_, -1);
        lua_pop(L_, 1);
        if (object.indices.empty()) {
            lua_pop(L_, 1);
            throw std::runtime_error("Scene object " + std::to_string(i) + " must supply indices");
        }

        lua_getfield(L_, -1, "compute_model_matrix");
        if (lua_isfunction(L_, -1)) {
            object.computeModelMatrixRef = luaL_ref(L_, LUA_REGISTRYINDEX);
        } else {
            lua_pop(L_, 1);
            object.computeModelMatrixRef = LUA_REFNIL;
        }

        lua_getfield(L_, -1, "shader_key");
        if (lua_isstring(L_, -1)) {
            object.shaderKey = lua_tostring(L_, -1);
        }
        lua_pop(L_, 1);

        objects.push_back(std::move(object));
        lua_pop(L_, 1);
    }

    lua_pop(L_, 1);
    return objects;
}

std::array<float, 16> CubeScript::ComputeModelMatrix(int functionRef, float time) {
    if (functionRef == LUA_REFNIL) {
        lua_getglobal(L_, "compute_model_matrix");
        if (!lua_isfunction(L_, -1)) {
            lua_pop(L_, 1);
            return IdentityMatrix();
        }
    } else {
        lua_rawgeti(L_, LUA_REGISTRYINDEX, functionRef);
    }

    lua_pushnumber(L_, time);
    if (lua_pcall(L_, 1, 1, 0) != LUA_OK) {
        std::string message = LuaErrorMessage(L_);
        lua_pop(L_, 1);
        throw std::runtime_error("Lua compute_model_matrix failed: " + message);
    }
    if (!lua_istable(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("'compute_model_matrix' did not return a table");
    }

    std::array<float, 16> matrix = detail::ReadMatrix(L_, -1);
    lua_pop(L_, 1);
    return matrix;
}

std::array<float, 16> CubeScript::GetViewProjectionMatrix(float aspect) {
    lua_getglobal(L_, "get_view_projection");
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("Lua function 'get_view_projection' is missing");
    }
    lua_pushnumber(L_, aspect);
    if (lua_pcall(L_, 1, 1, 0) != LUA_OK) {
        std::string message = LuaErrorMessage(L_);
        lua_pop(L_, 1);
        throw std::runtime_error("Lua get_view_projection failed: " + message);
    }
    if (!lua_istable(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("'get_view_projection' did not return a table");
    }
    std::array<float, 16> matrix = detail::ReadMatrix(L_, -1);
    lua_pop(L_, 1);
    return matrix;
}

std::vector<core::Vertex> CubeScript::ReadVertexArray(lua_State* L, int index) {
    int absIndex = lua_absindex(L, index);
    if (!lua_istable(L, absIndex)) {
        throw std::runtime_error("Expected table for vertex data");
    }

    size_t count = lua_rawlen(L, absIndex);
    std::vector<core::Vertex> vertices;
    vertices.reserve(count);

    for (size_t i = 1; i <= count; ++i) {
        lua_rawgeti(L, absIndex, static_cast<int>(i));
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            throw std::runtime_error("Vertex entry at index " + std::to_string(i) + " is not a table");
        }

        int vertexIndex = lua_gettop(L);
        core::Vertex vertex{};

        lua_getfield(L, vertexIndex, "position");
        vertex.position = detail::ReadVector3(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, vertexIndex, "color");
        vertex.color = detail::ReadVector3(L, -1);
        lua_pop(L, 1);

        lua_pop(L, 1);
        vertices.push_back(vertex);
    }

    return vertices;
}

std::vector<uint16_t> CubeScript::ReadIndexArray(lua_State* L, int index) {
    int absIndex = lua_absindex(L, index);
    if (!lua_istable(L, absIndex)) {
        throw std::runtime_error("Expected table for index data");
    }

    size_t count = lua_rawlen(L, absIndex);
    std::vector<uint16_t> indices;
    indices.reserve(count);

    for (size_t i = 1; i <= count; ++i) {
        lua_rawgeti(L, absIndex, static_cast<int>(i));
        if (!lua_isinteger(L, -1)) {
            lua_pop(L, 1);
            throw std::runtime_error("Index entry at position " + std::to_string(i) + " is not an integer");
        }
        lua_Integer value = lua_tointeger(L, -1);
        lua_pop(L, 1);
        if (value < 1) {
            throw std::runtime_error("Index values must be 1 or greater");
        }
        indices.push_back(static_cast<uint16_t>(value - 1));
    }

    return indices;
}

std::unordered_map<std::string, CubeScript::ShaderPaths> CubeScript::LoadShaderPathsMap() {
    lua_getglobal(L_, "get_shader_paths");
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("Lua function 'get_shader_paths' is missing");
    }
    if (lua_pcall(L_, 0, 1, 0) != LUA_OK) {
        std::string message = LuaErrorMessage(L_);
        lua_pop(L_, 1);
        throw std::runtime_error("Lua get_shader_paths failed: " + message);
    }
    if (!lua_istable(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("'get_shader_paths' did not return a table");
    }

    std::unordered_map<std::string, ShaderPaths> shaderMap;
    lua_pushnil(L_);
    while (lua_next(L_, -2) != 0) {
        if (lua_isstring(L_, -2) && lua_istable(L_, -1)) {
            std::string key = lua_tostring(L_, -2);
            shaderMap.emplace(key, ReadShaderPathsTable(L_, -1));
        }
        lua_pop(L_, 1);
    }

    lua_pop(L_, 1);
    if (shaderMap.empty()) {
        throw std::runtime_error("'get_shader_paths' did not return any shader variants");
    }
    return shaderMap;
}

CubeScript::ShaderPaths CubeScript::ReadShaderPathsTable(lua_State* L, int index) {
    ShaderPaths paths;
    int absIndex = lua_absindex(L, index);

    lua_getfield(L, absIndex, "vertex");
    if (!lua_isstring(L, -1)) {
        lua_pop(L, 1);
        throw std::runtime_error("Shader path 'vertex' must be a string");
    }
    paths.vertex = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, absIndex, "fragment");
    if (!lua_isstring(L, -1)) {
        lua_pop(L, 1);
        throw std::runtime_error("Shader path 'fragment' must be a string");
    }
    paths.fragment = lua_tostring(L, -1);
    lua_pop(L, 1);

    return paths;
}

std::string CubeScript::LuaErrorMessage(lua_State* L) {
    const char* message = lua_tostring(L, -1);
    return message ? message : "unknown lua error";
}

PhysicsBridge& CubeScript::GetPhysicsBridge() {
    return *physicsBridge_;
}

std::vector<CubeScript::GuiCommand> CubeScript::LoadGuiCommands() {
    std::vector<GuiCommand> commands;
    if (guiCommandsFnRef_ == LUA_REFNIL) {
        return commands;
    }
    lua_rawgeti(L_, LUA_REGISTRYINDEX, guiCommandsFnRef_);
    if (lua_pcall(L_, 0, 1, 0) != LUA_OK) {
        std::string message = LuaErrorMessage(L_);
        lua_pop(L_, 1);
        throw std::runtime_error("Lua get_gui_commands failed: " + message);
    }
    if (!lua_istable(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("'get_gui_commands' did not return a table");
    }

    size_t count = lua_rawlen(L_, -1);
    commands.reserve(count);

    for (size_t i = 1; i <= count; ++i) {
        lua_rawgeti(L_, -1, static_cast<int>(i));
        if (!lua_istable(L_, -1)) {
            lua_pop(L_, 1);
            throw std::runtime_error("GUI command at index " + std::to_string(i) + " is not a table");
        }
        int commandIndex = lua_gettop(L_);
        lua_getfield(L_, commandIndex, "type");
        const char* typeName = lua_tostring(L_, -1);
        if (!typeName) {
            lua_pop(L_, 2);
            throw std::runtime_error("GUI command at index " + std::to_string(i) + " is missing a type");
        }
        GuiCommand command{};
        if (std::strcmp(typeName, "rect") == 0) {
            command.type = GuiCommand::Type::Rect;
            command.rect = ReadRect(L_, commandIndex);
            command.color = ReadColor(L_, commandIndex, GuiColor{0.0f, 0.0f, 0.0f, 1.0f});
            command.borderColor = ReadColor(L_, commandIndex, GuiColor{0.0f, 0.0f, 0.0f, 0.0f});
            lua_getfield(L_, commandIndex, "borderWidth");
            if (lua_isnumber(L_, -1)) {
                command.borderWidth = static_cast<float>(lua_tonumber(L_, -1));
            }
            lua_pop(L_, 1);
        } else if (std::strcmp(typeName, "text") == 0) {
            command.type = GuiCommand::Type::Text;
            ReadStringField(L_, commandIndex, "text", command.text);
            lua_getfield(L_, commandIndex, "fontSize");
            if (lua_isnumber(L_, -1)) {
                command.fontSize = static_cast<float>(lua_tonumber(L_, -1));
            }
            lua_pop(L_, 1);
            std::string align;
            if (ReadStringField(L_, commandIndex, "alignX", align)) {
                command.alignX = align;
            }
            if (ReadStringField(L_, commandIndex, "alignY", align)) {
                command.alignY = align;
            }
            lua_getfield(L_, commandIndex, "clipRect");
            if (lua_istable(L_, -1)) {
                command.clipRect = ReadRect(L_, -1);
                command.hasClipRect = true;
            }
            lua_pop(L_, 1);
            lua_getfield(L_, commandIndex, "bounds");
            if (lua_istable(L_, -1)) {
                command.bounds = ReadRect(L_, -1);
                command.hasBounds = true;
            }
            lua_pop(L_, 1);
            command.color = ReadColor(L_, commandIndex, GuiColor{1.0f, 1.0f, 1.0f, 1.0f});
        } else if (std::strcmp(typeName, "clip_push") == 0) {
            command.type = GuiCommand::Type::ClipPush;
            command.rect = ReadRect(L_, commandIndex);
        } else if (std::strcmp(typeName, "clip_pop") == 0) {
            command.type = GuiCommand::Type::ClipPop;
        } else if (std::strcmp(typeName, "svg") == 0) {
            command.type = GuiCommand::Type::Svg;
            ReadStringField(L_, commandIndex, "path", command.svgPath);
            command.rect = ReadRect(L_, commandIndex);
            command.svgTint = ReadColor(L_, commandIndex, GuiColor{1.0f, 1.0f, 1.0f, 0.0f});
            lua_getfield(L_, commandIndex, "tint");
            if (lua_istable(L_, -1)) {
                command.svgTint = ReadColor(L_, -1, command.svgTint);
            }
            lua_pop(L_, 1);
        }
        lua_pop(L_, 1); // pop type
        lua_pop(L_, 1); // pop command table
        commands.push_back(std::move(command));
    }

    lua_pop(L_, 1);
    return commands;
}

void CubeScript::UpdateGuiInput(const GuiInputSnapshot& input) {
    if (guiInputRef_ == LUA_REFNIL) {
        return;
    }
    lua_rawgeti(L_, LUA_REGISTRYINDEX, guiInputRef_);
    int stateIndex = lua_gettop(L_);

    lua_getfield(L_, stateIndex, "resetTransient");
    lua_pushvalue(L_, stateIndex);
    lua_call(L_, 1, 0);

    lua_getfield(L_, stateIndex, "setMouse");
    lua_pushvalue(L_, stateIndex);
    lua_pushnumber(L_, input.mouseX);
    lua_pushnumber(L_, input.mouseY);
    lua_pushboolean(L_, input.mouseDown);
    lua_call(L_, 4, 0);

    lua_getfield(L_, stateIndex, "setWheel");
    lua_pushvalue(L_, stateIndex);
    lua_pushnumber(L_, input.wheel);
    lua_call(L_, 2, 0);

    if (!input.textInput.empty()) {
        lua_getfield(L_, stateIndex, "addTextInput");
        lua_pushvalue(L_, stateIndex);
        lua_pushstring(L_, input.textInput.c_str());
        lua_call(L_, 2, 0);
    }

    for (const auto& [key, pressed] : input.keyStates) {
        lua_getfield(L_, stateIndex, "setKey");
        lua_pushvalue(L_, stateIndex);
        lua_pushstring(L_, key.c_str());
        lua_pushboolean(L_, pressed);
        lua_call(L_, 3, 0);
    }

    lua_pop(L_, 1);
}

bool CubeScript::HasGuiCommands() const {
    return guiCommandsFnRef_ != LUA_REFNIL;
}

std::filesystem::path CubeScript::GetScriptDirectory() const {
    return scriptDirectory_;
}

CubeScript::GuiCommand::RectData CubeScript::ReadRect(lua_State* L, int index) {
    GuiCommand::RectData rect{};
    if (!lua_istable(L, index)) {
        return rect;
    }
    int absIndex = lua_absindex(L, index);
    auto readField = [&](const char* name, float defaultValue) -> float {
        lua_getfield(L, absIndex, name);
        float value = defaultValue;
        if (lua_isnumber(L, -1)) {
            value = static_cast<float>(lua_tonumber(L, -1));
        }
        lua_pop(L, 1);
        return value;
    };
    rect.x = readField("x", rect.x);
    rect.y = readField("y", rect.y);
    rect.width = readField("width", rect.width);
    rect.height = readField("height", rect.height);
    return rect;
}

GuiColor CubeScript::ReadColor(lua_State* L, int index, const GuiColor& defaultColor) {
    GuiColor color = defaultColor;
    if (!lua_istable(L, index)) {
        return color;
    }
    int absIndex = lua_absindex(L, index);
    for (int component = 0; component < 4; ++component) {
        lua_rawgeti(L, absIndex, component + 1);
        if (lua_isnumber(L, -1)) {
            float value = static_cast<float>(lua_tonumber(L, -1));
            switch (component) {
                case 0: color.r = value; break;
                case 1: color.g = value; break;
                case 2: color.b = value; break;
                case 3: color.a = value; break;
            }
        }
        lua_pop(L, 1);
    }
    return color;
}

bool CubeScript::ReadStringField(lua_State* L, int index, const char* name, std::string& outString) {
    int absIndex = lua_absindex(L, index);
    lua_getfield(L, absIndex, name);
    if (lua_isstring(L, -1)) {
        outString = lua_tostring(L, -1);
        lua_pop(L, 1);
        return true;
    }
    lua_pop(L, 1);
    return false;
}

} // namespace sdl3cpp::script
