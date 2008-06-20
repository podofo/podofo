
IF(HAVE_CPPUNIT)
  INCLUDE_DIRECTORIES( ${PROJ_SOURCE_DIR}/src ${PROJ_BINARY_DIR}/src ${PROJ_BINARY_DIR}/src/os ${PROJ_BINARY_DIR}/src/os/${OROCOS_TARGET})
  ADD_DEFINITIONS("-g")
  
  # repeat for each test
  ADD_EXECUTABLE( podofo-test main.cpp EncryptTest.cpp FilterTest.cpp NameTest.cpp TokenizerTest.cpp StringTest.cpp )
  ADD_DEPENDENCIES( podofo-test ${PODOFO_DEPEND_TARGET})
  TARGET_LINK_LIBRARIES( podofo-test ${PODOFO_LIB} ${PODOFO_LIB_DEPENDS} ${CPPUNIT_LIBRARIES} )
  SET_TARGET_PROPERTIES( podofo-test PROPERTIES COMPILE_FLAGS "${PODOFO_CFLAGS}")
  ADD_TEST( podofo-test podofo-test )
ENDIF(HAVE_CPPUNIT)
