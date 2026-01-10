#include "materialx_surface_node_resolver.hpp"

namespace sdl3cpp::services::impl {

MaterialX::NodePtr MaterialXSurfaceNodeResolver::ResolveSurfaceNode(
    const MaterialX::DocumentPtr& document,
    const std::string& materialName) const {
    if (!materialName.empty()) {
        MaterialX::NodePtr candidate = document->getNode(materialName);
        if (candidate && candidate->getCategory() == "surfacematerial") {
            MaterialX::NodePtr surfaceNode = candidate->getConnectedNode("surfaceshader");
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
            MaterialX::NodePtr surfaceNode = node->getConnectedNode("surfaceshader");
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

}  // namespace sdl3cpp::services::impl
