CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

#*****************  IMPORTANT  ************* IMPORTANT **********************
# Look at http://www.vtk.org/Wiki/CMake_HowToDoPlatformChecks
# and the other wiki entries before you add anything. You might not need to.
#****************************************************************************

#
# Project name and version
#
PROJECT(PoDoFo)

SET(PODOFO_VERSION_MAJOR "0" CACHE STRING "Major part of PoDoFo version number")
SET(PODOFO_VERSION_MINOR "9" CACHE STRING "Minor part of PoDoFo version number")
SET(PODOFO_VERSION_PATCH "7" CACHE STRING "Patchlevel part of PoDoFo version number")
SET(PODOFO_SOVERSION "${PODOFO_VERSION_MAJOR}.${PODOFO_VERSION_MINOR}.${PODOFO_VERSION_PATCH}")
SET(PODOFO_LIBVERSION "${PODOFO_SOVERSION}")


#
# Main includes
#
INCLUDE(CheckCXXSourceCompiles)
INCLUDE(CheckIncludeFile)
INCLUDE(CheckLibraryExists)
INCLUDE(TestBigEndian)
INCLUDE(CheckTypeSize)

#
# Setup CMake Policies
#

# Prefer files in CMAKE_MODULE_PATH over shipped ones in module directory
CMAKE_POLICY(SET CMP0017 NEW) # https://cmake.org/cmake/help/v3.0/policy/CMP0017.html
# Do not use export_library_dependencies() anymore
if(POLICY CMP0033)
CMAKE_POLICY(SET CMP0033 NEW) # https://cmake.org/cmake/help/v3.0/policy/CMP0033.html
endif()

# Load modules from our source tree too
SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")


# Builds must use this CMakeLists.txt, not the one in src/ or somewhere else.
# If users try to use something else the results can be confusing. We set a
# variable here that we require to be set elsewhere, otherwise we'll complain.
SET(PODOFO_MAIN_CMAKELISTS_READ TRUE)

# If the user hasn't told use specifically what they want, build only
# a static library or only a shared library on Windows.
IF(NOT DEFINED PODOFO_BUILD_SHARED AND NOT DEFINED PODOFO_BUILD_STATIC)
    IF(WIN32)
        SET(PODOFO_BUILD_STATIC FALSE)
        SET(PODOFO_BUILD_SHARED TRUE)
    ELSE(WIN32)
        SET(PODOFO_BUILD_STATIC TRUE)
        SET(PODOFO_BUILD_SHARED FALSE)
    ENDIF(WIN32)
ENDIF(NOT DEFINED PODOFO_BUILD_SHARED AND NOT DEFINED PODOFO_BUILD_STATIC)
IF(DEFINED PODOFO_BUILD_SHARED AND NOT DEFINED PODOFO_BUILD_STATIC)
    IF(PODOFO_BUILD_SHARED)
        SET(PODOFO_BUILD_STATIC FALSE)
    ELSE(PODOFO_BUILD_SHARED)
        SET(PODOFO_BUILD_STATIC TRUE)
    ENDIF(PODOFO_BUILD_SHARED)
ENDIF(DEFINED PODOFO_BUILD_SHARED AND NOT DEFINED PODOFO_BUILD_STATIC)
IF(NOT DEFINED PODOFO_BUILD_SHARED AND DEFINED PODOFO_BUILD_STATIC)
    IF(PODOFO_BUILD_STATIC)
        SET(PODOFO_BUILD_SHARED FALSE)
    ELSE(PODOFO_BUILD_STATIC)
        SET(PODOFO_BUILD_SHARED TRUE)
    ENDIF(PODOFO_BUILD_STATIC)
ENDIF(NOT DEFINED PODOFO_BUILD_SHARED AND DEFINED PODOFO_BUILD_STATIC)

IF(DEFINED LIB_SUFFIX)
	SET(LIBDIRNAME "lib${LIB_SUFFIX}")
ELSE(DEFINED LIB_SUFFIX)

