
cmake_minimum_required(VERSION 2.8.9)
project(fastbpe)

# The version number.
set (fasttext_VERSION_MAJOR 0)
set (fasttext_VERSION_MINOR 1)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost 1.68 REQUIRED)

add_subdirectory(fastBPE)



include(CMakePackageConfigHelpers)
configure_package_config_file(FastBPEConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/FastBPEConfig.cmake
  INSTALL_DESTINATION lib/cmake/FastBPE
  )

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/FastBPEConfigVersion.cmake
  VERSION 0.1.0
  COMPATIBILITY SameMajorVersion )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/FastBPEConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/FastBPEConfigVersion.cmake
  DESTINATION lib/cmake/FastBPE )
