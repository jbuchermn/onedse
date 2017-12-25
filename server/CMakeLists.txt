cmake_minimum_required (VERSION 3.1)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/Modules)
project (wigner_web)

#
# Compiler
#
set (CMAKE_CXX_STANDARD 17)
add_compile_options(-Ofast -g -pg)

# Specify all includes from root
include_directories(${CMAKE_SOURCE_DIR}/main)


#
# Dependencies
#

# std::experimental::filesystem
list(APPEND dependencies_LIBRARIES "stdc++fs")

# Boost
find_package(Boost 1.53.0 COMPONENTS system thread filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
list(APPEND dependencies_LIBRARIES ${Boost_LIBRARIES})

# FFTW
find_package(FFTW REQUIRED)
include_directories(${FFTW_INCLUDE_DIR})
list(APPEND dependencies_LIBRARIES ${FFTW_LIBRARIES})

# Lua
find_package(LUA REQUIRED)
include_directories(${LUA_INCLUDE_DIR})
list(APPEND dependencies_LIBRARIES ${LUA_LIBRARIES})

# alglib
find_package(Alglib REQUIRED)
include_directories(${Alglib_INCLUDE_DIR})
list(APPEND dependencies_LIBRARIES ${Alglib_LIBRARIES})

# Header libraries
include_directories(${CMAKE_SOURCE_DIR}/dependencies/Simple-Web-Server)
include_directories(${CMAKE_SOURCE_DIR}/dependencies/json/src)
include_directories(${CMAKE_SOURCE_DIR}/dependencies/eigen)
include_directories(${CMAKE_SOURCE_DIR}/dependencies/luawrapper/include)

# GTest
if(NOT EXISTS ${CMAKE_BINARY_DIR}/ThirdParty/googletest-src)
    find_package(Git REQUIRED)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/ThirdParty/googletest-src)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/ThirdParty/googletest-bin)
    execute_process(COMMAND ${GIT_EXECUTABLE} clone https://github.com/google/googletest.git googletest-src WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/ThirdParty)
endif(NOT EXISTS ${CMAKE_BINARY_DIR}/ThirdParty/googletest-src)
add_subdirectory(${CMAKE_BINARY_DIR}/ThirdParty/googletest-src ThirdParty/googletest-bin)
include_directories(${CMAKE_BINARY_DIR}/ThirdParty/googletest-src/googletest/include)

#
# Internal
#
add_subdirectory(main/wigner_web)
file(GLOB_RECURSE test_SRC ${CMAKE_SOURCE_DIR}/test/**/*.cpp)

#
# Targets
#
set(main_TARGET wigner_webd)
set(test_TARGET test_${main_TARGET})

add_executable(${main_TARGET} 
    main/main.cpp
)
target_link_libraries(${main_TARGET} 
    wigner_web
    ${dependencies_LIBRARIES}
)

add_executable(${test_TARGET} 
    test/main.cpp 
    ${test_SRC}
)
target_link_libraries(${test_TARGET} 
    wigner_web
    ${dependencies_LIBRARIES} 
    gtest 
    gtest_main
)

add_custom_command(
    TARGET ${main_TARGET}
    COMMENT "Run tests"
    POST_BUILD 
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND ${test_TARGET}
)
