
SET(PKIBOX_SOURCES
    Exception.cpp
    InvalidArgumentException.cpp
    PKIBox.cpp
    asn1/ObjectID.cpp
    openssl/Globals.cpp
    utils/ByteArray.cpp
)

SET(PKIBOX_HEADERS
    Exception.h
    InvalidArgumentException.h
    PKIBox.h
    asn1/ObjectID.h
    openssl/Globals.h
    utils/ByteArray.h
)

INSTALL(FILES ${PKIBOX_HEADERS}
    DESTINATION "include/podofo/PKIBox"
    )

 
MESSAGE("Building static PKIBox library")
ADD_LIBRARY(pkibox_static STATIC ${PKIBOX_SOURCES})
TARGET_LINK_LIBRARIES(pkibox_static ${PKIBOX_LIB_DEPENDS})

SET_TARGET_PROPERTIES(pkibox_static PROPERTIES
        VERSION "0.0.1"
        SOVERSION "0.0.1"
        CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "pkibox"
        )

SET(PKIBOX_DEPEND_TARGET pkibox_static
  CACHE INTERNAL "Which PKIBox library variant to depend on")

INSTALL(TARGETS pkibox_static
  RUNTIME DESTINATION "bin"
  LIBRARY DESTINATION "${LIBDIRNAME}"
  ARCHIVE DESTINATION "${LIBDIRNAME}"
  )