# Some 64 bit linux distros use /usr/lib64 for 64 bit libraries.
# on these platforms we must
IF(NOT DEFINED WANT_LIB64)
	# TODO: detect 64-bit build and existance of /usr/lib64 and set to TRUE.
	MESSAGE("WANT_LIB64 unset; assuming normal library directory names")
	SET(WANT_LIB64 FALSE)
ENDIF(NOT DEFINED WANT_LIB64)

IF(WANT_LIB64)
	SET(LIBDIRNAME "lib64")
ELSE(WANT_LIB64)
	SET(LIBDIRNAME "lib")
ENDIF(WANT_LIB64)

ENDIF(DEFINED LIB_SUFFIX)
MESSAGE("Will install libraries to ${CMAKE_INSTALL_PREFIX}/${LIBDIRNAME}")

CHECK_CXX_SOURCE_COMPILES("#include <memory>
			class Cls {
			public:
			Cls() {}
			~Cls() {}
			};
			int main(void) { std::unique_ptr<Cls> cls; return 0; }" PODOFO_HAVE_UNIQUE_PTR)

# Some headers that tend to vary a bit
CHECK_INCLUDE_FILE("strings.h" PODOFO_HAVE_STRINGS_H) 
CHECK_INCLUDE_FILE("arpa/inet.h" PODOFO_HAVE_ARPA_INET_H) 
CHECK_INCLUDE_FILE("winsock2.h" PODOFO_HAVE_WINSOCK2_H) 
CHECK_INCLUDE_FILE("mem.h" PODOFO_HAVE_MEM_H) 
CHECK_INCLUDE_FILE("ctype.h" PODOFO_HAVE_CTYPE_H) 

# Do some type size detection and provide yet another set of typedefs for fixed
# font sizes. We can't use the c99 / c++0x uint32_t etc, because people use
# ancient compilers that don't and will never support the standard.

CHECK_INCLUDE_FILE("sys/types.h" PODOFO_HAVE_SYS_TYPES_H) 
CHECK_INCLUDE_FILE("stdint.h" PODOFO_HAVE_STDINT_H) 
# See: http://msdn.microsoft.com/en-us/library/aa384264(VS.85).aspx
CHECK_INCLUDE_FILE("BaseTsd.h" PODOFO_HAVE_BASETSD_H) 

CHECK_TYPE_SIZE("long int" SZ_LONG)

# We cache integer type detection results, and don't repeat them
# (and overwrite the user's manual changes) if they've been done already.
IF(NOT PDF_INT64_TYPENAME)
  # I hate old compilers.
  IF(PODOFO_HAVE_STDINT_H)
    SET(pdfint8 "int8_t")
    SET(pdfint16 "int16_t")
    SET(pdfint32 "int32_t")
    SET(pdfint64 "int64_t")
    SET(pdfuint8 "uint8_t")
    SET(pdfuint16 "uint16_t")
    SET(pdfuint32 "uint32_t")
    SET(pdfuint64 "uint64_t")
    CHECK_TYPE_SIZE("int64_t" SZ_INT64)
  ELSE(PODOFO_HAVE_STDINT_H)
    # No stdint.h . Try BaseTsd.h windows types.
    IF(PODOFO_HAVE_BASETSD_H)
       # We have BaseTsd.h, so use those types.
       SET(pdfint8  "signed char")
       SET(pdfint16 "short")
       SET(pdfint32 "INT32")
       SET(pdfint64 "INT64")
       SET(pdfuint8 "unsigned char")
       SET(pdfuint16 "unsigned short")
       SET(pdfuint32 "UINT32")
       SET(pdfuint64 "UINT64")
       CHECK_TYPE_SIZE("INT64" SZ_INT64)
    ELSE(PODOFO_HAVE_BASETSD_H)
       # No BaseTsd.h either. Assume the standard types, and go poking
       # for a sane 64-bit integer.
       #
       # First, though, make sure sizeof(int) = 4 and if not, scream, because
       # hopefully this case will never be hit and we'll never have to write
       # the horrible code check for
       CHECK_TYPE_SIZE("signed char"        SZ_TINY_INT)
       CHECK_TYPE_SIZE("unsigned char"      SZ_TINY_UINT)
       CHECK_TYPE_SIZE("short int"          SZ_SHORT_INT)
       CHECK_TYPE_SIZE("int"                SZ_INT)
       CHECK_TYPE_SIZE("unsigned short int" SZ_UINT)
       CHECK_TYPE_SIZE("unsigned int"       SZ_SHORT_UINT)
       SET(smallintsok SZ_INT == 4 AND SZ_UINT == 4 AND SZ_SHORT_INT == 2 AND SZ_SHORT_UINT == 2 AND SZ_TINY_INT == 1 AND SZ_TINY_UINT == 1)
       IF(NOT smallintsok)
         MESSAGE(FATAL "sizeof(int) != 4 and/or sizeof(short) != 2 and no stdint.h or BaseTsd.h found. We don't know how to cope with this.")
       ENDIF(NOT smallintsok)
       SET(pdfint8 "signed char")
       SET(pdfint16 "short")
       SET(pdfint32 "int")
       SET(pdfuint8 "unsigned char")
       SET(pdfuint16 "unsigned short")
       SET(pdfuint32 "unsigned int")
       # Now we just have to figure out what 64-bit integer type we can use.
       #
       # Do we have VC >= 6's __uint64 and __int64?
       # See: http://icfun.blogspot.com/2008/04/use-of-int64-variable-in-c.html
       CHECK_TYPE_SIZE("__int64" SZ___INT64)
       CHECK_TYPE_SIZE("__uint64"  SZ___UINT64)
       IF(SZ___INT64 == 8 AND SZ___UINT64 == 8)
         # MS compiler, VC6 or newer without BaseTsd.h in SDK
         SET(pdfint64 "__int64")
         SET(pdfuint64 "__uint64")
	 CHECK_TYPE_SIZE("__int64" SZ_INT64)
       ELSE(SZ___INT64 AND SZ___UINT64)
         # Still no luck. Old unix compiler, Borland, or some other monster?
         # Are we lucky and sizeof(long) == 8?
         CHECK_TYPE_SIZE("unsigned long int" SZ_ULONG)
         IF(SZ_LONG == 8 AND SZ_ULONG == 8)
           # Must be on a LP64 platform, sizeof(long) = 8
           SET(pdfint64 "long int")
           SET(pdfuint64 "unsigned long int")
	   CHECK_TYPE_SIZE("long int" SZ_INT64)
         ELSE(SZ_LONG == 8 AND SZ_ULONG == 8)
           # See if the compiler implements "long long int", int64_t, int64,
           # or _int64 (in this order, __int64 check already done)
           IF(PODOFO_HAVE_SYS_TYPES_H)
             SET(CMAKE_EXTRA_INCLUDE_FILES "sys/types.h")
           ENDIF(PODOFO_HAVE_SYS_TYPES_H)
           CHECK_TYPE_SIZE("long long int"          SZ_LONG_LONG)
           CHECK_TYPE_SIZE("unsigned long long int" SZ_UNSIGNED_LONG_LONG)
           IF(SZ_LONG_LONG == 8 AND SZ_UNSIGNED_LONG_LONG == 8)
             SET(pdfint64 "long long int")
             SET(pdfuint64 "unsigned long long int")
             CHECK_TYPE_SIZE("long long int" SZ_INT64)
           ELSE(SZ_LONG_LONG == 8 AND SZ_UNSIGNED_LONG_LONG == 8)
             # The standard type int64_t is normally found in stdint.h,
             # but use a type of this name anyway if present
             CHECK_TYPE_SIZE("int64_t"       SZ_INT64_T)
             CHECK_TYPE_SIZE("uint64_t"      SZ_UINT64_T)
             IF(SZ_INT64_T AND SZ_UINT64_T)
               SET(pdfint64 "int64_t")
               SET(pdfuint64 "uint64_t")
	       CHECK_TYPE_SIZE("int64_t" SZ_INT64)
             ELSE(SZ_INT64_T AND SZ_UINT64_T)
               CHECK_TYPE_SIZE("int64"                SZ_INT64)
               CHECK_TYPE_SIZE("uint64"               SZ_UINT64)
               IF(SZ_INT64 AND SZ_UINT64)
                 SET(pdfint64 "int64")
                 SET(pdfuint64 "uint64")
		 CHECK_TYPE_SIZE("int64" SZ_INT64)
               ELSE(SZ_INT64 AND SZ_UINT64)
                 CHECK_TYPE_SIZE("_int64"                SZ__INT64)
                 CHECK_TYPE_SIZE("_uint64"               SZ__UINT64)
                 IF(SZ__INT64 AND SZ__UINT64)
                   SET(pdfint64 "_int64")
                   SET(pdfuint64 "_uint64")
		   CHECK_TYPE_SIZE("_int64" SZ_INT64)
                 ELSE(SZ__INT64 AND SZ__UINT64)
                   MESSAGE("Oh my lord, your compiler doesn't seem to support any of the int64 type flavours we tried.")
                   MESSAGE(WARNING "You must set the PDF_INTxx_TYPENAME and PDF_UINTxx_TYPENAME variables manually")
                 ENDIF(SZ__INT64 AND SZ__UINT64)
               ENDIF(SZ_INT64 AND SZ_UINT64)
             ENDIF(SZ_INT64_T AND SZ_UINT64_T)
           ENDIF(SZ_LONG_LONG == 8 AND SZ_UNSIGNED_LONG_LONG == 8)
         ENDIF(SZ_LONG == 8 AND SZ_ULONG == 8)
       ENDIF(SZ___INT64 AND SZ___UINT64)
    ENDIF(PODOFO_HAVE_BASETSD_H)
  ENDIF(PODOFO_HAVE_STDINT_H)
  SET(CMAKE_EXTRA_INCLUDE_FILES)
ENDIF(NOT PDF_INT64_TYPENAME)

IF(NOT PDF_INT64_TYPENAME AND pdfint64)
  SET(PDF_INT64_TYPENAME  "${pdfint64}" CACHE STRING "Name of detected 64-bit signed integer type to use")
  SET(PDF_INT32_TYPENAME  "${pdfint32}" CACHE STRING "Name of detected 32-bit signed integer type to use")
  SET(PDF_INT16_TYPENAME  "${pdfint16}" CACHE STRING "Name of detected 16-bit signed integer type to use")
  SET(PDF_INT8_TYPENAME   "${pdfint8}" CACHE STRING "Name of detected 8-bit signed integer type to use")
  SET(PDF_UINT64_TYPENAME "${pdfuint64}" CACHE STRING "Name of detected 64-bit unsigned integer type to use")
  SET(PDF_UINT32_TYPENAME "${pdfuint32}" CACHE STRING "Name of detected 32-bit unsigned integer type to use")
  SET(PDF_UINT16_TYPENAME "${pdfuint16}" CACHE STRING "Name of detected 16-bit unsigned integer type to use")
  SET(PDF_UINT8_TYPENAME  "${pdfuint8}" CACHE STRING "Name of detected 8-bit unsigned integer type to use")
ENDIF(NOT PDF_INT64_TYPENAME AND pdfint64)

# Linux packagers want an uninstall target.
CONFIGURE_FILE(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall "${CMAKE_COMMAND}"
    -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

# Check if we are big endian
TEST_BIG_ENDIAN(TEST_BIG)

IF(WIN32)
    # We must explicitly link to the core win32 libraries, and we need winsock2
    # to get some byte-order conversion routines too.
    SET(PLATFORM_SYSTEM_LIBRARIES kernel32 user32 gdi32 winspool comdlg32 advapi32 shell32 ole32 oleaut32 uuid ws2_32)
    # Microsoft deprecate certain POSIX functions that we use.
    # for now, turn off these warnings.
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
    # We need a fake unistd.h for some libraries to build. They try to include <unistd.h>
    # which is not available under win32 with MSVC++, but everything in unistd.h is defined,
    # so an empty file solves the issue.
    SET(EXTRA_INCLUDES ${PoDoFo_SOURCE_DIR}/vcincludes)
ELSE(WIN32)
    SET(PLATFORM_SYSTEM_LIBRARIES)
    SET(EXTRA_INCLUDES)
ENDIF(WIN32)

IF(UNIX AND NOT PODOFO_NO_FONTMANAGER)
    SET(WANT_FONTCONFIG TRUE CACHE INTERNAL
        "True if PoDoFo should be built with fontconfig support")
ELSE(UNIX AND NOT PODOFO_NO_FONTMANAGER)
    SET(WANT_FONTCONFIG FALSE CACHE INTERNAL
        "True if PoDoFo should be built with fontconfig support")
ENDIF(UNIX AND NOT PODOFO_NO_FONTMANAGER)


IF(CMAKE_COMPILER_IS_GNUCXX)
    MESSAGE("Using gcc specific compiler options")
    # We can be more specific about what we want out of g++
    # than with most other compilers.

    # If the user hasn't specifically said whether they want
    # -fvisibility=hidden or not, turn it on if it's said to
    # be supported, off for other gcc versions.
    IF(NOT DEFINED PODOFO_USE_VISIBILITY)
        SET(PODOFO_USE_VISIBILITY ${PODOFO_HAVE_GCC_SYMBOL_VISIBILITY})
    ENDIF(NOT DEFINED PODOFO_USE_VISIBILITY)

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Woverloaded-virtual -Wswitch -Wcast-qual -Wwrite-strings -Wredundant-decls -Wreorder -Wno-deprecated-declarations")

    #
    # Note that we do not need debug definitions here. Set
    # -DCMAKE_BUILD_TYPE=debug or (if you want an optimised
    # release build with debug info) -DCMAKE_CXX_FLAGS="-g3"
    #
    # We add -W unless we're using gcc on win32, where it produces
    # spurious warnings about dllimport of inlines because of a dllimport
    # declaration on the whole class.
    IF(NOT WIN32)
        ADD_DEFINITIONS(-W)
    ENDIF(NOT WIN32)
    # If they've enabled the use of gcc4 symbol visibility, use it.
    IF(PODOFO_USE_VISIBILITY)
        ADD_DEFINITIONS(
            -DPODOFO_HAVE_GCC_SYMBOL_VISIBILITY
            -fvisibility=hidden
            )
    ENDIF(PODOFO_USE_VISIBILITY)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

FIND_PACKAGE(ZLIB REQUIRED)
MESSAGE("Found zlib headers in ${ZLIB_INCLUDE_DIR}, library at ${ZLIB_LIBRARIES}")

FIND_PACKAGE(LIBCRYPTO)

IF(LIBCRYPTO_FOUND)
	SET(PODOFO_HAVE_OPENSSL TRUE)
	INCLUDE_DIRECTORIES(${LIBCRYPTO_INCLUDE_DIR})
	MESSAGE("Found OpenSSL's libCrypto headers in ${LIBCRYPTO_INCLUDE_DIR}, library at ${LIBCRYPTO_LIBRARIES}")
ELSE(LIBCRYPTO_FOUND)
	MESSAGE("OpenSSL's libCrypto not found. Encryption support will be disabled")
ENDIF(LIBCRYPTO_FOUND)

FIND_PACKAGE(LIBIDN)

IF(LIBIDN_FOUND)
  MESSAGE("Found libidn headers in ${LIBIDN_INCLUDE_DIR}, library at ${LIBIDN_LIBRARIES}")
ENDIF(LIBIDN_FOUND)

IF(LIBIDN_FOUND)
	SET(PODOFO_HAVE_LIBIDN TRUE)
	INCLUDE_DIRECTORIES(${LIBIDN_INCLUDE_DIR})
	MESSAGE("Libidn found. AES-256 Encryption support will be enabled")
ELSE(LIBIDN_FOUND)
	MESSAGE("Libidn not found. AES-256 Encryption support will be disabled")
ENDIF(LIBIDN_FOUND)

FIND_PACKAGE(LIBJPEG)

IF(LIBJPEG_FOUND)
  MESSAGE("Found libjpeg headers in ${LIBJPEG_INCLUDE_DIR}, library at ${LIBJPEG_LIBRARIES}")
  SET(PODOFO_HAVE_JPEG_LIB TRUE)
  INCLUDE_DIRECTORIES(${LIBJPEG_INCLUDE_DIR})
ELSE(LIBJPEG_FOUND)
  MESSAGE("Libjpeg not found. JPEG support will be disabled")
ENDIF(LIBJPEG_FOUND)

FIND_PACKAGE(TIFF)

IF(TIFF_FOUND)
  MESSAGE("Found libtiff headers in ${TIFF_INCLUDE_DIR}, library at ${TIFF_LIBRARIES}")
  SET(PODOFO_HAVE_TIFF_LIB TRUE)
  INCLUDE_DIRECTORIES(${TIFF_INCLUDE_DIR})
ELSE(TIFF_FOUND)
  MESSAGE("Libtiff not found. TIFF support will be disabled")
ENDIF(TIFF_FOUND)

FIND_PACKAGE(PNG)

IF(PNG_FOUND)
  MESSAGE("Found LibPng headers in ${PNG_INCLUDE_DIR}, library at ${PNG_LIBRARIES}")
  SET(PODOFO_HAVE_PNG_LIB TRUE)
  INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIR})
