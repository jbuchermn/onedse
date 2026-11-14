cmake_minimum_required(VERSION 3.16)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)
project(onedse CXX)

include(GNUInstallDirs)

#
# Compiler
#
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
# Note: no -ffast-math, validate() relies on NaN checks
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

option(ONEDSE_BUILD_TESTS "Build unit tests (requires GTest)" ON)

# Specify all includes from root
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/main)

# Where lua examples and the web frontend end up after `make install`
set(ONEDSE_DATA_DIR "${CMAKE_INSTALL_FULL_DATADIR}/onedse")
add_compile_definitions(ONEDSE_DATA_DIR="${ONEDSE_DATA_DIR}")
add_compile_definitions(ONEDSE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/..")

#
# Dependencies
#
find_package(Threads REQUIRED)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()
find_package(Boost REQUIRED) # header-only parts (variant, optional, math)
find_package(Eigen3 REQUIRED NO_MODULE)
find_package(nlohmann_json REQUIRED)
find_package(httplib REQUIRED)
find_package(FFTW REQUIRED)
find_package(Lua REQUIRED)

# luawrapper is vendored (header-only, with a local fix, see third_party/luawrapper/README.md)
set(LUAWRAPPER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/luawrapper)

add_library(dependencies INTERFACE)
target_include_directories(dependencies SYSTEM INTERFACE
    ${FFTW_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
    ${LUAWRAPPER_INCLUDE_DIR}
)
target_link_libraries(dependencies INTERFACE
    Boost::boost
    Eigen3::Eigen
    nlohmann_json::nlohmann_json
    httplib::httplib
    Threads::Threads
    ${FFTW_LIBRARIES}
    ${LUA_LIBRARIES}
)

#
# Targets
#
add_subdirectory(main/core)
add_subdirectory(main/script)
add_subdirectory(main/server)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../lua/ DESTINATION ${ONEDSE_DATA_DIR}/lua FILES_MATCHING PATTERN "*.lua")
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../web/ DESTINATION ${ONEDSE_DATA_DIR}/web)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../python/ DESTINATION ${ONEDSE_DATA_DIR}/python FILES_MATCHING PATTERN "*.py")

if(ONEDSE_BUILD_TESTS)
    find_package(GTest)
    if(GTest_FOUND)
        enable_testing()
        add_subdirectory(test)
    else()
        message(WARNING "GTest not found - unit tests are not built")
    endif()
endif()
