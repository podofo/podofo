# PoDoFo [![build-linux](https://github.com/podofo/podofo/actions/workflows/build-linux.yml/badge.svg)](https://github.com/podofo/podofo/actions/workflows/build-linux.yml) [![build-mac](https://github.com/podofo/podofo/actions/workflows/build-mac.yml/badge.svg)](https://github.com/podofo/podofo/actions/workflows/build-mac.yml) [![build-win](https://github.com/podofo/podofo/actions/workflows/build-win.yml/badge.svg)](https://github.com/podofo/podofo/actions/workflows/build-win.yml)

1.  [What is PoDoFo?](#what-is-podofo)
2.  [Requirements](#requirements)
3.  [Licensing](#licensing)
4.  [Development quickstart](#development-quickstart)
5.  [String encoding and buffer conventions](#string-encoding-and-buffer-conventions)
6.  [API Stability](#api-stability)
7.  [PoDoFo tools](#podofo-tools)
8.  [TODO](#todo)
9.  [FAQ](#faq)
10.  [No warranty](#no-warranty)
11.  [Contributions](#contributions)
12.  [Authors](#authors)

## What is PoDoFo?

PoDoFo is a s a free portable C++ library to work with the PDF file format.

PoDoFo provides classes to parse a PDF file and modify its content
into memory. The changes can be written back to disk easily.
Besides PDF parsing PoDoFo also provides facilities to create your
own PDF files from scratch. It currently does not
support rendering PDF content.

## Requirements

To build PoDoFo lib you'll need a c++17 compiler,
CMake 3.16 and the following libraries:

* freetype2
* fontconfig (required for Unix platforms, optional for Windows)
* OpenSSL
* LibXml2
* zlib
* libjpeg (optional)
* libtiff (optional)
* libpng (optional)
* libidn (optional)

For the most polular toolchains, PoDoFo requires the following
minimum versions:

* msvc++ 14.16 (VS 2017 15.9)
* gcc 8.1
* clang/llvm 7.0

It is regularly tested with the following IDE/toolchains versions:

* Visual Studio 2017 15.9
* Visual Studio 2019 16.11
* Visual Studio 2022 17.3
* gcc 9.3.1
* XCode 13.3
* NDK r23b

## Licensing

PoDoFo library is licensed under the [LGPL 2.0](https://spdx.org/licenses/LGPL-2.0-or-later.html) or later terms.
PoDoFo tools are licensed under the [GPL 2.0](https://spdx.org/licenses/GPL-2.0-or-later.html) or later terms.

## Development quickstart

Learn by example by looking at the github workflow [definitions](https://github.com/podofo/podofo/tree/master/.github/workflows).
There are examples using [`vcpkg`](https://vcpkg.io/) under
[Windows](https://github.com/podofo/podofo/blob/master/.github/workflows/build-win-vcpkg.yml),
[`brew`](https://brew.sh/) under [macos](https://github.com/podofo/podofo/blob/master/.github/workflows/build-linux.yml),
and `apt-get` under [ubuntu](https://github.com/podofo/podofo/blob/master/.github/workflows/build-linux.yml), installing all the
required dependencies, bootstrapping the CMake project, building and testing the library. In Windows,
follow the vcpkg [quickstart](https://vcpkg.io/en/getting-started.html) guide to setup the package manager repository first.
It may be also useful to set the enviroment variable `VCPKG_DEFAULT_TRIPLET` to `x64-windows` to default installing 64 bit dependencies
and define a `VCPKG_INSTALLATION_ROOT` variable with the location of the repository as created in the quickstart.

Additionally there's a playground area in the repository where you can have access to pre-build
dependencies for some popular architectures/operating systems. Have a look in the
[Readme](https://github.com/podofo/podofo/tree/master/playground) there.

## String encoding and buffer conventions

All ```std::strings``` or ```std::string_view``` in the library are intended
to hold UTF-8 encoded string content. ```PdfString``` and ```PdfName``` constructors
accept UTF-8 encoded strings by default (```PdfName``` accept only characters in the
```PdfDocEncoding``` char set, though). ```charbuff``` abd ```bufferview```
instead represent a generic octet buffer.

## API stability

PoDoFo has an unstable API that is the results of an extensive API review of PoDoFo 0.9.x.
It is expected to converge to a stable API as soon as the review process is completed.
See [API Stability](https://github.com/podofo/podofo/wiki/API-Stability) for more details.

## PoDoFo Tools

PoDoFo tools are still available in the source [tree](https://github.com/podofo/podofo/)
but are currently untested and unmaintained. It's currently not recommended
to include them in software distributions. If you want to build them make sure
to bootstrap the CMake project with ```-DPODOFO_ENABLE_TOOLS=TRUE```.
Tools are already enabled in the playground.

## TODO

There's a [TODO](https://github.com/podofo/podofo/blob/master/TODO.md) list in the wiki.
in the [issue](https://github.com/podofo/podofo/issues) tracker.

## FAQ

**Q: How do I sign a document?**

**A:** The signing procedure is still low level as it was PoDoFo. This is gonna
change, soon!, whith a new high-level API for signining being worked on,
which will be fully unit tested. For now you should check the `podofosign`
tool (**WARNING**: untested) which should give you the idea how to sign documents
creating a *CMS* structure directly with OpenSSL.
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


**Q: `PdfMemDocument::SaveUpdate()` or `PoDoFo::SignDocument()` write only a partial
file: why so and why there's no mechanism to seamlessly handle the incremental
update as it was in PoDoFo? What should be done to correctly update/sign the
document?**

**A:** The previous mechanism in PoDoFo required enablement of document for
incremental updates, which is a decisional step which I believe should be not
be necessary. Also:
1. In case of file loaded document it still required to perform the update in
the same file, and the check was performed on the path of the files being
operated to, which is unsafe;
2. In case of buffers worked for one update/signing operation but didn't work
for following operations.

The idea is to implement a more explicit mehcanism that makes more clear
and/or enforces the fact that the incremental update must be performed on the
same file in case of file loaded documents or that underlying buffer will grow
following subsequent operations in case of buffer loaded documents.
Before that, as a workaround, a couple of examples showing the correct operation
to either update or sign a document (file or buffer loaded) are presented.
Save an update on a file loaded document, by copying the source to another
destination:

```
    string inputPath;
    string outputPath;
    auto input = std::make_shared<FileStreamDevice>(inputPath);
    FileStreamDevice output(outputPath, FileMode::Create);
    input->CopyTo(output);

    PdfMemDocument doc;
    doc.LoadFromDevice(input);

    doc.SaveUpdate(output);
```

Sign a buffer loaded document:

```
    bufferview inputBuffer;
    charbuff outputBuffer;
    auto input = std::make_shared<SpanStreamDevice>(inputBuffer);
    BufferStreamDevice output(outputBuffer);
    input->CopyTo(output);

    PdfMemDocument doc;
    doc.LoadFromDevice(input);
    // Retrieve/create the signature, create the signer, ...
    PoDoFo::SignDocument(doc, output, signer, signature);
```

**Q: Can I sign a document a second time?**

**A:** Yes, this is tested, but to make sure this will work you'll to re-parse the document a second time,
as re-using the already loaded document is still untested (this may change later). For example do as it follows:

```
    bufferview inputBuffer;
    charbuff outputBuffer;
    auto input = std::make_shared<SpanStreamDevice>(inputBuffer);
    BufferStreamDevice output(outputBuffer);
    input->CopyTo(output);

    {
        PdfMemDocument doc;
        doc.LoadFromDevice(input);
        // Retrieve/create the signature, create the signer, ...
        PoDoFo::SignDocument(doc, output, signer, signature);
    }

    input = std::make_shared<SpanStreamDevice>(output);
    {
        PdfMemDocument doc;
        doc.LoadFromDevice(input);
        // Retrieve/create the signature, create the signer, ...
        PoDoFo::SignDocument(doc, output, signer, signature);
    }
```

## No warranty

PoDoFo may or may not work for your needs and comes with absolutely no warranty.
Serious bugs, including security flaws, may be fixed at arbitrary
timeframes, or not fixed at all.

## Contributions

Please subscribe to the project mailing [list](https://sourceforge.net/projects/podofo/lists/podofo-users)
which is still followed by several of the original developers of PoDoFO.
A gitter [community](https://gitter.im/podofo/community) has also been created to ease some more informal chatter.
If you find a bug and know how to fix it, or you want to add a small feature, you're welcome to send a [pull request](https://github.com/podofo/podofo/pulls),
providing it follows the [coding style](https://github.com/podofo/podofo/blob/master/CODING-STYLE.md)
of the project. Also, as a minimum requisite, any contribution should be valuable for a multitude of people,
to avoid it to be only self relevant for the contributor. Other reasons for the rejection, or hold,
of a pull request may be:

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

PoDoFo is currently developed and mantained by
[Francesco Pretto](mailto:ceztko@gmail.com), together with Dominik Seichter an others. See the file
[AUTHORS.md](https://github.com/podofo/podofo/blob/master/AUTHORS.md) for more details.
Please don't use personal emails for technical support requests, but create
github [issues](https://github.com/podofo/podofo/issues) instead.
