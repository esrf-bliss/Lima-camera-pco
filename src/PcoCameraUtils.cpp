//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2024
// European Synchrotron Radiation Facility
// BP 220, Grenoble 38043
// FRANCE
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//###########################################################################

#ifdef __linux__
#    include <sys/utsname.h>
#    include <sys/time.h> // for gettimeofday
#endif

#ifndef __linux__
#    include <windows.h>
#    include <tchar.h>
#endif
#include <stdio.h>

#include <cstdlib>

#ifndef __linux__
#    include <process.h>
#else
#    include <unistd.h>
#endif

#include <sys/stat.h>

#include <sys/timeb.h>
#include <time.h>

#include "lima/HwSyncCtrlObj.h"

#include "lima/Exceptions.h"

#include "PcoCamera.h"
#include "PcoCameraUtils.h"
#include "PcoSyncCtrlObj.h"
#include "PcoBufferCtrlObj.h"

using namespace lima;
using namespace lima::Pco;

// const char *timebaseUnits[] = {"ns", "us", "ms"};

#define BUFF_INFO_SIZE 10000

void print_hex_dump_buff(void *ptr_buff, size_t len);

//=========================================================================================================
// dummy comments for test 02ccc
//=========================================================================================================

//=========================================================================================================
//=========================================================================================================
int getNrBitsOn(WORD x)
{
    int count = 0;
    WORD mask = 1;

    for (int i = 0; i < 8; i++)
    {
        if (x & mask)
            count++;
        mask = mask << 1;
    }
    return count;
}
//=========================================================================================================
//=========================================================================================================
char *getTimestamp(timestampFmt fmtIdx, time_t xtime)
{
    static char timeline[128];
    time_t ltime;
    struct tm today;
    const char *fmt;

    if ((xtime == 0) && (fmtIdx == IsoMilliSec))
    {
#ifndef __linux__
        SYSTEMTIME st;
        GetLocalTime(&st);
        __sprintfSExt(timeline, sizeof(timeline),
                      "%4d/%02d/%02d %02d:%02d:%02d.%03d", st.wYear, st.wMonth,
                      st.wDay, st.wHour, st.wMinute, st.wSecond,
                      st.wMilliseconds);
#else
        struct timespec tickNow = {0, 0};
        time_t timeRaw;
        struct tm *timeInfo;

        clock_gettime(CLOCK_REALTIME, &tickNow);
        time(&timeRaw);

        timeInfo = localtime(&timeRaw);

        __sprintfSExt(timeline, sizeof(timeline),
                      "%4d/%02d/%02d %02d:%02d:%02d.%03d",
                      timeInfo->tm_year + 1900, timeInfo->tm_mon + 1,
                      timeInfo->tm_mday, timeInfo->tm_hour, timeInfo->tm_min,
                      timeInfo->tm_sec, int((tickNow.tv_nsec / 1000000)));
#endif
        return timeline;
    }

    switch (fmtIdx)
    {
        case Iso:
            fmt = "%Y/%m/%d %H:%M:%S";
            break;
        case IsoHMS:
            fmt = "%H:%M:%S";
            break;
        case FnDate:
            fmt = "%Y-%m-%d";
            break;

        default:
        case FnFull:
            fmt = "%Y-%m-%d-%H%M%S";
            break;
    }

    if (xtime == 0)
        time(&ltime);
    else
        ltime = xtime;

    localtime_s(&today, &ltime);
    strftime(timeline, 128, fmt, &today);

    return timeline;
}

time_t getTimestamp()
{
    return time(NULL);
}

//====================================================================
//====================================================================
//$Id: [Oct  8 2013 15:21:07] [Tue Oct  8 15:21:07 2013]
//(..\..\..\..\src\PcoCamera.cpp) $

#define LEN_BUFF_DATE 128
#define LEN_BUFF_PATH PATH_MAX // 260 in win
#define TOKNR_DT 5

int __xlat_date(char *s1, char &ptrTo, int lenTo)
{
    char *tok[TOKNR_DT];
    char *tokNext = NULL;
    int tokNr, iM, iD, iY, i;
    char *ptr;
    const char *months = "Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec";
    const char *sM, *sT;
    char buff[LEN_BUFF_DATE + 1];

    strcpy_s(buff, LEN_BUFF_DATE, s1);
    ptr = buff;

    for (tokNr = i = 0; i < TOKNR_DT; i++)
    {
        if ((tok[i] = strtok_s(ptr, " ", &tokNext)) == NULL)
            break;
        ptr = NULL;
        tokNr = i + 1;
    }

    if (tokNr == 4)
    {
        // Oct  8 2013 15:21:07
        sM = tok[0];
        iD = atoi(tok[1]);
        iY = atoi(tok[2]);
        sT = tok[3];
    }
    else if (tokNr == 5)
    {
        // Tue Oct  8 15:21:07 2013
        sM = tok[1];
        iD = atoi(tok[2]);
        iY = atoi(tok[4]);
        sT = tok[3];
    }
    else
    {
        sM = "xxx";
        iD = 99;
        iY = 9999;
        sT = "99:99:99";
    }

    ptr = strstr((char *)months, sM);
    iM = (ptr != NULL) ? (int(ptr - months) / 4) + 1 : 99;

    return __sprintfSExt(&ptrTo, lenTo, "%04d/%02d/%02d %s", iY, iM, iD, sT);
}

char *_xlat_date(char *s1, char *s2, char *s3)
{
    static char buff[2 * LEN_BUFF_DATE + LEN_BUFF_PATH];
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff) - 1;

    ptr += __sprintfSExt(ptr, ptrMax - ptr, "$Id: comp[");
    ptr += __xlat_date(s1, *ptr, (int)(ptrMax - ptr));
    ptr += __sprintfSExt(ptr, ptrMax - ptr, "] file[");
    ptr += __xlat_date(s2, *ptr, (int)(ptrMax - ptr));
    ptr += __sprintfSExt(ptr, ptrMax - ptr, "] [%s] $", s3);
    return buff;
}

