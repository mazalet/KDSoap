# This is the top-level CMakeLists.txt file for the KDSoap project.
#
# Pass the following variables to cmake to control the build:
# (See INSTALL-cmake.txt for more information)
#
# -DKDSoap_STATIC=[true|false]
#  Build static libraries
#  Default=false
#
# -DKDSoap_TESTS=[true|false]
#  Build the test harness.
#  Default=false
#
# -DKDSoap_EXAMPLES=[true|false]
#  Build the examples.
#  Default=true
#
cmake_minimum_required(VERSION 3.0.2)
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW)
endif()
if(POLICY CMP0054)
  cmake_policy(SET CMP0054 NEW)
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/kdwsdl2cpp/libkode/common")
    message(FATAL_ERROR "Please do git submodule update --init --recursive")
endif()

if("${CMAKE_INSTALL_PREFIX}" STREQUAL "")
  set(USE_DEFAULT_INSTALL_LOCATION True)
else()
  set(USE_DEFAULT_INSTALL_LOCATION False)
endif()

project(KDSoap CXX)

if(CMAKE_VERSION VERSION_LESS "3.1")
    if(CMAKE_COMPILER_IS_GNUCXX)
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif ()
else()
    set(CMAKE_CXX_STANDARD 11)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/ ${CMAKE_MODULE_PATH}")

option(${PROJECT_NAME}_STATIC "Build statically" OFF)
option(${PROJECT_NAME}_TESTS "Build the tests" OFF)
option(${PROJECT_NAME}_EXAMPLES "Build the examples" ON)

set(${PROJECT_NAME}_VERSION_MAJOR 1)
set(${PROJECT_NAME}_VERSION_MINOR 9)
set(${PROJECT_NAME}_VERSION_PATCH 50)
set(${PROJECT_NAME}_VERSION ${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.${${PROJECT_NAME}_VERSION_PATCH})
set(${PROJECT_NAME}_SOVERSION ${${PROJECT_NAME}_VERSION_MAJOR})

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_path(BOOST_OPTIONAL_DIR NAMES boost/optional.hpp)
if (BOOST_OPTIONAL_DIR)
    message(STATUS "Found boost/optional.hpp in ${BOOST_OPTIONAL_DIR}")
    include_directories(${BOOST_OPTIONAL_DIR})
    add_definitions(-DBOOST_OPTIONAL_FOUND) # I'd really prefer using configure_file but this is more compatible with qmake
endif()

# setup default install locations
include(InstallLocation)

find_package(Qt5 5.7.0 COMPONENTS Core Network Widgets CONFIG REQUIRED)
set(QT_LIBRARIES Qt5::Core)
set(QT_USE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Qt5Portability.cmake")

if(Qt5_POSITION_INDEPENDENT_CODE)
  if(Qt5Core_VERSION VERSION_LESS 5.1)
    set_property(TARGET Qt5::Core PROPERTY INTERFACE_POSITION_INDEPENDENT_CODE ON)
  endif()
endif()

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
set(CMAKE_AUTOMOC TRUE)

add_definitions(-DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_FROM_BYTEARRAY)
add_definitions(-DUSE_EXCEPTIONS -DQT_FATAL_ASSERT)
if(MSVC)
  add_definitions(-D_SCL_SECURE_NO_WARNINGS)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag(-Wsuggest-override HAVE_GXX_SUGGEST_OVERRIDE)
  if(HAVE_GXX_SUGGEST_OVERRIDE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wsuggest-override")
  endif()
endif()

if(${PROJECT_NAME}_STATIC)
  set(${PROJECT_NAME}_LIBRARY_MODE "STATIC")
else()
  set(${PROJECT_NAME}_LIBRARY_MODE "SHARED")
endif()

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(${PROJECT_NAME}_IS_ROOT_PROJECT TRUE)

  if(CMAKE_BUILD_TYPE MATCHES "Release")
    add_definitions(-DNDEBUG)
  endif()

  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
  endif()

  if(USE_DEFAULT_INSTALL_LOCATION)
    if(UNIX)
      set(CMAKE_INSTALL_PREFIX "/usr/local/KDAB/${PROJECT_NAME}-${${PROJECT_NAME}_VERSION}")
    elseif(WIN32)
      set(CMAKE_INSTALL_PREFIX "C:\\KDAB\\${PROJECT_NAME}-${${PROJECT_NAME}_VERSION}")
    endif()
  endif()

  message(STATUS "Building ${PROJECT_NAME} ${${PROJECT_NAME}_VERSION} in ${CMAKE_BUILD_TYPE} mode. Installing to ${CMAKE_INSTALL_PREFIX}")

  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")

  install(FILES LICENSE.GPL.txt LICENSE.US.txt LICENSE.txt README.txt kdsoap.pri kdwsdl2cpp.pri DESTINATION ${INSTALL_DOC_DIR})

  include(CMakePackageConfigHelpers)

  write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/KDSoap/KDSoapConfigVersion.cmake"
    VERSION ${${PROJECT_NAME}_VERSION}
    COMPATIBILITY AnyNewerVersion
  )

  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/KDSoapConfig-buildtree.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/KDSoap/KDSoapConfig.cmake"
    @ONLY
  )
  file(COPY
    "${CMAKE_CURRENT_SOURCE_DIR}/KDSoapMacros.cmake"
    DESTINATION
    "${CMAKE_CURRENT_BINARY_DIR}/KDSoap/"
  )
  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/KDSoapConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/install/KDSoapConfig.cmake"
    INSTALL_DESTINATION "${INSTALL_LIBRARY_DIR}/cmake/KDSoap"
    PATH_VARS INSTALL_INCLUDE_DIR
  )

  install(FILES
      "${CMAKE_CURRENT_BINARY_DIR}/install/KDSoapConfig.cmake"
      "${CMAKE_CURRENT_BINARY_DIR}/KDSoap/KDSoapConfigVersion.cmake"
      KDSoapMacros.cmake
    DESTINATION "${INSTALL_LIBRARY_DIR}/cmake/KDSoap"
  )
  install(EXPORT KDSoapTargets NAMESPACE KDSoap::
    DESTINATION "${INSTALL_LIBRARY_DIR}/cmake/KDSoap"
  )

  # Generate .pri file for qmake users
  include(ECMGeneratePriFile)
  set(PROJECT_VERSION_STRING ${${PROJECT_NAME}_VERSION})
  ecm_generate_pri_file(BASE_NAME KDSoapClient
                        LIB_NAME kdsoap
                        FILENAME_VAR pri_client_filename)
  ecm_generate_pri_file(BASE_NAME KDSoapServer
                        LIB_NAME kdsoap-server
                        FILENAME_VAR pri_server_filename)
  install(FILES ${pri_client_filename} ${pri_server_filename} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})

endif()

add_subdirectory(src)
add_subdirectory(kdwsdl2cpp)
if(${PROJECT_NAME}_IS_ROOT_PROJECT)

  export(TARGETS kdsoap kdsoap-server kdwsdl2cpp NAMESPACE KDSoap::
    FILE "${CMAKE_CURRENT_BINARY_DIR}/KDSoap/KDSoapTargets.cmake"
  )

  add_subdirectory(features)
  if(${PROJECT_NAME}_TESTS)
    enable_testing()
    add_subdirectory(testtools)
    add_subdirectory(unittests)
  endif()
  if(${PROJECT_NAME}_EXAMPLES)
    add_subdirectory(examples)
  endif()
endif()
