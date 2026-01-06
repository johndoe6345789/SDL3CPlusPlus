#include "script_engine_service.hpp"

#include "lua_helpers.hpp"
#include "services/interfaces/i_logger.hpp"

#include <btBulletDynamicsCommon.h>
#include <lua.hpp>
#include <MaterialXCore/Document.h>
#include <MaterialXCore/Types.h>
#include <MaterialXFormat/File.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXFormat/XmlIo.h>
#include <SDL3/SDL.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
namespace mx = MaterialX;

struct MaterialXSurfaceParameters {
    std::array<float, 3> albedo = {1.0f, 1.0f, 1.0f};
    float roughness = 0.3f;
    float metallic = 0.0f;
    bool hasAlbedo = false;
    bool hasRoughness = false;
    bool hasMetallic = false;
};

std::filesystem::path ResolveMaterialXPath(const std::filesystem::path& path,
                                           const std::filesystem::path& scriptDirectory) {
    if (path.empty()) {
        return {};
    }
    if (path.is_absolute()) {
        return path;
    }
    if (!scriptDirectory.empty()) {
        auto projectRoot = scriptDirectory.parent_path();
        if (!projectRoot.empty()) {
            std::error_code ec;
            auto resolved = std::filesystem::weakly_canonical(projectRoot / path, ec);
            if (!ec) {
                return resolved;
            }
        }
    }
    std::error_code ec;
    auto resolved = std::filesystem::weakly_canonical(path, ec);
    if (ec) {
        return {};
    }
    return resolved;
}

mx::FileSearchPath BuildMaterialXSearchPath(const sdl3cpp::services::MaterialXConfig& config,
                                            const std::filesystem::path& scriptDirectory) {
    mx::FileSearchPath searchPath;
    std::filesystem::path libraryPath = ResolveMaterialXPath(config.libraryPath, scriptDirectory);
    if (libraryPath.empty() && !scriptDirectory.empty()) {
        auto fallback = scriptDirectory.parent_path() / "MaterialX" / "libraries";
        if (std::filesystem::exists(fallback)) {
            libraryPath = fallback;
        }
    }
    if (!libraryPath.empty()) {
        searchPath.append(mx::FilePath(libraryPath.string()));
    }
    return searchPath;
}

mx::DocumentPtr LoadMaterialXDocument(const std::filesystem::path& documentPath,
                                      const sdl3cpp::services::MaterialXConfig& config,
                                      const std::filesystem::path& scriptDirectory) {
    mx::DocumentPtr document = mx::createDocument();
    mx::FileSearchPath searchPath = BuildMaterialXSearchPath(config, scriptDirectory);
    mx::readFromXmlFile(document, mx::FilePath(documentPath.string()), searchPath);

    if (!config.libraryFolders.empty()) {
        mx::DocumentPtr stdLib = mx::createDocument();
        mx::FilePathVec folders;
        folders.reserve(config.libraryFolders.size());
        for (const auto& folder : config.libraryFolders) {
            folders.emplace_back(folder);
        }
        mx::loadLibraries(folders, searchPath, stdLib);
        document->importLibrary(stdLib);
    }

    return document;
}

mx::NodePtr ResolveSurfaceNode(const mx::DocumentPtr& document, const std::string& materialName) {
    if (!materialName.empty()) {
        mx::NodePtr candidate = document->getNode(materialName);
        if (candidate && candidate->getCategory() == "surfacematerial") {
            mx::NodePtr surfaceNode = candidate->getConnectedNode("surfaceshader");
            if (surfaceNode) {
                return surfaceNode;
            }
        }
        if (candidate && (candidate->getCategory() == "standard_surface"
            || candidate->getCategory() == "usd_preview_surface")) {
            return candidate;
        }
    }

    for (const auto& node : document->getNodes()) {
        if (node->getCategory() == "surfacematerial") {
            mx::NodePtr surfaceNode = node->getConnectedNode("surfaceshader");
            if (surfaceNode) {
                return surfaceNode;
            }
        }
    }

    for (const auto& node : document->getNodes()) {
        if (node->getCategory() == "standard_surface"
            || node->getCategory() == "usd_preview_surface") {
            return node;
        }
    }

    return {};
}

