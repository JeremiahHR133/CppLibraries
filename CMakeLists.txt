cmake_minimum_required(VERSION 3.24)

project(CppLibraries)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(CMAKE_CXX_STANDARD 23)
  set(CMAKE_CXX_STANDARD_REQUIRED True)

  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/lib)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/lib)

  set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)
  include(GNUInstallDirs)
  set(INSTALL_NAMESPACE "CppLibraries::")
  set(CMAKE_BUILD_RPATH ON)
  set(CMAKE_INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")

  # Set default to Release if CMAKE_BUILD_TYPE is empty
  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose build type" FORCE)
  endif()

  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

if(MSVC)
    if(MSVC_VERSION LESS 1940)
        message(FATAL_ERROR
            "MSVC 19.40 or newer is required (Visual Studio 2022 17.10+). "
            "Your version: ${MSVC_VERSION}")
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "14.0.0")
        message(FATAL_ERROR "GCC version ${CMAKE_CXX_COMPILER_VERSION} is insufficient. GCC 14 or newer is required.")
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "CLANG")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "18.0.0")
        message(FATAL_ERROR "Clang version ${CMAKE_CXX_COMPILER_VERSION} is insufficient. Clang 18 or newer is required.")
    endif()
endif()

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.17.0
  )
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
  include(CTest)
endif()

add_subdirectory(src)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

install(
    EXPORT CppLibraries
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CppLibraries
    NAMESPACE ${INSTALL_NAMESPACE}
)
