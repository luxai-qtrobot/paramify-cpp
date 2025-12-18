cmake_minimum_required(VERSION 3.16)
project(paramify_examples LANGUAGES CXX)

add_executable(example_yaml example_yaml.cpp)
add_executable(example_cli example_cli.cpp)
add_executable(example_programmatic example_programmatic.cpp)

target_link_libraries(example_yaml PRIVATE paramify::paramify)
target_link_libraries(example_cli PRIVATE paramify::paramify)
target_link_libraries(example_programmatic PRIVATE paramify::paramify)
