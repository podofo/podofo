
# IF(OPENSSL_FOUND)
# ADD_SUBDIRECTORY(PKIBox)
# ENDIF(OPENSSL_FOUND)

SET(PODOFO_SOURCES
    PdfAction.cpp
    PdfAcroForm.cpp
    PdfAnnotation.cpp
    PdfArray.cpp
    PdfCanvas.cpp
    PdfColor.cpp
    PdfContents.cpp
    PdfContentsTokenizer.cpp
    PdfData.cpp
    PdfDataType.cpp
    PdfDate.cpp
    PdfDestination.cpp
    PdfDictionary.cpp
    PdfDifferenceEncoding.cpp
    PdfDocument.cpp
    PdfElement.cpp
    PdfEncoding.cpp
    PdfEncodingFactory.cpp
    PdfEncrypt.cpp
    PdfError.cpp
    PdfExtGState.cpp
    PdfField.cpp
    PdfFileSpec.cpp
    PdfFileStream.cpp
    PdfFilter.cpp
    PdfFiltersPrivate.cpp
    PdfFont.cpp
    PdfFontCache.cpp
    PdfFontCID.cpp
    PdfFontMetrics.cpp
    PdfFontFactory.cpp
    PdfFontSimple.cpp
    PdfFontType1.cpp
    PdfFontTrueType.cpp
    PdfFontTTFSubset.cpp
    PdfFunction.cpp
    PdfHintStream.cpp
    PdfImage.cpp
    PdfImmediateWriter.cpp
    PdfInfo.cpp
    PdfInputDevice.cpp
    PdfInputStream.cpp
    PdfLocale.cpp
    PdfMemDocument.cpp
    PdfMemStream.cpp
    PdfName.cpp
    PdfNamesTree.cpp
    PdfObject.cpp
    PdfOutlines.cpp
    PdfOutputDevice.cpp
    PdfOutputStream.cpp
    PdfPage.cpp
    PdfPagesTree.cpp
    PdfPainter.cpp
    PdfPainterMM.cpp
    PdfParser.cpp
    PdfParserObject.cpp
    PdfRect.cpp
    PdfRefCountedBuffer.cpp
    PdfRefCountedInputDevice.cpp
    PdfReference.cpp
    PdfRijndael.cpp
    PdfShadingPattern.cpp
    PdfStream.cpp
    PdfStreamedDocument.cpp
    PdfString.cpp
    PdfTable.cpp
    PdfTokenizer.cpp
    PdfTTFWriter.cpp
    PdfVariant.cpp
    PdfVecObjects.cpp
    PdfVersion.h
    PdfWriter.cpp
    PdfXObject.cpp
    PdfXRef.cpp
    PdfXRefStream.cpp
    util/PdfMutex.cpp
    util/PdfMutexWrapper.cpp
)

IF(WIN32)
    # If we build for windows systems, we also include the resource file
    SET(PODOFO_SOURCES ${PODOFO_SOURCES} podofo.rc)
ENDIF(WIN32)

SET(PODOFO_HEADERS
    Pdf3rdPtyForwardDecl.h
    PdfAction.h
    PdfAcroForm.h
    PdfAnnotation.h
    PdfArray.h
    PdfCanvas.h
    PdfColor.h
    PdfContents.h
    PdfContentsTokenizer.h
    PdfData.h
    PdfDataType.h
    PdfDate.h
    PdfDefines.h
    PdfDestination.h
    PdfDictionary.h
    PdfDifferenceEncoding.h
    PdfDocument.h
    PdfElement.h
    PdfEncoding.h
    PdfEncodingFactory.h
    PdfEncrypt.h
    PdfError.h
    PdfExtGState.h
    PdfField.h
    PdfFileSpec.h
    PdfFileStream.h
    PdfFilter.h
    PdfFiltersPrivate.h
    PdfFont.h
    PdfFontCache.h
    PdfFontCID.h
    PdfFontMetrics.h
    PdfFontFactory.h
    PdfFontSimple.h
    PdfFontType1.h
    PdfFontTrueType.h
    PdfFontTTFSubset.h
    PdfFunction.h    
    PdfHintStream.h
    PdfImage.h
    PdfImmediateWriter.h
    PdfInfo.h
    PdfInputDevice.h
    PdfInputStream.h
    PdfLocale.h
    PdfMemDocument.h
    PdfMemStream.h
    PdfName.h
    PdfNamesTree.h
    PdfObject.h
    PdfOutlines.h
    PdfOutputDevice.h
    PdfOutputStream.h
    PdfPage.h
    PdfPagesTree.h
    PdfPainter.h
    PdfPainterMM.h
    PdfParser.h
    PdfParserObject.h
    PdfRect.h
    PdfRefCountedBuffer.h
    PdfRefCountedInputDevice.h
    PdfReference.h
    PdfRijndael.h
    PdfShadingPattern.h
    PdfStream.h
    PdfStreamedDocument.h
    PdfString.h
    PdfTable.h
    PdfTokenizer.h
    PdfTTFWriter.h
    PdfVariant.h
    PdfVecObjects.h
    PdfVersion.h
    PdfWriter.h
    PdfXObject.h
    PdfXRef.h
    PdfXRefStream.h
    podofo.h
    podofoapi.h
    util/PdfMutex.h
    util/PdfMutexWrapper.h
)

INSTALL(FILES ${PODOFO_HEADERS}
    DESTINATION "include/podofo"
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
        VERSION "0.6.99"
        SOVERSION "0.6.99"
        CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "podofo"
        )
    SET(PODOFO_DEPEND_TARGET podofo_static
        CACHE INTERNAL "Which PoDoFo library variant to depend on")
    SET(USING_SHARED_PODOFO FALSE)
    INSTALL(TARGETS podofo_static
        RUNTIME DESTINATION "bin"
        LIBRARY DESTINATION "${LIBDIRNAME}"
        ARCHIVE DESTINATION "${LIBDIRNAME}"
        )
ENDIF(PODOFO_BUILD_STATIC)

IF(PODOFO_BUILD_SHARED)
    MESSAGE("Building shared PoDoFo library")
    ADD_LIBRARY(podofo_shared SHARED ${PODOFO_SOURCES})
    TARGET_LINK_LIBRARIES(podofo_shared ${PODOFO_LIB_DEPENDS})
    # TODO: set /wd4251 flag if we're doing a debug build with
    # Visual Studio, since it produces invalid warnings about STL
    # use.
    SET_TARGET_PROPERTIES(podofo_shared PROPERTIES
        VERSION "0.6.99"
        SOVERSION "0.6.99"
        CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "podofo"
        )
    # Since we're building a shared podofo, prefer to depend on this one for
    # tests and tools over the static library (if built).
    SET(PODOFO_DEPEND_TARGET podofo_shared
        CACHE INTERNAL "Which PoDoFo library variant to depend on")
    SET(USING_SHARED_PODOFO TRUE)
    INSTALL(TARGETS podofo_shared
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