ELSE(PNG_FOUND)
  MESSAGE("LibPng not found. PNG support will be disabled")
  SET(PNG_LIBRARIES "")
ENDIF(PNG_FOUND)

FIND_PACKAGE(UNISTRING)

IF(UNISTRING_FOUND)
  MESSAGE("Found LibUnistring headers in ${UNISTRING_INCLUDE_DIR}, library at ${UNISTRING_LIBRARY}")
  SET(PODOFO_HAVE_UNISTRING_LIB TRUE)
  INCLUDE_DIRECTORIES(${UNISTRING_INCLUDE_DIR})
ELSE(UNISTRING_FOUND)
  MESSAGE("LibUnistring not found. Unistring support will be disabled")
  SET(UNISTRING_LIBRARY "")
ENDIF(UNISTRING_FOUND)


IF(NOT PODOFO_BUILD_LIB_ONLY)

FIND_PACKAGE(CppUnit)

IF(CppUnit_FOUND)
  MESSAGE("Found cppunit. Unit tests will be built.")
  SET(PODOFO_HAVE_CPPUNIT CppUnit_FOUND)
  INCLUDE_DIRECTORIES(${CPPUNIT_INCLUDE_DIR})
ELSE(CppUnit_FOUND)
  MESSAGE("Cppunit not found. No unit tests will be built.")
