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

    options = {
        "workspaceBuild" : [True,False],
    }

    default_options = {
        "workspaceBuild" : False,
        "ubitrack_core:shared":True,
        "ubitrack_vision:shared":True,
        "ubitrack_dataflow:shared":True,
        "assimp:shared":True,
        }

    # all sources are deployed with the package
    exports_sources = "doc/*", "src/*", "CMakeLists.txt"

    def requirements(self):
        userChannel = "ubitrack/stable"
        if self.options.workspaceBuild:
            userChannel = "local/dev"

        self.requires("assimp/[>=4.1.0]@camposs/stable")
        self.requires("ubitrack_core/%s@%s" % (self.version, userChannel))
        self.requires("ubitrack_vision/%s@%s" % (self.version, userChannel))
        self.requires("ubitrack_dataflow/%s@%s" % (self.version, userChannel))
        self.requires("ubitrack_visualization/%s@%s" % (self.version, userChannel))
        self.requires("ubitrack_component_core/%s@%s" % (self.version, userChannel))

    # def imports(self):
    #     self.copy(pattern="*.dll", dst="bin", src="bin") # From bin to bin
    #     self.copy(pattern="*.dylib*", dst="lib", src="lib") 
    #     self.copy(pattern="*.so*", dst="lib", src="lib") 
       
    def build(self):
        cmake = CMake(self)
        cmake.definitions['WITH_OPENCL'] = self.options['ubitrack_vision'].with_opencl
        cmake.configure()
        cmake.build()
        cmake.install()

    def package(self):
        pass

    def package_info(self):
        pass


    def package_id(self):
        self.info.requires["ubitrack_vision"].full_package_mode()
        self.info.requires["ubitrack_visualization"].full_package_mode()
