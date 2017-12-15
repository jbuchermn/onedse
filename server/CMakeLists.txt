cmake_minimum_required (VERSION 3.1)
project (WebApp)

# Compiler
set (CMAKE_CXX_STANDARD 14)
add_compile_options(-Ofast -g -pg)

# Target
file(GLOB wigner_webd_SRC "wigner_web/*.cpp")
add_executable(wigner_webd main.cpp ${wigner_webd_SRC})

# Libraries
find_package(Boost 1.53.0 COMPONENTS system thread filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
target_link_libraries(wigner_webd ${Boost_LIBRARIES})

include_directories("${CMAKE_SOURCE_DIR}/dependencies/Simple-Web-Server")
include_directories("${CMAKE_SOURCE_DIR}/dependencies/json/src")
include_directories("${CMAKE_SOURCE_DIR}/dependencies/eigen/Eigen")