ENDIF(CppUnit_FOUND)

ENDIF(NOT PODOFO_BUILD_LIB_ONLY)

FIND_PACKAGE(OpenSSL)

FIND_PACKAGE(FREETYPE REQUIRED)
MESSAGE("Found freetype library at ${FREETYPE_LIBRARIES}, headers ${FREETYPE_INCLUDE_DIR}")

FIND_PACKAGE(LIBSTLPORT)
SET(stlport_libraries_if_use_stlport)
IF(USE_STLPORT)
	IF(LIBSTLPORT_FOUND)
		MESSAGE("Using STLPort")
		INCLUDE_DIRECTORIES(${LIBSTLPORT_HEADERS})
		LINK_DIRECTORIES(${LIBSTLPORT_LIB})
		SET(stlport_libraries_if_use_stlport stlport)
		# Use the threaded STLPort
		ADD_DEFINITIONS(-D_PTHREADS)
	ELSE(LIBSTLPORT_FOUND)
		MESSAGE(FATAL_ERROR "STLPort use requested, but STLPort not found.")
	ENDIF(LIBSTLPORT_FOUND)
ENDIF(USE_STLPORT)

IF(WANT_FONTCONFIG)
	FIND_PACKAGE(FONTCONFIG REQUIRED)
	SET(PODOFO_HAVE_FONTCONFIG TRUE)
	SET(PODOFO_LIB_FONTCONFIG:STRING fontconfig)
	IF(FONTCONFIG_FOUND)
	  MESSAGE("Found fontconfig headers in ${FONTCONFIG_INCLUDE_DIR}, library at ${FONTCONFIG_LIBRARIES}")
	ELSE(FONTCONFIG_FOUND)
	  MESSAGE("Could not find fontconfig.")
        ENDIF(FONTCONFIG_FOUND)
