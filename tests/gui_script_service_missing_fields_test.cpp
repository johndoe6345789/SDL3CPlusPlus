#include <gtest/gtest.h>

#include "services/impl/script/gui_script_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_script_engine_service.hpp"

#include <lua.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

class CapturingLogger final : public sdl3cpp::services::ILogger {
public:
    void SetLevel(sdl3cpp::services::LogLevel level) override { level_ = level; }
    sdl3cpp::services::LogLevel GetLevel() const override { return level_; }
    void SetOutputFile(const std::string&) override {}
    void SetMaxLinesPerFile(size_t) override {}
    void EnableConsoleOutput(bool) override {}
    void Log(sdl3cpp::services::LogLevel, const std::string& message) override {
        entries_.push_back(message);
    }
    void Trace(const std::string& message) override { entries_.push_back(message); }
    void Trace(const std::string& className,
               const std::string& methodName,
               const std::string& args = "",
               const std::string& message = "") override {
        std::string formatted = className + "::" + methodName;
        if (!args.empty()) {
            formatted += "(" + args + ")";
        }
        if (!message.empty()) {
            formatted += ": " + message;
        }
        entries_.push_back(formatted);
    }
    void Debug(const std::string& message) override { entries_.push_back(message); }
    void Info(const std::string& message) override { entries_.push_back(message); }
    void Warn(const std::string& message) override { entries_.push_back(message); }
    void Error(const std::string& message) override { entries_.push_back(message); }
    void TraceFunction(const std::string&) override {}
    void TraceVariable(const std::string&, const std::string&) override {}
    void TraceVariable(const std::string&, int) override {}
    void TraceVariable(const std::string&, size_t) override {}
    void TraceVariable(const std::string&, bool) override {}
    void TraceVariable(const std::string&, float) override {}
    void TraceVariable(const std::string&, double) override {}

    bool HasSubstring(const std::string& fragment) const {
        for (const auto& entry : entries_) {
            if (entry.find(fragment) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

private:
    sdl3cpp::services::LogLevel level_ = sdl3cpp::services::LogLevel::TRACE;
    std::vector<std::string> entries_;
};

class StubScriptEngineService final : public sdl3cpp::services::IScriptEngineService {
public:
    explicit StubScriptEngineService(lua_State* state) : state_(state) {}

    lua_State* GetLuaState() const override { return state_; }
    std::filesystem::path GetScriptDirectory() const override { return {}; }
    bool IsInitialized() const override { return state_ != nullptr; }

private:
    lua_State* state_;
};

TEST(GuiScriptServiceMissingFieldsTest, MissingBorderColorDefaultsToTransparent) {
    std::unique_ptr<lua_State, decltype(&lua_close)> state(luaL_newstate(), &lua_close);
    ASSERT_NE(state.get(), nullptr);
    luaL_openlibs(state.get());

    const char* script = R"(
        function get_gui_commands()
            return {
                {
                    type = "rect",
                    x = 10,
                    y = 20,
                    width = 30,
                    height = 40,
                    color = {0.1, 0.2, 0.3, 0.4}
                }
            }
        end
    )";

    ASSERT_EQ(luaL_dostring(state.get(), script), LUA_OK) << lua_tostring(state.get(), -1);

    auto engineService = std::make_shared<StubScriptEngineService>(state.get());
    auto logger = std::make_shared<CapturingLogger>();
    sdl3cpp::services::impl::GuiScriptService service(engineService, logger);

    service.Initialize();

    std::vector<sdl3cpp::services::GuiCommand> commands;
    ASSERT_NO_THROW(commands = service.LoadGuiCommands());
    ASSERT_EQ(commands.size(), 1u);

    const auto& command = commands.front();
    EXPECT_EQ(command.type, sdl3cpp::services::GuiCommand::Type::Rect);
    EXPECT_FLOAT_EQ(command.borderColor.r, 0.0f);
    EXPECT_FLOAT_EQ(command.borderColor.g, 0.0f);
    EXPECT_FLOAT_EQ(command.borderColor.b, 0.0f);
    EXPECT_FLOAT_EQ(command.borderColor.a, 0.0f);
    EXPECT_TRUE(logger->HasSubstring("Field not found or not table: borderColor"));

    service.Shutdown();
}

}  // namespace