char *_split_date(const char *s)
{
    static char s1[LEN_BUFF_DATE + 1];
    static char s2[LEN_BUFF_DATE + 1];
    static char s3[LEN_BUFF_DATE + 1];
    const char *ptr1, *ptr2;

    ptr1 = strchr(s, '[');
    ptr2 = strchr(ptr1, ']');
    strncpy_s(s1, LEN_BUFF_DATE, ptr1 + 1, ptr2 - ptr1 - 1);
    s1[ptr2 - ptr1 - 1] = 0;

    ptr1 = strchr(ptr2, '[');
    ptr2 = strchr(ptr1, ']');
    strncpy_s(s2, LEN_BUFF_DATE, ptr1 + 1, ptr2 - ptr1 - 1);
    s2[ptr2 - ptr1 - 1] = 0;

    ptr1 = strchr(ptr2, '[');
    ptr2 = strchr(ptr1, ']');
    strncpy_s(s3, LEN_BUFF_DATE, ptr1 + 1, ptr2 - ptr1 - 1);
    s3[ptr2 - ptr1 - 1] = 0;

    return _xlat_date(s1, s2, s3);
}

//====================================================================
//====================================================================

char *str_trim_left(char *s)
{
    char c;
    if (s == NULL)
        return NULL;
    while ((c = *s) != 0)
    {
        if (!isspace(c))
            break;
        s++;
    }
    return s;
}

char *str_trim_right(char *s)
{
    char *ptr;
    if (s == NULL)
        return NULL;
    ptr = s + strlen(s) - 1;
    while (s <= ptr)
    {
        if (!isspace(*ptr))
            break;
        *ptr-- = 0;
    }
    return s;
}
char *str_trim(char *s)
{
    return str_trim_left(str_trim_right(s));
}

char *str_toupper(char *s)
{
    char *ptr = s;
    while (*ptr)
    {
        *ptr = toupper(*ptr);
        ptr++;
    }
    return s;
}

//=========================================================================================================
//=========================================================================================================
void Camera::Sleep_ms(unsigned int time_ms) // time in ms
{
#ifdef __linux__

    useconds_t time_us = time_ms * 1000;
    usleep(time_us);

#else

    ::Sleep(time_ms);

#endif
}

//=========================================================================================================
//=========================================================================================================

void stcPcoData::traceAcqClean()
{
    void *ptr = &this->traceAcq;
    memset(ptr, 0, sizeof(STC_traceAcq));
}

void stcPcoData::traceMsg(char *s)
{
    char *ptr = traceAcq.msg;
    char *dt = getTimestamp(IsoHMS);
    size_t lg = strlen(ptr);
    ptr += lg;
    snprintf(ptr, LEN_TRACEACQ_MSG - lg, "%s> %s", dt, s);
}

//====================================================================
// utils
//====================================================================

#define LEN_LINE_BUFF 128
#define BYTES_PER_LINE 16
#define OFFSET_COL_HEX1 (4 + 1 + 3)
#define OFFSET_COL_HEX2 (OFFSET_COL_HEX1 + (8 * 3) + 2)
#define OFFSET_COL_ASC1 (OFFSET_COL_HEX2 + (8 * 3) + 3)
#define OFFSET_COL_ASC2 (OFFSET_COL_ASC1 + 8 + 1)

