# Linking to podofo::podofo is all that is needed: the target carries
# the public include directories, the required C++ standard and, for a
# static build, the PODOFO_STATIC definition. To build outside the
# PoDoFo source tree, just add find_package(podofo REQUIRED) before.
#
# Note that you don't need the headers for PoDoFo's dependencies in your
# header search path.

add_executable(helloworld-base14 helloworld-base14.cpp)
target_link_libraries(helloworld-base14 PRIVATE podofo::podofo)
