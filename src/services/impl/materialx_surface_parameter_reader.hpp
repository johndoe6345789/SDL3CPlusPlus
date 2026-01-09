#pragma once

#include "materialx_surface_parameters.hpp"

#include <MaterialXCore/Document.h>
#include <MaterialXCore/Types.h>

namespace sdl3cpp::services::impl {

class MaterialXSurfaceParameterReader {
public:
    MaterialXSurfaceParameters ReadStandardSurfaceParameters(const MaterialX::NodePtr& node) const;

private:
    bool TryReadColor3(const MaterialX::NodePtr& node, const char* name,
                       std::array<float, 3>& outColor) const;
    bool TryReadFloat(const MaterialX::NodePtr& node, const char* name, float& outValue) const;
};

}  // namespace sdl3cpp::services::impl