bool TryReadColor3(const mx::NodePtr& node, const char* name, std::array<float, 3>& outColor) {
    if (!node) {
        return false;
    }
    mx::InputPtr input = node->getInput(name);
    if (!input || !input->hasValueString()) {
        return false;
    }
    mx::ValuePtr value = input->getValue();
    if (!value || !value->isA<mx::Color3>()) {
        return false;
    }
    const mx::Color3& color = value->asA<mx::Color3>();
    outColor = {color[0], color[1], color[2]};
    return true;
}

bool TryReadFloat(const mx::NodePtr& node, const char* name, float& outValue) {
    if (!node) {
        return false;
    }
    mx::InputPtr input = node->getInput(name);
    if (!input || !input->hasValueString()) {
        return false;
    }
    mx::ValuePtr value = input->getValue();
    if (!value || !value->isA<float>()) {
        return false;
    }
    outValue = value->asA<float>();
    return true;
}

MaterialXSurfaceParameters ReadStandardSurfaceParameters(const mx::NodePtr& node) {
    MaterialXSurfaceParameters parameters;
    std::array<float, 3> baseColor = parameters.albedo;
    bool hasBaseColor = TryReadColor3(node, "base_color", baseColor);
    if (!hasBaseColor) {
        hasBaseColor = TryReadColor3(node, "diffuse_color", baseColor);
    }
    float baseStrength = 1.0f;
    bool hasBaseStrength = TryReadFloat(node, "base", baseStrength);
    if (hasBaseColor) {
        parameters.albedo = {
            baseColor[0] * baseStrength,
            baseColor[1] * baseStrength,
            baseColor[2] * baseStrength
        };
        parameters.hasAlbedo = true;
    } else if (hasBaseStrength) {
        parameters.albedo = {baseStrength, baseStrength, baseStrength};
        parameters.hasAlbedo = true;
    }

    float roughness = parameters.roughness;
    if (TryReadFloat(node, "specular_roughness", roughness)
        || TryReadFloat(node, "roughness", roughness)) {
        parameters.roughness = roughness;
        parameters.hasRoughness = true;
    }

    float metallic = parameters.metallic;
    if (TryReadFloat(node, "metalness", metallic)
        || TryReadFloat(node, "metallic", metallic)) {
        parameters.metallic = metallic;
        parameters.hasMetallic = true;
    }

    return parameters;
}

bool TryParseMouseButtonName(const char* name, uint8_t& button) {
    if (!name) {
        return false;
    }
    std::string lower(name);
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (lower == "left") {
        button = SDL_BUTTON_LEFT;
        return true;
    }
    if (lower == "right") {
        button = SDL_BUTTON_RIGHT;
        return true;
    }
    if (lower == "middle") {
        button = SDL_BUTTON_MIDDLE;
        return true;
    }
    if (lower == "x1" || lower == "extra1") {
        button = SDL_BUTTON_X1;
        return true;
    }
    if (lower == "x2" || lower == "extra2") {
        button = SDL_BUTTON_X2;
        return true;
    }
    return false;
}

SDL_Keycode ReadKeycodeFromLua(lua_State* L, int index, bool& ok) {
    ok = true;
    if (lua_isnumber(L, index)) {
        return static_cast<SDL_Keycode>(lua_tointeger(L, index));
    }
    if (lua_isstring(L, index)) {
        const char* name = lua_tostring(L, index);
        SDL_Keycode key = SDL_GetKeyFromName(name);
        if (key == SDLK_UNKNOWN) {
            ok = false;
        }
        return key;
    }
    ok = false;
    return SDLK_UNKNOWN;
}

void PushJsonValue(lua_State* L, const rapidjson::Value& value) {
    if (value.IsObject()) {
        lua_newtable(L);
        for (auto it = value.MemberBegin(); it != value.MemberEnd(); ++it) {
            lua_pushlstring(L, it->name.GetString(), it->name.GetStringLength());
            PushJsonValue(L, it->value);
            lua_settable(L, -3);
        }
        return;
    }
    if (value.IsArray()) {
        lua_newtable(L);
        for (rapidjson::SizeType i = 0; i < value.Size(); ++i) {
            PushJsonValue(L, value[i]);
            lua_rawseti(L, -2, static_cast<lua_Integer>(i + 1));
        }
        return;
    }
    if (value.IsString()) {
        lua_pushlstring(L, value.GetString(), value.GetStringLength());
        return;
    }
    if (value.IsBool()) {
        lua_pushboolean(L, value.GetBool());
        return;
    }
    if (value.IsNumber()) {
        lua_pushnumber(L, value.GetDouble());
        return;
    }
    lua_pushnil(L);
}

