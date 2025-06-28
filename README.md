# PoDoFo [![build-linux](https://github.com/podofo/podofo/actions/workflows/build-linux.yml/badge.svg)](https://github.com/podofo/podofo/actions/workflows/build-linux.yml) [![build-mac](https://github.com/podofo/podofo/actions/workflows/build-mac.yml/badge.svg)](https://github.com/podofo/podofo/actions/workflows/build-mac.yml) [![build-win](https://github.com/podofo/podofo/actions/workflows/build-win.yml/badge.svg)](https://github.com/podofo/podofo/actions/workflows/build-win.yml)

1.  [What is PoDoFo?](#what-is-podofo)
2.  [Features](#features)
3.  [Requirements](#requirements)
4.  [Licensing](#licensing)
5.  [Development quickstart](#development-quickstart)
6.  [Doxygen Documentation](#doxygen-documentation)
7.  [Software life cycle and API stability](#software-life-cycle-and-api-stability)
8.  [String encoding and buffer conventions](#string-encoding-and-buffer-conventions)
9.  [PoDoFo tools](#podofo-tools)
10.  [TODO](#todo)
11.  [FAQ](#faq)
12.  [No warranty](#no-warranty)
13.  [Contributions](#contributions)
14.  [Authors](#authors)

## What is PoDoFo?

PoDoFo is a free portable C++ library to work with the PDF file format.

PoDoFo provides classes to parse a PDF file and modify its content, allowing to write
it back to disk easily. Besides PDF parsing and manipulation PoDoFo also provides
facilities to create your own PDF files from scratch.

## Features

PoDoFo has a modern and user-friendly C++17 API that features:

- PDF parsing with high-level entity inspection (annotations, form fields and others)
- PDF writing with support for incremental updates
- PDF signing with [PAdES-B](https://en.wikipedia.org/wiki/PAdES) compliance,
  RSA/ECDSA encryption and asynchronous/deferred signing
- Text drawing with automatic CID encoding generation and font subsetting
- Full-featured low-level Unicode text extraction
- Advanced CJK language support (text extraction and automatic multi-byte encoding)
- PDF/A compliance preservation (e.g., font embedding, simultaneous PDF/A and PDF/UA compliance)
- PDF/UA compliance preservation (e.g., when adding annotations/form fields)
- Deferred font file data embedding

PoDoFo  does not support rendering PDF content yet. Text writing
is also limited as it currently does not perform proper text shaping/kerning.

## Requirements

To build PoDoFo lib you'll need a c++17 compiler,
CMake 3.16 and the following libraries (tentative minimum versions indicated):

* freetype2 (2.11)
* fontconfig (2.13.94, required for Unix platforms, optional for Windows)
* OpenSSL (1.1 and 3.0 are supported)
* LibXml2 (2.9.12)
* zlib
* libjpeg (9d, optional)
* libtiff (4.0.10, optional)
* libpng (1.6.37, optional)

For the most popular toolchains, PoDoFo requires the following
minimum versions:

* msvc++ 14.16 (VS 2017 15.9)
* gcc 9.0
* clang/llvm 7.0

It is regularly tested with the following IDE/toolchains versions:

* Visual Studio 2017 15.9
* Visual Studio 2019 16.11
* Visual Studio 2022 17.3
* gcc 9.3.1
* XCode 13.3
* NDK r23b

GCC 8.1 support [broke](https://github.com/podofo/podofo/issues/116), but it could be reinstanced.

## Licensing

PoDoFo library is licensed under the [LGPL 2.0](https://spdx.org/licenses/LGPL-2.0-or-later.html) or later terms.
PoDoFo tools are licensed under the [GPL 2.0](https://spdx.org/licenses/GPL-2.0-or-later.html) or later terms.

## Development quickstart

PoDoFo is known to compile through a multitude of package managers (including [APT](https://en.wikipedia.org/wiki/APT_(software)), [brew](https://brew.sh/), [vcpkg](https://vcpkg.io/), [Conan](https://conan.io/)), and has public continuous integration working in [Ubuntu Linux](https://github.com/podofo/podofo/blob/master/.github/workflows/build-linux.yml), [MacOS](https://github.com/podofo/podofo/blob/master/.github/workflows/build-linux.yml) and
[Windows](https://github.com/podofo/podofo/blob/master/.github/workflows/build-win.yml), bootstrapping the CMake project, building and testing the library. It's highly recommended to build PoDoFo using such package managers. 

There's also a playground area in the repository where you can have
access to pre-build dependencies for some popular architectures/operating systems:
the playground is the recommended setting to develop the library and reproduce bugs,
while it's not recommended for the deployment of your application using PoDoFo.
Have a look to the [Readme](https://github.com/podofo/podofo/tree/master/playground) there.

> **Warning**: PoDoFo is known to be working in cross-compilation toolchains (eg. Android/iOS development), but support may not provided in such scenarios. If you decide to manually build dependencies you are assumed to know how to identity possible library clashes/mismatches and how to deal with compilation/linking problems that can arise in your system.

### Build with APT

From the source root run:

```
sudo apt install -y libfontconfig1-dev libfreetype-dev libxml2-dev libssl-dev libjpeg-dev libpng-dev libtiff-dev
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . --config Debug
```

### Build with brew

Install [brew](https://brew.sh/), then from the source root run:

```
brew install fontconfig freetype openssl libxml2 jpeg-turbo libpng libtiff cmake
mkdir build
cd build
cmake  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_FIND_FRAMEWORK=NEVER -DCMAKE_PREFIX_PATH=`brew --prefix` -DFontconfig_INCLUDE_DIR=`brew --prefix fontconfig`/include -DOPENSSL_ROOT_DIR=`brew --prefix openssl@3` ..
cmake --build . --config Debug
```

### Build with Conan

Install [conan](https://docs.conan.io/1/installation.html), then from source root run:

```
mkdir build
cd build
conan install ..
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . --config Debug
```

### Build with vcpkg

Follow the vcpkg [quickstart](https://vcpkg.io/en/getting-started.html) guide to setup the package manager repository first.
In Windows, it may be also useful to set the environment variable `VCPKG_DEFAULT_TRIPLET` to `x64-windows` to default installing 64 bit dependencies
and define a `VCPKG_INSTALLATION_ROOT` variable with the location of the repository as created in the quickstart.

Then from source root run:

```
vcpkg install fontconfig freetype libxml2 openssl libjpeg-turbo libpng tiff zlib
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . --config Debug
```

### Consume PoDoFo from package managers with CMake

Starting with version 1.0, PoDoFo has a quite advanced CMake integration and can be consumed
as a [CMake package](https://cmake.org/cmake/help/latest/manual/cmake-packages.7.html).
As soon as it's correctly integrated in your favorite package manager (APT, vcpkg, Conan,
...), you will be able to locate PoDoFo and compile your application with the following
`CMakeLists.txt`:

```
cmake_minimum_required(VERSION 3.23)

project(PoDoFoSample)

# PoDoFo public header requires at least C++17,
# but it can be used with higher standards
set(CMAKE_CXX_STANDARD 23)

# If you are not using a package manager and/or you
# are installing PoDoFo to a non-standard path
#list(APPEND CMAKE_PREFIX_PATH "/path/to/podofo_install_dir")

find_package(PoDoFo 1.0.0 REQUIRED)
message(STATUS "Found PoDoFo: ${PoDoFo_VERSION}")

add_executable(helloworld main.cpp)
target_link_libraries(helloworld podofo::podofo)
```

### CMake switches

- `PODOFO_BUILD_TEST`: Build the unit tests, defaults to TRUE;

- `PODOFO_BUILD_EXAMPLES`: Build the examples, defaults to TRUE;

- `PODOFO_BUILD_UNSUPPORTED_TOOLS`: Build the PoDoFo tools, defaults to FALSE. See
the relevant [section](https://github.com/podofo/podofo/#podofo-tools) in the Readme;

- `PODOFO_BUILD_LIB_ONLY`: If TRUE, it will build only the library component.
This unconditionally disable building tests, examples and tools;

- `PODOFO_BUILD_STATIC`: If TRUE, build the library as a static object and use it in tests,
examples and tools. By default a shared library is built.

### Static linking

If you want to use a static build of PoDoFo and you are including the PoDoFo cmake project it's very simple. Do something like the following in your CMake project:

```
set(PODOFO_BUILD_LIB_ONLY TRUE CACHE BOOL "" FORCE)
set(PODOFO_BUILD_STATIC TRUE CACHE BOOL "" FORCE)
add_subdirectory(podofo)
# ...
target_link_libraries(MyTarget podofo::podofo)
```

If you are linking against a precompiled static build of PoDoFo this is a scenario where the support is limited, as you are really supposed to be able to identify and fix linking errors. The general steps are:
* Add `PODOFO_STATIC` compilation definition to your project, or before including `podofo.h`;
* Link the libraries `podofo.a`, `podofo_private.a`, `podofo_3rdparty.a` (`podofo.lib`, `podofo_private.lib`, `podofo_3rdparty.lib` in MSVC) and all the [dependent](https://github.com/podofo/podofo/blob/5a07b90f24747a5aafe6f6fd062ee81f4783ab22/CMakeLists.txt#L203C5-L203C24) libraries.

## Doxygen Documentation

The API documentation can be found at https://podofo.github.io/podofo/documentation/ .

### Generate the doxygen documentation

1. **Prerequisite**: Ensure you have Doxygen installed on your machine. If not, visit [Doxygen's official website](http://www.doxygen.nl/) to download and install it.

2. **Generating Documentation**: After completing the build process detailed in the [Development quickstart](#development-quickstart) chapter, navigate to the root directory of PoDoFo's source code.
Open a terminal or command prompt and run the following command:
    ```bash
    doxygen build/Doxyfile
    ```

3. **Viewing the Documentation**: Once the documentation generation completes, you'll find a `documentation` directory that contains the generated documentation. Open `index.html` in your favorite web browser to view the API documentation.
    ```bash
    cd build/doxygen/documentation
    open index.html
    ```
## Software life cycle and API stability

Refer to the main article in the [Wiki](https://github.com/podofo/podofo/wiki/PoDoFo-releases-life-cycle-and-API-stability). 
At this [page](https://github.com/podofo/podofo/blob/master/API-MIGRATION.md) you can find an incomplete guide on migrating 0.9.8 code to 0.10.x, and from 0.10.x to 1.0.

## String encoding and buffer conventions

All `std::strings` or `std::string_view` in the library are intended
to hold UTF-8 encoded string content. `PdfString` and `PdfName` constructors
accept UTF-8 encoded strings by default (`PdfName` accept only characters in the
`PdfDocEncoding` char set, though). `charbuff` abd `bufferview`
instead represent a generic octet buffer.

## PoDoFo Tools

> **Warning**: Tools are currently **unsupported**, **untested** and **unmaintained**.

PoDoFo tools are still available in the source [tree](https://github.com/podofo/podofo/)
but their compilation is disabled by default because they are unsted/unmaintained,
and will not receive support until their status is cleared. It's not recommended to include them in software distributions.
If you want to build them make sure to bootstrap the CMake project with ```-DPODOFO_BUILD_UNSUPPORTED_TOOLS=TRUE```.
Tools are conveniently enabled in the [playground](https://github.com/podofo/podofo/tree/master/playground)
at least to ensure library changes won't break their compilation.

## TODO

There's a [TODO](https://github.com/podofo/podofo/blob/master/TODO.md) list, or look
at the [issue](https://github.com/podofo/podofo/issues) tracker.

## FAQ

**Q: PoDoFo compilation requires a CMake version higher than what is present in my system, can you lower the requirement?<a id='faq-cmake'></a>**

**A:** No, CMake 2.23 introduced a functionality that makes it very easy to create [CMake packages](https://cmake.org/cmake/help/latest/manual/cmake-packages.7.html) that
is too conventient to ignore. In Windows it's easy to upgrade the CMake version to latest, while in MacOs it's the same thanks to the official KitWare installer or `brew`.
In a linux system it's also quite easy to install an upgraded parallel CMake installation. Just run the following script:

```
export CMAKE_VERSION=3.31.7 \
    && wget -nv https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Linux-x86_64.sh \
    && chmod +x cmake-${CMAKE_VERSION}-Linux-x86_64.sh \
    && ./cmake-${CMAKE_VERSION}-Linux-x86_64.sh --prefix=/usr/local --skip-license \
    && rm cmake-${CMAKE_VERSION}-Linux-x86_64.sh \
    && cmake --version
```

**Q: How do I sign a document?<a id='faq-sign'></a>**

**A:** PoDoFo HEAD now supplies a high level signing procedure which is very powerful
and that allows to sign a document without having to supply a *CMS* structure manually.
By default, it supports signing a document with the modern [`PAdES-B`](https://en.wikipedia.org/wiki/PAdES)
compliance profiles, but there's also a support for the legacy PKCS7 signatures.
Providing you have both ASN.1 encoded X509 certificate and RSA private key, you
can sign a document with the following code:

```cpp
auto inputOutput = std::make_shared<FileStreamDevice>(filepath, FileMode::Open);

PdfMemDocument doc;
doc.Load(inputOutput);

auto& page = doc.GetPages().GetPageAt(0);
auto& signature = page.CreateField<PdfSignature>("Signature", Rect());

auto signer = PdfSignerCms(x509certbuffer, pkeybuffer);
PoDoFo::SignDocument(doc, *inputOutput, signer, signature);
```

There's also a support for external signing services and/or signing the document
in memory buffers. See the various signing examples in the unit [tests](https://github.com/podofo/podofo/blob/master/test/unit/SignatureTest.cpp).

**Q: Can I still use an event based procedure to sign the document?<a id='faq-sign-event-based'></a>**

**A:** Yes, the old low level procedure hasn't changed and it's still available.
To describe the procedure briefly, one has to fully Implement a `PdfSigner`,
retrieve or create a `PdfSignature` field, create an output device (see next question)
and use `PoDoFo::SignDocument(doc, device, signer, signature)`. When signing,
the sequence of calls of `PdfSignature` works in this way: method `PdfSigner::Reset()`
is called first, then  the `PdfSigner::ComputeSignature(buffer, dryrun)` is called with
an empty buffer and the `dryrun` argument set to `true`. In this call one can just
resize the buffer overestimating the required size for the signature, or just
compute a fake signature that must be saved on the buffer. Then a sequence of
`PdfSigner::AppendData(buffer)` are called, receiving all the document data to
be signed. A final `PdfSigner::ComputeSignature(buffer, dryrun)` is called, with
the `dryrun` parameter set to `false`. The buffer on this call is cleared (capacity
is not altered) or not accordingly to the value of `PdfSigner::SkipBufferClear()`.


**Q: `PdfMemDocument::SaveUpdate()` or `PoDoFo::SignDocument()` write only a
partial file: why there's no mechanism to seamlessly handle the incremental
update as it was in PoDoFo 0.9.x? What should be done to correctly update/sign
the document?<a id='faq-manual-update'></a>**

**A:** The previous mechanism in PoDoFo 0.9.x required enablement of document
for incremental updates, which is a decision step which I believe should be
unnecessary. Also:
1. In case of file loaded document it still required to perform the update in
the same file, and the check was performed on the path of the files being
operated to, which is unsafe;
2. In case of buffers worked for one update/signing operation but didn't work
for following operations, meaning the mechanism was bugged/unreliable.

An alternative strategy that makes clearer the fact that the incremental update
must be performed on the same file from where the document was loaded, or that underlying
buffer will grow its mememory consumption following subsequent operations in case of
buffer loaded documents, is available. It follows a couple of examples showing the
correct operations to update a document, loaded from file or buffer:

1. Save an update on a file loaded document, by loading and saving the document on the same location:

```cpp
auto inputOutput = std::make_shared<FileStreamDevice>(filename, FileMode::Open);

PdfMemDocument doc;
doc.Load(inputOutput);

doc.SaveUpdate(*inputOutput);
```

2. Save an update on a buffer, by copying the source first to the buffer
that will be also used to load the document:

```cpp
charbuff outputBuffer;
FileStreamDevice input(filepath);
auto inputOutput = std::make_shared<BufferStreamDevice>(outputBuffer);
input.CopyTo(*inputOutput);

PdfMemDocument doc;
doc.Load(inputOutput);

doc.SaveUpdate(*inputOutput);
```

Signing documents can be done with same technique, read the other questions for more examples.

**Q: Can I sign a document a second time?<a id='faq-sign-second'></a>**

**A:** Yes, this is tested, but to make sure this will work you'll to re-parse the document a second time,
as re-using the already loaded document is still untested (this may change later). For example you can
do as it follows:

```cpp
auto inputOutput = std::make_shared<FileStreamDevice>(filepath, FileMode::Open);

{
    PdfMemDocument doc;
    doc.Load(inputOutput);
    auto& page = doc.GetPages().GetPageAt(0);
    auto& signature = page.CreateField<PdfSignature>("Signature1", Rect());

    PdfSignerCms signer(x509certbuffer, pkeybuffer);
    PoDoFo::SignDocument(doc, *inputOutput, signer, signature);
}

{
    PdfMemDocument doc;
    doc.Load(inputOutput);
    auto& page = doc.GetPages().GetPageAt(0);
    auto& signature = page.CreateField<PdfSignature>("Signature2", Rect());
    PdfSignerCms signer(x509certbuffer, pkeybuffer);
    PoDoFo::SignDocument(doc, *inputOutput, signer, signature);
}
```

## No warranty

PoDoFo may or may not work for your needs and comes with absolutely no
warranty. Serious bugs, including security flaws, may be fixed at arbitrary
timeframes, or not fixed at all. Priority of implementing new features
and bug fixing are decided according to the interests and personal
preferences of the maintainers. If you need PoDoFo to integrate a feature
or bug fix that is critical to your workflow, the most welcome and fastest
approach is to [contribute](https://github.com/podofo/podofo/edit/master/README.md#contributions)
high-quality patches.

## Contributions

Please subscribe to the project mailing [list](https://sourceforge.net/projects/podofo/lists/podofo-users)
which is still followed by several of the original developers of PoDoFo.
A gitter [community](https://gitter.im/podofo/community) has also been created to ease some more informal chatter.
If you find a bug and know how to fix it, or you want to add a small feature, you're welcome to send a [pull request](https://github.com/podofo/podofo/pulls),
providing it follows the [coding style](https://github.com/podofo/podofo/blob/master/CODING-STYLE.md)
of the project. As a minimum requisite, any contribution should be:
* valuable for a multitude of people and not only self relevant for the contributor;
* consistent with surrounding code and not result in unpredictable behavior and/or bugs.

Other reasons for the rejection, or hold, of a pull request may be:

* the proposed code is incomplete or hacky;
* the change doesn't fit the scope of PoDoFo;
* the change shows lack of knowledge/mastery of the PDF specification and/or C++ language;
* the change breaks automatic tests performed by the maintainer;
* general lack of time in reviewing and merging the change.

If you need to implement a bigger feature or refactor, ask first if
it was already planned. The feature may be up for grabs, meaning that it's open for external contributions.
Please write in the relevant issue that you started to work on that, to receive some feedback/coordination.
If it's not, it means that the refactor/feature is planned to be implemented later by the maintainer(s).
If the feature is not listed in the issues, add it and/or create a [discussion](https://github.com/podofo/podofo/discussions)
to receive some feedback and discuss some basic design choices.

## Authors

> **Warning**: Please don't use personal email addresses for technical support inquries, but create
github [issues](https://github.com/podofo/podofo/issues) instead.

PoDoFo is currently developed and maintained by
[Francesco Pretto](mailto:ceztko@gmail.com), together with Dominik Seichter and others. See the file
[AUTHORS.md](https://github.com/podofo/podofo/blob/master/AUTHORS.md) for more details.

