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
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, see <http://www.gnu.org/licenses/>.
############################################################################

include(FindPackageHandleStandardArgs)

set(SISO_ROOT_DIR $ENV{SISODIR5} CACHE PATH "location of SISO Linux SDK ")

find_path(SISO_INCLUDE_DIRS
  NAMES sisoboards.h
  HINTS ${SISO_ROOT_DIR}
  PATH_SUFFIXES include
DOC "SISO include directory")

find_library(SISO_FGLIB5 NAMES fglib5 HINTS ${SISODIR5}/lib64)
find_library(SISO_CLSERSIS NAMES clsersis HINTS ${SISODIR5}/lib64)
find_library(SISO_HAPRT NAMES haprt HINTS ${SISODIR5}/lib64)

message(STATUS "SISO_FGLIB5:       [${SISO_FGLIB5}]")
message(STATUS "SISO_CLSERSIS:     [${SISO_CLSERSIS}]")
message(STATUS "SISO_HAPRT:        [${SISO_HAPRT}]")

set(SISO_LIBRARIES ${SISO_FGLIB5} ${SISO_CLSERSIS} ${SISO_HAPRT}) 

if (SISO_LIBRARIES)
    get_filename_component(SISO_LIBRARIES_DIR ${SISO_FGLIB5} PATH)
endif()

mark_as_advanced(SISO_INCLUDE_DIRS SISO_LIBRARIES_DIR SISO_FGLIB5 SISO_CLSERSIS SISO_HAPRT)

find_package_handle_standard_args(SISO REQUIRED_VARS SISO_INCLUDE_DIRS SISO_FGLIB5 SISO_CLSERSIS SISO_HAPRT)

if(SISO_FOUND)
    if(NOT TARGET SISO::SISO)
        add_library(SISO::SISO INTERFACE IMPORTED)
    endif()
    if(SISO_INCLUDE_DIRS)
        set_target_properties(SISO::SISO PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${SISO_INCLUDE_DIRS}")
    endif()
    foreach(LIB IN LISTS SISO_LIBRARIES)
        get_filename_component(MOD ${LIB} NAME_WE)
        add_library(SISO::${MOD} SHARED IMPORTED)
        set_target_properties(SISO::${MOD} PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${LIB}")
        target_link_libraries(SISO::SISO INTERFACE SISO::${MOD})
    endforeach()
endif()
