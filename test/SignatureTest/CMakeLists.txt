SET(sign_srcs
  SignTest.cpp
  )

#IF(FALSE)
#  SET(sign_extra_libs -L/usr/lib -lssl3 -lsmime3 -lnss3 -lnssutil3)
#  SET(sign_srcs ${sign_srcs} NSSSignatureGenerator.cpp)
#  ADD_DEFINITIONS(-I/usr/include/nss -I/usr/include/nspr )
#ENDIF(FALSE)

ADD_EXECUTABLE(SignatureTest ${sign_srcs})

TARGET_LINK_LIBRARIES(SignatureTest ${PODOFO_LIB} ${PODOFO_LIB_DEPENDS} ${sign_extra_libs} )
SET_TARGET_PROPERTIES(SignatureTest PROPERTIES COMPILE_FLAGS "${PODOFO_CFLAGS}")

ADD_DEPENDENCIES(SignatureTest ${PODOFO_DEPEND_TARGET})
