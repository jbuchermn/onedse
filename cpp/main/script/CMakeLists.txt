add_executable(onedse main.cpp)
target_link_libraries(onedse core)
install(TARGETS onedse RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