bool PushConfigTableFromJson(lua_State* L, const std::string& json, std::string& error) {
    if (json.empty()) {
        error = "Config JSON not available";
        return false;
    }
    rapidjson::Document document;
    rapidjson::ParseResult result = document.Parse(json.c_str());
    if (!result) {
        error = std::string("Config JSON parse failed: ") + rapidjson::GetParseError_En(result.Code());
        return false;
    }
    PushJsonValue(L, document);
    return true;
}
}  // namespace

namespace sdl3cpp::services::impl {

ScriptEngineService::ScriptEngineService(const std::filesystem::path& scriptPath,
                                         std::shared_ptr<ILogger> logger,
                                         std::shared_ptr<IMeshService> meshService,
                                         std::shared_ptr<IAudioCommandService> audioCommandService,
                                         std::shared_ptr<IPhysicsBridgeService> physicsBridgeService,
                                         std::shared_ptr<IInputService> inputService,
                                         std::shared_ptr<IWindowService> windowService,
                                         std::shared_ptr<IConfigService> configService,
                                         bool debugEnabled)
    : logger_(std::move(logger)),
      meshService_(std::move(meshService)),
      audioCommandService_(std::move(audioCommandService)),
      physicsBridgeService_(std::move(physicsBridgeService)),
      inputService_(std::move(inputService)),
      windowService_(std::move(windowService)),
      configService_(std::move(configService)),
      scriptPath_(scriptPath),
      debugEnabled_(debugEnabled) {
    if (logger_) {
        logger_->Trace("ScriptEngineService", "ScriptEngineService",
                       "scriptPath=" + scriptPath_.string() +
                       ", debugEnabled=" + std::string(debugEnabled_ ? "true" : "false") +
                       ", meshService=" + std::string(meshService_ ? "set" : "null") +
                       ", audioCommandService=" + std::string(audioCommandService_ ? "set" : "null") +
                       ", physicsBridgeService=" + std::string(physicsBridgeService_ ? "set" : "null") +
                       ", inputService=" + std::string(inputService_ ? "set" : "null") +
                       ", windowService=" + std::string(windowService_ ? "set" : "null") +
                       ", configService=" + std::string(configService_ ? "set" : "null"));
    }
}

ScriptEngineService::~ScriptEngineService() {
    if (logger_) {
        logger_->Trace("ScriptEngineService", "~ScriptEngineService");
    }
    if (initialized_) {
        Shutdown();
    }
}

void ScriptEngineService::Initialize() {
    if (initialized_) {
        return;
    }
    logger_->Trace("ScriptEngineService", "Initialize",
                   "scriptPath=" + scriptPath_.string() +
                   ", debugEnabled=" + std::string(debugEnabled_ ? "true" : "false"));

    bindingContext_ = std::make_shared<LuaBindingContext>();
    bindingContext_->meshService = meshService_;
    bindingContext_->audioCommandService = audioCommandService_;
    bindingContext_->physicsBridgeService = physicsBridgeService_;
    bindingContext_->inputService = inputService_;
    bindingContext_->windowService = windowService_;
    bindingContext_->configService = configService_;
    bindingContext_->logger = logger_;

    luaState_ = luaL_newstate();
    if (!luaState_) {
        logger_->Error("Failed to create Lua state");
        throw std::runtime_error("Failed to create Lua state");
    }

    luaL_openlibs(luaState_);
    RegisterBindings(luaState_);

    lua_pushboolean(luaState_, debugEnabled_);
    lua_setglobal(luaState_, "lua_debug");

    if (configService_) {
        std::string error;
        if (PushConfigTableFromJson(luaState_, configService_->GetConfigJson(), error)) {
            lua_setglobal(luaState_, "config");
        } else {
            if (logger_) {
                logger_->Error("ScriptEngineService: " + error);
            }
            lua_newtable(luaState_);
            lua_setglobal(luaState_, "config");
        }
    } else {
        if (logger_) {
            logger_->Error("ScriptEngineService: Config service not available");
        }
        lua_newtable(luaState_);
        lua_setglobal(luaState_, "config");
    }

    scriptDirectory_ = scriptPath_.parent_path();
    if (!scriptDirectory_.empty()) {
        lua_getglobal(luaState_, "package");
        if (lua_istable(luaState_, -1)) {
            lua_getfield(luaState_, -1, "path");
            const char* currentPath = lua_tostring(luaState_, -1);
            std::string newPath = scriptDirectory_.string() + "/?.lua;";
            if (currentPath) {
                newPath += currentPath;
            }
            lua_pop(luaState_, 1);
            lua_pushstring(luaState_, newPath.c_str());
            lua_setfield(luaState_, -2, "path");
        }
        lua_pop(luaState_, 1);
    }

    if (luaL_dofile(luaState_, scriptPath_.string().c_str()) != LUA_OK) {
        std::string message = lua::GetLuaError(luaState_);
        lua_pop(luaState_, 1);
        lua_close(luaState_);
        luaState_ = nullptr;
        if (logger_) {
            logger_->Error("Failed to load Lua script: " + message);
        }
        throw std::runtime_error("Failed to load Lua script: " + message);
    }

    initialized_ = true;
    logger_->Info("Script engine service initialized");
}

void ScriptEngineService::Shutdown() noexcept {
    if (!initialized_) {
        return;
    }
    logger_->Trace("ScriptEngineService", "Shutdown");

    if (luaState_) {
        lua_close(luaState_);
        luaState_ = nullptr;
    }
    bindingContext_.reset();
    initialized_ = false;

    logger_->Info("Script engine service shutdown");
}

lua_State* ScriptEngineService::GetLuaState() const {
    if (logger_) {
        logger_->Trace("ScriptEngineService", "GetLuaState");
    }
    if (!luaState_) {
        throw std::runtime_error("Script engine service not initialized");
    }
    return luaState_;
}

std::filesystem::path ScriptEngineService::GetScriptDirectory() const {
    if (logger_) {
        logger_->Trace("ScriptEngineService", "GetScriptDirectory");
    }
    if (!luaState_) {
        throw std::runtime_error("Script engine service not initialized");
    }
    return scriptDirectory_;
}

void ScriptEngineService::RegisterBindings(lua_State* L) {
    if (logger_) {
        logger_->Trace("ScriptEngineService", "RegisterBindings");
    }
    if (!bindingContext_) {
        throw std::runtime_error("Lua binding context not initialized");
    }

    auto bind = [&](const char* name, lua_CFunction fn) {
        lua_pushlightuserdata(L, bindingContext_.get());
        lua_pushcclosure(L, fn, 1);
        lua_setglobal(L, name);
    };

    bind("load_mesh_from_file", &ScriptEngineService::LoadMeshFromFile);
    bind("load_mesh_from_pk3", &ScriptEngineService::LoadMeshFromArchive);
    bind("physics_create_box", &ScriptEngineService::PhysicsCreateBox);
    bind("physics_step_simulation", &ScriptEngineService::PhysicsStepSimulation);
    bind("physics_get_transform", &ScriptEngineService::PhysicsGetTransform);
    bind("glm_matrix_from_transform", &ScriptEngineService::GlmMatrixFromTransform);
    bind("audio_play_background", &ScriptEngineService::AudioPlayBackground);
    bind("audio_play_sound", &ScriptEngineService::AudioPlaySound);
    bind("audio_stop_background", &ScriptEngineService::AudioStopBackground);
    bind("input_get_mouse_position", &ScriptEngineService::InputGetMousePosition);
    bind("input_get_mouse_delta", &ScriptEngineService::InputGetMouseDelta);
    bind("input_get_mouse_wheel", &ScriptEngineService::InputGetMouseWheel);
    bind("input_is_key_down", &ScriptEngineService::InputIsKeyDown);
    bind("input_is_action_down", &ScriptEngineService::InputIsActionDown);
    bind("input_is_mouse_down", &ScriptEngineService::InputIsMouseDown);
    bind("input_get_text", &ScriptEngineService::InputGetText);
    bind("config_get_json", &ScriptEngineService::ConfigGetJson);
    bind("config_get_table", &ScriptEngineService::ConfigGetTable);
    bind("materialx_get_surface_parameters", &ScriptEngineService::MaterialXGetSurfaceParameters);
    bind("window_get_size", &ScriptEngineService::WindowGetSize);
    bind("window_set_title", &ScriptEngineService::WindowSetTitle);
    bind("window_is_minimized", &ScriptEngineService::WindowIsMinimized);
    bind("window_set_mouse_grabbed", &ScriptEngineService::WindowSetMouseGrabbed);
    bind("window_get_mouse_grabbed", &ScriptEngineService::WindowGetMouseGrabbed);
    bind("window_set_relative_mouse_mode", &ScriptEngineService::WindowSetRelativeMouseMode);
    bind("window_get_relative_mouse_mode", &ScriptEngineService::WindowGetRelativeMouseMode);
    bind("window_set_cursor_visible", &ScriptEngineService::WindowSetCursorVisible);
    bind("window_is_cursor_visible", &ScriptEngineService::WindowIsCursorVisible);
    bind("print", &ScriptEngineService::LuaPrint);
}

int ScriptEngineService::LoadMeshFromFile(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto logger = context ? context->logger : nullptr;
    if (!context || !context->meshService) {
        lua_pushnil(L);
        lua_pushstring(L, "Mesh service not available");
        return 2;
    }

    const char* path = luaL_checkstring(L, 1);
    if (logger) {
        logger->Trace("ScriptEngineService", "LoadMeshFromFile",
                      "path=" + std::string(path));
    }

    MeshPayload payload;
    std::string error;
    if (!context->meshService->LoadFromFile(path, payload, error)) {
        lua_pushnil(L);
        lua_pushstring(L, error.c_str());
        return 2;
    }

    context->meshService->PushMeshToLua(L, payload);
    lua_pushnil(L);
    return 2;
}

int ScriptEngineService::LoadMeshFromArchive(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto logger = context ? context->logger : nullptr;
    if (!context || !context->meshService) {
        lua_pushnil(L);
        lua_pushstring(L, "Mesh service not available");
        return 2;
    }

    const char* archivePath = luaL_checkstring(L, 1);
    const char* entryPath = luaL_checkstring(L, 2);
    if (logger) {
        logger->Trace("ScriptEngineService", "LoadMeshFromArchive",
                      "archivePath=" + std::string(archivePath) +
                      ", entryPath=" + std::string(entryPath));
    }

    MeshPayload payload;
    std::string error;
    if (!context->meshService->LoadFromArchive(archivePath, entryPath, payload, error)) {
        lua_pushnil(L);
        lua_pushstring(L, error.c_str());
        return 2;
    }

    context->meshService->PushMeshToLua(L, payload);
    lua_pushnil(L);
    return 2;
}

int ScriptEngineService::PhysicsCreateBox(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto logger = context ? context->logger : nullptr;
    if (!context || !context->physicsBridgeService) {
        lua_pushnil(L);
        lua_pushstring(L, "Physics service not available");
        return 2;
    }

    const char* name = luaL_checkstring(L, 1);
    if (logger) {
        logger->Trace("ScriptEngineService", "PhysicsCreateBox",
                      "name=" + std::string(name));
    }

    if (!lua_istable(L, 2) || !lua_istable(L, 4) || !lua_istable(L, 5)) {
        luaL_error(L, "physics_create_box expects vector tables for half extents, origin, and rotation");
    }

    std::array<float, 3> halfExtents = lua::ReadVector3(L, 2);
    float mass = static_cast<float>(luaL_checknumber(L, 3));
    std::array<float, 3> origin = lua::ReadVector3(L, 4);
    std::array<float, 4> rotation = lua::ReadQuaternion(L, 5);

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(origin[0], origin[1], origin[2]));
    transform.setRotation(btQuaternion(rotation[0], rotation[1], rotation[2], rotation[3]));

