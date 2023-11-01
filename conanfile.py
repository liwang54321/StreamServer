from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.env import VirtualRunEnv

class DeviceDevelopConan(ConanFile):
    name = "DeviceDevelop"
    version = "0.1.0"
    license = "GPL"
    author = "lw liwang54321@gmail.com"
    url = "www.github.com/device_develop"
    description = "device develop control soft"
    topics = ("jetson", "device")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": True, "fPIC": True}
    # generators = "CMakeDeps", "CMakeToolchain", "VirtualBuildEnv"

    def generate(self):
        ms = VirtualRunEnv(self)
        ms.generate()
        tc = CMakeToolchain(self)
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def layout(self):
        pass
        # cmake_layout(self)

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC
    
    def configure(self):
        # For ffmpeg
        self.options['ffmpeg'].with_vaapi = False
        self.options['ffmpeg'].with_vdpau = False
        self.options['ffmpeg'].with_xcb = False
        self.options['ffmpeg'].with_pulse = False
        self.options['ffmpeg'].with_vulkan = False
        self.options['ffmpeg'].with_asm = False
        self.options['ffmpeg'].with_libalsa = False
        self.options['ffmpeg'].with_libfdk_aac = False
        self.options['ffmpeg'].with_libmp3lame = False
        self.options['ffmpeg'].with_opus = False
        self.options['ffmpeg'].with_vorbis = False
        self.options['ffmpeg'].postproc = False
        self.options['ffmpeg'].with_openjpeg = False
        self.options['ffmpeg'].with_openh264 = False
        self.options['ffmpeg'].with_libiconv = False
        self.options['ffmpeg'].with_zeromq = False
        self.options['ffmpeg'].with_libvpx = False
        self.options['ffmpeg'].with_zlib = False
        self.options['ffmpeg'].with_bzip2 = False
        self.options['ffmpeg'].with_lzma = False
        self.options['ffmpeg'].with_freetype = False
        self.options['ffmpeg'].with_libwebp = False

        # macos options
        # self.options['ffmpeg'].with_appkit = False
        # self.options['ffmpeg'].with_videotoolbox = False
        # self.options['ffmpeg'].with_avfoundation = False
        # self.options['ffmpeg'].with_coreimage = False
        # self.options['ffmpeg'].with_audiotoolbox = False

        # For opencv
        # self.options['opencv'].neon = False
        self.options['opencv'].with_ffmpeg = False
        self.options['opencv'].with_gtk = False
        self.options['opencv'].with_webp = False
        self.options['opencv'].gapi = False
        self.options['opencv'].objdetect = False
        self.options['opencv'].dnn = False
        self.options['opencv'].freetype = False
        self.options['opencv'].wechat_qrcode = False
        self.options['opencv'].with_openexr = False
        self.options['opencv'].with_tiff = False
        # self.options['opencv'].calib3d = False

        # For spdlog
        self.options["spdlog"].header_only = True
        # for spdlog windows
        # self.options["spdlog"].wchar_support = True
        # self.options["spdlog"].wchar_filenames = True
        # self.options["spdlog"].no_exceptions = True

    def source(self):
        pass

    def requirements(self):
        self.requires("ffmpeg/6.0")
        self.requires("opencv/4.5.5")
        self.requires("magic_enum/0.8.1")
        self.requires("spdlog/1.11.0")
        self.requires("zlmediakit/0.1.0")
        self.requires("mpp/0.1.0")
        self.requires("readerwriterqueue/1.0.6")

        # self.requires("gstreamer/1.19.2")
        # self.requires("boost/1.80.0")

        # override
        # self.requires("zlib/1.2.13")
        # self.requires("freetype/2.13.0")
        # self.requires("libpng/1.6.40")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        # self.env_info.PATH.append(os.path.join(self.package_folder, "bin"))
        self.buildenv_info.append_path("PATH", os.path.join(self.package_folder, 'bin'))
        self.runenv_info.append_path("PATH", os.path.join(self.package_folder, 'bin'))

