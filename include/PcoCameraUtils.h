/**************************************************************************
###########################################################################
 This file is part of LImA, a Library for Image Acquisition

 Copyright (C) : 2009-2011
 European Synchrotron Radiation Facility
 BP 220, Grenoble 38043
 FRANCE

 This is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This software is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, see <http://www.gnu.org/licenses/>.
###########################################################################
**************************************************************************/
#ifndef PCOCAMERAUTILS_H
#define PCOCAMERAUTILS_H

#include <time.h>
#include "processlib/Compatibility.h"

#define CAMINFO_ALL				0xffffffffffffffffLL

#define CAMINFO_BLOCK			(0x1LL << 0)
#define CAMINFO_UNSORTED		(0x1LL << 1)
#define CAMINFO_LOG				(0x1LL << 2)

#define CAMINFO_PIXELRATE		(0x1LL << 8)
#define CAMINFO_ADC				(0x1LL << 9)
#define CAMINFO_FIRMWARE		(0x1LL << 10)
#define CAMINFO_GENERAL			(0x1LL << 11)
#define CAMINFO_VERSION			(0x1LL << 12)
#define CAMINFO_DIMAX			(0x1LL << 13)
#define CAMINFO_EXP				(0x1LL << 14)
#define CAMINFO_ROI				(0x1LL << 15)

#define CAMINFO_CAMERALINK		(0x1LL << 16)
#define CAMINFO_CAMERATYPE		(0x1LL << 17)

#define FILE_PCO_DLL				"liblimapco.dll"
#define FILENAME_INSTALL_VERSION	"INSTALL_VERSION"
#define FILEEXT_INSTALL_VERSION		"txt"

char * _getComputerName(char *infoBuff, DWORD  bufCharCount);
char * _getUserName(char *infoBuff, DWORD  bufCharCount);
char * _getVSconfiguration(char *infoBuff, DWORD  bufCharCount);
char * _getPcoSdkVersion(char *infoBuff, int strLen, char *lib);
char * _getDllPath(const char* pzFileName, char *path, size_t strLen);


#endif
