file(GLOB_RECURSE core_SRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(core STATIC ${core_SRC})
target_link_libraries(core PUBLIC dependencies)
