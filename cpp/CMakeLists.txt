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


#
# Targets
#
add_subdirectory(main/core)
add_subdirectory(main/script)
add_subdirectory(main/server)
add_subdirectory(test)
