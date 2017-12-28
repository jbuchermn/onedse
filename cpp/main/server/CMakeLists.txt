file(GLOB_RECURSE server_SRC ${CMAKE_CURRENT_SOURCE_DIR}/**/*.cpp)
set(server_TARGET onedse_server)
add_executable(${server_TARGET} 
    ${server_SRC}
    main.cpp
)
target_link_libraries(${server_TARGET}
    core
    ${dependencies_LIBRARIES}
)
