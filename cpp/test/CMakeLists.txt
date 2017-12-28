# GTest
if(NOT EXISTS ${CMAKE_BINARY_DIR}/ThirdParty/googletest-src)
    find_package(Git REQUIRED)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/ThirdParty/googletest-src)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/ThirdParty/googletest-bin)
    execute_process(COMMAND ${GIT_EXECUTABLE} clone https://github.com/google/googletest.git googletest-src WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/ThirdParty)
endif(NOT EXISTS ${CMAKE_BINARY_DIR}/ThirdParty/googletest-src)
add_subdirectory(${CMAKE_BINARY_DIR}/ThirdParty/googletest-src ThirdParty/googletest-bin)
include_directories(${CMAKE_BINARY_DIR}/ThirdParty/googletest-src/googletest/include)

file(GLOB_RECURSE test_SRC ${CMAKE_SOURCE_DIR}/test/**/*.cpp)

#
# Targets
#
set(test_TARGET onedse_test)
add_executable(${test_TARGET} 
    main.cpp 
    ${test_SRC}
)
target_link_libraries(${test_TARGET} 
    core
    ${dependencies_LIBRARIES} 
    gtest 
    gtest_main
)
