from conan import ConanFile
from conan.tools.microsoft import is_msvc


class PoDoFo(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    def requirements(self):
        self.requires("freetype/2.13.2")
        self.requires("zlib/1.3.1")
        self.requires("libxml2/2.12.5")
        self.requires("openssl/3.6.0")

        if self.settings.os != "Windows":
            self.requires("fontconfig/2.15.0")

        # Optional dependencies for clients
        self.requires("libidn/1.36")
        self.requires("libtiff/4.6.0")
        self.requires("libpng/1.6.53")
        self.requires("libjpeg/9e")

        if not is_msvc(self):
            self.requires("libunistring/0.9.10")

        # Alternative JPEG libraries
        # self.requires("libjpeg-turbo/3.0.2")
        # self.requires("mozjpeg/4.1.5")

        # 3rd-party dependencies that shouldn't be part of PoDoFo source-tree
        # Enable again when PoDoFo doesn't ship these libraries itself
        # self.requires("fmt/11.0.2")
        # self.requires("date/3.0.4")
        # self.requires("fast_float/6.1.0")
        # self.requires("tcb-span/cci.20220616", transitive_headers=True)
        # self.requires("utfcpp/4.0.6")
        # self.requires("utf8proc/2.9.0")

    def configure(self):
        self.options["freetype"].shared = False
        self.options["zlib"].shared = False
        self.options["libxml2"].shared = False
        self.options["openssl"].shared = False
        self.options["fontconfig"].shared = False
        self.options["libidn"].shared = False
        self.options["libtiff"].shared = False
        self.options["libpng"].shared = False
        self.options["libjpeg"].shared = False
        self.options["libunistring"].shared = False

        # self.options["fmt"].header_only = True
