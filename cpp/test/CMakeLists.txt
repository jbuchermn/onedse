file(GLOB_RECURSE test_SRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_executable(onedse_test main.cpp ${test_SRC})
target_link_libraries(onedse_test core GTest::gtest)

add_test(NAME unit COMMAND onedse_test)

# Every Lua example has to run through headless
file(GLOB examples CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/../lua/*.lua)
foreach(example ${examples})
    get_filename_component(name ${example} NAME_WE)
    add_test(NAME example_${name} COMMAND onedse --check ${example})
endforeach()
