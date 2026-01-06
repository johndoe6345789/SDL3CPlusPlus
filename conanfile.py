from conan import ConanFile
from conan.tools.cmake import cmake_layout

class SDL3CppConan(ConanFile):
    name = "sdl3cpp"
    version = "0.1"
    settings = "os", "arch", "compiler", "build_type"
    options = {"build_app": [True, False]}
    default_options = {
        "build_app": True,
        "lua/*:shared": False,
        "lua/*:fPIC": True,
        "lua/*:compile_as_cpp": False,
        "lua/*:with_tools": False,
    }
    generators = "CMakeDeps", "VirtualRunEnv"

    def configure(self):
        self.requires("wayland/1.23.92", override=True)

    def layout(self):
        cmake_layout(self)

    def generate(self):
        from conan.tools.cmake import CMakeToolchain
        tc = CMakeToolchain(self)
        import os
        vitasdk = os.environ.get("VITASDK")
        if vitasdk:
            tc.toolchain_file = f"{vitasdk}/share/vita.toolchain.cmake"
            self.output.trace(f"Using VITASDK toolchain file: {tc.toolchain_file}")
        else:
            self.output.trace("Using default CMake toolchain file.")
        tc.generate()

    def requirements(self):
        self.requires("lua/5.4.8")
        self.requires("sdl/3.2.20")
        self.requires("vulkan-loader/1.4.313.0")
        self.requires("vulkan-headers/1.4.313.0")
        self.requires("vulkan-memory-allocator/3.3.0")
        self.requires("spirv-tools/1.4.313.0")
        self.requires("spirv-headers/1.4.313.0")
        self.requires("shaderc/2025.3")
        self.requires("cpptrace/1.0.4")
        self.requires("ogg/1.3.5")
        self.requires("theora/1.1.1")
        self.requires("cli11/2.6.0")
        self.requires("bullet3/3.25")
        self.requires("box2d/3.1.1")
        self.requires("assimp/6.0.2")
        self.requires("glm/1.0.1")
        self.requires("vorbis/1.3.7")
        self.requires("rapidjson/cci.20230929")
        self.requires("bgfx/1.129.8930-495")
        self.requires("entt/3.16.0")
