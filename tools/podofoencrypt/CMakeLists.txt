add_executable(podofoencrypt podofoencrypt.cpp)
target_link_libraries(podofoencrypt tools_private)
install(TARGETS podofoencrypt RUNTIME DESTINATION "bin")