//--------------------------------------------------------------------
//--------------------------------------------------------------------
char *nibble_to_hex(char *s, BYTE nibble)
{
    nibble &= 0x0f;
    *s = (nibble <= 9) ? '0' + nibble : 'a' + nibble - 10;
    return s + 1;
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
char *byte_to_hex(char *s, BYTE byte)
{
    s = nibble_to_hex(s, byte >> 4);
    return nibble_to_hex(s, byte);
    ;
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
char *word_to_hex(char *s, WORD word)
{
    s = byte_to_hex(s, word >> 8);
    return byte_to_hex(s, (BYTE)word);
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
char *dword_to_hex(char *s, DWORD dword)
{
    s = word_to_hex(s, dword >> 16);
    return word_to_hex(s, (WORD)dword);
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
char *_hex_dump_bytes(void *obj, size_t lenObj, char *buff, size_t lenBuff)
{
    char *ptr = buff;
    char *ptrMax = buff + lenBuff;
    char *ptrObj = (char *)obj;

    memset(buff, ' ', lenBuff);

    for (unsigned int i = 0; i < lenObj; i++)
    {
        if (ptr + 3 > ptrMax)
            break;
        ptr = byte_to_hex(ptr, *ptrObj++);
        if (ptr + 1 == ptrMax)
            break;
        ptr++;
    }

    *ptr = 0;
    return buff;
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
const char *hex_dump_line(void *buff, size_t len, size_t *nr, WORD *offset)
{
    static char line_buff[LEN_LINE_BUFF];

    char *s;
    BYTE *ptr_buff;
    BYTE *ptr_buff0;
    int i, nr1, nr2;

    ptr_buff = (BYTE *)buff;
    ptr_buff0 = ptr_buff;

    s = line_buff;

    if (len <= 0)
    {
        strcpy_s(s, LEN_LINE_BUFF, "<empty>");
        return line_buff;
    }

    memset(line_buff, ' ', LEN_LINE_BUFF);

    *nr = (len < BYTES_PER_LINE) ? len : BYTES_PER_LINE;

    s = word_to_hex(s, *offset);
    *offset += (WORD)*nr;
    *s = ':';
    s = line_buff + OFFSET_COL_HEX1;

    if (*nr <= BYTES_PER_LINE / 2)
    {
        nr1 = (int)*nr;
        nr2 = -1;
    }
    else
    {
        nr1 = BYTES_PER_LINE / 2;
        nr2 = ((int)*nr) - nr1;
    }

    for (i = 0; i < nr1; i++)
    {
        s = byte_to_hex(s, *ptr_buff++);
        s++;
    }

    if (nr2 > 0)
    {
        *s = '-';
        s = line_buff + OFFSET_COL_HEX2;
        for (i = 0; i < nr2; i++)
        {
            s = byte_to_hex(s, *ptr_buff++);
            s++;
        }
    }

    ptr_buff = ptr_buff0;
    s = line_buff + OFFSET_COL_ASC1;
    for (i = 0; i < nr1; i++)
    {
        *s++ = (isprint(*ptr_buff)) ? *ptr_buff : '.';
        ptr_buff++;
    }

    if (nr2 > 0)
    {
        s = line_buff + OFFSET_COL_ASC2;
        for (i = 0; i < nr2; i++)
        {
            *s++ = (isprint(*ptr_buff)) ? *ptr_buff : '.';
            ptr_buff++;
        }
    }

    *s = 0;

    return line_buff;
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
void print_hex_dump_buff(void *ptr_buff, size_t len)
{
    WORD offset = 0;
    size_t nr = 0;
    BYTE *ptr = (BYTE *)ptr_buff;
    ;

    // printf("dump buff / len: %ld\n", len);
    printf("dump buff / len: %zd\n", len);

    while (len > 0)
    {
        printf("%s\n", hex_dump_line(ptr, len, &nr, &offset));
        len -= nr;
        ptr += nr;
    }
}

//=========================================================================================================
//=========================================================================================================

unsigned long long Camera::_getDebug(unsigned long long mask = ULLONG_MAX)
{
    return m_pcoData->debugLevel & mask;
}
//=========================================================================================================
//=========================================================================================================

const char *Camera::_checkLogFiles(bool firstCall)
{
    const char *logFiles[] = {"C:\\ProgramData\\pco\\SC2_Cam.log",
                              "C:\\ProgramData\\pco\\PCO_CDlg.log",
                              "C:\\ProgramData\\pco\\PCO_Conv.log",
                              "C:\\ProgramData\\pco\\me4_memlog_end.log", NULL};
    const char **ptr = logFiles;
    const char *logOn = "\n\n"
                        "######################################################"
                        "#########################\n"
                        "######################################################"
                        "#########################\n"
                        "######################################################"
                        "#########################\n"
                        "###                                                   "
                        "                      ###\n"
                        "###                           !!!  ATTENTION !!!      "
                        "                      ###\n"
                        "###                                                   "
                        "                      ###\n"
                        "###                     THE PCO LOG FILES ARE ENABLED "
                        "                      ###\n"
                        "###                                                   "
                        "                      ###\n"
                        "###                 this option is ONLY for DEBUG & "
                        "TESTS                   ###\n"
                        "###                                                   "
                        "                      ###\n"
                        "###                   it downgrades the acquisition "
                        "time                    ###\n"
                        "###                                                   "
                        "                      ###\n"
                        "###                                                   "
                        "                      ###\n"
                        "###     to DISABLE it:                                "
                        "                      ###\n"
                        "###          * stop the device server                 "
                        "                      ###\n"
                        "###          * open the directory "
                        "C:\\ProgramData\\pco\\                       ###\n"
                        "###          * rename all the .log files as .txt      "
                        "                      ###\n"
                        "###          * start again the device server          "
                        "                      ###\n"
                        "###                                                   "
                        "                      ###\n"
                        "######################################################"
                        "#########################\n"
                        "######################################################"
                        "#########################\n"
                        "######################################################"
                        "#########################\n\n\n";

    const char *logOff = "";
    struct stat fileStat;
    int error;
    bool found = false;

    if (firstCall)
    {
        while (*ptr != NULL)
        {
            error = stat(*ptr, &fileStat);
            // printf("----------- [%d][%s]\n", error, *ptr);
            found |= !error;
            ptr++;
        }
        m_pcoData->pcoLogActive = found;
    }

    return m_pcoData->pcoLogActive ? logOn : logOff;
};

//====================================================================
//====================================================================

#define INFO_BUFFER_SIZE 1024
// void printError( TCHAR* msg );

char *_getComputerName(char *infoBuff, DWORD bufCharCount)
{
#ifndef __linux__
    // Get and display the name of the computer.
    if (!GetComputerName(infoBuff, &bufCharCount))
        __sprintfSExt(infoBuff, bufCharCount, "ERROR: GetComputerName");
#else
    char hostname[1024];
    gethostname(hostname, 1024);

    __sprintfSExt(infoBuff, bufCharCount, hostname);
#endif

    return infoBuff;
}

char *_getUserName(char *infoBuff, DWORD bufCharCount)
{
#ifndef __linux__
    // Get and display the user name.
    if (!GetUserName(infoBuff, &bufCharCount))
        __sprintfSExt(infoBuff, bufCharCount, "ERROR: GetUserName");
#else
    int err = getlogin_r(infoBuff, bufCharCount);
    if (err)
    {
        __sprintfSExt(infoBuff, bufCharCount, "ERROR getlogin_r");
    }
#endif

    return infoBuff;
}

#ifdef __linux__
char *_getDllPath(const char *pzFileName, char *path, size_t strLen)
{
    *path = 0;
    return path;
}

#else
//====================================================================
//====================================================================
#    define PCOSDK_FILENAME "sc2_cam.dll"

#    include <winver.h>
#    pragma comment(lib, "version.lib")

#    define LEN_DRIVE 7
#    define LEN_DIR PATH_MAX

//====================================================================
//====================================================================
char *_getDllPath(const char *pzFileName, char *path, size_t strLen)
{
    errno_t err;

    char drive[LEN_DRIVE + 1];
    char dir[LEN_DIR + 1];
    char _pathFn[PATH_MAX + 1];
    char _pathFnInstall[PATH_MAX + 1];
    char *ptr;
    size_t nr;
    FILE *stream;

    *path = 0;
    ptr = path;

#    ifndef ENABLE_GETDLLPATH
    __sprintfSExt(ptr, strLen - 1, "_getDllPath DISABLED");
    return path;
#    endif

    GetModuleFileNameA(GetModuleHandleA(pzFileName), _pathFn, PATH_MAX);

    err =
        _splitpath_s(_pathFn, drive, LEN_DRIVE, dir, LEN_DIR, NULL, 0, NULL, 0);

    size_t l = strlen(dir);
    ptr = dir + l - 2;
    while (*ptr != '\\')
        ptr--;
    *ptr = 0;

    ptr = path;
    err = _makepath_s(_pathFnInstall, PATH_MAX, drive, dir,
                      FILENAME_INSTALL_VERSION, FILEEXT_INSTALL_VERSION);
    printf("----- path[%s] path1[%s] drive[%s] dir[%s]\n", _pathFn,
           _pathFnInstall, drive, dir);
    nr = __sprintfSExt(ptr, strLen - 1, "%s\n", _pathFnInstall);
    ptr += nr;
    strLen -= nr;

    err = fopen_s(&stream, _pathFnInstall, "r");
#    if 0
	if(err == 0) {
		if( fgets( path, strLen, stream ) == NULL){
			*ptr = 0;
		}
	}
#    endif
    nr = fread(ptr, 1, strLen - 1, stream);
    ptr[nr] = 0;
    fclose(stream);
    return path;
}

//=======================================================================================================
//=======================================================================================================
HRESULT LastError()
{
    return -1;
}

HRESULT GetFileVersion(TCHAR *filename, VS_FIXEDFILEINFO *pvsf)
{
    DWORD dwHandle;
    DWORD cchver = GetFileVersionInfoSize(filename, &dwHandle);
    if (cchver == 0)
        return LastError();

    char *pver = new char[cchver];
    BOOL bret = GetFileVersionInfo(filename, 0, cchver, pver);
    if (!bret)
    {
        delete[] pver;
        return LastError();
    }
    UINT uLen;
    void *pbuf;
    bret = VerQueryValue(pver, _T("\\"), &pbuf, &uLen);
    if (!bret)
    {
        delete[] pver;
        return LastError();
    }
    memcpy(pvsf, pbuf, sizeof(VS_FIXEDFILEINFO));
    delete[] pver;
    return S_OK;
}

int GetFileVerStructA(TCHAR *pzFileName, int *ima, int *imi, int *imb)
{
    HRESULT ret;

    VS_FIXEDFILEINFO vsf1;
    if (SUCCEEDED(ret = GetFileVersion(pzFileName, &vsf1)))
    {
        *ima = vsf1.dwFileVersionMS >> 16;
        *imi = vsf1.dwFileVersionMS & 0xFFFF;
        *imb = vsf1.dwFileVersionLS & 0xFFFF;
        return 0;
    }
    return -1;
}

#endif


//====================================================================
//====================================================================
char *Camera::_camInfo(char *ptr, char *ptrMax, long long int flag)
{
    DEB_MEMBER_FUNCT();

    long long int lgbuff, lgbuffmax;
    lgbuffmax = ptrMax - ptr;

    if (flag & CAMINFO_BLOCK)
    {
        ptr += __sprintfSExt(ptr, ptrMax - ptr, "\n* camInfo [begin]\n");
        ptr += __sprintfSExt(ptr, ptrMax - ptr, "* ... timestamp[%s]\n",
                             getTimestamp(Iso));
    }

    //--------------- CAMERA TYPE
    if (flag & CAMINFO_CAMERATYPE)
    {
        ptr += __sprintfSExt(ptr, ptrMax - ptr, "* camera type \n");

        ptr += __sprintfSExt(ptr, ptrMax - ptr, "* ... cam_name[%s]\n",
                             _getCameraIdn());

        ptr += __sprintfSExt(ptr, ptrMax - ptr, "* ... dwSerialNumber[%d]\n",
                             (int)_getCameraSerialNumber());

        ptr += __sprintfSExt(ptr, ptrMax - ptr, "* ... wCamType[0x%x] [%s]\n",
                             (int)_getCameraType(), _getCameraTypeStr());

        ptr +=
            __sprintfSExt(ptr, ptrMax - ptr, "* ... wCamSubType[0x%x] [%s]\n",
                          (int)_getCameraSubType(), _getCameraSubTypeStr());

        ptr += __sprintfSExt(ptr, ptrMax - ptr,
                             "* ... wInterfaceType[0x%x] [%s]\n",
                             (int)_getInterfaceType(), _getInterfaceTypeStr());
    }

    //--------------- general info
    if (flag & CAMINFO_GENERAL)
    {
        ptr += __sprintfSExt(ptr, ptrMax - ptr, "* general info \n");

        double pixSizeX, pixSizeY;
        _get_PixelSize(pixSizeX, pixSizeY);
        ptr +=
            __sprintfSExt(ptr, ptrMax - ptr, "* ... pixelSize (um) [%g,%g] \n",
                          pixSizeX, pixSizeY);
    }

    //--------------- version
    if (flag & CAMINFO_VERSION)
    {
        ptr += __sprintfSExt(ptr, ptrMax - ptr, "* lima version \n");
        ptr += __sprintfSExt(ptr, ptrMax - ptr, "%s", m_pcoData->version);
    }
    //--------------- firmware

    if (flag & CAMINFO_FIRMWARE)
    {
        int err;
        _pco_GetFirmwareInfo(ptr, (int)(ptrMax - ptr), err);
        ptr += strlen(ptr);
    }

    //--------------- adc, pixelrate,
    if (flag & CAMINFO_ADC)
    {
        ptr += __sprintfSExt(ptr, ptrMax - ptr, "*** adc\n");
        int adc_working, adc_max;

        // int err = _pco_GetADCOperation(adc_working, adc_max);
        _pco_GetADCOperation(adc_working, adc_max);
        ptr += __sprintfSExt(ptr, ptrMax - ptr, "* ADC working[%d] max[%d]\n",
                             adc_working, adc_max);
    }

    if (flag & CAMINFO_PIXELRATE)
    {
        DWORD dwPixRate, dwPixRateNext;
        int error, i;
        _pco_GetPixelRate(dwPixRate, dwPixRateNext, error);

        ptr += __sprintfSExt(ptr, ptrMax - ptr, "* pixelRate\n");
        ptr += __sprintfSExt(ptr, ptrMax - ptr, "* ... pixelRate[%d](%g MHz)\n",
                             (int)dwPixRate, (double)(dwPixRate / 1000000.));
        ptr += __sprintfSExt(
            ptr, ptrMax - ptr, "* ...    pixelRateRequested[%d](%g MHz) \n",
            (int)dwPixRateNext, (double)(dwPixRateNext / 1000000.));
        ptr += __sprintfSExt(ptr, ptrMax - ptr, "* ...    valid pixelRates ");
        for (i = 0; i < 4; i++)
        {
            dwPixRate = m_pcoData->stcPcoDescription.dwPixelRateDESC[i];
            if (dwPixRate)
            {
                ptr +=
                    __sprintfSExt(ptr, ptrMax - ptr, " [%d]", (int)dwPixRate);
            }
        }
        ptr += __sprintfSExt(ptr, ptrMax - ptr, "\n");
    }

    //--------------------- acqinfo
    if (flag & CAMINFO_ACQ)
    {
        int err;

        double _exposure, _delay;
        m_sync->getExpTime(_exposure);
        m_sync->getLatTime(_delay);

        ptr += __sprintfSExt(
            ptr, ptrMax - ptr, "... exp[%g ms][%g s] delay[%g ms][%g s]\n",
            _exposure * 1.0e3, _exposure, _delay * 1.0e3, _delay);

        Roi limaRoi;
        int error;

        int forcedFifo = 0;
        getRecorderForcedFifo(forcedFifo);

        Bin aBin;
        _pco_GetBinning(aBin, err);

        int binX = aBin.getX();
        int binY = aBin.getY();

        _pco_GetROI(limaRoi, error);

        Point top_left = limaRoi.getTopLeft();
        Point bot_right = limaRoi.getBottomRight();
        Size size = limaRoi.getSize();

        ptr += __sprintfSExt(ptr, ptrMax - ptr,
                             "... roiLima XY0[%d,%d] XY1[%d,%d] size[%d,%d]\n",
                             top_left.x, top_left.y, bot_right.x, bot_right.y,
                             size.getWidth(), size.getHeight());

        int iFrames;
        m_sync->getNbFrames(iFrames);

        ptr += __sprintfSExt(ptr, ptrMax - ptr,
                             "... bin[%d,%d] nrFrames[%d] forcedFifo[%d]\n",
                             binX, binY, iFrames, forcedFifo);
    }

    //--------------------- size, roi, ...
    if (flag & CAMINFO_ROI)
    {
        ptr += __sprintfSExt(ptr, ptrMax - ptr, "* size, roi, ... \n");
        unsigned int maxWidth, maxHeight, Xstep, Ystep, xMinSize, yMinSize;
        getXYdescription(Xstep, Ystep, maxWidth, maxHeight, xMinSize, yMinSize);
        WORD _wArmWidth, _wArmHeight, _wMaxWidth, _wMaxHeight;
        int err = _pco_GetSizes(&_wArmWidth, &_wArmHeight, &_wMaxWidth, &_wMaxHeight);
                      

        ptr += __sprintfSExt(ptr, ptrMax - ptr,
                             "* ... maxWidth=[%d] maxHeight=[%d] \n", maxWidth,
                             maxHeight);
        ptr += __sprintfSExt(ptr, ptrMax - ptr,
                             "* ...    Xstep=[%d] Ystep=[%d] (PCO ROI steps)\n",
                             Xstep, Ystep);
        ptr += __sprintfSExt(ptr, ptrMax - ptr,
                             "* ... xMinSize=[%d] yMinSize=[%d] \n", xMinSize,
                             yMinSize);

        ptr += __sprintfSExt(ptr, ptrMax - ptr,
                             "* ... wXResActual=[%d] wYResActual=[%d] \n",
                             (int)_wArmWidth, (int)_wArmHeight);

        ptr += __sprintfSExt(ptr, ptrMax - ptr,
                             "* ... wXResMax=[%d] wYResMax=[%d] \n",
                             (int)_wMaxWidth, (int)_wMaxHeight);

        unsigned int x0, x1, y0, y1;
        Roi limaRoi;
        int error;

        _pco_GetROI(limaRoi, error);
        _xlatRoi_lima2pco(limaRoi, x0, x1, y0, y1);

        ptr += __sprintfSExt(ptr, ptrMax - ptr,
                             "* ... roi X[%d,%d] Y[%d,%d] size[%d,%d]\n", x0,
                             x1, y0, y1, x1 - x0 + 1, y1 - y0 + 1);

        Point top_left = limaRoi.getTopLeft();
        Point bot_right = limaRoi.getBottomRight();
        Size size = limaRoi.getSize();

        ptr +=
            __sprintfSExt(ptr, ptrMax - ptr,
                          "* ... roiLima XY0[%d,%d] XY1[%d,%d] size[%d,%d]\n",
                          top_left.x, top_left.y, bot_right.x, bot_right.y,
                          size.getWidth(), size.getHeight());

        unsigned int bytesPerPix;
        getBytesPerPixel(bytesPerPix);
        WORD bitsPerPix;
        getBitsPerPixel(bitsPerPix);

        long long imgSizePix = size.getWidth() * size.getHeight();
        long long imgSize = imgSizePix * bytesPerPix;
        double imgSizeMb = imgSize / (1024. * 1024.);
        ptr += __sprintfSExt(ptr, ptrMax - ptr,
                             "* ... imgSize[%lld px][%lld B][%g MB] \n",
                             imgSizePix, imgSize, imgSizeMb);
        ptr += __sprintfSExt(ptr, ptrMax - ptr,
                             "* bitsPerPix[%d] bytesPerPix[%d] \n",
                             (int)bitsPerPix, bytesPerPix);

        int iFrames;
        m_sync->getNbFrames(iFrames);
        double totalImgSizeMb = imgSizeMb * iFrames;
        ptr += __sprintfSExt(
            ptr, ptrMax - ptr,
            "* ... m_sync->getNbFrames=[%d frames] totalSize[%g MB]\n", iFrames,
            totalImgSizeMb);
    }
    //--------------------- exp, coc, ...
    if (flag & CAMINFO_EXP)
    {
        ptr += __sprintfSExt(ptr, ptrMax - ptr, "* exp, coc, ... \n");
        double _exposure, _delay;
        struct lima::HwSyncCtrlObj::ValidRangesType valid_ranges;
        m_sync->getValidRanges(valid_ranges);
        m_sync->getExpTime(_exposure);
        m_sync->getLatTime(_delay);

        DWORD _dwMinExposureStepDESC =
            m_pcoData->stcPcoDescription.dwMinExposureStepDESC;
        DWORD _dwMinDelayStepDESC =
            m_pcoData->stcPcoDescription.dwMinDelayStepDESC;

        ptr += __sprintfSExt(
            ptr, ptrMax - ptr, "* ... exp[%g ms][%g s] delay[%g ms][%g s]\n",
            _exposure * 1.0e3, _exposure, _delay * 1.0e3, _delay);

        ptr += __sprintfSExt(
            ptr, ptrMax - ptr,
            "* ...    exp valid min[%g us] max[%g ms] step[%g us]\n",
            valid_ranges.min_exp_time * 1.0e6,
            valid_ranges.max_exp_time * 1.0e3,
            (double)(_dwMinExposureStepDESC * 1.0e-3));

        ptr += __sprintfSExt(
            ptr, ptrMax - ptr,
            "* ...    delay valid min[%g us] max[%g ms] step[%g us]\n",
            valid_ranges.min_lat_time * 1.0e6,
            valid_ranges.max_lat_time * 1.0e3,
            (double)(_dwMinDelayStepDESC * 1.0e-3));

        ptr +=
            __sprintfSExt(ptr, ptrMax - ptr,
                          "* ...    cocRunTime[%g] (s) frameRate[%g] (fps)\n",
                          m_pcoData->cocRunTime, m_pcoData->frameRate);
    }

    //------ DIMAX
    if ((flag & CAMINFO_DIMAX) && (_isCameraType(Dimax | Pco2k | Pco4k)))
    {
        unsigned int bytesPerPix;
        getBytesPerPixel(bytesPerPix);
        WORD bitsPerPix;
        getBitsPerPixel(bitsPerPix);
        int segmentPco = m_pcoData->wActiveRamSegment;
        int segmentArr = segmentPco - 1;

        ptr += __sprintfSExt(ptr, ptrMax - ptr, "*** DIMAX - 2k - 4k info \n");
        ptr +=
            __sprintfSExt(ptr, ptrMax - ptr,
                          "* pagesInRam[%d] pixPerPage[%d] bitsPerPix[%d] "
                          "ramGB[%.3g] bytesPerPix[%d] imgGB[%.3g]\n",
                          (int)m_pcoData->dwRamSize,
                          (int)m_pcoData->wPixPerPage, (int)bitsPerPix,
                          (double)((1.0e-9 * m_pcoData->dwRamSize) *
                                   m_pcoData->wPixPerPage * bitsPerPix / 8.0),
                          bytesPerPix,
                          (double)((1.0e-9 * m_pcoData->dwRamSize) *
                                   m_pcoData->wPixPerPage * bytesPerPix));

        ptr += __sprintfSExt(ptr, ptrMax - ptr, "* PcoActiveSegment=[%d]\n",
                             segmentArr + 1);
        ptr += __sprintfSExt(
            ptr, ptrMax - ptr,
            "* m_pcoData->dwMaxFramesInSegment[%d]=[%d frames]\n", segmentArr,
            (int)m_pcoData->dwMaxFramesInSegment[segmentArr]);
        ptr += __sprintfSExt(
            ptr, ptrMax - ptr, "* m_pcoData->dwSegmentSize[%d]=[%d pages]\n",
            segmentArr, (int)m_pcoData->dwSegmentSize[segmentArr]);
        ptr += __sprintfSExt(
            ptr, ptrMax - ptr, "* m_pcoData->dwValidImageCnt[%d]=[%d]\n",
            segmentArr, (int)m_pcoData->dwValidImageCnt[segmentArr]);
        ptr += __sprintfSExt(
            ptr, ptrMax - ptr, "* m_pcoData->dwMaxImageCnt[%d]=[%d]\n",
            segmentArr, (int)m_pcoData->dwMaxImageCnt[segmentArr]);

        int err;
        _pco_GetSegmentInfo(err);

        struct stcSegmentInfo *_stc;
        for (int iseg = 0; iseg < PCO_MAXSEGMENTS; iseg++)
        {
            _stc = &m_pcoData->m_stcSegmentInfo[iseg];
            ptr += __sprintfSExt(ptr, ptrMax - ptr,
                                 "* segmentInformation seg[%d][%d] err[%d]\n",
                                 iseg + 1, _stc->iSegId, _stc->iErr);
            ptr += __sprintfSExt(
                ptr, ptrMax - ptr,
                "*    bin[%d,%d] res[%d,%d] roiX[%d,%d] roiY[%d,%d]\n",
                (int)_stc->wBinHorz, (int)_stc->wBinVert, (int)_stc->wXRes,
                (int)_stc->wYRes, (int)_stc->wRoiX0, (int)_stc->wRoiX1,
                (int)_stc->wRoiY0, (int)_stc->wRoiY1);
            ptr += __sprintfSExt(
                ptr, ptrMax - ptr, "*    validImgCount[%d] maxImgCount[%d]\n",
                (int)_stc->dwValidImageCnt, (int)_stc->dwMaxImageCnt);
        }

        _pco_GetStorageMode_GetRecorderSubmode();

        ptr +=
            __sprintfSExt(ptr, ptrMax - ptr,
                          "* mode[%s] storage_mode[%d] recorder_submode[%d]\n",
                          m_pcoData->storage_str, (int)m_pcoData->storage_mode,
                          (int)m_pcoData->recorder_submode);
        ptr += __sprintfSExt(
            ptr, ptrMax - ptr,
            "* Acq: rec[%ld] xfer[%ld] recNow[%ld] recTout[%ld] (ms) [%s]\n",
            m_pcoData->msAcqRec, m_pcoData->msAcqXfer, m_pcoData->msAcqTnow,
            m_pcoData->msAcqTout,
            getTimestamp(Iso, m_pcoData->msAcqRecTimestamp));
    }

    //------ unsorted

    if (flag & CAMINFO_CAMERALINK)
    {
        if (_isInterfaceType(ifCameralinkAll))
        {
            ptr += __sprintfSExt(ptr, ptrMax - ptr,
                                 "*** CameraLink transfer parameters\n");
            ptr += __sprintfSExt(ptr, ptrMax - ptr, "*  settings[%s]\n",
                                 m_pcoData->sClTransferParameterSettings);
            ptr += __sprintfSExt(
                ptr, ptrMax - ptr, "*  baudrate[%u] %g Kbps\n",
                (int)m_pcoData->clTransferParam.baudrate,
                (double)(m_pcoData->clTransferParam.baudrate / 1000.));
            ptr += __sprintfSExt(
                ptr, ptrMax - ptr, "*clock freq[%u] %g MHz\n",
                (int)m_pcoData->clTransferParam.ClockFrequency,
                (double)(m_pcoData->clTransferParam.ClockFrequency / 1000000.));
            ptr += __sprintfSExt(ptr, ptrMax - ptr, "*    CCline[%u]\n",
                                 (int)m_pcoData->clTransferParam.CCline);
            ptr += __sprintfSExt(ptr, ptrMax - ptr, "*DataFormat[0x%x]\n",
                                 (int)m_pcoData->clTransferParam.DataFormat);
            ptr += __sprintfSExt(ptr, ptrMax - ptr, "*  Transmit[%u]\n",
                                 (int)m_pcoData->clTransferParam.Transmit);
        }
        else
        {
            ptr += __sprintfSExt(ptr, ptrMax - ptr,
                                 "*** CameraLink transfer parameters - NO "
                                 "CAMERALINK interface\n");
        }
    }

    if (flag & CAMINFO_UNSORTED)
    {
        ptr += __sprintfSExt(ptr, ptrMax - ptr, "*** unsorted parameters\n");
        ptr += __sprintfSExt(
            ptr, ptrMax - ptr,
            "* bMetaDataAllowed[%d] wMetaDataMode[%d] wMetaDataSize[%d] "
            "wMetaDataVersion[%d] \n",
            (int)m_pcoData->bMetaDataAllowed, (int)m_pcoData->wMetaDataMode,
            (int)m_pcoData->wMetaDataSize, (int)m_pcoData->wMetaDataVersion);

        ptr += __sprintfSExt(
            ptr, ptrMax - ptr,
            "* wLUT_Identifier[x%04x] wLUT_Parameter [x%04x]\n",
            (int)m_pcoData->wLUT_Identifier, (int)m_pcoData->wLUT_Parameter);

        int alignment;
        const char *res;
        _pco_GetBitAlignment(alignment);
        res = alignment == 0 ? "[0][MSB]" : "[1][LSB]";
        ptr +=
            __sprintfSExt(ptr, ptrMax - ptr, "* data bit alignment%s\n", res);

        WORD wTimeStampMode;
        const char *mode;
        int error;
        _pco_GetTimestampMode(wTimeStampMode, error);
        if (error)
            wTimeStampMode = 100;
        switch (wTimeStampMode)
        {
            case 0:
                mode = "no stamp in image";
                break;
            case 1:
                mode = "BCD coded stamp in the first 14 pixel";
                break;
            case 2:
                mode = "BCD coded stamp in the first 14 pixel + ASCII text";
                break;
            case 3:
                mode = "ASCII text only";
                break;
            default:
                mode = "unknown";
                break;
        }

        ptr +=
            __sprintfSExt(ptr, ptrMax - ptr, "* imageTimestampMode[%d][%s]\n",
                          (int)wTimeStampMode, mode);

        _pco_GetTemperatureInfo(error);

        ptr += __sprintfSExt(ptr, ptrMax - ptr,
                             "* temperature: CCD[%.1f]  CAM[%d]  PS[%d]\n",
                             (double)(m_pcoData->temperature.sCcd / 10.),
                             (int)m_pcoData->temperature.sCam,
                             (int)m_pcoData->temperature.sPower);

        ptr += __sprintfSExt(
            ptr, ptrMax - ptr,
            "*    cooling: min[%d]  max[%d]  default[%d]  Setpoint[%d]\n",
            (int)m_pcoData->temperature.sMinCoolSet,
            (int)m_pcoData->temperature.sMaxCoolSet,
            (int)m_pcoData->temperature.sDefaultCoolSet,
            (int)m_pcoData->temperature.sSetpoint);
    }

    if (flag & CAMINFO_BLOCK)
    {
        lgbuff = ptrMax - ptr;
        int used = int((100. * (lgbuffmax - lgbuff)) / lgbuffmax);
        ptr += __sprintfSExt(ptr, ptrMax - ptr,
                             "\n*** camInfo (buff free[%lld B] used[%d %%] "
                             "size[%lld B]) [end]\n",
                             lgbuff, used, lgbuffmax);
    }

    return ptr;
}

//===============================================================
//===============================================================
// include these definitions in the PreprocessorDefinitions
//    liblimapco -> properties -> c/c++ -> preprocessor -> preprocessor
//    definitions
//
//    VS_PLATFORM=$(PlatformName)
//    VS_CONFIGURATION=$(ConfigurationName)
//===============================================================
//===============================================================

#define Win32 "Win32"
#define x64 "x64"
#define Release_Win7_Sync "Release_Win7_Sync"
#define Release "Release"

#ifndef VS_PLATFORM
#    pragma message("--- VS_PLATFORM - UNDEFINED")
#    define VS_PLATFORM "undef"
#endif

#ifndef VS_CONFIGURATION
#    pragma message("--- VS_CONFIGURATION - UNDEFINED")
#    define VS_CONFIGURATION "undef"
#endif

#ifndef VS_PLATFORM
#    pragma message("--- VS_PLATFORM - UNDEFINED")
#    define VS_PLATFORM "undef"
#endif

#ifndef VS_CONFIGURATION
#    pragma message("--- VS_CONFIGURATION - UNDEFINED")
#    define VS_CONFIGURATION "undef"
#endif

char *_getVSconfiguration(char *infoBuff, DWORD bufCharCount)
{
    __sprintfSExt(infoBuff, bufCharCount, "platform[%s] configuration[%s]",
                  VS_PLATFORM, VS_CONFIGURATION);
    return infoBuff;
}
//====================================================================
//====================================================================

void Camera::_traceMsg(char *msg)
{
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << "\n>>>  " << msg;
}

//====================================================================
//====================================================================
#define LEN_COMMENT 511
const char *Camera::_sprintComment(bool bAlways, const char *comment,
                                   const char *comment1, const char *comment2)
{
    DEB_MEMBER_FUNCT();

    static char buff[LEN_COMMENT + 1];

    __sprintfSExt(buff, LEN_COMMENT, "%s %s %s", comment, comment1, comment2);

    if (bAlways)
    {
        DEB_ALWAYS() << buff;
    }

    return buff;
}

long msElapsedTime(std::chrono::steady_clock::time_point const& start)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
}

long usElapsedTime(std::chrono::steady_clock::time_point const& start)
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count();
}

//------------------------------------------------

double usElapsedTimeTicsPerSec()
{
    double ticks = 0;
#ifdef __linux__
    TIME_UTICKS tick; // A point in time
    clock_gettime(CLOCK_REALTIME, &tick);

#else
    LARGE_INTEGER ticksPerSecond;
    QueryPerformanceFrequency(&ticksPerSecond);
    ticks = (double)ticksPerSecond.QuadPart;
#endif
    return ticks;
}

//==========================================================================================================
//==========================================================================================================

int __sprintfSExt(char *ptr, size_t nrMax, const char *format, ...)
{
    char *ptrMax = ptr + nrMax;

    int nr;

    if (nrMax <= 1)
    {
        *ptr = 0;
        return 0;
    }

    va_list args;
    va_start(args, format);

#ifdef __linux__
    // int vsnprintf(char*, size_t, const char*, __va_list_tag*)
    nr = vsnprintf(ptr, nrMax, format, args);
#else
    nr = vsnprintf_s(ptr, nrMax, _TRUNCATE, format, args);
#endif

    va_end(args);

    if (nr >= 0)
    {
        *(ptr + nr) = 0;
        return nr;
    }

    if (nrMax > 1)
    {
        *ptr = '@';
        *(ptr + 1) = 0;
        return 1;
    }

    *ptr = 0;
    return 0;
}

//====================================================================
//====================================================================
const char *_getEnviroment(const char *env)
{
#ifndef __linux__
    return "_getEnviroment: WIN - not defined";
#else
    return std::getenv(env);
#endif
}

//====================================================================
//====================================================================
const char *_getOs()
{
    static char os[MSG1K];

#ifndef __linux__
    return "WINDOWS";
#else
    struct utsname buff;
    int err, nr;

    err = uname(&buff);

    nr = snprintf(os, sizeof(os), "os[%s] rel[%s] ver[%s] mach[%s]",
                  buff.sysname, buff.release, buff.version, buff.machine);

    return os;
#endif
}

//==========================================================================================================
//==========================================================================================================
