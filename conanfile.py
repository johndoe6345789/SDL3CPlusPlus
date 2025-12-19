from conan import ConanFile

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
    generators = "CMakeDeps", "CMakeToolchain", "VirtualRunEnv"

    def requirements(self):
        self.requires("lua/5.4.6")
        self.requires("sdl/3.2.20")
        self.requires("vulkan-loader/1.3.243.0")
        self.requires("vulkan-headers/1.3.243.0")
        self.requires("vulkan-memory-allocator/3.3.0")
        self.requires("vulkan-validationlayers/1.3.243.0")
        self.requires("ogg/1.3.5")
        self.requires("theora/1.1.1")
        self.requires("rapidjson/1.1.0")
        self.requires("cli11/2.6.0")
        self.requires("bullet3/3.25")
        self.requires("box2d/3.1.1")
