from conans import ConanFile, CMake, tools


class UbitrackCoreConan(ConanFile):
    name = "ubitrack_component_visualization"
    version = "1.3.0"

    description = "Ubitrack Visualization Components"
    url = "https://github.com/Ubitrack/component_visualization.git"
    license = "GPL"

    short_paths = True
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"

    requires = (
        "assimp/[>=4.1.0]@camposs/stable",

        "ubitrack_core/%s@ubitrack/stable" % version,
        "ubitrack_vision/%s@ubitrack/stable" % version,
        "ubitrack_dataflow/%s@ubitrack/stable" % version,
        "ubitrack_visualization/%s@ubitrack/stable" % version,
        "ubitrack_component_core/%s@ubitrack/stable" % version,
       )

    default_options = (
        "ubitrack_core:shared=True",
        "ubitrack_vision:shared=True",
        "ubitrack_dataflow:shared=True",
        "assimp:shared=True",
        )

    # all sources are deployed with the package
    exports_sources = "doc/*", "src/*", "CMakeLists.txt"

    def imports(self):
        self.copy(pattern="*.dll", dst="bin", src="bin") # From bin to bin
        self.copy(pattern="*.dylib*", dst="lib", src="lib") 
        self.copy(pattern="*.so*", dst="lib", src="lib") 
       
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        cmake.install()

    def package(self):
        pass

    def package_info(self):
        pass


    def package_id(self):
        self.requires["ubitrack_vision"].full_package_mode()
        self.requires["ubitrack_visualization"].full_package_mode()
