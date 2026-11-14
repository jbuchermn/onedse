add_executable(onedse_server main.cpp)
target_link_libraries(onedse_server core)
install(TARGETS onedse_server RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
