if (PODOFO_BUILD_UNSUPPORTED_TOOLS)
    message(WARNING "PoDoFo tools are enabled: tools are currently untested and unsupported")
else()
    return()
endif()

add_subdirectory(private)

if (MSVC)
    # Force requirement for unicode entry point
    add_link_options(/ENTRY:wmainCRTStartup)
else()
    if (MINGW)
        add_link_options(-municode)
    endif()
    # TODO: Remove me when all the tools are cleaned of use of C functions
    add_compile_options(-Wno-deprecated-declarations)
endif()

add_subdirectory(podofobox)
add_subdirectory(podofocolor)
add_subdirectory(podofocountpages)
add_subdirectory(podofocrop)
add_subdirectory(podofoencrypt)
add_subdirectory(podofogc)
add_subdirectory(podofoimgextract)
add_subdirectory(podofoimg2pdf)
add_subdirectory(podofomerge)
add_subdirectory(podofopages)
add_subdirectory(podofopdfinfo)
add_subdirectory(podofotxt2pdf)
add_subdirectory(podofotxtextract)
add_subdirectory(podofouncompress)
add_subdirectory(podofoimpose)
add_subdirectory(podofoincrementalupdates)
add_subdirectory(podofoxmp)
add_subdirectory(podofonooc)
add_subdirectory(podofosign)
