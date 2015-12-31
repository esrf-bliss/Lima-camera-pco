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
#ifndef PCO_H
#define PCO_H

#define _LINUX

#ifdef __x86_64
	#define _x64
#else
	#define _x86
#endif


#include "processlib/Compatibility.h"
#include "PCO_Structures.h"
#include "PCO_ConvStructures.h"
#include "Pco_ConvDlgExport.h"
#include "sc2_SDKStructures.h"
#include "sc2_common.h"
#include "SC2_CamExport.h"
#include "sc2_defs.h"
#include "SC2_SDKAddendum.h"
#include "PCO_errt.h"


#include <math.h>


#define DWORD_MAX ULONG_MAX 

#define ERR_SIZE	256
#define ERRMSG_SIZE	(256+128)
#define MODEL_TYPE_SIZE	32
#define INTERFACE_TYPE_SIZE	32
#define CAMERA_NAME_SIZE	128
#define MSG_SIZE	512
#define BUFF_XLAT_SIZE 128

#define ID_TIMESTAMP "$Id: [" __DATE__ " " __TIME__ "] [" __TIMESTAMP__ "] [" __FILE__ "] $"

typedef DWORD tPvUint32;
typedef int tPvErr;

#define THROW_LIMA_HW_EXC(e, x)  { \
	printf("========*** LIMA_HW_EXC %s\n", x ); \
			throw LIMA_HW_EXC(e, x); \
} 



#if 0
#define PCO_TRACE(x)  \
{ \
		if(error){ \
			char msg[ERRMSG_SIZE+1]; \
			sprintf_s(msg, "=== %s PcoError[x%08x][%s]", x, m_pcoData->pcoError,  m_pcoData->pcoErrorMsg); \
			DEB_TRACE() << msg; \
			throw LIMA_HW_EXC(Error, msg); \
		} \
		DEB_TRACE() << "*** " <<  x << " OK" ; \
}
#define _PCO_TRACE(x, s)  \
{ \
		if(error){ \
			DEB_TRACE() << "*** " <<  x << " PCO ERROR " << s; \
			throw LIMA_HW_EXC(Error, x); \
		} \
		DEB_TRACE() << "*** " <<  x << " OK" ; \
}

#endif

#define PCO_THROW_OR_TRACE(__err__, __msg__)  \
{ \
		if(__err__){ \
			char ___buff___[ERRMSG_SIZE+1]; \
			sprintf_s(___buff___, ERRMSG_SIZE, "LIMA_HW_EXC ===> %s PcoError[x%08x][%s]", __msg__, m_pcoData->pcoError,  m_pcoData->pcoErrorMsg); \
			DEB_ALWAYS() << ___buff___; \
			throw LIMA_HW_EXC(Error, ___buff___); \
		} \
		DEB_TRACE() << "*** " <<  __msg__ << " OK" ; \
}

#define PCO_PRINT_ERR(__err__, __msg__)  \
{ \
		if(__err__){ \
			char ___buff___[ERRMSG_SIZE+1]; \
			sprintf_s(___buff___,  ERRMSG_SIZE, "=== %s PcoError[x%08x][%s]", __msg__, m_pcoData->pcoError,  m_pcoData->pcoErrorMsg); \
			printf("%s [%s][%d]\n", ___buff___, __FILE__, __LINE__); \
			DEB_TRACE() << ___buff___; \
		} \
}

#define DEF_FNID 	static char *fnId =__FUNCTION__;

#define PRINTLINES { for(int i = 0; i<50;i++) printf("=====  %s [%d]/[%d]\n", __FILE__, __LINE__,i); }

#define PCO_FN1(er,mg, fn, x1) {mg = #fn; er = PcoCheckError(__LINE__, __FILE__, fn ( (x1) ), #fn ); }
#define PCO_FN2(er,mg, fn, x1, x2) {mg = #fn; er = PcoCheckError(__LINE__, __FILE__, fn ( (x1),(x2) ), #fn ) ; }
#define PCO_FN3(er,mg, fn, x1, x2, x3) {mg = #fn; er = PcoCheckError(__LINE__, __FILE__, fn ( (x1),(x2),(x3) ), #fn ) ; }
#define PCO_FN4(er,mg, fn, x1, x2, x3, x4) {mg = #fn; er = PcoCheckError(__LINE__, __FILE__, fn ( (x1),(x2),(x3),(x4) ), #fn ) ; }
#define PCO_FN5(er,mg, fn, x1, x2, x3, x4, x5) {mg = #fn; er = PcoCheckError(__LINE__, __FILE__, fn ( (x1),(x2),(x3),(x4),(x5) ), #fn ) ; }


#endif
