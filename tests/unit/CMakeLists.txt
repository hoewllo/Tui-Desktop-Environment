add_executable(test_buffer test_buffer.cpp ../../src/core/buffer.cpp)
target_include_directories(test_buffer PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_buffer COMMAND test_buffer)
