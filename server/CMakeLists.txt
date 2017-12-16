cmake_minimum_required (VERSION 3.1)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
project (WebApp)

# Compiler
set (CMAKE_CXX_STANDARD 17)
add_compile_options(-Ofast -g -pg)

# Target
file(GLOB wigner_webd_SRC "wigner_web/**/*.cpp")

# Not the nicest of solutions
file(GLOB alglib_SRC "dependencies/alglib/alglib/*.cpp")
include_directories("${CMAKE_SOURCE_DIR}/dependencies/alglib")

# Specify all includes from root
include_directories("${CMAKE_SOURCE_DIR}")
add_executable(wigner_webd main.cpp ${wigner_webd_SRC} ${alglib_SRC})
add_executable(wigner_webd_test main_test.cpp ${wigner_webd_SRC} ${alglib_SRC})

# Boost
find_package(Boost 1.53.0 COMPONENTS system thread filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
target_link_libraries(wigner_webd ${Boost_LIBRARIES})
target_link_libraries(wigner_webd_test ${Boost_LIBRARIES})

# FFTW
find_package(FFTW REQUIRED)
include_directories(${FFTW_INCLUDE_DIR})
target_link_libraries(wigner_webd ${FFTW_LIBRARIES})
target_link_libraries(wigner_webd_test ${FFTW_LIBRARIES})


# Header libraries
include_directories("${CMAKE_SOURCE_DIR}/dependencies/Simple-Web-Server")
include_directories("${CMAKE_SOURCE_DIR}/dependencies/json/src")
include_directories("${CMAKE_SOURCE_DIR}/dependencies/eigen")
