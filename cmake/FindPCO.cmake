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

find_path(PCO_INCLUDE_DIRS
  NAMES PCO_Structures.h
  HINTS ${PCO_ROOT_DIR}
  PATH_SUFFIXES include
DOC "PCO include directory")

find_library(PCO_LIBRARIES
  NAMES SC2_Cam.lib
  HINTS ${PCO_ROOT_DIR}
DOC "PCO libraries")

if (PCO_LIBRARIES)
    get_filename_component(PCO_LIBRARIES_DIR ${PCO_LIBRARIES} PATH)
endif()

mark_as_advanced(PCO_INCLUDE_DIRS PCO_LIBRARIES_DIR PCO_LIBRARIES)

find_package_handle_standard_args(PCO REQUIRED_VARS PCO_INCLUDE_DIRS PCO_LIBRARIES)

if(PCO_FOUND)
    if(NOT TARGET PCO::PCO)
        add_library(PCO::PCO SHARED IMPORTED)
    endif()
    if(PCO_INCLUDE_DIRS)
        set_target_properties(PCO::PCO PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${PCO_INCLUDE_DIRS}")
    endif()
    if(EXISTS "${PCO_LIBRARIES}")
        set_target_properties(PCO::PCO PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
          IMPORTED_LOCATION "${PCO_LIBRARIES}")
    endif()
endif()

list(APPEND PCO_DEFINITIONS WITH_GIT_VERSION)
