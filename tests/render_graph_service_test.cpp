#include <gtest/gtest.h>

#include "services/impl/render/render_graph_service.hpp"

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

sdl3cpp::services::RenderPassIR MakePass(const std::string& id,
                                         const std::string& inputPassId = {}) {
    sdl3cpp::services::RenderPassIR pass;
    pass.id = id;
    pass.jsonPath = "/render/passes/" + id;
    if (!inputPassId.empty()) {
        sdl3cpp::services::RenderPassInputIR input;
        input.name = "color";
        input.jsonPath = pass.jsonPath + "/inputs/color";
        input.ref.type = sdl3cpp::services::RenderResourceRefType::PassOutput;
        input.ref.passId = inputPassId;
        input.ref.outputName = "output";
        input.ref.jsonPath = input.jsonPath;
        pass.inputs.push_back(input);
    }
    return pass;
}

TEST(RenderGraphServiceTest, EmptyGraphSucceeds) {
    sdl3cpp::services::RenderGraphIR graph;
    sdl3cpp::services::impl::RenderGraphService service(nullptr);
    auto result = service.BuildGraph(graph);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.passOrder.empty());
    EXPECT_TRUE(result.diagnostics.empty());
}

TEST(RenderGraphServiceTest, ValidDependencyOrdersPasses) {
    sdl3cpp::services::RenderGraphIR graph;
    graph.passes.push_back(MakePass("gbuffer"));
    graph.passes.push_back(MakePass("tonemap", "gbuffer"));

    sdl3cpp::services::impl::RenderGraphService service(nullptr);
    auto result = service.BuildGraph(graph);

    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.passOrder.size(), 2u);
    EXPECT_EQ(result.passOrder[0], "gbuffer");
    EXPECT_EQ(result.passOrder[1], "tonemap");
}

TEST(RenderGraphServiceTest, DetectsUnknownPassInput) {
    sdl3cpp::services::RenderGraphIR graph;
    graph.passes.push_back(MakePass("main", "missing"));

    sdl3cpp::services::impl::RenderGraphService service(nullptr);
    auto result = service.BuildGraph(graph);

    EXPECT_FALSE(result.success);
    EXPECT_TRUE(HasDiagnosticCode(result.diagnostics, "RG_INPUT_UNKNOWN_PASS"));
}

TEST(RenderGraphServiceTest, DetectsDuplicatePassIds) {
    sdl3cpp::services::RenderGraphIR graph;
    graph.passes.push_back(MakePass("main"));
    graph.passes.push_back(MakePass("main"));

    sdl3cpp::services::impl::RenderGraphService service(nullptr);
    auto result = service.BuildGraph(graph);

    EXPECT_FALSE(result.success);
    EXPECT_TRUE(HasDiagnosticCode(result.diagnostics, "RG_PASS_ID_DUPLICATE"));
}

TEST(RenderGraphServiceTest, DetectsCycles) {
    sdl3cpp::services::RenderGraphIR graph;
    graph.passes.push_back(MakePass("a", "b"));
    graph.passes.push_back(MakePass("b", "a"));

    sdl3cpp::services::impl::RenderGraphService service(nullptr);
    auto result = service.BuildGraph(graph);

    EXPECT_FALSE(result.success);
    EXPECT_TRUE(HasDiagnosticCode(result.diagnostics, "RG_CYCLE_DETECTED"));
    EXPECT_LT(result.passOrder.size(), graph.passes.size());
}

}  // namespace
