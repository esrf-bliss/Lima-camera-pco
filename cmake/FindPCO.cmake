###########################################################################
# This file is part of LImA, a Library for Image Acquisition
#
#  Copyright (C) : 2009-2017
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
#  MERCHANTABILITY or FIMODESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, see <http://www.gnu.org/licenses/>.
############################################################################

set(PCO_INTERFACE_OPTIONS ME4 CLHS 1394 USB3)
set(PCO_INTERFACE ME4 CACHE STRING "Select the target PCO interface")
set_property(CACHE PCO_INTERFACE PROPERTY STRINGS ${PCO_INTERFACE_OPTIONS})

include(FindPackageHandleStandardArgs)

set(PCO_ROOT_DIR CACHE PATH "location of PCO Linux SDK ")

find_path(PCO_INCLUDE_DIR
    NAMES VersionNo.h
    HINTS ${PCO_ROOT_DIR}
    PATH_SUFFIXES pco/common
DOC "PCO include directory")

list(APPEND PCO_INCLUDE_DIRS ${PCO_INCLUDE_DIR})

# Common libraries
find_library(PCO_LIB_CO_FILE pcofile HINTS ${PCO_SDK_LIB_DIR})
find_library(PCO_LIB_CO_LOG pcolog HINTS ${PCO_SDK_LIB_DIR})
find_library(PCO_LIB_CO_RFUNC reorderfunc HINTS ${PCO_SDK_LIB_DIR})

message(STATUS "PCO_LIB_CO_FILE:   [${PCO_LIB_CO_FILE}]")
message(STATUS "PCO_LIB_CO_LOG:    [${PCO_LIB_CO_LOG}]")
message(STATUS "PCO_LIB_CO_RFUNC:  [${PCO_LIB_CO_RFUNC}]")

if(PCO_INTERFACE STREQUAL "ME4")
    find_path(PCO_INCLUDE_ME4_DIR
        NAMES VersionNo.h
        HINTS ${PCO_ROOT_DIR}
        PATH_SUFFIXES pco/me4
    DOC "PCO ME4 include directory")
    
    list(APPEND PCO_INCLUDE_DIRS ${PCO_INCLUDE_ME4_DIR})
    
    find_library(PCO_LIB_ME4 pcocam_me4 HINTS ${PCO_SDK_LIB_DIR})

    message(STATUS "PCO_LIB_ME4:   [${PCO_LIB_ME4}]")

    find_package_handle_standard_args(PCO REQUIRED_VARS
        PCO_INCLUDE_DIR
        PCO_INCLUDE_ME4_DIR
        PCO_LIB_ME4
        PCO_LIB_CO_FILE PCO_LIB_CO_LOG PCO_LIB_CO_RFUNC)

    set(PCO_LIBRARIES
        ${PCO_LIB_ME4}
        ${PCO_LIB_CO_FILE} ${PCO_LIB_CO_LOG} ${PCO_LIB_CO_RFUNC}
    )

    list(APPEND PCO_DEFINITIONS ME4)

endif()

if(PCO_INTERFACE STREQUAL "CLHS")
    find_path(PCO_INCLUDE_CLHS_DIR
        NAMES VersionNo.h
        HINTS ${PCO_ROOT_DIR}
        PATH_SUFFIXES pco/clhs
    DOC "PCO ME4 include directory")
    
    list(APPEND PCO_INCLUDE_DIRS ${PCO_INCLUDE_CLHS_DIR})
  
    find_library(PCO_LIB_CLHS_1 pcocam_clhs HINTS ${PCO_SDK_LIB_DIR})
    find_library(PCO_LIB_CLHS_2 pcoclhs HINTS ${PCO_SDK_LIB_DIR})

    message(STATUS "PCO_LIB_CLHS_1: [${PCO_LIB_CLHS_1}]")
    message(STATUS "PCO_LIB_CLHS_2: [${PCO_LIB_CLHS_2}]")

    find_package_handle_standard_args(PCO REQUIRED_VARS
        PCO_INCLUDE_DIR
        PCO_INCLUDE_CLHS_DIR
        PCO_LIB_CLHS_1 PCO_LIB_CLHS_2
        PCO_LIB_CO_FILE PCO_LIB_CO_LOG PCO_LIB_CO_RFUNC)

    set(PCO_LIBRARIES
        ${PCO_LIB_CLHS_1} ${PCO_LIB_CLHS_2}
        ${PCO_LIB_CO_FILE} ${PCO_LIB_CO_LOG} ${PCO_LIB_CO_RFUNC}
    )

    list(APPEND PCO_DEFINITIONS CLHS)
endif()

if (PCO_LIBRARIES)
    get_filename_component(PCO_LIBRARIES_DIR ${PCO_LIB_CO_FILE} PATH)
endif()

mark_as_advanced(PCO_INCLUDE_DIRS PCO_LIBRARIES_DIR PCO_LIBRARIES)

if(PCO_FOUND)
    if(NOT TARGET PCO::PCO)
        add_library(PCO::PCO INTERFACE IMPORTED)
    endif()
    if(PCO_INCLUDE_DIRS)
        set_target_properties(PCO::PCO PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${PCO_INCLUDE_DIRS}")
    endif()
    if(PCO_DEFINITIONS)
        set_target_properties(PCO::PCO PROPERTIES
            INTERFACE_COMPILE_DEFINITIONS "${PCO_DEFINITIONS}")
    endif()
    foreach(LIB IN LISTS PCO_LIBRARIES)
        get_filename_component(MOD ${LIB} NAME_WE)
        add_library(PCO::${MOD} SHARED IMPORTED)
        set_target_properties(PCO::${MOD} PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${LIB}")
        target_link_libraries(PCO::PCO INTERFACE PCO::${MOD})
    endforeach()
endif()

list(APPEND PCO_DEFINITIONS WITH_GIT_VERSION)
