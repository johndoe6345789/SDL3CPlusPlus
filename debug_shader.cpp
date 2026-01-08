#include <iostream>
#include <fstream>
#include "src/services/impl/materialx_shader_generator.hpp"

int main() {
    sdl3cpp::services::impl::MaterialXShaderGenerator generator(nullptr);
    
    sdl3cpp::services::impl::MaterialXConfig config;
    config.enabled = true;
    config.shaderKey = "materialx";
    config.useConstantColor = true;
    config.constantColor = {0.8f, 0.2f, 0.2f};
    config.libraryPath = "MaterialX/libraries";
    
    try {
        auto paths = generator.Generate(config, std::filesystem::path("./scripts"));
        
        std::cout << "===== VERTEX SHADER =====\n";
        std::cout << paths.vertexSource << std::endl;
        std::cout << "\n===== FRAGMENT SHADER =====\n";
        std::cout << paths.fragmentSource << std::endl;
        
        // Save to files
        std::ofstream vfile("debug_vertex.glsl");
        vfile << paths.vertexSource;
        vfile.close();
        
        std::ofstream ffile("debug_fragment.glsl");
        ffile << paths.fragmentSource;
        ffile.close();
        
        std::cout << "\nShaders saved to debug_vertex.glsl and debug_fragment.glsl\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