    std::string error;
    if (!context->physicsBridgeService->AddBoxRigidBody(
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

int ScriptEngineService::PhysicsStepSimulation(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto logger = context ? context->logger : nullptr;
    if (!context || !context->physicsBridgeService) {
        lua_pushinteger(L, 0);
        return 1;
    }
    if (logger) {
        logger->Trace("ScriptEngineService", "PhysicsStepSimulation");
    }
    float deltaTime = static_cast<float>(luaL_checknumber(L, 1));
    int steps = context->physicsBridgeService->StepSimulation(deltaTime);
    lua_pushinteger(L, steps);
    return 1;
}

int ScriptEngineService::PhysicsGetTransform(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto logger = context ? context->logger : nullptr;
    if (!context || !context->physicsBridgeService) {
        lua_pushnil(L);
        lua_pushstring(L, "Physics service not available");
        return 2;
    }
    const char* name = luaL_checkstring(L, 1);
    if (logger) {
        logger->Trace("ScriptEngineService", "PhysicsGetTransform",
                      "name=" + std::string(name));
    }

    btTransform transform;
    std::string error;
    if (!context->physicsBridgeService->GetRigidBodyTransform(name, transform, error)) {
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

int ScriptEngineService::AudioPlayBackground(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto logger = context ? context->logger : nullptr;
    if (!context || !context->audioCommandService) {
        lua_pushnil(L);
        lua_pushstring(L, "Audio service not available");
        return 2;
    }
    const char* path = luaL_checkstring(L, 1);
    bool loop = true;
    if (lua_gettop(L) >= 2 && lua_isboolean(L, 2)) {
        loop = lua_toboolean(L, 2);
    }
    if (logger) {
        logger->Trace("ScriptEngineService", "AudioPlayBackground",
                      "path=" + std::string(path) +
                      ", loop=" + std::string(loop ? "true" : "false"));
    }

    std::string error;
    if (!context->audioCommandService->QueueAudioCommand(
            AudioCommandType::Background, path, loop, error)) {
        lua_pushnil(L);
        lua_pushstring(L, error.c_str());
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

int ScriptEngineService::AudioPlaySound(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto logger = context ? context->logger : nullptr;
    if (!context || !context->audioCommandService) {
        lua_pushnil(L);
        lua_pushstring(L, "Audio service not available");
        return 2;
    }
    const char* path = luaL_checkstring(L, 1);
    bool loop = false;
    if (lua_gettop(L) >= 2 && lua_isboolean(L, 2)) {
        loop = lua_toboolean(L, 2);
    }
    if (logger) {
        logger->Trace("ScriptEngineService", "AudioPlaySound",
                      "path=" + std::string(path) +
                      ", loop=" + std::string(loop ? "true" : "false"));
    }

    std::string error;
    if (!context->audioCommandService->QueueAudioCommand(
            AudioCommandType::Effect, path, loop, error)) {
        lua_pushnil(L);
        lua_pushstring(L, error.c_str());
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

int ScriptEngineService::AudioStopBackground(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto logger = context ? context->logger : nullptr;
    if (!context || !context->audioCommandService) {
        lua_pushnil(L);
        lua_pushstring(L, "Audio service not available");
        return 2;
    }
    if (logger) {
        logger->Trace("ScriptEngineService", "AudioStopBackground");
    }

    std::string error;
    if (!context->audioCommandService->StopBackground(error)) {
        lua_pushnil(L);
        lua_pushstring(L, error.c_str());
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

int ScriptEngineService::InputGetMousePosition(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!context || !context->inputService) {
        lua_pushnil(L);
        lua_pushstring(L, "Input service not available");
        return 2;
    }
    auto [x, y] = context->inputService->GetMousePosition();
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    return 2;
}

int ScriptEngineService::InputGetMouseDelta(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!context || !context->inputService) {
        lua_pushnil(L);
        lua_pushstring(L, "Input service not available");
        return 2;
    }
    const auto& state = context->inputService->GetState();
    lua_pushnumber(L, state.mouseDeltaX);
    lua_pushnumber(L, state.mouseDeltaY);
    return 2;
}

int ScriptEngineService::InputGetMouseWheel(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!context || !context->inputService) {
        lua_pushnil(L);
        lua_pushstring(L, "Input service not available");
        return 2;
    }
    const auto& state = context->inputService->GetState();
    lua_pushnumber(L, state.mouseWheelDeltaX);
    lua_pushnumber(L, state.mouseWheelDeltaY);
    return 2;
}

int ScriptEngineService::InputIsKeyDown(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!context || !context->inputService) {
        lua_pushnil(L);
        lua_pushstring(L, "Input service not available");
        return 2;
    }
    bool ok = false;
    SDL_Keycode key = ReadKeycodeFromLua(L, 1, ok);
    if (!ok) {
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_pushboolean(L, context->inputService->IsKeyPressed(key));
    return 1;
}

int ScriptEngineService::InputIsActionDown(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!context || !context->inputService) {
        lua_pushnil(L);
        lua_pushstring(L, "Input service not available");
        return 2;
    }
    const char* action = luaL_checkstring(L, 1);
    lua_pushboolean(L, context->inputService->IsActionPressed(action));
    return 1;
}

int ScriptEngineService::InputIsMouseDown(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!context || !context->inputService) {
        lua_pushnil(L);
        lua_pushstring(L, "Input service not available");
        return 2;
    }
    uint8_t button = SDL_BUTTON_LEFT;
    if (lua_isnumber(L, 1)) {
        button = static_cast<uint8_t>(lua_tointeger(L, 1));
    } else if (lua_isstring(L, 1)) {
        const char* name = lua_tostring(L, 1);
        if (!TryParseMouseButtonName(name, button)) {
            lua_pushboolean(L, 0);
            return 1;
        }
    } else {
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_pushboolean(L, context->inputService->IsMouseButtonPressed(button));
    return 1;
}

int ScriptEngineService::InputGetText(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!context || !context->inputService) {
        lua_pushnil(L);
        lua_pushstring(L, "Input service not available");
        return 2;
    }
    const auto& state = context->inputService->GetState();
    lua_pushstring(L, state.textInput.c_str());
    return 1;
}

int ScriptEngineService::ConfigGetJson(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!context || !context->configService) {
        lua_pushnil(L);
        lua_pushstring(L, "Config service not available");
        return 2;
    }
    const std::string& json = context->configService->GetConfigJson();
    if (json.empty()) {
        lua_pushnil(L);
        lua_pushstring(L, "Config JSON not available");
        return 2;
    }
    lua_pushlstring(L, json.c_str(), json.size());
    return 1;
}

int ScriptEngineService::ConfigGetTable(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!context || !context->configService) {
        lua_pushnil(L);
        lua_pushstring(L, "Config service not available");
        return 2;
    }
    std::string error;
    if (!PushConfigTableFromJson(L, context->configService->GetConfigJson(), error)) {
        lua_pushnil(L);
        lua_pushstring(L, error.c_str());
        return 2;
    }
    return 1;
}

int ScriptEngineService::MaterialXGetSurfaceParameters(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto logger = context ? context->logger : nullptr;
    if (!context || !context->configService) {
        lua_pushnil(L);
        lua_pushstring(L, "Config service not available");
        return 2;
    }

    const char* documentArg = luaL_checkstring(L, 1);
    const char* materialArg = luaL_optstring(L, 2, "");
    std::filesystem::path scriptDirectory = context->configService->GetScriptPath().parent_path();
    if (logger) {
        logger->Trace("ScriptEngineService", "MaterialXGetSurfaceParameters",
                      "document=" + std::string(documentArg ? documentArg : "") +
                      ", material=" + std::string(materialArg ? materialArg : ""));
    }

    std::filesystem::path documentPath = ResolveMaterialXPath(documentArg ? documentArg : "", scriptDirectory);
    if (documentPath.empty()) {
        lua_pushnil(L);
        lua_pushstring(L, "MaterialX document path could not be resolved");
        return 2;
    }
    if (!std::filesystem::exists(documentPath)) {
        lua_pushnil(L);
        std::string message = "MaterialX document not found: " + documentPath.string();
        lua_pushstring(L, message.c_str());
        return 2;
    }

    const auto& materialConfig = context->configService->GetMaterialXConfig();
    mx::DocumentPtr document;
    try {
        document = LoadMaterialXDocument(documentPath, materialConfig, scriptDirectory);
    } catch (const std::exception& ex) {
        lua_pushnil(L);
        std::string message = "MaterialX document load failed: ";
        message += ex.what();
        lua_pushstring(L, message.c_str());
        return 2;
    }

    mx::NodePtr surfaceNode = ResolveSurfaceNode(document, materialArg ? materialArg : "");
    if (!surfaceNode) {
        lua_pushnil(L);
        lua_pushstring(L, "MaterialX document has no standard_surface material");
        return 2;
    }

    MaterialXSurfaceParameters parameters = ReadStandardSurfaceParameters(surfaceNode);
    if (!parameters.hasAlbedo && !parameters.hasRoughness && !parameters.hasMetallic) {
        lua_pushnil(L);
        lua_pushstring(L, "MaterialX material does not expose supported PBR parameters");
        return 2;
    }

    lua_newtable(L);
    if (parameters.hasAlbedo) {
        lua_newtable(L);
        lua_pushnumber(L, parameters.albedo[0]);
        lua_rawseti(L, -2, 1);
        lua_pushnumber(L, parameters.albedo[1]);
        lua_rawseti(L, -2, 2);
        lua_pushnumber(L, parameters.albedo[2]);
        lua_rawseti(L, -2, 3);
        lua_setfield(L, -2, "material_albedo");
    }
    if (parameters.hasRoughness) {
        lua_pushnumber(L, parameters.roughness);
        lua_setfield(L, -2, "material_roughness");
    }
    if (parameters.hasMetallic) {
        lua_pushnumber(L, parameters.metallic);
        lua_setfield(L, -2, "material_metallic");
    }

    lua_pushnil(L);
    return 2;
}

int ScriptEngineService::WindowGetSize(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!context || !context->windowService) {
        lua_pushnil(L);
        lua_pushstring(L, "Window service not available");
        return 2;
    }
    auto [width, height] = context->windowService->GetSize();
    lua_pushinteger(L, static_cast<lua_Integer>(width));
    lua_pushinteger(L, static_cast<lua_Integer>(height));
    return 2;
}

int ScriptEngineService::WindowSetTitle(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!context || !context->windowService) {
        lua_pushnil(L);
        lua_pushstring(L, "Window service not available");
        return 2;
    }
    const char* title = luaL_checkstring(L, 1);
    context->windowService->SetTitle(title);
    lua_pushboolean(L, 1);
    return 1;
}

int ScriptEngineService::WindowIsMinimized(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!context || !context->windowService) {
        lua_pushnil(L);
        lua_pushstring(L, "Window service not available");
        return 2;
    }
    lua_pushboolean(L, context->windowService->IsMinimized());
    return 1;
}

int ScriptEngineService::WindowSetMouseGrabbed(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!context || !context->windowService) {
        lua_pushnil(L);
        lua_pushstring(L, "Window service not available");
        return 2;
    }
    bool grabbed = lua_toboolean(L, 1);
    context->windowService->SetMouseGrabbed(grabbed);
    lua_pushboolean(L, 1);
    return 1;
}

