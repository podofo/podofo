CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

IF(NOT PODOFO_MAIN_CMAKELISTS_READ)
  MESSAGE(FATAL_ERROR "Run cmake on the CMakeLists.txt in the project root, not the one in the 'src' directory. You will need to delete CMakeCache.txt from the current directory.")
ENDIF(NOT PODOFO_MAIN_CMAKELISTS_READ)

SET(PODOFO_BASE_DEPEND_TARGET CACHE INTERNAL
    "Which PoDoFo library target to depend on when building tools and tests")
SET(PODOFO_DEPEND_TARGET CACHE INTERNAL
    "Which PoDoFo library target to depend on when building tools and tests")

SET(PODOFO_BASE_SOURCES
  base/PdfArray.cpp
  base/PdfCanvas.cpp
  base/PdfColor.cpp
  base/PdfContentsTokenizer.cpp
  base/PdfData.cpp
  base/PdfDataType.cpp
  base/PdfDate.cpp
  base/PdfDictionary.cpp
  base/PdfEncoding.cpp
  base/PdfEncodingFactory.cpp
  base/PdfEncrypt.cpp
  base/PdfError.cpp
  base/PdfFileStream.cpp
  base/PdfFilter.cpp
  base/PdfFiltersPrivate.cpp
  base/PdfImmediateWriter.cpp
  base/PdfInputDevice.cpp
  base/PdfInputStream.cpp
  base/PdfLocale.cpp
  base/PdfMemStream.cpp
  base/PdfMemoryManagement.cpp
  base/PdfName.cpp
  base/PdfObject.cpp
  base/PdfObjectStreamParserObject.cpp
  base/PdfOutputDevice.cpp
  base/PdfOutputStream.cpp
  base/PdfParser.cpp
  base/PdfParserObject.cpp
  base/PdfRect.cpp
  base/PdfRefCountedBuffer.cpp
  base/PdfRefCountedInputDevice.cpp
  base/PdfReference.cpp
  base/PdfSigIncWriter.cpp
  base/PdfStream.cpp
  base/PdfString.cpp
  base/PdfTokenizer.cpp
  base/PdfVariant.cpp
  base/PdfVecObjects.cpp
  base/PdfWriter.cpp
  base/PdfXRef.cpp
  base/PdfXRefStream.cpp
  base/PdfXRefStreamParserObject.cpp
  )

SET(PODOFO_DOC_SOURCES
  doc/PdfAcroForm.cpp
  doc/PdfAction.cpp
  doc/PdfAnnotation.cpp
  doc/PdfCMapEncoding.cpp
  doc/PdfContents.cpp
  doc/PdfDestination.cpp
  doc/PdfDifferenceEncoding.cpp
  doc/PdfDocument.cpp
  doc/PdfElement.cpp
  doc/PdfEncodingObjectFactory.cpp
  doc/PdfExtGState.cpp
  doc/PdfField.cpp
  doc/PdfFileSpec.cpp
  doc/PdfFont.cpp
  doc/PdfFontCID.cpp
  doc/PdfFontCache.cpp
  doc/PdfFontConfigWrapper.cpp
  doc/PdfFontFactory.cpp
  doc/PdfFontMetrics.cpp
  doc/PdfFontMetricsBase14.cpp
  doc/PdfFontMetricsFreetype.cpp
  doc/PdfFontMetricsObject.cpp
  doc/PdfFontSimple.cpp
  doc/PdfFontTTFSubset.cpp
  doc/PdfFontTrueType.cpp
  doc/PdfFontType1.cpp
  doc/PdfFontType3.cpp
  doc/PdfFontType1Base14.cpp
  doc/PdfFunction.cpp
  doc/PdfHintStream.cpp
  doc/PdfIdentityEncoding.cpp
  doc/PdfImage.cpp
  doc/PdfInfo.cpp
  doc/PdfMemDocument.cpp
  doc/PdfNamesTree.cpp
  doc/PdfOutlines.cpp
  doc/PdfPage.cpp
  doc/PdfPagesTree.cpp
  doc/PdfPagesTreeCache.cpp
  doc/PdfPainter.cpp
  doc/PdfPainterMM.cpp
  doc/PdfShadingPattern.cpp
  doc/PdfSigIncMemDocument.cpp
  doc/PdfSigIncPainter.cpp
  doc/PdfSigIncSignatureField.cpp
  doc/PdfSignOutputDevice.cpp
  doc/PdfSignatureField.cpp
  doc/PdfStreamedDocument.cpp
  doc/PdfTable.cpp
  doc/PdfTilingPattern.cpp
  doc/PdfXObject.cpp
  )

