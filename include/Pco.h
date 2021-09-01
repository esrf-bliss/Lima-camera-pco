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
#    define _x64
#else
#    define _x86
#endif

#    define NOT_IMPLEMENTED "NOT IMPLEMENTED IN LINUX"
#    define VS_PLATFORM "osLinux"
#    define VS_CONFIGURATION x64

//===============================================================
// INCLUDES
//===============================================================
#    include <defs.h>
#    include <stdint.h>
#    include <linux/limits.h>

#    include "processlib/Compatibility.h"
#    include "sc2_common.h"
#    include "sc2_defs.h"
#    include "SC2_SDKAddendum.h"

#    define sprintf_s snprintf
#    include "PCO_errt.h"


#include <math.h>

//===============================================================
// defs
//===============================================================

#define DWORD_MAX 0xffffffff
#define INT_DWORD_MAX 4294967295

#define ERR_SIZE 256
#define ERRMSG_SIZE (256 + 128)
#define MODEL_TYPE_SIZE 64
#define MODEL_SUBTYPE_SIZE 64
#define INTERFACE_TYPE_SIZE 64
#define CAMERA_NAME_SIZE 128
#define MSG_SIZE 512
#define BUFF_XLAT_SIZE 128
#define MSG1K (1024 * 1)
#define MSG2K (1024 * 2)
#define MSG4K (1024 * 4)
#define MSG8K (1024 * 8)

#define WAITMS_1S 1000
#define WAITMS_100MS 100
#define WAITMS_10MS 10
#define WAITMS_1MS 1

#define KBYTEF (1024.)
#define MBYTEF (KBYTEF * KBYTEF)
#define GBYTEF (KBYTEF * KBYTEF * KBYTEF)

#define ID_FILE_TIMESTAMP                                                      \
    "$Id: [" __DATE__ " " __TIME__ "] [" __TIMESTAMP__ "] [" __FILE__ "] $"

typedef DWORD tPvUint32;
typedef int tPvErr;

//-------------------- linux
typedef uint32_t DWORD;
// typedef unsigned long       DWORD;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef bool BOOL;

#ifndef HANDLE
typedef int HANDLE;
#endif

typedef short SHORT;
// typedef long LONG;

typedef uint64_t UINT64;

#ifndef ULLONG_MAX
#define LONG_MAX 2147483647L
#define ULONG_MAX 0xffffffffUL
#define ULLONG_MAX 0xffffffffffffffffULL /* maximum unsigned long long int value */
#endif

#define far

#define DECLARE_HANDLE(n)                                                  \
        typedef struct n##__                                                   \
        {                                                                      \
            int i;                                                             \
        } * n

DECLARE_HANDLE(HWND);

#ifndef __TIMESTAMP__
#define __TIMESTAMP__
#endif


//-------------------- linux / win
typedef struct timeval TIME_USEC;
#define TIME_UTICKS struct timespec
#define UNUSED __attribute__((unused))

//===============================================================
// def funct
//===============================================================

#define sprintf_s snprintf

#define strcpy_s(d, l, s) strncpy((d), (s), (l))
#define strncpy_s(d, l, s, n) strncpy((d), (s), (l))

#define localtime_s(stc, tm) (localtime_r((tm), (stc)))


//===============================================================
// MACROS
//===============================================================


#define PCO_THROW_OR_TRACE(__err__, __msg__)                                   \
    {                                                                          \
        if (__err__)                                                           \
        {                                                                      \
            char ___buff___[128];                                              \
            sprintf_s(                                                         \
                ___buff___, sizeof(___buff___),                                \
                "%s >> PcoError[0x%08x][%s] error[0x%08x]",                    \
                __msg__, m_pcoData->pcoError, m_pcoData->pcoErrorMsg,          \
                __err__);                                                      \
            THROW_FATAL(Hardware, Error) << ___buff___;                        \
        }                                                                      \
        DEB_TRACE() << "*** " << __msg__ << " OK";                             \
    }

#define PCO_PRINT_ERR(__err__, __msg__)                                        \
    {                                                                          \
        if (__err__)                                                           \
        {                                                                      \
            char ___buff___[128];                                  \
            sprintf_s(___buff___, sizeof(___buff___), "=== %s PcoError[x%08x][%s]", \
                      __msg__, m_pcoData->pcoError, m_pcoData->pcoErrorMsg);        \
            printf("... %s [%s][%d]\n", ___buff___, __FILE__, __LINE__);            \
            DEB_TRACE() << ___buff___;                                              \
        }                                                                           \
    }

#define PCO_CHECK_ERROR(er, fn) (PcoCheckError(__LINE__, __FILE__, (er), (fn)))

#define PCO_CHECK_ERROR_CAM(er, fn)                                            \
    (m_cam.PcoCheckError(__LINE__, __FILE__, (er), (fn)))

#define PRINTLINES                                                             \
    {                                                                          \
        for (int i = 0; i < 50; i++)                                           \
            printf("=====  %s [%d]/[%d]\n", __FILE__, __LINE__, i);            \
    }

#define PCO_CHECK_ERROR1(__err__, __comments__)                            \
        {                                                                      \
            if (__err__)                                                       \
            {                                                                  \
                __err__ = m_cam.PcoCheckError(__LINE__, __FILE__, __err__,     \
                                              fnId, __comments__);             \
            }                                                                  \
        }

#define DEF_FNID const char *fnId __attribute__((unused)) = __FUNCTION__;




#endif
