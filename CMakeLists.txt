cmake_minimum_required(VERSION 3.13)
project(paramify VERSION 0.4.2 LANGUAGES CXX)

include(GNUInstallDirs)

option(PARAMIFY_BUILD_EXAMPLES "Build paramify examples" ON)
option(PARAMIFY_BUILD_TESTS "Build paramify tests" OFF)


# ----------------------------
# Library target (header-only)
# ----------------------------
add_library(paramify INTERFACE)
add_library(paramify::paramify ALIAS paramify)

target_compile_features(paramify INTERFACE cxx_std_14)

target_include_directories(paramify INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# ----------------------------
# Dependencies
# ----------------------------
# yaml-cpp (required)
find_package(yaml-cpp REQUIRED)
target_link_libraries(paramify INTERFACE yaml-cpp)


# CLI11 (header-only). You can remove this if you vendor CLI11 yourself.
include(FetchContent)
FetchContent_Declare(
  CLI11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
  GIT_TAG        v2.6.1
)
FetchContent_MakeAvailable(CLI11)

# target_link_libraries(paramify INTERFACE CLI11::CLI11)

target_include_directories(paramify INTERFACE
  $<BUILD_INTERFACE:${cli11_SOURCE_DIR}/include>
)

if (PARAMIFY_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()


if (PARAMIFY_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()



# ----------------------------
# Install headers + CMake config
# ----------------------------
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT core)
install(DIRECTORY ${cli11_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT core)

install(TARGETS paramify
  EXPORT paramifyTargets
  COMPONENT core
)

install(EXPORT paramifyTargets
  FILE paramifyTargets.cmake
  NAMESPACE paramify::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/paramify
  COMPONENT core
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  cmake/paramifyConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/paramifyConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/paramify
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/paramifyConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/paramifyConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/paramifyConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/paramify
  COMPONENT core
)

# ----------------------------
# CPack / Debian package
# ----------------------------
set(CPACK_GENERATOR "DEB")
set(CPACK_DEB_COMPONENT_INSTALL ON)

set(CPACK_PACKAGE_NAME "paramify")
set(CPACK_PACKAGE_VENDOR "LuxAI")
set(CPACK_PACKAGE_CONTACT "support@luxai.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Paramify: dynamic parameter management for C++14 (YAML + CLI11)")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})

# Install prefix inside packages (typical Debian)
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")

# Architecture (Debian-compliant)
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
else()
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
endif()

# Distro version (dynamic: e.g. 22.04 -> release suffix 1deb22.04)
execute_process(
    COMMAND lsb_release -rs
    OUTPUT_VARIABLE LINUX_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(LINUX_VERSION)
    set(CPACK_DEBIAN_PACKAGE_RELEASE "1deb${LINUX_VERSION}")
else()
    set(CPACK_DEBIAN_PACKAGE_RELEASE "1")
endif()

set(CPACK_COMPONENTS_ALL "core")
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")

# Package name per component
set(CPACK_DEBIAN_CORE_PACKAGE_NAME "libparamify-dev")

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_CORE_PACKAGE_SHLIBDEPS ON)

# Runtime deps (best-effort). Adjust names per distro if needed.
# On Ubuntu/Debian yaml-cpp runtime package is typically libyaml-cpp0.6 (version may vary).
set(CPACK_DEBIAN_CORE_PACKAGE_DEPENDS "libyaml-cpp0.6 | libyaml-cpp0.8")

include(CPack)