ELSE(WANT_FONTCONFIG)
	# Might as well look for it anyway. This also sets the appropriate
	# variables to empty values.
	FIND_PACKAGE(FONTCONFIG)
	SET(PODOFO_LIB_FONTCONFIG:STRING)
ENDIF(WANT_FONTCONFIG)

IF(NOT PODOFO_BUILD_LIB_ONLY)
FIND_PACKAGE(LUA)
IF(LUA_FOUND)
	# If we have lua, we can build podofoimpose.
	MESSAGE("Lua found - PoDoFoImpose and PoDoFoColor will be built with Lua support")
	MESSAGE(" * Lua include directory: ${LUA_INCLUDE_DIR}")
	MESSAGE(" * Lua libraries: ${LUA_LIBRARIES}")
	INCLUDE_DIRECTORIES(${LUA_INCLUDE_DIR})
	SET(PODOFO_HAVE_LUA TRUE)
ELSE(LUA_FOUND)
	MESSAGE("Lua not found - PoDoFoImpose and PoDoFoColor will be built without Lua support")
ENDIF(LUA_FOUND)
ENDIF(NOT PODOFO_BUILD_LIB_ONLY)


# Check if we should build a multithreaded version of PoDoFo
IF(DEFINED PODOFO_NO_MULTITHREAD)
  MESSAGE("Building non multithreaded version of PoDoFo.")
  SET(PODOFO_MULTI_THREAD FALSE)
