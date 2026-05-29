# PDF Portfolio examples demonstrating the experimental Collection API

add_compile_options(${PODOFO_CFLAGS})

add_executable(create_portfolio create_portfolio.cpp)
target_link_libraries(create_portfolio ${PODOFO_LIBRARIES} podofo_private podofo_3rdparty)

add_executable(read_portfolio read_portfolio.cpp)
target_link_libraries(read_portfolio ${PODOFO_LIBRARIES} podofo_private podofo_3rdparty)
