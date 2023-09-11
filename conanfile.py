from conans import ConanFile, CMake, tools
import os

class DeviceDevelopConan(ConanFile):
    name = "device_develop"
    version = "0.1.0"
    license = "GPL"
    author = "lw liwang54321@gmail.com"
    url = "www.github.com/device_develop"
    description = "device develop control soft"
    topics = ("jetson", "device")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "cmake", "cmake_find_package"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def source(self):
        pass

    def requirements(self):
        self.requires.add("ffmpeg/5.0")
        self.options['ffmpeg'].with_vaapi = False
        self.options['ffmpeg'].with_vdpau = False
        self.options['ffmpeg'].with_xcb = False
        self.options['ffmpeg'].with_pulse = False
        self.options['ffmpeg'].with_vulkan = False
        self.options['ffmpeg'].with_asm = False
        self.options['ffmpeg'].with_programs = False
        self.options['ffmpeg'].with_libalsa = False
        self.options['ffmpeg'].with_libfdk_aac = False
        self.options['ffmpeg'].with_libmp3lame = False

        # self.options['ffmpeg'].with_appkit = False
        # self.options['ffmpeg'].with_videotoolbox = False
        # self.options['ffmpeg'].with_avfoundation = False
        # self.options['ffmpeg'].with_coreimage = False
        # self.options['ffmpeg'].with_audiotoolbox = False

        self.options['ffmpeg'].with_opus = False
        self.options['ffmpeg'].with_pulse = False
        self.options['ffmpeg'].with_vorbis = False
        self.options['ffmpeg'].with_xcb = False
        self.options['ffmpeg'].postproc = False
        self.options['ffmpeg'].with_openjpeg = False

        self.requires.add("opencv/4.5.5")
        self.options['opencv'].contrib = True
        self.options['opencv'].contrib_freetype = True
        self.options['opencv'].neon = False
        self.options['opencv'].with_ffmpeg = False
        self.options['opencv'].with_gtk = False
        self.options['opencv'].with_webp = False
        self.options['opencv'].with_gtk = False

        # self.requires.add("gstreamer/1.19.2")
        self.requires.add("magic_enum/0.8.1")
        self.requires.add("spdlog/1.11.0")
        self.options["spdlog"].header_only = True
        self.options["spdlog"].wchar_support = True
        self.options["spdlog"].wchar_filenames = True
        self.options["spdlog"].no_exceptions = True

        self.requires.add("readerwriterqueue/1.0.6")
        self.requires.add("boost/1.80.0")

        # override
        self.requires.add("zlib/1.2.13")
        self.requires.add("freetype/2.12.1")
        self.requires.add("libpng/1.6.38")


    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder="hello")
        cmake.build()

    def package(self):
        self.copy("*.h", dst="include", src="hello")
        self.copy("*hello.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.env_info.PATH.append(os.path.join(self.package_folder, "bin"))

