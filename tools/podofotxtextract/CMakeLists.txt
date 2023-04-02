add_executable(podofotxtextract podofotxtextract.cpp)
target_link_libraries(podofotxtextract
    ${PODOFO_LIBRARIES}
    podofo_private
    tools_private
)
install(TARGETS podofotxtextract RUNTIME DESTINATION "bin")
