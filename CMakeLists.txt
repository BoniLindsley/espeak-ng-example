cmake_minimum_required(VERSION 3.16)

# Installation will try use a standard file directory structure.
# The included module provides functions for that.
include(GNUInstallDirs)
# Installation will create packages.
# The included module makes it easier to create them.
include(CMakePackageConfigHelpers)

project(espeak-ng-example)

#
#
#
# ## Build the example

set(TARGET_NAME ${PROJECT_NAME})
add_executable(${TARGET_NAME} "main.cpp")

install(TARGETS ${TARGET_NAME} EXPORT ${TARGET_NAME})

install(
  EXPORT ${TARGET_NAME}
  DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/cmake"
  NAMESPACE "${PROJECT_NAME}::"
  FILE "${TARGET_NAME}-target.cmake"
)

#
# ### Link dependencies

find_package(espeak-ng REQUIRED)
target_link_libraries(${TARGET_NAME} espeak-ng::espeak-ng)

find_package(SDL2 REQUIRED)
target_link_libraries(${TARGET_NAME} SDL2::SDL2)

#
# ### Documentation

find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
  set(DOXYGEN_EXTRACT_ALL TRUE)
  set(DOXYGEN_EXTRACT_STATIC TRUE)
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "README.md")
  set(
    DOXYGEN_OUTPUT_DIRECTORY
    "${CMAKE_CURRENT_BINARY_DIR}/doc/${PROJECT_NAME}"
  )
  doxygen_add_docs(
    ${TARGET_NAME}-doc
    "README.md"
    "main.cpp"
    "boni.hpp"
    "espeak-ng.hpp"
    "sdl2.hpp"
    ALL
  )
  install(
    DIRECTORY "${DOXYGEN_OUTPUT_DIRECTORY}"
    TYPE "DOC"
    OPTIONAL
  )
endif()

#
# ### Create a CMake package that imports all targets in this project.

# Generate the template needed to for creating the package file.
# This is done at configuration time.
set(TEMPLATE_DIR "${CMAKE_CURRENT_BINARY_DIR}/cmake_modules")
set(CONFIG_TEMPLATE "${TEMPLATE_DIR}/package_config_template.cmake.in")
set(CONFIG_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/cmake")
set(INSTALLED_BUILDSYSTEM_TARGETS "espeak-ng-example")
file(
  WRITE "${CONFIG_TEMPLATE}"
  [=[
@PACKAGE_INIT@

include(CMakeFindDependencyMacro)

foreach(TARGET_TO_INCLUDE @INSTALLED_BUILDSYSTEM_TARGETS@)
  include("${CMAKE_CURRENT_LIST_DIR}/${TARGET_TO_INCLUDE}-target.cmake")
endforeach()

find_dependency(espeak-ng)
find_dependency(SDL2)

check_required_components(@INSTALLED_BUILDSYSTEM_TARGETS@)
]=]
)

# Tell CMake to generate the package.
set(CONFIG_FILENAME "${PROJECT_NAME}-config.cmake")
configure_package_config_file(
  # Tell CMake to use the template generated above.
  "${CONFIG_TEMPLATE}"
  # The file to contain the generated package file at build time.
  "${TEMPLATE_DIR}/${CONFIG_FILENAME}"
  # Directory to copy the package to at install time.
  # Note that it does not actually install the file created.
  INSTALL_DESTINATION
  "${CONFIG_INSTALL_DIR}"
)
# So install the file manually.
install(
  FILES "${TEMPLATE_DIR}/${CONFIG_FILENAME}"
  DESTINATION "${CONFIG_INSTALL_DIR}"
)