IF(WIN32)
    # If we build for windows systems, we also include the resource file
    SET(PODOFO_DOC_SOURCES ${PODOFO_DOC_SOURCES} doc/podofo-doc.rc)
ENDIF(WIN32)

SET(PODOFO_HEADERS
  podofo-base.h
  podofo.h)

SET(PODOFO_BASE_HEADERS
   ${PoDoFo_BINARY_DIR}/podofo_config.h
   base/Pdf3rdPtyForwardDecl.h
   base/PdfArray.h
   base/PdfCanvas.h
   base/PdfColor.h
   base/PdfCompilerCompat.h
   base/PdfCompilerCompatPrivate.h
   base/PdfContentsTokenizer.h
   base/PdfData.h
   base/PdfDataType.h
   base/PdfDate.h
   base/PdfDefines.h
   base/PdfDefinesPrivate.h
   base/PdfDictionary.h
   base/PdfEncoding.h
   base/PdfEncodingFactory.h
   base/PdfEncrypt.h
   base/PdfExtension.h
   base/PdfError.h
   base/PdfFileStream.h
   base/PdfFilter.h
   base/PdfFiltersPrivate.h
   base/PdfImmediateWriter.h
   base/PdfInputDevice.h
   base/PdfInputStream.h
   base/PdfLocale.h
   base/PdfMemStream.h
   base/PdfMemoryManagement.h
   base/PdfName.h
   base/PdfObject.h
   base/PdfObjectStreamParserObject.h
   base/PdfOutputDevice.h
   base/PdfOutputStream.h
   base/PdfParser.h
   base/PdfParserObject.h
   base/PdfRect.h
   base/PdfRefCountedBuffer.h
   base/PdfRefCountedInputDevice.h
   base/PdfReference.h
   base/PdfSigIncWriter.h
   base/PdfStream.h
   base/PdfString.h
   base/PdfTokenizer.h
   base/PdfVariant.h
   base/PdfVecObjects.h
   base/PdfVersion.h
   base/PdfWriter.h
   base/PdfXRef.h
   base/PdfXRefStream.h
   base/PdfXRefStreamParserObject.h
   base/podofoapi.h
   )

SET(PODOFO_BASE_HEADERS2 
    base/util/PdfMutex.h
    base/util/PdfMutexImpl_noop.h
    base/util/PdfMutexImpl_win32.h
    base/util/PdfMutexImpl_pthread.h
    base/util/PdfMutexWrapper.h
    )

