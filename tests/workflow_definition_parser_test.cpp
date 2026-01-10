#include <gtest/gtest.h>

#include "services/impl/workflow/workflow_definition_parser.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

struct TempFile {
    explicit TempFile(const std::string& contents) {
        const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path = std::filesystem::temp_directory_path() /
               ("workflow_definition_parser_" + std::to_string(timestamp) + ".json");
        std::ofstream stream(path);
        stream << contents;
    }

    ~TempFile() {
        std::error_code error;
        std::filesystem::remove(path, error);
    }

    std::filesystem::path path;
};

}  // namespace

TEST(WorkflowDefinitionParserTest, ParsesTemplateAndSteps) {
    const std::string json = R"json({
  "template": "boot.default",
  "steps": [
    {
      "id": "load_config",
      "plugin": "config.load",
      "inputs": { "path": "config.path" },
      "outputs": { "document": "config.document" }
    },
    {
      "id": "validate_schema",
      "plugin": "config.schema.validate",
      "inputs": { "document": "config.document", "path": "config.path" }
    }
  ]
})json";

    TempFile file(json);
    sdl3cpp::services::impl::WorkflowDefinitionParser parser;
    const auto workflow = parser.ParseFile(file.path);

    EXPECT_EQ(workflow.templateName, "boot.default");
    ASSERT_EQ(workflow.steps.size(), 2u);
    EXPECT_EQ(workflow.steps[0].id, "load_config");
    EXPECT_EQ(workflow.steps[0].plugin, "config.load");
    EXPECT_EQ(workflow.steps[0].inputs.at("path"), "config.path");
    EXPECT_EQ(workflow.steps[0].outputs.at("document"), "config.document");
    EXPECT_EQ(workflow.steps[1].id, "validate_schema");
    EXPECT_EQ(workflow.steps[1].plugin, "config.schema.validate");
}

TEST(WorkflowDefinitionParserTest, FailsWhenPluginIsMissing) {
    const std::string json = R"json({
  "template": "boot.invalid",
  "steps": [
    {
      "id": "load_config"
    }
  ]
})json";

    TempFile file(json);
    sdl3cpp::services::impl::WorkflowDefinitionParser parser;
    try {
        parser.ParseFile(file.path);
        FAIL() << "Expected workflow parser to reject missing plugin";
    } catch (const std::runtime_error& error) {
        const std::string message = error.what();
        EXPECT_NE(message.find("plugin"), std::string::npos);
    }
}

TEST(WorkflowDefinitionParserTest, ParsesN8nNodesWithConnections) {
    const std::string json = R"json({
  "template": "boot.default",
  "nodes": [
    {
      "id": "load_config",
      "plugin": "config.load",
      "inputs": { "path": "config.path" },
      "outputs": { "document": "config.document" }
    },
    {
      "name": "validate_schema",
      "type": "config.schema.validate",
      "inputs": { "document": "config.document", "path": "config.path" }
    }
  ],
  "connections": {
    "load_config": {
      "main": [
        [
          { "node": "validate_schema", "type": "main", "index": 0 }
        ]
      ]
    }
  }
})json";

    TempFile file(json);
    sdl3cpp::services::impl::WorkflowDefinitionParser parser;
    const auto workflow = parser.ParseFile(file.path);

    ASSERT_EQ(workflow.steps.size(), 2u);
    EXPECT_EQ(workflow.steps[0].id, "load_config");
    EXPECT_EQ(workflow.steps[0].plugin, "config.load");
    EXPECT_EQ(workflow.steps[1].id, "validate_schema");
    EXPECT_EQ(workflow.steps[1].plugin, "config.schema.validate");
}

TEST(WorkflowDefinitionParserTest, RejectsUnknownNodeInConnections) {
    const std::string json = R"json({
  "nodes": [
    {
      "id": "start",
      "plugin": "config.load"
    }
  ],
  "connections": {
    "start": {
      "main": [
        [
          { "node": "missing", "type": "main", "index": 0 }
        ]
      ]
    }
  }
})json";

    TempFile file(json);
    sdl3cpp::services::impl::WorkflowDefinitionParser parser;
    try {
        parser.ParseFile(file.path);
        FAIL() << "Expected workflow parser to reject unknown node connection";
    } catch (const std::runtime_error& error) {
        const std::string message = error.what();
        EXPECT_NE(message.find("unknown node"), std::string::npos);
    }
}
