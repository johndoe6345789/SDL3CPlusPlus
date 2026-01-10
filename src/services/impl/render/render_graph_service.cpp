#include "render_graph_service.hpp"

#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace sdl3cpp::services::impl {

namespace {

bool IsErrorSeverity(ProbeSeverity severity) {
    return severity == ProbeSeverity::Error || severity == ProbeSeverity::Fatal;
}

}  // namespace

RenderGraphService::RenderGraphService(std::shared_ptr<ILogger> logger,
                                       std::shared_ptr<IProbeService> probeService)
    : logger_(std::move(logger)),
      probeService_(std::move(probeService)) {
    if (logger_) {
        logger_->Trace("RenderGraphService", "RenderGraphService", "initialized=true");
    }
}

RenderGraphBuildResult RenderGraphService::BuildGraph(const RenderGraphIR& renderGraph) {
    RenderGraphBuildResult result;
    result.success = true;

    if (renderGraph.passes.empty()) {
        return result;
    }

    std::unordered_map<std::string, size_t> passIndex;
    passIndex.reserve(renderGraph.passes.size());

    for (size_t i = 0; i < renderGraph.passes.size(); ++i) {
        const auto& pass = renderGraph.passes[i];
        if (pass.id.empty()) {
            AddDiagnostic(result,
                          ProbeSeverity::Error,
                          "RG_PASS_ID_MISSING",
                          pass.jsonPath,
                          "Render pass is missing an id");
            continue;
        }
        const auto [it, inserted] = passIndex.emplace(pass.id, i);
        if (!inserted) {
            AddDiagnostic(result,
                          ProbeSeverity::Error,
                          "RG_PASS_ID_DUPLICATE",
                          pass.jsonPath,
                          "Duplicate render pass id: " + pass.id);
        }
    }

    std::vector<std::vector<size_t>> edges(renderGraph.passes.size());
    std::vector<size_t> indegree(renderGraph.passes.size(), 0);

    for (size_t i = 0; i < renderGraph.passes.size(); ++i) {
        const auto& pass = renderGraph.passes[i];
        for (const auto& input : pass.inputs) {
            if (input.ref.type != RenderResourceRefType::PassOutput) {
                continue;
            }
            if (input.ref.passId.empty()) {
                AddDiagnostic(result,
                              ProbeSeverity::Error,
                              "RG_INPUT_MISSING_PASS",
                              input.jsonPath,
                              "Render pass input is missing a pass reference");
                continue;
            }
            auto it = passIndex.find(input.ref.passId);
            if (it == passIndex.end()) {
                AddDiagnostic(result,
                              ProbeSeverity::Error,
                              "RG_INPUT_UNKNOWN_PASS",
                              input.jsonPath,
                              "Render pass input references unknown pass: " + input.ref.passId);
                continue;
            }
            if (it->second == i) {
                AddDiagnostic(result,
                              ProbeSeverity::Error,
                              "RG_INPUT_SELF_REFERENCE",
                              input.jsonPath,
                              "Render pass input references its own output");
                continue;
            }
            edges[it->second].push_back(i);
            indegree[i] += 1;
        }
    }

    std::queue<size_t> ready;
    for (size_t i = 0; i < indegree.size(); ++i) {
        if (indegree[i] == 0) {
            ready.push(i);
        }
    }

    while (!ready.empty()) {
        const size_t current = ready.front();
        ready.pop();
        result.passOrder.push_back(renderGraph.passes[current].id);
        for (size_t neighbor : edges[current]) {
            if (indegree[neighbor] > 0) {
                indegree[neighbor] -= 1;
            }
            if (indegree[neighbor] == 0) {
                ready.push(neighbor);
            }
        }
    }

    if (result.passOrder.size() != renderGraph.passes.size()) {
        std::string remaining;
        for (size_t i = 0; i < renderGraph.passes.size(); ++i) {
            if (indegree[i] > 0) {
                if (!remaining.empty()) {
                    remaining += ", ";
                }
                remaining += renderGraph.passes[i].id;
            }
        }
        AddDiagnostic(result,
                      ProbeSeverity::Error,
                      "RG_CYCLE_DETECTED",
                      "/render/passes",
                      "Render graph cycle detected",
                      "remainingPasses=" + remaining);
    }

    if (logger_) {
        logger_->Trace("RenderGraphService", "BuildGraph",
                       "passCount=" + std::to_string(renderGraph.passes.size()) +
                       ", scheduled=" + std::to_string(result.passOrder.size()));
    }

    return result;
}

void RenderGraphService::AddDiagnostic(RenderGraphBuildResult& result,
                                       ProbeSeverity severity,
                                       const std::string& code,
                                       const std::string& jsonPath,
                                       const std::string& message,
                                       const std::string& details) const {
    ProbeReport report;
    report.severity = severity;
    report.code = code;
    report.jsonPath = jsonPath;
    report.message = message;
    report.details = details;
    result.diagnostics.push_back(report);
    if (IsErrorSeverity(severity)) {
        result.success = false;
    }
    if (probeService_) {
        probeService_->Report(report);
    }
    if (logger_) {
        if (severity == ProbeSeverity::Warn) {
            logger_->Warn("RenderGraphService::BuildGraph: " + message + " (" + code + ")");
        } else if (IsErrorSeverity(severity)) {
            logger_->Error("RenderGraphService::BuildGraph: " + message + " (" + code + ")");
        } else {
            logger_->Info("RenderGraphService::BuildGraph: " + message + " (" + code + ")");
        }
    }
}

}  // namespace sdl3cpp::services::impl