SET(PODOFO_DOC_HEADERS
  doc/PdfAcroForm.h
  doc/PdfAction.h
  doc/PdfAnnotation.h
  doc/PdfCMapEncoding.h
  doc/PdfContents.h
  doc/PdfDestination.h
  doc/PdfDifferenceEncoding.h
  doc/PdfDocument.h
  doc/PdfElement.h
  doc/PdfEncodingObjectFactory.h
  doc/PdfExtGState.h
  doc/PdfField.h
  doc/PdfFileSpec.h
  doc/PdfFont.h
  doc/PdfFontCID.h
  doc/PdfFontCache.h
  doc/PdfFontConfigWrapper.h
  doc/PdfFontFactory.h
  doc/PdfFontFactoryBase14Data.h
  doc/PdfFontMetrics.h
  doc/PdfFontMetricsBase14.h
  doc/PdfFontMetricsFreetype.h
  doc/PdfFontMetricsObject.h
  doc/PdfFontSimple.h
  doc/PdfFontTTFSubset.h
  doc/PdfFontTrueType.h
  doc/PdfFontType1.h
  doc/PdfFontType3.h
  doc/PdfFontType1Base14.h
  doc/PdfFunction.h
  doc/PdfHintStream.h
  doc/PdfIdentityEncoding.h
  doc/PdfImage.h
  doc/PdfInfo.h
  doc/PdfMemDocument.h
  doc/PdfNamesTree.h
  doc/PdfOutlines.h
  doc/PdfPage.h
  doc/PdfPagesTree.h
  doc/PdfPagesTreeCache.h
  doc/PdfPainter.h
  doc/PdfPainterMM.h
  doc/PdfShadingPattern.h
  doc/PdfSigIncMemDocument.h
  doc/PdfSigIncPainter.h
  doc/PdfSigIncSignatureField.h
  doc/PdfSignOutputDevice.h
  doc/PdfSignatureField.h
  doc/PdfStreamedDocument.h
  doc/PdfTable.h
  doc/PdfTilingPattern.h
  doc/PdfXObject.h
  )

# Create a Source Group for Visual Studio
# so that headers are listed in the folder view
# and are easier accessible
SOURCE_GROUP(Headers FILES 
  ${PODOFO_HEADERS} 
  ${PODOFO_BASE_HEADERS} 
  ${PODOFO_BASE_HEADERS2} 
  ${PODOFO_DOC_HEADERS})

INSTALL(FILES ${PODOFO_HEADERS}
  DESTINATION "include/podofo"
  )

INSTALL(FILES ${PODOFO_BASE_HEADERS}
    DESTINATION "include/podofo/base"
    )

INSTALL(FILES ${PODOFO_BASE_HEADERS2}
    DESTINATION "include/podofo/base/util"
    )

INSTALL(FILES ${PODOFO_DOC_HEADERS}
    DESTINATION "include/podofo/doc"
    )

IF(NOT PODOFO_BUILD_SHARED AND NOT PODOFO_BUILD_STATIC)
    MESSAGE(FATAL_ERROR "At least one of PODOFO_BUILD_SHARED and PODOF_BUILD_STATIC must be set")
ENDIF(NOT PODOFO_BUILD_SHARED AND NOT PODOFO_BUILD_STATIC)

IF(PODOFO_BUILD_STATIC)
    MESSAGE("Building static PoDoFo library")
    ADD_LIBRARY(podofo_static STATIC ${PODOFO_BASE_SOURCES} ${PODOFO_DOC_SOURCES})
    TARGET_LINK_LIBRARIES(podofo_static ${PODOFO_LIB_DEPENDS})
    SET_TARGET_PROPERTIES(podofo_static PROPERTIES
        VERSION "${PODOFO_LIBVERSION}"
        SOVERSION "${PODOFO_SOVERSION}"
        CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "podofo"
        COMPILE_FLAGS "-DBUILDING_PODOFO"
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
    ADD_LIBRARY(podofo_shared SHARED ${PODOFO_BASE_SOURCES} ${PODOFO_DOC_SOURCES})
    TARGET_LINK_LIBRARIES(podofo_shared ${PODOFO_LIB_DEPENDS})
    # TODO: set /wd4251 flag if we're doing a debug build with
    # Visual Studio, since it produces invalid warnings about STL
    # use.
    SET_TARGET_PROPERTIES(podofo_shared PROPERTIES
        VERSION "${PODOFO_LIBVERSION}"
        SOVERSION "${PODOFO_SOVERSION}"
        CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "podofo"
        COMPILE_FLAGS "-DBUILDING_PODOFO"
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




