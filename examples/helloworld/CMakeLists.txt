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

ADD_EXECUTABLE(helloworld helloworld.cpp)

TARGET_LINK_LIBRARIES(helloworld ${PODOFO_LIB})
SET_TARGET_PROPERTIES(helloworld PROPERTIES COMPILE_FLAGS "${PODOFO_CFLAGS}")
ADD_DEPENDENCIES(helloworld ${PODOFO_DEPEND_TARGET})
INCLUDE_DIRECTORIES(${PoDoFo_SOURCE_DIR})
