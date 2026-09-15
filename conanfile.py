from conan import ConanFile
from conan.tools.cmake import cmake_layout

class SDL3CppConan(ConanFile):
    name = "sdl3cpp"
    version = "0.1"
    settings = "os", "arch", "compiler", "build_type"
    options = {"build_app": [True, False]}
    default_options = {
        "build_app": True,
    }
    generators = "CMakeDeps", "VirtualRunEnv"
    BASE_REQUIRES = (
        "sdl/3.2.20",
        "cpptrace/1.0.4",
        "ogg/1.3.5",
        "theora/1.1.1",
        "cli11/2.6.0",
        "bullet3/3.25",
        "box2d/3.1.1",
        "assimp/6.0.2",
        "glm/1.0.1",
        "vorbis/1.3.7",
        "rapidjson/cci.20230929",
        "nlohmann_json/3.11.3",
        "lunasvg/3.0.1",
        "freetype/2.13.2",
        "ffmpeg/8.0.1",
        "cairo/1.18.0",
        "libzip/1.10.1",
        "stb/cci.20230920",
        "gtest/1.17.0",
        "spdlog/1.15.1"
    )
    RENDER_STACK_REQUIRES = (
        "entt/3.16.0",
    )

    def configure(self):
        # Only require Linux audio/display dependencies on Linux
        if self.settings.os == "Linux":
            self.requires("wayland/1.23.92", override=True)
            self.requires("libalsa/1.2.10", override=True)
            self.requires("pulseaudio/17.0", override=True)
        # Disable 3mf exporter which uses kuba-zip to avoid duplicate symbols with libzip
        self.options["assimp"].with_3mf_exporter = False

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
        self._add_requirements(self.BASE_REQUIRES)
        self._add_requirements(self.RENDER_STACK_REQUIRES)

    def _add_requirements(self, requirements):
        for requirement in requirements:
            self.requires(requirement)
