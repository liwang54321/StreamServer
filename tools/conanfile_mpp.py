from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout
from conan.tools.scm import Git
from conan.tools.files import copy
import os


class MppConan(ConanFile):
    name = "mpp"
    version = "0.1.0"
    license = "GPL"
    author = "Mpp"
    url = "https://github.com/rockchip-linux/mpp"
    description = "An high-performance, enterprise-level streaming media service framework based on C++11"
    topics = ("media", "service", "framework")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": True, "fPIC": True}
    generators = "CMakeDeps", "CMakeToolchain", "VirtualBuildEnv"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC
    
    def configure(self):
        pass

    def source(self):
        git = Git(self)
        # Cloning in current dir, not a children folder
        git.clone(url="https://github.com/rockchip-linux/mpp.git", target=".")
    
    def export_sources(self):
        copy(self, "CMakeLists.txt", src=self.recipe_folder, dst=self.export_sources_folder)

    def requirements(self):
        pass

    def build(self):
        cmake = CMake(self)
        cmake.configure(build_script_folder=self.export_sources_folder, 
                        cli_args=['-DHAVE_DRM=ON', '-DBUILD_TEST=OFF', '-DCMAKE_C_FLAGS="-Dlinux -DARMLINUX -fPIC"'])
        cmake.build(cli_args=['-v'])

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "mpp")
        self.cpp_info.filenames["cmake_find_package"] = "mpp"
        self.cpp_info.filenames["cmake_find_package_multi"] = "mpp"
        self.cpp_info.names["cmake_find_package"] = "mpp"
        self.cpp_info.names["cmake_find_package_multi"] = "mpp"

        self.cpp_info.components["mpp"].set_property("cmake_target_name", "mpp::mpp")
        self.cpp_info.components["mpp"].libs = ["rockchip_mpp", "rockchip_vpu"]
        self.cpp_info.components["mpp"].includedirs = ["include"]
        
        # self.cpp_info.set_property("cmake_file_name", "mpp")
        # self.cpp_info.filenames["cmake_find_package"] = "mpp"
        # self.cpp_info.filenames["cmake_find_package_multi"] = "mpp"
        # self.cpp_info.names["cmake_find_package"] = "mpp"
        # self.cpp_info.names["cmake_find_package_multi"] = "mpp"
        # self.cpp_info.components["mpp"].set_property("cmake_target_name", "mpp::mpp")

        # self.cpp_info.components["mpp"].includedirs = ["include"]
        # self.cpp_info.components["mpp"].libdirs = ["lib"]
        # self.cpp_info.components["mpp"].libs = ["rockchip_mpp", "rockchip_vpu"]

        # self.env_info.PATH.append(os.path.join(self.package_folder, "bin"))
        # self.cpp_info.libs = ["rockchip_mpp", "rockchip_vpu"]
        # self.cpp_info.libdirs = ["lib"]
        # self.cpp_info.includedirs  = ["include"]


