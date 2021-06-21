###########################################################################
# This file is part of LImA, a Library for Image Acquisition
#
#  Copyright (C) : 2009-2019
#  European Synchrotron Radiation Facility
#  CS40220 38043 Grenoble Cedex 9
#  FRANCE
#
#  Contact: lima@esrf.fr
#
#  This is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This software is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, see <http://www.gnu.org/licenses/>.
############################################################################

cmake_minimum_required(VERSION 3.1)

project(pco)

# Include additional modules that are used inconditionnaly
include(GNUInstallDirs)
include(GenerateExportHeader)

# If conda build, always set lib dir to 'lib'
if($ENV{CONDA_BUILD})
  set(CMAKE_INSTALL_LIBDIR "lib")
endif()

# Set lower / upper case project names
string(TOUPPER "${PROJECT_NAME}" PROJECT_NAME_UPPER)
string(TOLOWER "${PROJECT_NAME}" PROJECT_NAME_LOWER)

# Set targets export name (used by lima and dependencies)
set(TARGETS_EXPORT_NAME "${PROJECT_NAME_LOWER}-targets")
#set(namespace "${PROJECT_NAME}::")

# Check if project is being used directly or via add_subdirectory
set(CAMERA_MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(CAMERA_MASTER_PROJECT ON)
endif()

if (CAMERA_MASTER_PROJECT)
  find_package(Lima REQUIRED COMPONENTS devel tools)
endif()

# CMake additional macros
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${LIMA_CMAKE_INCLUDE_DIRS} ${CMAKE_MODULE_PATH})

include(LimaTools)

# Set version
include(project_version)

# Enable git hash in the version
option(WITH_GIT_VERSION "include the git commit hash" OFF)

# Enable python binding code compilation using sip generator
if (CAMERA_MASTER_PROJECT)
  option(LIMA_ENABLE_PYTHON "compile python binding code?" OFF)
  option(LIMA_ENABLE_PYTANGO_SERVER "install python tango server?" OFF)

  if (LIMA_ENABLE_PYTHON)
    limatools_find_python_and_sip()
  endif()
endif()

find_package(Pco REQUIRED)

file(GLOB_RECURSE PCO_INCS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")

add_library(pco SHARED
	src/PcoBinCtrlObj.cpp
	src/PcoBufferCtrlObj.cpp
	src/PcoCamera.cpp
	src/PcoCameraSdk.cpp
	src/PcoCameraSdkLin.cpp
	src/PcoCameraSip.cpp
	src/PcoCameraUtils.cpp
	src/PcoDetInfoCtrlObj.cpp
	src/PcoHwEventCtrlObj.cpp
	src/PcoInterface.cpp
	src/PcoRoiCtrlObj.cpp
	src/PcoSyncCtrlObj.cpp
	${PCO_INCS}
)

# Generate export macros
generate_export_header(pco)

# Add dependencies
target_include_directories(pco
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"           # For export header
    PUBLIC "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

if(WITH_GIT_VERSION)
	target_compile_definitions(pco PUBLIC WITH_GIT_VERSION)
endif()

# Set LIMA_NO_DEBUG if LIMA_ENABLE_DEBUG is set
if(NOT LIMA_ENABLE_DEBUG)
    target_compile_definitions(pco PUBLIC LIMA_NO_DEBUG)
endif()



# Set version and output name
set_target_properties(pco PROPERTIES
  OUTPUT_NAME "lima${PROJECT_NAME_LOWER}"
  VERSION "${PROJECT_VERSION}"
  SOVERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}")

target_link_libraries(pco PUBLIC limacore)

target_compile_definitions(pco PUBLIC ${PCO_DEFINITIONS})

message("... PCO_INCLUDE_DIRS: ${PCO_INCLUDE_DIRS}")

target_include_directories(pco
	PUBLIC "$<BUILD_INTERFACE:${PCO_INCLUDE_DIRS}>"
	PUBLIC "$<BUILD_INTERFACE:${PCO_INCLUDE_DIRS}/include>")

target_link_libraries(pco PUBLIC ${PCO_LIBRARIES})


# Binding code for python
if(LIMA_ENABLE_PYTHON)
  limatools_run_sip_for_camera(pco)
endif()

# Generate and install package config file and version
if(CAMERA_MASTER_PROJECT)
    set(PROJECT_LIBRARIES pco)
    set(SIP_INSTALL_DIR ${CMAKE_INSTALL_DATADIR}/sip/lima)
    set(CMAKE_INSTALL_DIR ${CMAKE_INSTALL_DATADIR}/cmake/lima)
    include(package_config)
endif()

## Installation

install(
  TARGETS pco
  EXPORT "${TARGETS_EXPORT_NAME}"
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}   # import library
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}   # .so files are libraries
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}   # .dll files are binaries
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}  # this does not actually install anything (but used by downstream projects)
)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
  COMPONENT devel
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h"
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/project_version.h
    ${PROJECT_BINARY_DIR}/pco_export.h
  COMPONENT devel
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
	PROGRAMS
	  ${PCO_SDK_BIN_DIR}/pco_clhs_info
	  ${PCO_SDK_BIN_DIR}/pco_clhs_mgr
	  ${PCO_SDK_BIN_DIR}/pco_clhs_svc
	DESTINATION ${CMAKE_INSTALL_BINDIR}
)

message("... LIMA_ENABLE_PYTHON: [${LIMA_ENABLE_PYTHON}]")
message("... PYTHON_SITE_PACKAGES_DIR: [${PYTHON_SITE_PACKAGES_DIR}]")
if(LIMA_ENABLE_PYTHON)
  install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python/
    DESTINATION "${PYTHON_SITE_PACKAGES_DIR}/Lima/Pco"
    )
  if (LIMA_ENABLE_PYTANGO_SERVER)
    add_subdirectory(tango)
  endif()
endif()

## Tests
if(CAMERA_ENABLE_TESTS)
    enable_testing()
    #add_subdirectory(test)
endif()
