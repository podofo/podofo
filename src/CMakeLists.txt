SET(PODOFO_SOURCES
    PdfAction.cpp
    PdfAnnotation.cpp
    PdfArray.cpp
    PdfCanvas.cpp
    PdfContents.cpp
    PdfDataType.cpp
    PdfDate.cpp
    PdfDestination.cpp
    PdfDictionary.cpp
    PdfDocument.cpp
    PdfElement.cpp
    PdfEncrypt.cpp
    PdfError.cpp
    PdfFilter.cpp
    PdfFileSpec.cpp
    PdfExtGState.cpp
    PdfFont.cpp
    PdfFontMetrics.cpp
    PdfHintStream.cpp
    PdfImage.cpp
    PdfInfo.cpp
    PdfInputDevice.cpp
    PdfName.cpp
    PdfNamesTree.cpp
    PdfObject.cpp
    PdfOutlines.cpp
    PdfOutputDevice.cpp
    PdfPage.cpp
    PdfPagesTree.cpp
    PdfPainter.cpp
    PdfPainterMM.cpp
    PdfParser.cpp
    PdfParserBase.cpp
    PdfParserObject.cpp
    PdfRect.cpp
    PdfRefCountedBuffer.cpp
    PdfRefCountedInputDevice.cpp
    PdfReference.cpp
    PdfStream.cpp
    PdfString.cpp
    PdfVariant.cpp
    PdfVecObjects.cpp
    PdfWriter.cpp
    PdfXObject.cpp
)

IF(NOT PODOFO_BUILD_SHARED AND NOT PODOFO_BUILD_STATIC)
    MESSAGE(FATAL_ERROR "At least one of PODOFO_BUILD_SHARED and PODOF_BUILD_STATIC must be set")
ENDIF(NOT PODOFO_BUILD_SHARED AND NOT PODOFO_BUILD_STATIC)

SET(PODOFO_DEPEND_TARGET CACHE INTERNAL
    "Which PoDoFo library target to depend on when building tools and tests")

IF(PODOFO_BUILD_STATIC)
    MESSAGE("Building static PoDoFo library")
    ADD_LIBRARY(podofo_static STATIC ${PODOFO_SOURCES})
    TARGET_LINK_LIBRARIES(podofo_static ${PODOFO_LIB_DEPENDS})
    SET_TARGET_PROPERTIES(podofo_static PROPERTIES
        VERSION "0.4.0"
        SOVERSION "0.4.0"
        CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "podofo"
        )
    SET(PODOFO_DEPEND_TARGET podofo_static
        CACHE INTERNAL "Which PoDoFo library variant to depend on")
    SET(USING_SHARED_PODOFO FALSE)
ENDIF(PODOFO_BUILD_STATIC)

IF(PODOFO_BUILD_SHARED)
    MESSAGE("Building shared PoDoFo library")
    ADD_LIBRARY(podofo_shared SHARED ${PODOFO_SOURCES})
    TARGET_LINK_LIBRARIES(podofo_shared ${PODOFO_LIB_DEPENDS})
    # TODO: set /wd4251 flag if we're doing a debug build with
    # Visual Studio, since it produces invalid warnings about STL
    # use.
    SET_TARGET_PROPERTIES(podofo_shared PROPERTIES
        VERSION "0.4.0"
        SOVERSION "0.4.0"
        CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "podofo"
        )
    # Since we're building a shared podofo, prefer to depend on this one for
    # tests and tools over the static library (if built).
    SET(PODOFO_DEPEND_TARGET podofo_shared
        CACHE INTERNAL "Which PoDoFo library variant to depend on")
    SET(USING_SHARED_PODOFO TRUE)
ENDIF(PODOFO_BUILD_SHARED)

MESSAGE("PDEBUG: USING_SHARED_PODOFO: ${USING_SHARED_PODOFO}")
MESSAGE("PDEBUG: PODOFO_DEPEND_TARGET: ${PODOFO_DEPEND_TARGET}")

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
