add_executable(podofopages podofopages.cpp
    DeleteOperation.cpp
    DeleteOperation.h
    MoveOperation.cpp
    MoveOperation.h
    Operation.h
)
target_link_libraries(podofopages tools_private)
install(TARGETS podofopages RUNTIME DESTINATION "bin")
