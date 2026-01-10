#include <gtest/gtest.h>

#include "services/impl/config/config_compiler_service.hpp"

namespace {

bool HasDiagnosticCode(const std::vector<sdl3cpp::services::ProbeReport>& diagnostics,
                       const std::string& code) {
    for (const auto& report : diagnostics) {
        if (report.code == code) {
            return true;
        }
    }
    return false;
}

TEST(ConfigCompilerReferenceValidationTest, FlagsUnknownShaderReference) {
    const std::string json = R"({
  "assets": {
    "shaders": {
      "known": { "vs": "shaders/known.vs", "fs": "shaders/known.fs" }
    }
  },
  "materials": {
    "mat": {
      "shader": "missing"
    }
  }
})";

    sdl3cpp::services::impl::ConfigCompilerService compiler(nullptr, nullptr, nullptr, nullptr);
    auto result = compiler.Compile(json);

    EXPECT_FALSE(result.success);
    EXPECT_TRUE(HasDiagnosticCode(result.diagnostics, "MATERIAL_SHADER_UNKNOWN"));
}

TEST(ConfigCompilerReferenceValidationTest, FlagsUnknownTextureReference) {
    const std::string json = R"({
  "assets": {
    "textures": {
      "tex1": { "uri": "textures/tex1.png" }
    },
    "shaders": {
      "known": { "vs": "shaders/known.vs", "fs": "shaders/known.fs" }
    }
  },
  "materials": {
    "mat": {
      "shader": "known",
      "textures": {
        "u_albedo": "missing"
      }
    }
  }
})";

    sdl3cpp::services::impl::ConfigCompilerService compiler(nullptr, nullptr, nullptr, nullptr);
    auto result = compiler.Compile(json);

    EXPECT_FALSE(result.success);
    EXPECT_TRUE(HasDiagnosticCode(result.diagnostics, "MATERIAL_TEXTURE_UNKNOWN"));
}

TEST(ConfigCompilerReferenceValidationTest, FlagsUnknownPassOutput) {
    const std::string json = R"({
  "render": {
    "passes": [
      {
        "id": "first",
        "outputs": {
          "color": { "format": "RGBA8", "usage": "renderTarget" }
        }
      },
      {
        "id": "second",
        "inputs": {
          "source": "@pass.first.missing"
        }
      }
    ]
  }
})";

    sdl3cpp::services::impl::ConfigCompilerService compiler(nullptr, nullptr, nullptr, nullptr);
    auto result = compiler.Compile(json);

    EXPECT_FALSE(result.success);
    EXPECT_TRUE(HasDiagnosticCode(result.diagnostics, "RG_INPUT_UNKNOWN_OUTPUT"));
}

TEST(ConfigCompilerReferenceValidationTest, FlagsUnknownPassReference) {
    const std::string json = R"({
  "render": {
    "passes": [
      {
        "id": "second",
        "inputs": {
          "source": "@pass.missing.color"
        }
      }
    ]
  }
})";

    sdl3cpp::services::impl::ConfigCompilerService compiler(nullptr, nullptr, nullptr, nullptr);
    auto result = compiler.Compile(json);

    EXPECT_FALSE(result.success);
    EXPECT_TRUE(HasDiagnosticCode(result.diagnostics, "RG_INPUT_UNKNOWN_PASS"));
}

TEST(ConfigCompilerReferenceValidationTest, FlagsSelfReference) {
    const std::string json = R"({
  "render": {
    "passes": [
      {
        "id": "self",
        "inputs": {
          "source": "@pass.self.color"
        },
        "outputs": {
          "color": { "format": "RGBA8", "usage": "renderTarget" }
        }
      }
    ]
  }
})";

    sdl3cpp::services::impl::ConfigCompilerService compiler(nullptr, nullptr, nullptr, nullptr);
    auto result = compiler.Compile(json);

    EXPECT_FALSE(result.success);
    EXPECT_TRUE(HasDiagnosticCode(result.diagnostics, "RG_INPUT_SELF_REFERENCE"));
}

TEST(ConfigCompilerReferenceValidationTest, FlagsInvalidViewId) {
    const std::string json = R"({
  "render": {
    "passes": [
      {
        "id": "main",
        "view_id": "bad"
      }
    ]
  }
})";

    sdl3cpp::services::impl::ConfigCompilerService compiler(nullptr, nullptr, nullptr, nullptr);
    auto result = compiler.Compile(json);

    EXPECT_FALSE(result.success);
    EXPECT_TRUE(HasDiagnosticCode(result.diagnostics, "RG_VIEW_ID_TYPE"));
}

TEST(ConfigCompilerReferenceValidationTest, FlagsInvalidClearColor) {
    const std::string json = R"({
  "render": {
    "passes": [
      {
        "id": "main",
        "clear": {
          "flags": ["color"],
          "color": [1.0, 0.5, 0.25]
        }
      }
    ]
  }
})";

    sdl3cpp::services::impl::ConfigCompilerService compiler(nullptr, nullptr, nullptr, nullptr);
    auto result = compiler.Compile(json);

    EXPECT_FALSE(result.success);
    EXPECT_TRUE(HasDiagnosticCode(result.diagnostics, "RG_CLEAR_COLOR"));
}

TEST(ConfigCompilerReferenceValidationTest, FlagsInvalidShaderSystemType) {
    const std::string json = R"({
  "assets": {
    "shaders": {
      "pbr": { "vs": "shaders/pbr.vs", "fs": "shaders/pbr.fs", "system": 3 }
    }
  }
})";

    sdl3cpp::services::impl::ConfigCompilerService compiler(nullptr, nullptr, nullptr, nullptr);
    auto result = compiler.Compile(json);

    EXPECT_FALSE(result.success);
    EXPECT_TRUE(HasDiagnosticCode(result.diagnostics, "ASSET_SHADER_SYSTEM_TYPE"));
}

TEST(ConfigCompilerReferenceValidationTest, FlagsUnknownShaderSystem) {
    const std::string json = R"({
  "shader_systems": {
    "systems": {
      "glsl": { "enabled": true }
    }
  },
  "assets": {
    "shaders": {
      "pbr": { "vs": "shaders/pbr.vs", "fs": "shaders/pbr.fs", "system": "materialx" }
    }
  }
})";

    sdl3cpp::services::impl::ConfigCompilerService compiler(nullptr, nullptr, nullptr, nullptr);
    auto result = compiler.Compile(json);

    EXPECT_FALSE(result.success);
    EXPECT_TRUE(HasDiagnosticCode(result.diagnostics, "ASSET_SHADER_SYSTEM_UNKNOWN"));
}

TEST(ConfigCompilerReferenceValidationTest, FlagsUnknownActiveShaderSystem) {
    const std::string json = R"({
  "shader_systems": {
    "active": "missing",
    "systems": {
      "glsl": { "enabled": true }
    }
  }
})";

    sdl3cpp::services::impl::ConfigCompilerService compiler(nullptr, nullptr, nullptr, nullptr);
    auto result = compiler.Compile(json);

    EXPECT_FALSE(result.success);
    EXPECT_TRUE(HasDiagnosticCode(result.diagnostics, "SHADER_SYSTEMS_ACTIVE_UNKNOWN"));
}

}  // namespace
