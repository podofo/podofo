cmake_minimum_required(VERSION 2.6)

IF(NOT PODOFO_MAIN_CMAKELISTS_READ)
  MESSAGE(FATAL_ERROR "Run cmake on the CMakeLists.txt in the project root, not the one in the 'src' directory. You will need to delete CMakeCache.txt from the current directory.")
ENDIF(NOT PODOFO_MAIN_CMAKELISTS_READ)

SET(PODOFO_DOC_SOURCES
  PdfAcroForm.cpp
  PdfAction.cpp
  PdfAnnotation.cpp
  PdfContents.cpp
  PdfDestination.cpp
  PdfDifferenceEncoding.cpp
  PdfDocument.cpp
  PdfElement.cpp
  PdfEncodingObjectFactory.cpp
  PdfExtGState.cpp
  PdfField.cpp
  PdfFileSpec.cpp
  PdfFontCache.cpp
  PdfFontCID.cpp
  PdfFont.cpp
  PdfFontFactory.cpp
  PdfFontMetricsBase14.cpp
  PdfFontMetrics.cpp
  PdfFontMetricsFreetype.cpp
  PdfFontMetricsObject.cpp
  PdfFontSimple.cpp
  PdfFontTrueType.cpp
  PdfFontTTFSubset.cpp
  PdfFontType1Base14.cpp
  PdfFontType1.cpp
  PdfFunction.cpp
  PdfHintStream.cpp
  PdfIdentityEncoding.cpp
  PdfImage.cpp
  PdfInfo.cpp
  PdfMemDocument.cpp
  PdfNamesTree.cpp
  PdfOutlines.cpp
  PdfPage.cpp
  PdfPagesTreeCache.cpp
  PdfPagesTree.cpp
  PdfPainter.cpp
  PdfPainterMM.cpp
  PdfShadingPattern.cpp
  PdfSignatureField.cpp
  PdfSignOutputDevice.cpp
  PdfStreamedDocument.cpp
  PdfTable.cpp
  PdfXObject.cpp
  )

IF(WIN32)
    # If we build for windows systems, we also include the resource file
    SET(PODOFO_DOC_SOURCES ${PODOFO_DOC_SOURCES} podofo-doc.rc)
ENDIF(WIN32)

SET(PODOFO_DOC_HEADERS
  PdfAcroForm.h
  PdfAction.h
  PdfAnnotation.h
  PdfContents.h
  PdfDestination.h
  PdfDifferenceEncoding.h
  PdfDocument.h
  PdfElement.h
  PdfEncodingObjectFactory.h
  PdfExtGState.h
  PdfField.h
  PdfFileSpec.h
  PdfFontCache.h
  PdfFontCID.h
  PdfFontFactoryBase14Data.h
  PdfFontFactory.h
  PdfFont.h
  PdfFontMetricsBase14.h
  PdfFontMetricsFreetype.h
  PdfFontMetrics.h
  PdfFontMetricsObject.h
  PdfFontSimple.h
  PdfFontTrueType.h
  PdfFontTTFSubset.h
  PdfFontType1Base14.h
  PdfFontType1.h
  PdfFunction.h
  PdfHintStream.h
  PdfIdentityEncoding.h
  PdfImage.h
  PdfInfo.h
  PdfMemDocument.h
  PdfNamesTree.h
  PdfOutlines.h
  PdfPage.h
  PdfPagesTreeCache.h
  PdfPagesTree.h
  PdfPainter.h
  PdfPainterMM.h
  PdfShadingPattern.h
  PdfSignatureField.h
  PdfSignOutputDevice.h
  PdfStreamedDocument.h
  PdfTable.h
  PdfXObject.h
    )

# Create a Source Group for Visual Studio
# so that headers are listed in the folder view
# and are easier accessible
SOURCE_GROUP(Headers FILES ${PODOFO_DOC_HEADERS})

INSTALL(FILES ${PODOFO_DOC_HEADERS}
    DESTINATION "include/podofo/doc"
    )