int ScriptEngineService::WindowGetMouseGrabbed(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!context || !context->windowService) {
        lua_pushnil(L);
        lua_pushstring(L, "Window service not available");
        return 2;
    }
    lua_pushboolean(L, context->windowService->IsMouseGrabbed());
    return 1;
}

int ScriptEngineService::WindowSetRelativeMouseMode(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!context || !context->windowService) {
        lua_pushnil(L);
        lua_pushstring(L, "Window service not available");
        return 2;
    }
    bool enabled = lua_toboolean(L, 1);
    context->windowService->SetRelativeMouseMode(enabled);
    if (context->inputService) {
        context->inputService->SetRelativeMouseMode(enabled);
    }
    lua_pushboolean(L, 1);
    return 1;
}

int ScriptEngineService::WindowGetRelativeMouseMode(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!context || !context->windowService) {
        lua_pushnil(L);
        lua_pushstring(L, "Window service not available");
        return 2;
    }
    lua_pushboolean(L, context->windowService->IsRelativeMouseMode());
    return 1;
}

int ScriptEngineService::WindowSetCursorVisible(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!context || !context->windowService) {
        lua_pushnil(L);
        lua_pushstring(L, "Window service not available");
        return 2;
    }
    bool visible = lua_toboolean(L, 1);
    context->windowService->SetCursorVisible(visible);
    lua_pushboolean(L, 1);
    return 1;
}