ELSE(DEFINED PODOFO_NO_MULTITHREAD)
  MESSAGE("Building multithreaded version of PoDoFo.")
  SET(PODOFO_MULTI_THREAD TRUE)
  FIND_PACKAGE(Threads)
  SET(PLATFORM_SYSTEM_LIBRARIES ${CMAKE_THREAD_LIBS_INIT} ${PLATFORM_SYSTEM_LIBRARIES})
ENDIF(DEFINED PODOFO_NO_MULTITHREAD)

IF(WANT_BOOST)
    MESSAGE("Looking optional for Boost.")
    MESSAGE("Boost is optional, so don't worry if it is not found.")
    MESSAGE("Set the BOOST_ROOT env var if you have problems.")
    FIND_PACKAGE(Boost)
    IF(BOOST_FOUND)
       SET(PODOFO_HAVE_BOOST TRUE)
       INCLUDE_DIRECTORIES(${BOOST_INCLUDE_DIR})
    ELSE(BOOST_FOUND)
       MESSAGE("If you don't need graph support you can ignore the above error.")
    ENDIF(BOOST_FOUND)
ENDIF(WANT_BOOST)

INCLUDE_DIRECTORIES(BEFORE # before toolchain include dir (to ignore installed)
    ${PoDoFo_SOURCE_DIR}/src   # order will be reversed, so this is the second dir
    ${PoDoFo_BINARY_DIR}   # because of BEFORE, this is the first include dir
    )

