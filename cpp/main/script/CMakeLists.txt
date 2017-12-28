set(script_TARGET onedse)
add_executable(${script_TARGET} main.cpp)
target_link_libraries(${script_TARGET}
    core
    ${dependencies_LIBRARIES}
)
