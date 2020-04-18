set_source_files_properties(main.cpp
    PROPERTIES
    COMPILE_DEFINITIONS DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN)
add_executable(test main.cpp)
target_link_libraries(test PRIVATE csv2::csv2)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/inputs
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})