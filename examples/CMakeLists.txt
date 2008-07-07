IF(BOOST_FOUND)
	SET(graph pdfcontentsgraph)
ELSE(BOOST_FOUND)
	SET(graph)
ENDIF(BOOST_FOUND)

SUBDIRS(
	helloworld
	${graph}
	)
