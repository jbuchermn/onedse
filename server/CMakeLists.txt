cmake_minimum_required (VERSION 3.1)
project (WebApp)

# Compiler
set (CMAKE_CXX_STANDARD 17)
add_compile_options(-Ofast -g -pg)

# TODO Needs to be up here...
link_directories("${CMAKE_SOURCE_DIR}/dependencies/alglib/alglib")

# Target
file(GLOB wigner_webd_SRC "wigner_web/**/*.cpp")
file(GLOB alglib_SRC "dependencies/alglib/alglib/*.cpp")
include_directories("${CMAKE_SOURCE_DIR}")
add_executable(wigner_webd main.cpp ${wigner_webd_SRC} ${alglib_SRC})
add_executable(wigner_webd_test main_test.cpp ${wigner_webd_SRC} ${alglib_SRC})

# Libraries
find_package(Boost 1.53.0 COMPONENTS system thread filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
target_link_libraries(wigner_webd ${Boost_LIBRARIES})
target_link_libraries(wigner_webd_test ${Boost_LIBRARIES})

include_directories("${CMAKE_SOURCE_DIR}/dependencies/alglib")

include_directories("${CMAKE_SOURCE_DIR}/dependencies/Simple-Web-Server")
include_directories("${CMAKE_SOURCE_DIR}/dependencies/json/src")
include_directories("${CMAKE_SOURCE_DIR}/dependencies/eigen")
