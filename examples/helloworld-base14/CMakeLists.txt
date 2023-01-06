# This is not a full standalone CMake configuration for the hello world
# example.
#
# To build it outside the PoDoFo source tree, you must set your build system
# make the PoDoFo headers available and must link to the PoDoFo library
# and any libraries it depends on (see the README,
#     "5. Using PoDoFo in Your Application") .
#
# Note that you don't need the headers for PoDoFo's dependencies in your
# header search path.

add_compile_options(${PODOFO_CFLAGS})
add_executable(helloworld-base14 helloworld-base14.cpp)
target_link_libraries(helloworld-base14 ${PODOFO_LIBRARIES})