IF(NOT PODOFO_BUILD_SHARED AND NOT PODOFO_BUILD_STATIC)
    MESSAGE(FATAL_ERROR "At least one of PODOFO_BUILD_SHARED and PODOF_BUILD_STATIC must be set")
ENDIF(NOT PODOFO_BUILD_SHARED AND NOT PODOFO_BUILD_STATIC)

IF(PODOFO_BUILD_STATIC)
    MESSAGE("Building static PoDoFo library")
    ADD_LIBRARY(podofo-doc_static STATIC ${PODOFO_DOC_SOURCES})
    TARGET_LINK_LIBRARIES(podofo-doc_static ${PODOFO_BASE_LIB} ${PODOFO_DOC_LIB_DEPENDS})
    SET_TARGET_PROPERTIES(podofo-doc_static PROPERTIES
        VERSION "${PODOFO_LIBVERSION}"
        SOVERSION "${PODOFO_SOVERSION}"
        CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "podofo-doc"
        COMPILE_FLAGS "-DBUILDING_PODOFO"
        )
    SET(PODOFO_DEPEND_TARGET podofo-doc_static
        CACHE INTERNAL "Which PoDoFo library variant to depend on")
    SET(USING_SHARED_PODOFO FALSE)
    INSTALL(TARGETS podofo-doc_static
        RUNTIME DESTINATION "bin"
        LIBRARY DESTINATION "${LIBDIRNAME}"
        ARCHIVE DESTINATION "${LIBDIRNAME}"
        )
ENDIF(PODOFO_BUILD_STATIC)

IF(PODOFO_BUILD_SHARED)
    MESSAGE("Building shared PoDoFo library")
    ADD_LIBRARY(podofo-doc_shared SHARED ${PODOFO_DOC_SOURCES})
    TARGET_LINK_LIBRARIES(podofo-doc_shared ${PODOFO_BASE_LIB} ${PODOFO_DOC_LIB_DEPENDS})
    # TODO: set /wd4251 flag if we're doing a debug build with
    # Visual Studio, since it produces invalid warnings about STL
    # use.
    SET_TARGET_PROPERTIES(podofo-doc_shared PROPERTIES
        VERSION "${PODOFO_LIBVERSION}"
        SOVERSION "${PODOFO_SOVERSION}"
        CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "podofo-doc"
        COMPILE_FLAGS "-DBUILDING_PODOFO"
        )
    # Since we're building a shared podofo, prefer to depend on this one for
    # tests and tools over the static library (if built).
    SET(PODOFO_DEPEND_TARGET podofo-doc_shared
        CACHE INTERNAL "Which PoDoFo library variant to depend on")
    SET(USING_SHARED_PODOFO TRUE)
    INSTALL(TARGETS podofo-doc_shared
        RUNTIME DESTINATION "bin"
        LIBRARY DESTINATION "${LIBDIRNAME}"
        ARCHIVE DESTINATION "${LIBDIRNAME}"
        )
ENDIF(PODOFO_BUILD_SHARED)

# Use these flags when compiling code that includes PoDoFo headers.
# Failure to do so will result in compilation or link-time errors
# on some platforms, and can even cause undefined results at runtime.
IF(WIN32 AND USING_SHARED_PODOFO)
    SET(PODOFO_CFLAGS "-DUSING_SHARED_PODOFO" CACHE INTERNAL "Extra flags required when linking to PoDoFo")
ELSE(WIN32 AND USING_SHARED_PODOFO)
    SET(PODOFO_CFLAGS "" CACHE INTERNAL "Extra flags required when linking to PoDoFo")
ENDIF(WIN32 AND USING_SHARED_PODOFO)

# Write the cflags and depend target to the config file
FILE(APPEND 
     "${PoDoFo_BINARY_DIR}/PoDoFoConfig.cmake"
     "SET(PODOFO_CFLAGS ${PODOFO_CFLAGS})\n"
     )
FILE(APPEND
     "${PoDoFo_BINARY_DIR}/PoDoFoConfig.cmake"
     "SET(PODOFO_DEPEND_TARGET ${PODOFO_DEPEND_TARGET})\n"
     )
