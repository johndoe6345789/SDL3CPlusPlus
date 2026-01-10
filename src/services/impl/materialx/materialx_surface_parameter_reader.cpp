#include "materialx_surface_parameter_reader.hpp"

namespace sdl3cpp::services::impl {

bool MaterialXSurfaceParameterReader::TryReadColor3(const MaterialX::NodePtr& node,
                                                    const char* name,
                                                    std::array<float, 3>& outColor) const {
    if (!node) {
        return false;
    }
    MaterialX::InputPtr input = node->getInput(name);
    if (!input || !input->hasValueString()) {
        return false;
    }
    MaterialX::ValuePtr value = input->getValue();
    if (!value || !value->isA<MaterialX::Color3>()) {
        return false;
    }
    const MaterialX::Color3& color = value->asA<MaterialX::Color3>();
    outColor = {color[0], color[1], color[2]};
    return true;
}

bool MaterialXSurfaceParameterReader::TryReadFloat(const MaterialX::NodePtr& node,
                                                   const char* name,
                                                   float& outValue) const {
    if (!node) {
        return false;
    }
    MaterialX::InputPtr input = node->getInput(name);
    if (!input || !input->hasValueString()) {
        return false;
    }
    MaterialX::ValuePtr value = input->getValue();
    if (!value || !value->isA<float>()) {
        return false;
    }
    outValue = value->asA<float>();
    return true;
}

MaterialXSurfaceParameters MaterialXSurfaceParameterReader::ReadStandardSurfaceParameters(
    const MaterialX::NodePtr& node) const {
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

}  // namespace sdl3cpp::services::impl
