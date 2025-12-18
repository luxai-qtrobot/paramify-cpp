cmake_minimum_required(VERSION 3.13)

include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.13.10
)
FetchContent_MakeAvailable(Catch2)
include("${Catch2_SOURCE_DIR}/extras/Catch.cmake")

# Make CMake able to find Catch.cmake (provides catch_discover_tests)
list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")

include(Catch)
include(CTest)


add_executable(paramify_tests
  test_core.cpp
  test_cli.cpp
  test_save.cpp
)

target_link_libraries(paramify_tests PRIVATE
  paramify::paramify
  Catch2::Catch2
)

target_compile_features(paramify_tests PRIVATE cxx_std_14)

catch_discover_tests(paramify_tests)
