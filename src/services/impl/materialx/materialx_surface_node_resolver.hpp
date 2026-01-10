#pragma once

#include <MaterialXCore/Document.h>

#include <string>

namespace sdl3cpp::services::impl {

class MaterialXSurfaceNodeResolver {
public:
    MaterialX::NodePtr ResolveSurfaceNode(const MaterialX::DocumentPtr& document,
                                          const std::string& materialName) const;
};

}  // namespace sdl3cpp::services::impl
