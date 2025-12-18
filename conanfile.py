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
        self.requires("sdl3/3.2.0")
        if self.options.build_app:
            self.requires("vulkan-loader/1.4.313.0")
