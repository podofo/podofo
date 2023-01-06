## Source layout

### Root directory

- `3rdparty`: 3rd party headers and sources needed to compile PoDoFo;
- `cmake`: [CMake](https://cmake.org/) scripts and includes needed to compile PoDoFo;
- `examples`: Samples that use the PoDoFo API;
- `extern`: external git submodules needed to compile PoDoFo (no one is currently needed);
- `src`: the main source directory;
- `test`: test suite;
- `tools`: the PoDoFo tools suite.

### Source directory
The `src` directory contains only a single `podofo` folder.
In this way it's easy to just include `src` when using PoDoFo
externally of the source tree, simulating the layout of prefixed
`include` after installation. The content of `podofo` is:

- `base`: base source directory with most of the library classes;
- `common`: directory with common general purpose headers that
  are deployed as part of the public API;
- `contrib`: directory with more PoDoFo API classes supplied
    by external contributors, or with unclear ownerships;
- `private`: directory with private headers/data needed to compile PoDoFo.
