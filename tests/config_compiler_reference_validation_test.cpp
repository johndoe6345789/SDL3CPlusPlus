#include <gtest/gtest.h>

#include "services/impl/config_compiler_service.hpp"

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

}  // namespace
