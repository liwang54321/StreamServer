from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout
from conan.tools.scm import Git
from conan.tools.files import copy
import os


class ZLMediaKitConan(ConanFile):
    name = "zlmediakit"
    version = "0.1.0"
    license = "GPL"
    author = "ZLMediaKit"
    url = "https://github.com/ZLMediaKit/ZLMediaKit"
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
        git.clone(url="https://github.com/ZLMediaKit/ZLMediaKit.git", target=".")
        # git.checkout(commit="<commit>")
        self.run("git submodule update --init --recursive")
    
    def export_sources(self):
        copy(self, "CMakeLists.txt", src=self.recipe_folder, dst=self.export_sources_folder)

    def requirements(self):
        self.requires('openssl/1.1.1s')
        self.requires('libsrtp/2.4.2')
        self.requires('usrsctp/0.9.5.0')

    def build(self):
        cmake = CMake(self)
        cmake.configure(build_script_folder=self.export_sources_folder)
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "zlmediakit")
        self.cpp_info.filenames["cmake_find_package"] = "zlmediakit"
        self.cpp_info.filenames["cmake_find_package_multi"] = "zlmediakit"

        self.cpp_info.components["zlmediakit"].set_property("cmake_target_name", "zlmediakit::zlmediakit")
        self.cpp_info.components["zlmediakit"].libs = ["mk_api"]
        self.cpp_info.components["zlmediakit"].includedirs = ["include"]
        self.cpp_info.components["zlmediakit"].requires = ["libsrtp::libsrtp", "openssl::openssl", "usrsctp::usrsctp"]
