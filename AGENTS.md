# AGENTS.md

## Project overview

PoDoFo is a C++17 library for reading, writing and manipulating PDF documents. The build configuration is handled by CMake. The library can be built as shared (default) or static (`-DPODOFO_BUILD_STATIC=TRUE`).
Required toolchains (minimum versions):
- CMake 3.23
- GCC 9
- Clang 9
- Apple Clang 11
- MSVC 15.9

## Source layout

```
src/podofo/
├── main/         Core public API classes (PdfDocument, PdfPage, PdfFont, ...)
├── auxiliary/    General-purpose types not specific to PDF (nullable, cspan, mspan, StreamDevice, ...)
├── optional/     PDF-specific helpers not pulled in by podofo.h
├── staging/      Unstable API under development
├── private/      Internal implementation (podofo_private static lib target)
├── 3rdparty/     Vendored 3rd-party headers, conditionally part of the public API (tcb::span)
└── podofo.h      Umbrella public include
```

- `3rdparty/` vendored headers/sources (fmtlib, fast_float, utf8cpp, date, tcb::span, chromium numerics, Adobe AFDKO)
- `test/unit/`: Catch2-based unit tests (single executable `podofo-unit`)
- `test/common/`: shared test utilities (`PdfTest.h`, `TestUtils`)
- `tools/`: CLI utilities built on top of the library
- `examples/`: simple sample programs
- `playground/`: development and test bed using pre-built dependencies
- `extern/resources/`: test fixture files (git submodule)
- `extern/deps/`: depedencies for the `playground` (git submodule)

## Dependencies

Required: zlib, OpenSSL, FreeType, libxml2, Fontconfig (on Windows it can be optional with `PODOFO_WITH_WIN32GDI_FONT_SEARCH` option enabled)

Optional: libjpeg, libpng, libtiff

Vendored: fmt, fast_float, utf8cpp, utf8proc, tcb-span, date

## Building and running test

It's recommented to use the `playground`. First ensure `extern/{deps|resources}` are fetched. If not, do it before continuing:

```
git submodule update --init
```

The `playground` shall be used with a CMake build dir named accordingly to the host platform and build type as described in the commands below. Prefer debug builds while testing.

### Bootstrap:

```
cd playground
cmake -B build-{win|linux|macos}-{Debug|Release} -DCMAKE_BUILD_TYPE={Debug|Release}
```
### Build:
```
cmake --build build-{win|linux|macos}-{Debug|Release}
```

### Run unit tests:

```
ctest --test-dir build-{win|linux|macos}-{Debug|Release}
```

The test executable is `build-{win|linux|macos}-{Debug|Release}/target/podofo-unit`.

## CMake targets

- `podofo_shared` / `podofo_static`: Main library (aliased as `podofo::podofo`)
- `podofo_private`: Internal static library with private implementation details
- `podofo_3rdparty`: Vendored 3rd-party code compiled as a static library
- `podofo-unit`: Unit test executable

## Coding style

#[[file:CODING-STYLE.md]]

Refer to `CODING-STYLE.md`, but don't follow any hyperlink there. In addition:
- never use Em Dash when writing, just the regular ASCII hyphen `-`
- never use Horizontal Ellipsis when writing, just regular ASCII dots `...`

## Key classes

- `PdfMemDocument`: in-memory document loaded from file or buffer
- `PdfStreamedDocument`: write-only streaming document
- `PdfPage` / `PdfPageCollection`: page access and manipulation
- `PdfObject` / `PdfVariant`: the generic PDF object model
- `PdfDictionary` / `PdfArray` / `PdfName` / `PdfString`: PDF data types
- `PdfElement`: a wrapper for a `PdfDictionary` or `PdfArray` within a document. Can be extended to add functionalities
- `PdfFont` / `PdfFontManager`: font loading, subsetting and encoding
- `PdfPainter`: high-level drawing API (paths, text, images)
- `PdfEncoding` / `PdfEncodingMap`: character code <-> Unicode mapping
- `PdfEncrypt` / `PdfSigningContext`: digital signatures

## Developing steering

- No excess tests: one or two simple tests per small fix/feature are usually fine, unless exhaustive coverage is genuinely needed
- If a PDF fixture is needed in a test, it should be precomputed as a constant string, not assembled on the fly with text-writing operations
- Catch2 `SECTION(s)` should be avoided in tests when possible
- No long, repetitive comments. If a comment must be repeated it should fit on one or two lines at most
- Test files live in `test/unit/` and follow the naming pattern `<Topic>Test.cpp`
- Tests include `<PdfTest.h>` (which pulls in Catch2, PoDoFo headers, and `TestUtils`). The macros `ASSERT_EQUAL` and `ASSERT_THROW_WITH_ERROR_CODE` are available for common assertions
- Test resource files are read via `TestUtils::GetTestInputFilePath(...)` and output goes to `TestUtils::GetTestOutputFilePath(...)`
