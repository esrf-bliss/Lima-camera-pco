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

set(INTERFACE "ME4")
#set(INTERFACE "CLHS")

set(PCO_INCLUDE_DIRS)
set(PCO_LIBRARIES)
set(PCO_DEFINITIONS)



set(SISODIR5 $ENV{SISODIR5} CACHE PATH "location of SISO Linux SDK ")
set(PCO_SDKLIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sdkPcoLin" CACHE PATH "location of PCO Linux SDK")
set(PCO_SDK_LIB_DIR "${PCO_SDKLIN_DIR}/pco_common/pco_lib" CACHE PATH "location of PCO Linux SDK LIBS")
set(PCO_SDK_BIN_DIR "${PCO_SDKLIN_DIR}/pco_common/pco_bin" CACHE PATH "location of PCO Linux SDK binary")

find_path(SISO_INCLUDE NAMES sisoboards.h HINTS ${PCO_SDKLIN_DIR}/siso_include)
#find_path(SISO_INCLUDE sisoboards.h)

message("==========================================================")
message("PCO_SDKLIN_DIR: [${PCO_SDKLIN_DIR}]")
message("PCO_SDK_LIB_DIR: [${PCO_SDK_LIB_DIR}]")
message("SISO_INCLUDE: [${SISO_INCLUDE}]")
message("==========================================================")

list(APPEND PCO_INCLUDE_DIRS
	${SISO_INCLUDE}

	${PCO_SDKLIN_DIR}
	${PCO_SDKLIN_DIR}/pco_common/pco_include
	${PCO_SDKLIN_DIR}/pco_common/pco_classes
	#${PCO_SDKLIN_DIR}/include
	${PCO_SDKLIN_DIR}/pco_me4/pco_classes
	${PCO_SDKLIN_DIR}/pco_clhs/pco_classes
	${PCO_SDKLIN_DIR}/pco_clhs/pco_clhs_common
	

)

set(PCOLIB_ME4)
set(PCOLIB_CLHS_1)
set(PCOLIB_CLHS_2)
set(PCOLIB2)
set(PCOLIB3)
set(PCOLIB4)
set(PCOLIB7)

if(INTERFACE  STREQUAL "ME4")
	# ------ PCO ME4 libs
	find_library(PCOLIB_ME4 pcocam_me4 HINTS ${PCO_SDK_LIB_DIR})
endif()

if(INTERFACE  STREQUAL "CLHS")
	# ------ PCO CLHS libs
	find_library(PCOLIB_CLHS_1 pcocam_clhs HINTS ${PCO_SDK_LIB_DIR})
	find_library(PCOLIB_CLHS_2 pcoclhs HINTS ${PCO_SDK_LIB_DIR})
endif()

find_library(PCOLIB2 pcofile HINTS ${PCO_SDK_LIB_DIR})
find_library(PCOLIB3 pcolog HINTS ${PCO_SDK_LIB_DIR})
find_library(PCOLIB4 reorderfunc HINTS ${PCO_SDK_LIB_DIR})

#find_library(PCOLIB_CLHS_2 pcodisp HINTS ${PCO_SDK_LIB_DIR})

#DISPLIB    = $(PCOLIBDIR)/libpcodisp.a
#LOGLIB     = $(PCOLIBDIR)/libpcolog.a
#FILELIB    = $(PCOLIBDIR)/libpcofile.a
#REORDERLIB = $(PCOLIBDIR)/libreorderfunc.a
#CAMLIB     = $(PCOLIBDIR)/libpcocam_clhs.a

message("==========================================================")
message("PCOLIB_ME4:    [${PCOLIB_ME4}]")
message("PCOLIB_CLHS_1: [${PCOLIB_CLHS_1}]")
message("PCOLIB_CLHS_2: [${PCOLIB_CLHS_2}]")
message("PCOLIB2:       [${PCOLIB2}]")
message("PCOLIB3:       [${PCOLIB3}]")
message("PCOLIB4:       [${PCOLIB4}]")
message("PCOLIB7:       [${PCOLIB7}]")
message("==========================================================")


set(SISOLIB1)
set(SISOLIB2)
set(SISOLIB3)

find_library(SISOLIB1 NAMES fglib5 HINTS ${SISODIR5}/lib64)
find_library(SISOLIB2 NAMES clsersis HINTS ${SISODIR5}/lib64)
find_library(SISOLIB3 NAMES haprt HINTS ${SISODIR5}/lib64)


message("==========================================================")
message("SISODIR5: [${SISODIR5}]")
message("SISOLIB1: [${SISOLIB1}]")
message("SISOLIB2: [${SISOLIB2}]")
message("SISOLIB3: [${SISOLIB3}]")
message("==========================================================")

if(INTERFACE  STREQUAL "ME4")
	list(APPEND PCO_LIBRARIES 
		${PCOLIB_ME4} 
		${PCOLIB2} ${PCOLIB3} ${PCOLIB4}
		${SISOLIB1} ${SISOLIB2} ${SISOLIB3}
	)

	list(APPEND PCO_DEFINITIONS ME4)

endif()

if(INTERFACE  STREQUAL "CLHS")
	list(APPEND PCO_LIBRARIES 
		${PCOLIB_CLHS_1} 
		${PCOLIB_CLHS_2} 
		${PCOLIB2} ${PCOLIB3} ${PCOLIB4}
		${SISOLIB1} ${SISOLIB2} ${SISOLIB3}
	)

	list(APPEND PCO_DEFINITIONS CLHS)
endif()
    

message("==========================================================")
message("PCO_INCLUDE_DIRS: [${PCO_INCLUDE_DIRS}]")
message("PCO_LIBRARIES: [${PCO_LIBRARIES}]")
message("PCO_DEFINITIONS: [${PCO_DEFINITIONS}]")
message("==========================================================")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PCO DEFAULT_MSG
  PCO_LIBRARIES
  PCO_INCLUDE_DIRS
)

list(APPEND PCO_DEFINITIONS WITH_GIT_VERSION)