INCLUDE_DIRECTORIES(
    ${PoDoFo_SOURCE_DIR}/src/podofo
    ${FREETYPE_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${EXTRA_INCLUDES}
     )
LINK_DIRECTORIES(
    ${PoDoFo_BINARY_DIR}/src/podofo
    )

#
# The PoDoFo library needs to be linked to these libraries,
# as do any apps or libraries linking to PoDoFo. PODOFO_LIB
# will include these and the correct podofo target, so clients
# should specify only PODOFO_LIB .
#
# The LIBCRYPTO_LDFLAGS is defined only if the libcrypto was found
# by pkg-config, otherwise it will be empty. It can contain proper
# path for the libcrypto too, thus it's added here.
#
SET(PODOFO_LIB_DEPENDS
  ${ZLIB_LIBRARIES}
  ${LIBIDN_LIBRARIES}
  ${LIBCRYPTO_LDFLAGS}
  ${LIBCRYPTO_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${LIBJPEG_LIBRARIES}
  ${PLATFORM_SYSTEM_LIBRARIES}
  ${stlport_libraries_if_use_stlport}
  ${FREETYPE_LIBRARIES}
  ${PNG_LIBRARIES}
  ${TIFF_LIBRARIES}
  ${UNISTRING_LIBRARY}
  )

IF(FONTCONFIG_FOUND AND WANT_FONTCONFIG)
  SET(PODOFO_LIB_DEPENDS ${FONTCONFIG_LIBRARIES} ${PODOFO_LIB_DEPENDS})
  INCLUDE_DIRECTORIES(${FONTCONFIG_INCLUDE_DIR})
ENDIF(FONTCONFIG_FOUND AND WANT_FONTCONFIG)

IF(WIN32 OR PODOFO_BUILD_STATIC)
  SET(PODOFO_LIB
      podofo
      ${PODOFO_LIB_DEPENDS}
      )
ELSE(WIN32 OR PODOFO_BUILD_STATIC)
  SET(PODOFO_LIB podofo
      ${stlport_libraries_if_use_stlport}
      )
ENDIF(WIN32 OR PODOFO_BUILD_STATIC)

#
# Setup directories we will need
#
SET(MANDIR "share/man/")

# Create the config file. It'll be appended to as the subdirs run though
# then dependency information will be written to it at the end of the
# build.
FILE(WRITE
     "${PoDoFo_BINARY_DIR}/PoDoFoConfig.cmake"
     "# CMake module for PoDoFo\n"
     )
FILE(APPEND 
     "${PoDoFo_BINARY_DIR}/PoDoFoConfig.cmake"
     "SET(PODOFO_INCLUDES ${PoDoFo_SOURCE_DIR}/src)\n"
     )

ADD_SUBDIRECTORY(src/podofo)
IF(NOT PODOFO_BUILD_LIB_ONLY)
ADD_SUBDIRECTORY(test)
ADD_SUBDIRECTORY(tools)
ADD_SUBDIRECTORY(examples)
ADD_SUBDIRECTORY(man)
ENDIF(NOT PODOFO_BUILD_LIB_ONLY)

# Generate our configure file
CONFIGURE_FILE(${PoDoFo_SOURCE_DIR}/podofo_config.h.in ${PoDoFo_BINARY_DIR}/podofo_config.h)

# Export some variables into the config file so it's easier for others
# to build and link against PoDoFo

# To use these dependencies set PODOFO_DIR to the podofo BUILD directory in
# your build (eg -DPODOFO_DIR=/path/to/podofo when running cmake to configure
# the app that'll use podofo). See: FIND_PACKAGE(...) in the cmake docs.
IF(PODOFO_BUILD_SHARED)
  EXPORT(TARGETS podofo_shared FILE "${CMAKE_CURRENT_BINARY_DIR}/PoDoFoConfig.cmake")
ENDIF(PODOFO_BUILD_SHARED)
IF(PODOFO_BUILD_STATIC)
  EXPORT(TARGETS podofo_static FILE "${CMAKE_CURRENT_BINARY_DIR}/PoDoFoConfig.cmake")
ENDIF(PODOFO_BUILD_STATIC)

