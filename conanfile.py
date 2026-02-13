from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps


class MyProjectConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    def requirements(self):
        self.requires("spdlog/1.17.0")
        self.requires("gtest/1.17.0")
        self.requires("boost/1.78.0")
        self.requires("sqlitecpp/3.3.3")
        self.requires("nlohmann_json/3.12.0")

    def layout(self):
        import os
        self.folders.build = os.path.join(os.path.dirname(__file__), "build")

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        tc = CMakeToolchain(self, generator="Ninja Multi-Config")
        tc.generate()
