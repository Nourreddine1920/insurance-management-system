add_executable(validation_tests test_validation.c)

target_include_directories(validation_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(validation_tests PRIVATE SQLite::SQLite3)

add_test(NAME validation_tests COMMAND validation_tests)