int ScriptEngineService::WindowIsCursorVisible(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!context || !context->windowService) {
        lua_pushnil(L);
        lua_pushstring(L, "Window service not available");
        return 2;
    }
    lua_pushboolean(L, context->windowService->IsCursorVisible());
    return 1;
}

int ScriptEngineService::GlmMatrixFromTransform(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto logger = context ? context->logger : nullptr;
    if (logger) {
        logger->Trace("ScriptEngineService", "GlmMatrixFromTransform",
                      "luaStateIsNull=" + std::string(L ? "false" : "true"));
    }
    return lua::LuaGlmMatrixFromTransform(L);
}

int ScriptEngineService::LuaPrint(lua_State* L) {
    auto* context = static_cast<LuaBindingContext*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto logger = context ? context->logger : nullptr;

    int nargs = lua_gettop(L);
    std::string message;

    for (int i = 1; i <= nargs; ++i) {
        if (i > 1) {
            message += "\t";
        }

        const char* str = nullptr;
        if (lua_isstring(L, i)) {
            str = lua_tostring(L, i);
        } else if (lua_isnil(L, i)) {
            str = "nil";
        } else if (lua_isboolean(L, i)) {
            str = lua_toboolean(L, i) ? "true" : "false";
        } else if (lua_isnumber(L, i)) {
            lua_pushvalue(L, i);
            str = lua_tostring(L, -1);
            lua_pop(L, 1);
        } else {
            lua_pushvalue(L, i);
            str = lua_tostring(L, -1);
            if (!str) {
                str = lua_typename(L, lua_type(L, i));
            }
            lua_pop(L, 1);
        }

        if (str) {
            message += str;
        }
    }

    if (logger) {
        logger->Info("[Lua] " + message);
    }

    return 0;
}

}  // namespace sdl3cpp::services::impl
