file(GLOB SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "[^.]*.h" "[^.]*.cpp")
source_group("" FILES ${SOURCE_FILES})

include_directories(
    ${Fontconfig_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# repeat for each test
add_executable(podofo-unit ${SOURCE_FILES})
target_link_libraries(podofo-unit
    podofo_test
    ${PODOFO_LIBRARIES}
    podofo_private
    podofo_3rdparty
    ${PODOFO_LIB_DEPENDS}
)
add_compile_options(${PODOFO_CFLAGS})

include(CTest)
include(Catch)
catch_discover_tests(podofo-unit)
