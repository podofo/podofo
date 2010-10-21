cmake_minimum_required(VERSION 2.6)
IF(NOT PODOFO_MAIN_CMAKELISTS_READ)
MESSAGE(FATAL_ERROR "Run cmake on the CMakeLists.txt in the project root, not the one in the 'src' directory. You will need to delete CMakeCache.txt from the current directory.")
ENDIF(NOT PODOFO_MAIN_CMAKELISTS_READ)

SET(PODOFO_BASE_SOURCES
  PdfArray.cpp
  PdfCanvas.cpp
  PdfColor.cpp
  PdfContentsTokenizer.cpp
  PdfData.cpp
  PdfDataType.cpp
  PdfDate.cpp
  PdfDictionary.cpp
  PdfEncoding.cpp
  PdfEncodingFactory.cpp
  PdfEncrypt.cpp
  PdfError.cpp
  PdfFileStream.cpp
  PdfFilter.cpp
  PdfFiltersPrivate.cpp
  PdfImmediateWriter.cpp
  PdfInputDevice.cpp
  PdfInputStream.cpp
  PdfLocale.cpp
  PdfMemoryManagement.cpp
  PdfMemStream.cpp
  PdfName.cpp
  PdfObject.cpp
  PdfObjectStreamParserObject.cpp
  PdfOutputDevice.cpp
  PdfOutputStream.cpp
  PdfParser.cpp
  PdfParserObject.cpp
  PdfRect.cpp
  PdfRefCountedBuffer.cpp
  PdfRefCountedInputDevice.cpp
  PdfReference.cpp
  PdfRijndael.cpp
  PdfStream.cpp
  PdfString.cpp
  PdfTokenizer.cpp
  PdfVariant.cpp
  PdfVecObjects.cpp
  PdfWriter.cpp
  PdfXRef.cpp
  PdfXRefStream.cpp
  PdfXRefStreamParserObject.cpp
  )

IF(WIN32)
    # If we build for windows systems, we also include the resource file
    SET(PODOFO_BASE_SOURCES ${PODOFO_BASE_SOURCES} podofo-base.rc)
ENDIF(WIN32)

SET(PODOFO_BASE_HEADERS
    ${PoDoFo_BINARY_DIR}/podofo_config.h
   Pdf3rdPtyForwardDecl.h
   PdfArray.h
   PdfCanvas.h
   PdfColor.h
   PdfCompilerCompat.h
   PdfCompilerCompatPrivate.h
   PdfContentsTokenizer.h
   PdfData.h
   PdfDataType.h
   PdfDate.h
   PdfDefines.h
   PdfDefinesPrivate.h
   PdfDictionary.h
   PdfEncoding.h
   PdfEncodingFactory.h
   PdfEncrypt.h
   PdfError.h
   PdfFileStream.h
   PdfFilter.h
   PdfFiltersPrivate.h
   PdfImmediateWriter.h
   PdfInputDevice.h
   PdfInputStream.h
   PdfLocale.h
   PdfMemoryManagement.h
   PdfMemStream.h
   PdfName.h
   PdfObject.h
   PdfObjectStreamParserObject.h
   PdfOutputDevice.h
   PdfOutputStream.h
   PdfParser.h
   PdfParserObject.h
   PdfRect.h
   PdfRefCountedBuffer.h
   PdfRefCountedInputDevice.h
   PdfReference.h
   PdfRijndael.h
   PdfStream.h
   PdfString.h
   PdfTokenizer.h
   PdfVariant.h
   PdfVecObjects.h
   PdfVersion.h
   PdfWriter.h
   PdfXRef.h
   PdfXRefStream.h
   PdfXRefStreamParserObject.h
   )

SET(PODOFO_BASE_HEADERS2 
    util/PdfMutex.h
    util/PdfMutexImpl_noop.h
    util/PdfMutexImpl_win32.h
    util/PdfMutexImpl_pthread.h
    util/PdfMutexWrapper.h
    )

# Create a Source Group for Visual Studio
# so that headers are listed in the folder view
# and are easier accessible
SOURCE_GROUP(Headers FILES ${PODOFO_BASE_HEADERS} ${PODOFO_BASE_HEADERS2})

INSTALL(FILES ${PODOFO_BASE_HEADERS}
    DESTINATION "include/podofo/base"
    )

INSTALL(FILES ${PODOFO_BASE_HEADERS2}
    DESTINATION "include/podofo/base/util"
    )

IF(NOT PODOFO_BUILD_SHARED AND NOT PODOFO_BUILD_STATIC)
    MESSAGE(FATAL_ERROR "At least one of PODOFO_BUILD_SHARED and PODOF_BUILD_STATIC must be set")
ENDIF(NOT PODOFO_BUILD_SHARED AND NOT PODOFO_BUILD_STATIC)

IF(PODOFO_BUILD_STATIC)
    MESSAGE("Building static PoDoFo library")
    ADD_LIBRARY(podofo-base_static STATIC ${PODOFO_BASE_SOURCES})
    TARGET_LINK_LIBRARIES(podofo-base_static ${PODOFO_BASE_LIB_DEPENDS})
    SET_TARGET_PROPERTIES(podofo-base_static PROPERTIES
        VERSION "${PODOFO_LIBVERSION}"
        SOVERSION "${PODOFO_SOVERSION}"
        CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "podofo-base"
        COMPILE_FLAGS "-DBUILDING_PODOFO"
        )
    SET(PODOFO_BASE_DEPEND_TARGET podofo-base_static
        CACHE INTERNAL "Which PoDoFo library variant to depend on")
    SET(USING_SHARED_PODOFO FALSE)
    INSTALL(TARGETS podofo-base_static
        RUNTIME DESTINATION "bin"
        LIBRARY DESTINATION "${LIBDIRNAME}"
        ARCHIVE DESTINATION "${LIBDIRNAME}"
        )
ENDIF(PODOFO_BUILD_STATIC)

IF(PODOFO_BUILD_SHARED)
    MESSAGE("Building shared PoDoFo library")
    ADD_LIBRARY(podofo-base_shared SHARED ${PODOFO_BASE_SOURCES})
    TARGET_LINK_LIBRARIES(podofo-base_shared ${PODOFO_BASE_LIB_DEPENDS})
    # TODO: set /wd4251 flag if we're doing a debug build with
    # Visual Studio, since it produces invalid warnings about STL
    # use.
    SET_TARGET_PROPERTIES(podofo-base_shared PROPERTIES
        VERSION "${PODOFO_LIBVERSION}"
        SOVERSION "${PODOFO_SOVERSION}"
        CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "podofo-base"
        COMPILE_FLAGS "-DBUILDING_PODOFO"
        )
    # Since we're building a shared podofo, prefer to depend on this one for
    # tests and tools over the static library (if built).
    SET(PODOFO_BASE_DEPEND_TARGET podofo-base_shared
        CACHE INTERNAL "Which PoDoFo library variant to depend on")
    SET(USING_SHARED_PODOFO TRUE)
    INSTALL(TARGETS podofo-base_shared
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
     "SET(PODOFO_BASE_DEPEND_TARGET ${PODOFO_BASE_DEPEND_TARGET})\n"
     )
