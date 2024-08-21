## Source layout

### Root directory

- `3rdparty`: 3rd party headers and sources needed to compile PoDoFo;
- `cmake`: [CMake](https://cmake.org/) scripts and includes needed to compile PoDoFo;
- `examples`: Samples that use the PoDoFo API;
- `extern`: external git submodules. No one is currently needed to
  compile PoDoFo library, but they may be needed to build the playground or tests;
- `src`: the main source directory;
- `test`: test suite;
- `tools`: the PoDoFo tools suite;
- `staging`: directory with immature PoDoFo API extensions classes,
  or with unclear ownership/maintenance. May be promoted to
  main API or removed at any time.

### Source directory
The `src` directory contains only a single `podofo` folder.
In this way it's easy to just include `src` when using PoDoFo
externally of the source tree, simulating the layout of prefixed
`include` after installation. The content of `podofo` is:

- `podofo.h`: The main include file that pulls in all recommended
  public API types
- `main`: source directory with most of the library classes;
- `auxiliary`: directory with general purpose types/commodities
  that are not specific to handling of the PDF specification.
  Headers here are deployed as part of the public API;
- `optional`: directory with PDF specific types/commodities
  that are not generally included by `podofo.h`. They may
  be used during PoDoFo compilation. Headers here are deployed
  as part of the public API;
- `staging`: directory with components with unstable API being
  developed. They may end up (or not) in the public API
- `private`: directory with private commodities/data needed to
  compile PoDoFo library, and can be called by tools and tests
  as well by means of a `podofo_private` static library target.
  Headers here are not deployed as part of the public API;
