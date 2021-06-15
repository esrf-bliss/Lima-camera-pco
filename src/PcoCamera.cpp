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

#define PCO_ERRT_H_CREATE_OBJECT
#define BYPASS

#define TOUT_MIN_DIMAX 500
#define ERROR_MSG_LINE 128

//#define BUFF_INFO_SIZE 5000

#include <cstdlib>

#    include <sys/stat.h>
#    include <sys/time.h>

#include <time.h>

#include "lima/Exceptions.h"
#include "lima/HwSyncCtrlObj.h"

#include "PcoCamera.h"
#include "PcoSyncCtrlObj.h"
#include "PcoBufferCtrlObj.h"
#include "PcoCameraUtils.h"

using namespace lima;
using namespace lima::Pco;

// const char *timebaseUnits[] = {"ns", "us", "ms"};

// char *_checkLogFiles();

const char *_timestamp_pcosyncctrlobj();
const char *_timestamp_pcointerface();
const char *_timestamp_pcobufferctrlobj();
const char *_timestamp_pcodetinfoctrlobj();
const char *_timestamp_pcocamerautils();
const char *_timestamp_pcoroictrlobj();

char *_split_date(const char *s);
int __xlat_date(char *s1, char &ptrTo, int lenTo);
char *_xlat_date(char *s1, char *s2, char *s3);

//=========================================================================================================
//=========================================================================================================
const char *_timestamp_pcocamera()
{
    return ID_FILE_TIMESTAMP;
}

#ifdef WITH_GIT_VERSION
#    include "PcoGitVersion.h"

char *_timestamp_gitversion(char *buffVersion, int len)
{
    __sprintfSExt(buffVersion, len,
#    ifdef PCO_GIT_VERSION
                  PCO_GIT_VERSION "\n"
#    endif
#    ifdef PCO_SDK_VERSION
                  PCO_SDK_VERSION "\n"
#    endif
#    ifdef PROCLIB_GIT_VERSION
                  PROCLIB_GIT_VERSION "\n"
#    endif
#    ifdef LIBCONFIG_GIT_VERSION
                  LIBCONFIG_GIT_VERSION "\n"
#    endif
#    ifdef LIMA_GIT_VERSION
                  LIMA_GIT_VERSION "\n"
#    endif
#    ifdef TANGO_GIT_VERSION
                  TANGO_GIT_VERSION "\n"
#    endif
#    ifdef SPEC_GIT_VERSION
                  SPEC_GIT_VERSION "\n"
#    endif
                                  "\n");
    return buffVersion;
}
#endif

//=========================================================================================================

//=========================================================================================================
//=========================================================================================================
const char *Camera::xlatCode2Str(int code, struct stcXlatCode2Str *stc)
{
    const char *type;

    while ((type = stc->str) != NULL)
    {
        if (stc->code == code)
            return type;
        stc++;
    }

    return NULL;
}

//=========================================================================================================
//=========================================================================================================

const char *Camera::_xlatPcoCode2Str(int code, enumTblXlatCode2Str table,
                                     int &err)
{
    DEB_CONSTRUCTOR();
    struct stcXlatCode2Str modelType[] = {
        {CAMERATYPE_PCO1200HS, "PCO 1200 HS"},
        {CAMERATYPE_PCO1300, "PCO 1300"},
        {CAMERATYPE_PCO1400, "PCO 1400"},
        {CAMERATYPE_PCO1600, "PCO 1600"},
        {CAMERATYPE_PCO2000, "PCO 2000"},
        {CAMERATYPE_PCO4000, "PCO 4000"},
        {CAMERATYPE_PCO_DIMAX_STD, "PCO DIMAX STD"},
        {CAMERATYPE_PCO_DIMAX_TV, "PCO DIMAX TV"},
        {CAMERATYPE_PCO_DIMAX_AUTOMOTIVE, "PCO DIMAX AUTOMOTIVE"},
        {CAMERATYPE_PCO_EDGE, "PCO EDGE 5.5 RS"},
        {CAMERATYPE_PCO_EDGE_42, "PCO EDGE 4.2 RS"},
        {CAMERATYPE_PCO_EDGE_GL, "PCO EDGE 5.5 GL"},
        {CAMERATYPE_PCO_EDGE_USB3, "PCO EDGE USB3"},
        {CAMERATYPE_PCO_EDGE_HS, "PCO EDGE hs"},
        {CAMERATYPE_PCO_EDGE, "PCO EDGE"},
        {CAMERATYPE_PCO_EDGE_GL, "PCO EDGE GL"},
        {0, "NO_modelType"},
        {0, NULL}};

    struct stcXlatCode2Str modelSubType[] = {
        {CAMERATYPE_PCO1200HS, "PCO 1200 HS"},
        {CAMERASUBTYPE_PCO_DIMAX_Weisscam, "DIMAX_Weisscam"},
        {CAMERASUBTYPE_PCO_DIMAX_HD, "DIMAX_HD"},
        {CAMERASUBTYPE_PCO_DIMAX_HD_plus, "DIMAX_HD_plus"},
        {CAMERASUBTYPE_PCO_DIMAX_X35, "DIMAX_X35"},
        {CAMERASUBTYPE_PCO_DIMAX_HS1, "DIMAX_HS1"},
        {CAMERASUBTYPE_PCO_DIMAX_HS2, "DIMAX_HS2"},
        {CAMERASUBTYPE_PCO_DIMAX_HS4, "DIMAX_HS4"},

        {CAMERASUBTYPE_PCO_EDGE_SPRINGFIELD, "EDGE_SPRINGFIELD"},
        {CAMERASUBTYPE_PCO_EDGE_31, "EDGE_31"},
        {CAMERASUBTYPE_PCO_EDGE_42, "EDGE_42"},
        {CAMERASUBTYPE_PCO_EDGE_55, "EDGE_55"},
        {CAMERASUBTYPE_PCO_EDGE_DEVELOPMENT, "EDGE_DEVELOPMENT"},
        {CAMERASUBTYPE_PCO_EDGE_X2, "EDGE_X2"},
        {CAMERASUBTYPE_PCO_EDGE_RESOLFT, "EDGE_RESOLFT"},
        {CAMERASUBTYPE_PCO_EDGE_GOLD, "EDGE_GOLD"},
        {CAMERASUBTYPE_PCO_EDGE_DUAL_CLOCK, "DUAL_CLOCK"},
        {CAMERASUBTYPE_PCO_EDGE_DICAM, "DICAM"},
        {CAMERASUBTYPE_PCO_EDGE_42_LT, "EDGE_42_LT"},
        {0, "NO_subType"},
        {0, NULL}};

    struct stcXlatCode2Str interfaceType[] = {
        {INTERFACE_FIREWIRE, "FIREWIRE"},
        {INTERFACE_CAMERALINK, "CAMERALINK"},
        {INTERFACE_USB, "USB"},
        {INTERFACE_ETHERNET, "ETHERNET"},
        {INTERFACE_SERIAL, "SERIAL"},
        {INTERFACE_USB3, "USB3"},
        {INTERFACE_CAMERALINKHS, "CAMERALINK_HS"},
        {INTERFACE_COAXPRESS, "COAXPRESS"},
        {0, "NO_interfaceType"},
        {0, NULL}};

    struct stcXlatCode2Str *stc;
    const char *ptr;
    static char buff[BUFF_XLAT_SIZE + 1];
    const char *errTable;

    switch (table)
    {
        case ModelType:
            stc = modelType;
            errTable = "modelType";
            break;
        case ModelSubType:
            stc = modelSubType;
            errTable = "modelSubType";
            break;
        case InterfaceType:
            stc = interfaceType;
            errTable = "interfaceType";
            break;

        default:
            __sprintfSExt(buff, BUFF_XLAT_SIZE, "UNKNOWN XLAT TABLE [%d]",
                          table);
            err = 1;
            return buff;
    }

    if ((ptr = xlatCode2Str(code, stc)) != NULL)
    {
        err = 0;
        return ptr;
    }

    if ((stc == modelSubType) && _isCameraType(Dimax))
    {
        switch (code)
        {
            case 0:
                ptr = "STD";
                break;
            case 100:
                ptr = "OEM Variant 100";
                break;
            case 200:
                ptr = "OEM Variant 200";
                break;
            default:
                switch (code / 0x100)
                {
                    case 0x01:
                        ptr = "S1";
                        break;
                    case 0x02:
                        ptr = "S2";
                        break;
                    case 0x04:
                        ptr = "S4";
                        break;

                    case 0x80:
                        ptr = "HD";
                        break;
                    case 0xC0:
                        ptr = "HD+";
                        break;

                    case 0x20:
                        ptr = "HS1";
                        break;
                    case 0x21:
                        ptr = "HS2";
                        break;
                    case 0x23:
                        ptr = "HS4";
                        break;

                    default:
                        ptr = NULL;
                } // switch(camtype.wCamSubType / 0x100) ...
        }         // switch(camtype.wCamSubType) ...

        if (ptr)
        {
            __sprintfSExt(buff, BUFF_XLAT_SIZE,
                          "subType DIMAX %s code [0x%04x]", ptr, code);
            err = 0;
            return buff;
        }
    }

    {
        __sprintfSExt(buff, BUFF_XLAT_SIZE, "UNKNOWN %s code [0x%04x]",
                      errTable, code);
        err = 1;
        return buff;
    }
}

//=========================================================================================================
//=========================================================================================================

#define BUFFER_LEN 256
#define BUFFVERSION_LEN (MSG8K)
stcPcoData::stcPcoData()
{
    char *ptr, *ptrMax;
    int i;
    char buff[BUFFER_LEN + 1];

    memset(this, 0, sizeof(struct stcPcoData));

    version = new char[BUFFVERSION_LEN];
    if (!version)
    {
        throw LIMA_HW_EXC(Error, "version > creation error");
    }

    ptr = version;
    *ptr = 0;
    ptrMax = ptr + BUFFVERSION_LEN - 1;

    ptr += __sprintfSExt(ptr, ptrMax - ptr, "\n");
    ptr += __sprintfSExt(ptr, ptrMax - ptr, "%s\n",
                         _split_date(_timestamp_pcocamera()));
    ptr += __sprintfSExt(ptr, ptrMax - ptr, "%s\n",
                         _split_date(_timestamp_pcosyncctrlobj()));
    ptr += __sprintfSExt(ptr, ptrMax - ptr, "%s\n",
                         _split_date(_timestamp_pcointerface()));
    ptr += __sprintfSExt(ptr, ptrMax - ptr, "%s\n",
                         _split_date(_timestamp_pcobufferctrlobj()));
    ptr += __sprintfSExt(ptr, ptrMax - ptr, "%s\n",
                         _split_date(_timestamp_pcodetinfoctrlobj()));
    ptr += __sprintfSExt(ptr, ptrMax - ptr, "%s\n",
                         _split_date(_timestamp_pcocamerautils()));
    ptr += __sprintfSExt(ptr, ptrMax - ptr, "%s\n",
                         _split_date(_timestamp_pcoroictrlobj()));

#ifdef WITH_GIT_VERSION
    char buffVersion[BUFFVERSION_LEN + 1];
    ptr += __sprintfSExt(ptr, ptrMax - ptr, "%s\n",
                         _timestamp_gitversion(buffVersion, BUFFVERSION_LEN));
#endif

    ptr += __sprintfSExt(ptr, ptrMax - ptr, "       timestamp: %s\n",
                         getTimestamp(Iso));
    ptr += __sprintfSExt(ptr, ptrMax - ptr, "   computer name: %s\n",
                         _getComputerName(buff, BUFFER_LEN));
    ptr += __sprintfSExt(ptr, ptrMax - ptr, "              OS: %s\n", _getOs());
    ptr += __sprintfSExt(ptr, ptrMax - ptr, "       user name: %s\n",
                         _getUserName(buff, BUFFER_LEN));
    ptr += __sprintfSExt(ptr, ptrMax - ptr, "   configuration: %s\n",
                         _getVSconfiguration(buff, BUFFER_LEN));
    ptr += __sprintfSExt(ptr, ptrMax - ptr, "        SISO dir: %s\n",
                         _getEnviroment("SISODIR5"));

    ptr += __sprintfSExt(ptr, ptrMax - ptr, " PCO SDK version: %s\n",
                         _getPcoSdkVersion(buff, BUFFER_LEN, NULL));

    stcPcoGeneral.wSize = sizeof(stcPcoGeneral);
    stcPcoGeneral.strCamType.wSize = sizeof(stcPcoGeneral.strCamType);
    stcPcoCamType.wSize = sizeof(stcPcoCamType);
    stcPcoSensor.wSize = sizeof(stcPcoSensor);
    stcPcoSensor.strDescription.wSize = sizeof(stcPcoSensor.strDescription);
    stcPcoSensor.strDescription2.wSize = sizeof(stcPcoSensor.strDescription2);
    stcPcoDescription.wSize = sizeof(stcPcoDescription);
    stcPcoTiming.wSize = sizeof(stcPcoTiming);
    stcPcoStorage.wSize = sizeof(stcPcoStorage);
    stcPcoRecording.wSize = sizeof(stcPcoRecording);

    for (i = 0; i < SIZEARR_stcPcoHWIOSignal; i++)
    {
        stcPcoHWIOSignal[i].wSize = sizeof(stcPcoHWIOSignal[i]);
        stcPcoHWIOSignalDesc[i].wSize = sizeof(stcPcoHWIOSignalDesc[i]);
    }

    bAllocatedBufferDone = false;

    msAcqRecTimestamp = msAcqXferTimestamp = getTimestamp();
}



//=========================================================================================================
// Camera::stopAcq
//=========================================================================================================
void Camera::stopAcq(bool clearQueue)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    int _stopRequestIn, _stopRequestOut, _nrStop;
    bool _started, _started0;
    bool resWait;

    _stopRequestIn = getRequestStop(_nrStop);


    DEB_TRACE() << "--- [ENTRY]";

    _started0 = getStarted();

 
    setRequestStop(stopRequest);
    
    
    int iSleepTimeoutMs = 15000;
    int iSleepMs = 100;


    while ((_started=getStarted()) && ((iSleepTimeoutMs-=iSleepMs) > 0) )
    {
        DEB_TRACE() << "waiting for getStarted() RETRY! " << DEB_VAR1(iSleepTimeoutMs);
        setRequestStop(stopRequest);
        Sleep_ms(iSleepMs);
    }

    if (_started)
    {
        Event *ev = new Event(Hardware,Event::Error,Event::Camera,Event::CamFault, "ERROR can NOT STOP");
        _getPcoHwEventCtrlObj()->reportEvent(ev);
    }    

    DEB_TRACE() << " [exit]: " 
        <<  DEB_VAR6(_started0, _started, _stopRequestIn, _stopRequestOut, _nrStop, resWait);
}

//=========================================================================================================
int Camera::getRequestStop(int &nrStop)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;


    AutoMutex lock(m_cond_a.mutex());

    nrStop = m_requestStopRetry;

    DEB_TRACE() << DEB_VAR2(m_requestStop, nrStop);

    return m_requestStop;
}

//=========================================================================================================

void Camera::setRequestStop(int requestStop)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    AutoMutex lock(m_cond_a.mutex());

    int m_requestStop0 = m_requestStop;

    switch (requestStop)
    {
        case stopNone:
            m_requestStopRetry = 0;
            m_requestStop = requestStop;
            break;

        case stopRequest:
            m_requestStopRetry++;
            m_requestStop = requestStop;
            break;
    }
    DEB_TRACE() << DEB_VAR4(m_requestStop0, m_requestStop, m_requestStopRetry,
                             requestStop);
}

//=========================================================================================================
//=========================================================================================================


//=========================================================================================================
//=========================================================================================================


//=========================================================================================================
// startAcq
//=========================================================================================================
void Camera::startAcq()
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    char msg[512];

    bool _started = getStarted();

    DEB_TRACE() << "[ENTRY]";

    if(_started)
    {
        DEB_ALWAYS() << "WARNING camera was started - BYPASSED" ;
        return;
    }

    _camInfo(msg, msg + sizeof(msg), CAMINFO_ACQ);

    DEB_ALWAYS() << msg << " [ENTRY]";

    if (m_buffer)
    {
        m_buffer->startAcq();
    }

    setRequestStop(stopNone);
    setExposing(pcoAcqStart);
    __startAcq();
    if (getExposing() == pcoAcqError)
    {
        setStarted(false);
        DEB_ALWAYS() << "ERROR during __startAcq()" ;
        return;
    }

    int iSleepTimeoutMs = 5000;
    int iSleepMs = 100;


    while ((!(_started=getStarted())) && ((iSleepTimeoutMs-=iSleepMs) > 0))
    {
        DEB_TRACE() << "waiting for getStarted() RETRY! " << DEB_VAR1(iSleepTimeoutMs);
        Sleep_ms(iSleepMs);
    }


    if (!_started)
    {
        DEB_ALWAYS() << "ERROR never started getStarted()" ;
        setStarted(false);
        return;
    }
    
}

//=========================================================================================================

//=========================================================================================================
//=========================================================================================================
bool Camera::getStarted()
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    AutoMutex lock(m_cond_a.mutex());

    DEB_TRACE() <<  DEB_VAR1(m_started);
 
    return m_started;
}

//=========================================================================================================
void Camera::setStarted(bool started)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    AutoMutex lock(m_cond_a.mutex());

    bool m_started0 = m_started;
    m_started = started;

    DEB_TRACE() << DEB_VAR3(m_started0, m_started, started);
}


//=========================================================================================================
void Camera::setExposing(pcoAcqStatus exposing)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    AutoMutex lock(m_cond_a.mutex());

    m_exposing = exposing;

    DEB_TRACE() << DEB_VAR2(m_exposing, exposing);
}

//=========================================================================================================
pcoAcqStatus Camera::getExposing()
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    AutoMutex lock(m_cond_a.mutex());

    DEB_TRACE() << DEB_VAR1(m_exposing);
    return m_exposing;
}
//=========================================================================================================
//=========================================================================================================



//=========================================================================================================
//=========================================================================================================
bool Camera::getProperty(const char *key, char *&value)
{
    // DEF_FNID;
    DEB_CONSTRUCTOR();
    bool ret;

    for (int i = 0; i < m_pcoData->properties.nr; i++)
    {
        if (_stricmp(key, m_pcoData->properties.ptrKey[i]) == 0)
        {
            ret = true;
            value = m_pcoData->properties.ptrValue[i];
            DEB_TRACE() << DEB_VAR3(key, ret, value);
            return ret;
        }
    }
    ret = false;
    value = (char *)"";
    DEB_TRACE() << DEB_VAR3(key, ret, value);
    return ret;
}

//=========================================================================================================
//=========================================================================================================

// pco -> properties -> params
//   params   	bitAlignment=MSB
// 				trigSingleMulti=1
// 				logBits=FFFF
// 				logPath=/tmp
//
// received params str:
//      "key1 = value1 ; key2 = value2 ; key3 ; ...."
//       str=bitAlignment=MSB;trigSingleMulti=1;logBits=FFFF;logPath=/tmp;

void Camera::init_properties(const char *str)
{
    DEF_FNID;
    DEB_CONSTRUCTOR();

    DEB_TRACE() << "[ENTRY] " << DEB_VAR1(str);

    int i;
    char *tokNext = NULL;
    char *buff = m_pcoData->properties.buff;
    int &nrList = m_pcoData->properties.nr;
    int nr;

    memset((void *)&m_pcoData->properties, 0, sizeof(m_pcoData->properties));

    // --- split of params string
    strcpy_s(buff, PROPERTIES_LEN_BUFF, str);
    char *ptr = buff;

    for (nr = i = 0; i < PROPERTIES_NR; i++)
    {
        if ((m_pcoData->properties.ptrKey[i] = strtok_s(ptr, ";", &tokNext)) ==
            NULL)
            break;
        ptr = NULL;
        nr = i + 1;
    }

    nrList = 0;

    for (i = 0; i < nr; i++)
    {
        char *key, *value;
        bool found;

        ptr = str_trim(m_pcoData->properties.ptrKey[i]);
        key = strtok_s(ptr, "=", &tokNext);
        value = strtok_s(NULL, "=", &tokNext);
        str_toupper(key);
        m_pcoData->properties.ptrKey[i] = key = str_trim(key);
        value = str_trim(value);
        if (value == NULL)
            value = (char *)"";
        m_pcoData->properties.ptrValue[i] = value;

        found = false;
        for (int j = 0; j < nrList; j++)
        {
            if (_stricmp(m_pcoData->properties.ptrKey[j],
                         m_pcoData->properties.ptrKey[i]) == 0)
            {
                m_pcoData->properties.ptrValue[j] = m_pcoData->properties.ptrValue[i];
                found = true;
                break;
            }
        }
        if (!found)
        {
            key = m_pcoData->properties.ptrKey[nrList] =
                m_pcoData->properties.ptrKey[i];
            value = m_pcoData->properties.ptrValue[nrList] =
                m_pcoData->properties.ptrValue[i];
            nrList++;
        }
    }

    char bla[MSG4K + 1];
   
	__sprintfSExt(bla, sizeof(bla), "Properties [%d]\n",nrList);
	m_log.append(bla);

    for (int j = 0; j < nrList; j++)
    {
        char *key, *value;
        key = m_pcoData->properties.ptrKey[j];
        value = m_pcoData->properties.ptrValue[j];
        DEB_TRACE() << DEB_VAR3(j, key, value);

		__sprintfSExt(bla, sizeof(bla), "  %15s : %s\n",key, value);
		m_log.append(bla);
    }
    DEB_TRACE() << "[EXIT]";
};

//=========================================================================================================
//=========================================================================================================
void Camera::_init()
{
    DEB_CONSTRUCTOR();
    DEF_FNID;

    DEB_ALWAYS() << fnId << " [INIT]";

    int error = 0;
    char msg[MSG4K + 1];
	const char *cmsg;

    // ----- linux [begin]

    UNUSED const char *pcoFn;

    //-------------------- linux

    board = 0;
    int iErr;
    
    char *value;
    bool ret = getProperty("portNr", value);
    board = ret ? atoi(value) : 0;


    // int bufnum=20;


    __sprintfSExt(msg, sizeof(msg), "*** %s (linux) [%s] [ENTRY]\n",fnId, getTimestamp(Iso));
 	DEB_ALWAYS() << msg;
	m_log.append(msg);
    
    // SDK call -> depends on the interface!
    //THROW_FATAL(Hardware, Error) << "TRACE1";

    _pco_Open_Cam(iErr);
    if (iErr)
    {
        camera->Close_Cam();
        delete camera;
        camera = NULL;
        DEB_ALWAYS() << "FATAL - _pco_Open_Cam " << DEB_VAR2(iErr, board);
        THROW_HW_ERROR(Error);
    }

     __sprintfSExt(msg, sizeof(msg), "_pco_GetCameraType\n");
	DEB_ALWAYS() << msg;
	m_log.append(msg);
   
    _pco_GetCameraType(iErr);
    if (iErr)
    {
        DEB_ALWAYS() << "WARNING - _pco_GetCameraType " << DEB_VAR1(iErr);
    }

    DEB_ALWAYS() << "++++++++++++++ _pco_Open_Grab";
    _pco_Open_Grab(iErr);
    __sprintfSExt(msg, sizeof(msg), "_pco_Open_Grab err[0x%08x]\n", iErr );
	m_log.append(msg);
	DEB_ALWAYS() << msg;

    __sprintfSExt(msg, sizeof(msg), "_pco_GetCameraInfo\n");
	m_log.append(msg);
	DEB_ALWAYS() << msg;
    _pco_GetCameraInfo(iErr);
    
    __sprintfSExt(msg, sizeof(msg), "_pco_ResetSettingsToDefault\n");
	m_log.append(msg);
	DEB_ALWAYS() << msg;
    _pco_ResetSettingsToDefault(iErr);

    __sprintfSExt(msg, sizeof(msg), "_pco_SetCameraToCurrentTime\n");
	m_log.append(msg);
	DEB_ALWAYS() << msg;
    _pco_SetCameraToCurrentTime(iErr);

    __sprintfSExt(msg, sizeof(msg), "_pco_GetTransferParameter\n");
	m_log.append(msg);
	DEB_ALWAYS() << msg;
    _pco_GetTransferParameter(iErr);

    _pco_GetTemperatureInfo(iErr);
    __sprintfSExt(msg, sizeof(msg), "_pco_GetTemperatureInfo err[0x%08x]\n", iErr );
	m_log.append(msg);
	DEB_ALWAYS() << msg;

    DWORD pixRateActual, pixRateNext;
    _pco_GetPixelRate(pixRateActual, pixRateNext, iErr);
    DEB_ALWAYS() << DEB_VAR2(pixRateActual, pixRateNext);
    __sprintfSExt(msg, sizeof(msg), "_pco_GetPixelRate err[0x%08x]\n"
		"  pixRateActual[%d] pixRateNext[%d]\n", iErr, pixRateActual, pixRateNext );
	m_log.append(msg);
    DEB_ALWAYS() << msg;

    _pco_GetLut(iErr);
    __sprintfSExt(msg, sizeof(msg), "_pco_GetLut err[0x%08x]\n", iErr );
	m_log.append(msg);
	DEB_ALWAYS() << msg;

    _pco_SetRecordingState(0, iErr);
    __sprintfSExt(msg, sizeof(msg), "_pco_SetRecordingState(0) err[0x%08x]\n", iErr);
	m_log.append(msg);
    DEB_ALWAYS() << msg;

    //_pco_SetTimestampMode(2, iErr);

    DEB_ALWAYS() << "++++++++++++++ _pco_initHWIOSignal";
    // 0x01: "Low level active"
    // 0x02: "High Level active"
    // 0x04: "Rising edge active"
    // 0x08: "Falling edge active"
    _pco_initHWIOSignal(0, 0x04, iErr);
    __sprintfSExt(msg, sizeof(msg), "_pco_initHWIOSignal(0, 0x04) err[0x%08x]\n", iErr);
	m_log.append(msg);
    DEB_ALWAYS() << msg;


    __sprintfSExt(msg, sizeof(msg), "*** %s (linux) [%s] [EXIT]\n",fnId, getTimestamp(Iso));
 	DEB_ALWAYS() << msg;
    m_log.append(msg);

    // ----- linux [end]

    _pco_SetCameraToCurrentTime(error);

    DEB_TRACE() << "END OF CAMERA";
    DEB_ALWAYS() << fnId << " [EXIT]";
}

//=========================================================================================================
//=========================================================================================================
void Camera::_init_dimax()
{
    DEB_CONSTRUCTOR();
    char msg[MSG_SIZE + 1];
    const char *pcoFn = "Camera::_init_dimax";

    int error = 0;
    DWORD _dwValidImageCnt, _dwMaxImageCnt;

    // block #1 -- Get RAM size
    {
        int segmentPco, segmentArr;

        DWORD ramSize;
        WORD pageSize;

        WORD bitsPerPix;
        getBitsPerPixel(bitsPerPix);

        _pco_GetCameraRamSize(ramSize, pageSize, error);
        PCO_THROW_OR_TRACE(error, "_pco_GetCameraRamSize");

        m_pcoData->dwRamSize = ramSize;    // nr of pages of the ram
        m_pcoData->wPixPerPage = pageSize; // nr of pixels of the page

        __sprintfSExt(msg, MSG_SIZE,
                      "* ramPages[%d] pixPerPage[%d] bitsPerPix[%d]\n",
                      m_pcoData->dwRamSize, m_pcoData->wPixPerPage, bitsPerPix);
        DEB_TRACE() << msg;
        m_log.append(msg);

        double nrBytes = (double)m_pcoData->dwRamSize *
                         (double)m_pcoData->wPixPerPage * (double)bitsPerPix /
                         9.; // 8 bits data + 1 bit CRC -> 9

        __sprintfSExt(msg, MSG_SIZE, "* camMemorySize [%lld B] [%g GB]\n",
                      (long long int)nrBytes, nrBytes / GIGABYTE);
        DEB_TRACE() << msg;
        m_log.append(msg);

        // ----------------- get initial seg Size - images & print

        // ---- get the size in pages of each of the 4 segments

        DWORD segSize[4];
        _pco_GetCameraRamSegmentSize(segSize, error);
        PCO_THROW_OR_TRACE(error, "_pco_GetCameraRamSegmentSize");

        for (segmentArr = 0; segmentArr < PCO_MAXSEGMENTS; segmentArr++)
        {
            segmentPco = segmentArr + 1; // PCO segment (1 ... 4)
            m_pcoData->dwSegmentSize[segmentArr] = segSize[segmentArr];

            __sprintfSExt(msg, MSG_SIZE, "* segment[%d] number of pages[%d]\n",
                          segmentPco, m_pcoData->dwSegmentSize[segmentArr]);
            DEB_TRACE() << msg;
            m_log.append(msg);
        }

        //---- get nr de images in each segment & nr max of img on each segmente
        for (segmentArr = 0; segmentArr < PCO_MAXSEGMENTS; segmentArr++)
        {
            segmentPco = segmentArr + 1;

            _pco_GetNumberOfImagesInSegment(segmentPco, _dwValidImageCnt,
                                            _dwMaxImageCnt, error);
            PCO_THROW_OR_TRACE(error, pcoFn);

            m_pcoData->dwValidImageCnt[segmentArr] = _dwValidImageCnt;
            m_pcoData->dwMaxImageCnt[segmentArr] = _dwMaxImageCnt;

            __sprintfSExt(msg, MSG_SIZE,
                          "* segment[%d] nr images [%d]  max imag [%d]\n",
                          segmentPco, _dwValidImageCnt, _dwMaxImageCnt);
            DEB_TRACE() << msg;
            m_log.append(msg);

        } // for

        // set the first segment to the max ram size, the others = 0
        // This function will result in all segments being cleared.
        // All previously recorded images will be lost!

        // m_pcoData->dwSegmentSize[0] = m_pcoData->dwRamSize;
        for (segmentArr = 1; segmentArr < PCO_MAXSEGMENTS; segmentArr++)
        {
            m_pcoData->dwSegmentSize[0] += m_pcoData->dwSegmentSize[segmentArr];
            m_pcoData->dwSegmentSize[segmentArr] = 0;
        }
        __sprintfSExt(
            msg, MSG_SIZE,
            "* m_pcoData->dwSegmentSize0 [%d]  m_pcoData->dwRamSize [%d]\n",
            m_pcoData->dwSegmentSize[0], m_pcoData->dwRamSize);
        DEB_TRACE() << msg;
        m_log.append(msg);

        _pco_SetCameraRamSegmentSize(&m_pcoData->dwSegmentSize[0], error);
        PCO_THROW_OR_TRACE(error, pcoFn);
    } // block #1

    DEB_TRACE() << "end block 1 / get initial seg Size - images";

    {
        int segmentArr;

        unsigned int maxWidth, maxHeight;
        getMaxWidthHeight(maxWidth, maxHeight);

        DWORD pages_per_image = maxWidth * maxHeight / m_pcoData->wPixPerPage;

        ///------------------------------------------------------------------------TODO
        ///?????
        for (segmentArr = 0; segmentArr < PCO_MAXSEGMENTS; segmentArr++)
        {
            if (m_pcoData->dwMaxImageCnt[segmentArr] == 0)
            {
                m_pcoData->dwMaxImageCnt[segmentArr] =
                    m_pcoData->dwSegmentSize[segmentArr] / pages_per_image;
                if (m_pcoData->dwMaxImageCnt[segmentArr] > 4)
                    m_pcoData->dwMaxImageCnt[segmentArr] -= 2;
            }
        }
    } // block

    // -- Get Active RAM segment

    WORD wActSeg;
    _pco_GetActiveRamSegment(wActSeg, error);

    _pco_GetNumberOfImagesInSegment(m_pcoData->wActiveRamSegment,
                                    _dwValidImageCnt, _dwMaxImageCnt, error);
    PCO_THROW_OR_TRACE(error, pcoFn);

    DEB_TRACE() << "original DONE";
}

//=========================================================================================================
//=========================================================================================================
void Camera::_init_edge()
{
    m_pcoData->fTransferRateMHzMax = 550.;
}

//=================================================================================================
// Camera::prepareAcq() LINUX
//=================================================================================================
void Camera::prepareAcq()
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    //DEB_ALWAYS() << "[ENTRY] " << _checkLogFiles();
    DEB_ALWAYS() << "[ENTRY] " ;

    int error, err;
    WORD binhorz,binvert;
    WORD wRoiX0, wRoiY0, wRoiX1, wRoiY1;

    const char *msg ;
    
    m_pcoData->traceAcqClean();

    
    DEB_ALWAYS() << "sync" ;
    int iRequestedFrames;
    // live video requested frames = 0
    m_sync->getNbFrames(iRequestedFrames);


    //------------------------------------------------- check recording state to 0
    // camera->PCO_GetRecordingState(&wRecState_actual);
    // if != 0
    //      if != edge
    //          err = camera->PCO_CancelImage();
    //          err = camera->PCO_CancelImageTransfer();
    //      camera->PCO_SetRecordingState(wRecState_new);
    //      camera->PCO_GetRecordingState(&wRecState_actual);
    //-------------------------------------------------
    
    DEB_ALWAYS() << "recording " ;
    WORD recmode;

    msg = "camera->PCO_GetRecordingState()";
    err=camera->PCO_GetRecordingState(&recmode);
    printf("%s %s> %s [%d]\n",getTimestamp(Iso), fnId, msg, recmode); 
    PCO_THROW_OR_TRACE(err, msg);
 
    if(recmode)
    {
        msg = "camera->PCO_SetRecordingState(0)";
        err=camera->PCO_SetRecordingState(0);
        printf("%s %s> %s\n",getTimestamp(Iso), fnId, msg); 
        PCO_THROW_OR_TRACE(err, msg);

        msg = "camera->PCO_GetRecordingState()";
        err=camera->PCO_GetRecordingState(&recmode);
        printf("%s %s> %s [%d]\n",getTimestamp(Iso), fnId, msg, recmode); 
        PCO_THROW_OR_TRACE(err, msg);
    }


    //------------------------------------------------- check binning 
    msg = "camera->PCO_GetBinning()";
    err=camera->PCO_GetBinning(&binhorz ,&binvert);
    printf("%s %s> %s binhorz[%d] binvert[%d]\n",getTimestamp(Iso), fnId, msg,binhorz ,binvert); 
    PCO_THROW_OR_TRACE(err, msg);



    //------------------------------------------------- check roi
    // info only, hw already set

    //msg = "camera->PCO_SetROI()";
    //err=camera->PCO_SetROI(wRoiX0,wRoiY0,wRoiX1,wRoiY1);
    //printf("%s %s> %s wRoiX0[%d] wRoiY0[%d] wRoiX1[%d] wRoiY1[%d]\n",getTimestamp(Iso), fnId, msg,wRoiX0, wRoiY0, wRoiX1, wRoiY1); 
    //PCO_THROW_OR_TRACE(err, msg);

    msg = "camera->PCO_GetROI()";
    err=camera->PCO_GetROI(&wRoiX0,&wRoiY0,&wRoiX1,&wRoiY1);
    printf("%s %s> %s wRoiX0[%d] wRoiY0[%d] wRoiX1[%d] wRoiY1[%d]\n",getTimestamp(Iso), fnId, msg,wRoiX0, wRoiY0, wRoiX1, wRoiY1); 
    PCO_THROW_OR_TRACE(err, msg);

    m_pcoData->traceAcq.iPcoRoiX0 = m_pcoData->wRoiX0Now = wRoiX0;
    m_pcoData->traceAcq.iPcoRoiX1 = m_pcoData->wRoiX1Now = wRoiX1;
    m_pcoData->traceAcq.iPcoRoiY0 = m_pcoData->wRoiY0Now = wRoiY0;
    m_pcoData->traceAcq.iPcoRoiY1 = m_pcoData->wRoiY1Now = wRoiY1;


    //-------------------------------------------------



    //------------------------------------------------- triggering mode
    //------------------------------------- acquire mode : ignore or not ext.
    // signal

    WORD wPcoTrigMode, wPcoAcqMode;
    const char *sLimaTriggerMode, *sPcoTriggerMode, *sPcoAcqMode;
    bool extTrig;
                                       
    TrigMode lima_trig_mode;
    m_sync->getTrigMode(lima_trig_mode);
    
    m_sync->xlatLimaTrigMode2Pco(lima_trig_mode,
        wPcoTrigMode, wPcoAcqMode,
        &sLimaTriggerMode, &sPcoTriggerMode, &sPcoAcqMode, 
        extTrig, err);

    msg = "camera->PCO_SetTriggerMode()";
    err=camera->PCO_SetTriggerMode(wPcoTrigMode);
    printf("%s %s> %s wPcoTrigMode[%d]\n",getTimestamp(Iso), fnId, msg,wPcoTrigMode); 
    PCO_THROW_OR_TRACE(err, msg);

    msg = "camera->PCO_SetAcquireMode";
    err=camera->PCO_SetAcquireMode(wPcoAcqMode);
    printf("%s %s> %s wPcoAcqMode[%d]\n",getTimestamp(Iso), fnId, msg, wPcoAcqMode); 
    PCO_THROW_OR_TRACE(err, msg);

    
    WORD ts_mode,bitalign;

    //------------------------------------------------- GET timestampMode
    msg = "camera->PCO_GetTimestampMode()";
    err=camera->PCO_GetTimestampMode(&ts_mode);
    printf("%s %s> %s ts_mode[%d]\n",getTimestamp(Iso), fnId, msg, ts_mode); 
    PCO_THROW_OR_TRACE(err, msg);

    //------------------------------------------------- PCO_GetBitAlignment
    msg = "camera->PCO_GetBitAlignment()";
    err=camera->PCO_GetBitAlignment(&bitalign);
    printf("%s %s> %s bitalign[%d]\n",getTimestamp(Iso), fnId, msg, bitalign); 
    PCO_THROW_OR_TRACE(err, msg);

   

    //-------------------------------------------------

    // ----------------------------------------- storage mode (recorder +
    // sequence)
    if (_isCameraType(Dimax | Pco4k | Pco2k))
    {
        //TODO
    }



    //----------------------------------- set exposure time & delay time


    const char tb[3][3]={"ns","us","ms"};
    DWORD pco_exp_time,pco_del_time,pixelrate;
    WORD pco_exp_timebase,pco_del_timebase;
    double lima_exp_time, lima_del_time;

    msg = "camera->PCO_GetDelayExposureTime()";
    err=camera->PCO_GetDelayExposureTime(&pco_del_time,&pco_exp_time,&pco_del_timebase,&pco_exp_timebase);
    printf("%s %s> %s pco_del_time[%d] pco_exp_time[%d] pco_del_timebase[%d] pco_exp_timebase[%d] ---> [%d]%s\n",
        getTimestamp(Iso), fnId, msg, 
        pco_del_time,pco_exp_time,pco_del_timebase,pco_exp_timebase, 
        pco_exp_time, tb[pco_exp_timebase]); 
    PCO_THROW_OR_TRACE(err, msg);


    m_sync->getExpTime(lima_exp_time);
    m_sync->setLatTime(lima_del_time);
    _pco_time2dwbase(lima_exp_time,  pco_exp_time, pco_exp_timebase);
    _pco_time2dwbase(lima_del_time,  pco_del_time, pco_del_timebase);


    msg = "camera->PCO_SetDelayExposureTime()";
    err=camera->PCO_SetDelayExposureTime(pco_del_time,pco_exp_time,pco_del_timebase,pco_exp_timebase);
    printf("%s %s> %s pco_del_time[%d] pco_exp_time[%d] pco_del_timebase[%d] pco_exp_timebase[%d] ---> [%d]%s\n",
        getTimestamp(Iso), fnId, msg, 
        pco_del_time,pco_exp_time,pco_del_timebase,pco_exp_timebase, 
        pco_exp_time, tb[pco_exp_timebase]); 
    PCO_THROW_OR_TRACE(err, msg);


    msg = "camera->PCO_GetDelayExposureTime()";
    err=camera->PCO_GetDelayExposureTime(&pco_del_time,&pco_exp_time,&pco_del_timebase,&pco_exp_timebase);
    printf("%s %s> %s pco_del_time[%d] pco_exp_time[%d] pco_del_timebase[%d] pco_exp_timebase[%d] ---> [%d]%s\n",
        getTimestamp(Iso), fnId, msg, 
        pco_del_time,pco_exp_time,pco_del_timebase,pco_exp_timebase, 
        pco_exp_time, tb[pco_exp_timebase]); 
    PCO_THROW_OR_TRACE(err, msg);

    //-------------------------------------------------

    DWORD width,height;
    unsigned int w,h,bp;



    msg = "camera->PCO_GetPixelRate()";
    err=camera->PCO_GetPixelRate(&pixelrate);
    printf("%s %s> %s pixelrate[%d]\n",getTimestamp(Iso), fnId, msg, pixelrate); 
    PCO_THROW_OR_TRACE(err, msg);

    msg = "camera->PCO_GetActualSize()";
    err=camera->PCO_GetActualSize(&width,&height);
    printf("%s %s> %s width[%d] height[%d]\n",getTimestamp(Iso), fnId, msg, width,height); 
    PCO_THROW_OR_TRACE(err, msg);


    msg = "grabber_clhs->Get_actual_size()";
    err=grabber_clhs->Get_actual_size(&w,&h,&bp);
    printf("%s %s> %s w[%d] h[%d] b[%d]\n",getTimestamp(Iso), fnId, msg, w, h, bp); 
    PCO_THROW_OR_TRACE(err, msg);




    //------------------------------------------------- TOCHECK

    // ----------------------------------------- set Record Stop Event (used for
    // dimax for ExtTrigSingle)
    if (_isCameraType(Dimax))
    {
        //TODO
    }

    //-------------------------------------------------


    //--------------------------- metadata
    // for dimax only
    //    camera->PCO_SetMetadataMode(wMetaDataMode,
    //            &m_pcoData->wMetaDataSize,
    //            &m_pcoData->wMetaDataVersion)
        //TODO
    //    _pco_SetMetaDataMode(0, error);
    //PCO_THROW_OR_TRACE(error, "_pco_SetMetaDataMode");
    //-------------------------------------------------

    //--------------------------- PREPARE / pixel rate - ARM required
    // camera->PCO_GetPixelRate(&_dwPixRate)
    // if req is valid and != actual
    //    camera->PCO_SetPixelRate(_dwPixelRateReq)
    //    camera->PCO_GetPixelRate(&_dwPixRate)
    
    
    

    if (_isCameraType(Edge))
    {
        DWORD _dwPixelRateReq, _dwPixRate;

        msg = "camera->PCO_GetPixelRate()";
        err = camera->PCO_GetPixelRate(&_dwPixRate);
        printf("%s %s> %s _dwPixRate[%d]\n",getTimestamp(Iso), fnId, msg, _dwPixRate); 
        PCO_THROW_OR_TRACE(err, msg);

        m_pcoData->dwPixelRate = _dwPixRate;

        _dwPixelRateReq = m_pcoData->dwPixelRateRequested;

        if (_isValid_pixelRate(_dwPixelRateReq) &&
            (_dwPixRate != _dwPixelRateReq))
        {
            msg = "camera->PCO_SetPixelRate()";
            err = camera->PCO_SetPixelRate(_dwPixelRateReq);
            printf("%s %s> %s _dwPixelRateReq[%d]\n",getTimestamp(Iso), fnId, msg, _dwPixelRateReq); 
            PCO_THROW_OR_TRACE(err, msg);

            msg = "camera->PCO_GetPixelRate()";
            err = camera->PCO_GetPixelRate(&_dwPixRate);
            printf("%s %s> %s _dwPixRate[%d]\n",getTimestamp(Iso), fnId, msg, _dwPixRate); 
            PCO_THROW_OR_TRACE(err, msg);

            m_pcoData->dwPixelRate = _dwPixRate;
            m_pcoData->dwPixelRateRequested = 0;
        }
    }
    
    //-------------------------------------------------

    //--------------------------- PREPARE / clXferParam, LUT - ARM required
    //_pco_SetTransferParameter_SetActiveLookupTable(error);
    //PCO_THROW_OR_TRACE(error, "_pco_SetTransferParameter_SetActiveLookupTable");
    //-------------------------------------------------



    msg = "camera->PCO_ArmCamera()";
    err=camera->PCO_ArmCamera();
    printf("%s %s> %s\n",getTimestamp(Iso), fnId, msg); 
    PCO_THROW_OR_TRACE(err, msg);


    msg = "grabber_clhs->PostArm(0)";
    err = grabber_clhs->PostArm(0);
    printf("%s %s> %s\n",getTimestamp(Iso), fnId, msg); 
    PCO_THROW_OR_TRACE(err, msg);



#if 0
    msg = "camera->PCO_GetActualSize()";
    err=camera->PCO_GetActualSize(&width,&height);
    printf("%s %s> %s width[%d] height[%d]\n",getTimestamp(Iso), fnId, msg, width,height); 
    PCO_THROW_OR_TRACE(err, msg);




    msg = "grabber_clhs->Get_actual_size()";
    err=grabber_clhs->Get_actual_size(&w,&h,&bp);
    printf("%s %s> %s w[%d] h[%d] b[%d]\n",getTimestamp(Iso), fnId, msg, w, h, bp); 
    PCO_THROW_OR_TRACE(err, msg);



    msg = "grabber_clhs->PostArm(1)";
    err = grabber_clhs->PostArm(1);
    printf("%s %s> %s\n",getTimestamp(Iso), fnId, msg); 
    PCO_THROW_OR_TRACE(err, msg);




    DWORD dwTime_s, dwTime_ns;
    double runTime, frameRate;

    msg = "camera->PCO_GetCOCRuntime()";
    err = camera->PCO_GetCOCRuntime(&dwTime_s, &dwTime_ns);
    printf("%s %s> %s dwTime_s[%d] dwTime_ns[%d]\n",getTimestamp(Iso), fnId, msg, dwTime_s, dwTime_ns); 
    PCO_THROW_OR_TRACE(err, msg);

    m_pcoData->cocRunTime = runTime =
        ((double)dwTime_ns * NANO) + (double)dwTime_s;
    m_pcoData->frameRate = frameRate = (dwTime_ns | dwTime_s) ? 1.0 / runTime : 0.0;

    printf("... cocRunTime[%g] frameRate[%g]\n", runTime, frameRate); 
#endif   // 0/1

 ::Sleep(100);

}


//==========================================================================================================
//==========================================================================================================


//==========================================================================================================
//==========================================================================================================

//==========================================================================================================
//==========================================================================================================

//==========================================================================================================
//==========================================================================================================

const char *sPcoAcqStatus[] = {
    "pcoAcqOK",
    "pcoAcqIdle",
    "pcoAcqStart",
    "pcoAcqRecordStart",
    "pcoAcqRecordEnd",
    "pcoAcqRecordStop",
    "pcoAcqRecordTimeout",
    "pcoAcqTransferStart",
    "pcoAcqTransferEnd",
    "pcoAcqStop",
    "pcoAcqTransferStop",
    "pcoAcqWaitTimeout",
    "pcoAcqWaitError",
    "pcoAcqError",
    "pcoAcqPcoError",
};

//=====================================================================
//=====================================================================

//=====================================================================
//=====================================================================
void Camera::reset(int reset_level)
{
    DEB_MEMBER_FUNCT();
    int error;

    switch (reset_level)
    {
        case RESET_CLOSE_INTERFACE:
            DEB_TRACE() << "\n... RESET - freeBuff, closeCam, resetLib  "
                        << DEB_VAR1(reset_level);

            _pco_CloseCamera(error);
            PCO_PRINT_ERR(error, "_pco_CloseCamera");
            m_handle = 0;

            _pco_ResetLib(error);
            PCO_PRINT_ERR(error, "_pco_ResetLib");
            break;

        default:
            DEB_TRACE() << "\n... RESET -  " << DEB_VAR1(reset_level);
            //_init();
            break;
    }
}

//=========================================================================================================
//=========================================================================================================
int Camera::PcoCheckError(int line, const char *file, int err, const char *fn,
                          const char *comments)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    static char tmpMsg[512];
    char *msg;
    size_t lg;

	if (err == 0)
		return 0;

	__sprintfSExt(tmpMsg, sizeof(tmpMsg), 
			" FROM: PCOfn[%s] file[%s] line[%d] comments[%s]", 
			fn, file, line, comments);
	msgLog(tmpMsg);

	DWORD dwErr = err;
	m_pcoData->pcoError = err;
	msg = m_pcoData->pcoErrorMsg;

	memset(msg, 0, ERR_SIZE);
	PCO_GetErrorText(dwErr, msg, ERR_SIZE - 14);

	//lg = strlen(msg);
	// __sprintfSExt(msg + lg, ERR_SIZE - lg, " [%s][%d]", file, line);

	if (err & PCO_ERROR_IS_WARNING)
	{
		DEB_WARNING() << fnId << ": --- WARNING - IGNORED --- "
					  << DEB_VAR1(m_pcoData->pcoErrorMsg);
		return 0;
	}

	DEB_ALWAYS() << "PCO ERROR: "
				<< DEB_VAR2(msg, err) 
				<< " = " <<	DEB_HEX(err) 
				<< tmpMsg;
		
	return (err);
}

//=========================================================================================================
//=========================================================================================================
char *Camera::_PcoCheckError(int line, const char *file, int err, int &error,
                             const char *fn)
{
    static char lastErrorMsg[ERR_SIZE];
    static char tmpMsg[512];
    char *msg;
    size_t lg;

    error = m_pcoData->pcoError = err;
    msg = m_pcoData->pcoErrorMsg;

    if (err != 0)
    {
        __sprintfSExt(tmpMsg, sizeof(tmpMsg), "ERROR %s (%d)", fn, line);

        PCO_GetErrorText(err, lastErrorMsg, ERR_SIZE - 14);
        // strncpy_s(msg, ERR_SIZE, lastErrorMsg, _TRUNCATE);
        strncpy_s(msg, ERR_SIZE, lastErrorMsg, ERR_SIZE - 1);

        lg = strlen(msg);
        __sprintfSExt(msg + lg, ERR_SIZE - lg, " [%s][%d]", file, line);

        return lastErrorMsg;
    }
    return NULL;
}

//=================================================================================================
//=================================================================================================
int Camera::dumpRecordedImages(int &nrImages, int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    WORD wSegment;
    _pco_GetActiveRamSegment(wSegment, error);
    DWORD _dwValidImageCnt, _dwMaxImageCnt;

    WORD wRecState_actual;

    nrImages = -1;

    if (!_isCameraType(Dimax))
        return -2;

    wRecState_actual = _pco_GetRecordingState(error);
    PCO_PRINT_ERR(error, "_pco_GetRecordingState");

    if (error)
        return -100;
    if (wRecState_actual != 0)
        return -1;

    _pco_GetNumberOfImagesInSegment(wSegment, _dwValidImageCnt, _dwMaxImageCnt,
                                    error);
    if (error)
    {
        printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__,
               "_pco_GetNumberOfImagesInSegment");
        throw LIMA_HW_EXC(Error, "PCO_GetNumberOfImagesInSegment");
    }

    nrImages = _dwValidImageCnt;

    return 0;
}

//=================================================================================================
//=================================================================================================
bool Camera::_isValid_pixelRate(DWORD dwPixelRate)
{
    DEB_MEMBER_FUNCT();

    // pixelrate 1     (long word; frequency in Hz)
    // pixelrate 2,3,4 (long word; frequency in Hz; if not available, then value
    // = 0)

    if (dwPixelRate > 0)
        for (int i = 0; i < 4; i++)
        {
            if (dwPixelRate == m_pcoData->stcPcoDescription.dwPixelRateDESC[i])
                return true;
        }

    return false;
}

//=========================================================================================================
//=========================================================================================================
void Camera::_get_ImageType(ImageType &image_type)
{
    unsigned int pixbytes;
    getBytesPerPixel(pixbytes);
    image_type = (pixbytes == 2) ? Bpp16 : Bpp8;
}

//=================================================================================================
//=================================================================================================

// 31/10/2013 PCO Support Team <support@pco.de>
// Pixelsize is not implemented in the complete SW- and HW-stack.

void Camera::_get_PixelSize(double &x_size, double &y_size)
{
    // pixel size in micrometer

    if (_isCameraType(Pco2k))
    {
        x_size = y_size = 7.4; // um / BR_pco_2000_105.pdf
        return;
    }

    if (_isCameraType(Pco4k))
    {
        x_size = y_size = 9.0; // um / BR_pco_4000_105.pdf
        return;
    }

    if (_isCameraType(Edge))
    {
        x_size = y_size = 6.5; // um / pco.edge User Manual V1.01, page 34
        return;
    }

    if (_isCameraType(Dimax))
    {
        x_size = y_size = 11; // um / pco.dimax User's Manual V1.01
        return;
    }

    x_size = y_size = -1.;
}

//=================================================================================================
//=================================================================================================
void Camera::_set_ImageType(ImageType curr_image_type)
{
    // ---- DONE
    // only check if it valid, BUT don't set it ????
    switch (curr_image_type)
    {
        case Bpp16:
        case Bpp8:
            break;

        default:
            throw LIMA_HW_EXC(InvalidValue, "This image type is not Managed");
    }
}
//=========================================================================================================
//=========================================================================================================
void Camera::_get_DetectorType(std::string &det_type)
{
    // ---- DONE
    det_type = "Pco";
}

//=========================================================================================================
//=========================================================================================================
void Camera::_get_MaxImageSize(Size &max_image_size)
{
    unsigned width, height;

    getMaxWidthHeight(width, height);
    max_image_size = Size(width, height);
}

//=================================================================================================
//=================================================================================================
bool Camera::_isCameraType(unsigned long long tp)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    WORD wCameraType = _getCameraType();
    WORD wCameraSubtype = _getCameraSubType();

    DEB_TRACE() << "[entry] " << DEB_VAR3(tp, wCameraType, wCameraSubtype);

    switch (wCameraType)
    {
        case CAMERATYPE_PCO_DIMAX_STD:
        case CAMERATYPE_PCO_DIMAX_TV:
        case CAMERATYPE_PCO_DIMAX_CS:
            if (tp & (Dimax | camRAM))
            {
                DEB_TRACE() << "Dimax [exit] ";
                return true;
            }
            switch (wCameraSubtype >> 8)
            {
                case 0x20:
                    if (tp & (DimaxHS1 | DimaxHS))
                    {
                        DEB_TRACE() << "DimaxHS1 / HS [exit] ";
                        return true;
                    }
                    break;
                case 0x21:
                    if (tp & (DimaxHS2 | DimaxHS))
                    {
                        DEB_TRACE() << "DimaxHS2 / HS [exit] ";
                        return true;
                    }
                    break;
                case 0x23:
                    if (tp & (DimaxHS4 | DimaxHS))
                    {
                        DEB_TRACE() << "DimaxHS4 / HS [exit] ";
                        return true;
                    }
                    break;
                default:
                    break;
            }
            DEB_TRACE() << "Dimax SUBTYPE NONE false [exit] ";
            return false;

        case CAMERATYPE_PCO_EDGE_GL:
            return !!(tp & (EdgeGL | Edge));

        case CAMERATYPE_PCO_EDGE_USB3:
            return !!(tp & (EdgeUSB | EdgeRolling | Edge));

        case CAMERATYPE_PCO_EDGE_HS:
            return !!(tp & (EdgeHS | EdgeRolling | Edge));

        case CAMERATYPE_PCO_EDGE_42:
        case CAMERATYPE_PCO_EDGE:
            return !!(tp & (EdgeRolling | Edge));

        case CAMERATYPE_PCO2000:
            return !!(tp & (Pco2k | camRAM));

        case CAMERATYPE_PCO4000:
            return !!(tp & (Pco4k | camRAM));

        default:
            break;
    }

    return false;
}

//=================================================================================================
//=================================================================================================
bool Camera::_isInterfaceType(int tp)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    switch (_getInterfaceType())
    {
        case INTERFACE_FIREWIRE:
            return !!(tp & ifFirewire);

        case INTERFACE_CAMERALINK:
            return !!(tp & (ifCameralink | ifCameralinkAll));

        case INTERFACE_CAMERALINKHS:
            return !!(tp & (ifCameralinkHS | ifCameralinkAll));

        case INTERFACE_USB:
            return !!(tp & (ifUsb));

        case INTERFACE_USB3:
            return !!(tp & (ifUsb3));

        case INTERFACE_ETHERNET:
            return !!(tp & (ifEth));

        case INTERFACE_SERIAL:
            return !!(tp & (ifSerial));

        case INTERFACE_COAXPRESS:
            return !!(tp & (ifCoaxpress));

        default:
            return false;
    }
}

//=================================================================================================
//=================================================================================================
void Camera::_get_XYsteps(Point &xy_steps)
{
    DEB_MEMBER_FUNCT();

    unsigned int xSteps, ySteps;

    getXYsteps(xSteps, ySteps);

    xy_steps.x = xSteps;
    xy_steps.y = ySteps;
}

//=================================================================================================
//=================================================================================================
void Camera::_presetPixelRate(DWORD &pixRate, int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    if (!(_isCameraType(Edge) || _isCameraType(Pco2k) || _isCameraType(Pco4k)))
    {
        DEB_TRACE() << "WARNING - this camera doesn't allows setPixelRate";
        pixRate = 0;
        error = -1;
        return;
    }

    if (!_isValid_pixelRate(pixRate))
    {
        DEB_ALWAYS() << "INVALID requested pixel Rate" << DEB_VAR1(pixRate);
        pixRate = 0;
        error = -1;
        return;
    }

    m_pcoData->dwPixelRateRequested = pixRate;
    error = 0;
}

//=================================================================================================
//=================================================================================================
void Camera::msgLog(const char *s)
{
    m_msgLog->add(s);
}

//=================================================================================================
//=================================================================================================

DWORD Camera::_getCameraSerialNumber()
{
    DEB_MEMBER_FUNCT();
    return m_pcoData->stcPcoCamType.dwSerialNumber;
}

WORD Camera::_getInterfaceType()
{
    DEB_MEMBER_FUNCT();
    return m_pcoData->stcPcoCamType.wInterfaceType;
}

const char *Camera::_getInterfaceTypeStr()
{
    DEB_MEMBER_FUNCT();
    return m_pcoData->iface;
}

const char *Camera::_getCameraIdn()
{
    DEB_MEMBER_FUNCT();
    return m_pcoData->camera_name;
}

//=================================================================================================
//=================================================================================================
WORD Camera::_getCameraType()
{
    DEB_MEMBER_FUNCT();
    return m_pcoData->stcPcoCamType.wCamType;
}

const char *Camera::_getCameraTypeStr()
{
    DEB_MEMBER_FUNCT();
    return m_pcoData->model;
}

//=================================================================================================
//=================================================================================================
WORD Camera::_getCameraSubType()
{
    DEB_MEMBER_FUNCT();
    return m_pcoData->stcPcoCamType.wCamSubType;
}

const char *Camera::_getCameraSubTypeStr()
{
    DEB_MEMBER_FUNCT();
    return m_pcoData->modelSubType;
}

//=================================================================================================
//=================================================================================================

//=================================================================================================
//=================================================================================================

bool Camera::_getCameraState(long long flag)
{
    return !!(m_state & flag);
}

//=================================================================================================
//=================================================================================================

void Camera::_setCameraState(long long flag, bool val)
{
    if (val)
    {
        m_state |= flag;
    }
    else
    {
        m_state |= flag;
        m_state ^= flag;
    }
    return;
}

//=================================================================================================
//=================================================================================================

time_t Camera::_getActionTimestamp(int action)
{
    if ((action < 0) || (action >= DIM_ACTION_TIMESTAMP))
        return 0;
    time_t ts = m_pcoData->action_timestamp.ts[action];
    return ts ? ts : 1;
}

void Camera::_setActionTimestamp(int action)
{
    if ((action >= 0) && (action < DIM_ACTION_TIMESTAMP))
    {
        m_pcoData->action_timestamp.ts[action] = time(NULL);
    }
}

//=================================================================================================
//=================================================================================================

void Camera::getStatus(HwInterface::StatusType &status)
{

// called by interface / windows

    bool _started = getStarted();
    pcoAcqStatus _exposing = getExposing();

    AutoMutex lock(m_cond.mutex());
    bool _buffer = m_buffer;
    lock.unlock();
    
    DEB_MEMBER_FUNCT();
    // DEB_TRACE() << DEB_VAR3(_started, m_buffer, m_exposing);

    if (_started)
    {
        if (_buffer)
        {
            switch (_exposing)
            {
                case pcoAcqStart:
                case pcoAcqRecordStart:
                    status.set(HwInterface::StatusType::Exposure);
                    break;

                case pcoAcqStop:
                case pcoAcqTransferStop:
                case pcoAcqIdle:
                case pcoAcqTransferEnd:
                case pcoAcqRecordEnd:
                case pcoAcqTransferStart:
                    status.set(HwInterface::StatusType::Latency);
                    break;

                case pcoAcqRecordStop:
                case pcoAcqRecordTimeout:
                case pcoAcqWaitTimeout:
                case pcoAcqWaitError:
                case pcoAcqError:
                case pcoAcqPcoError:
                    status.set(HwInterface::StatusType::Fault);
                    break;

                default:
                    THROW_HW_ERROR(NotSupported) << "Undefined value";
            } // sw
        }     // m_buffer
    }
    else
    { // not started
        status.set(HwInterface::StatusType::Ready);
    }

    if (_getDebug(DBG_STATUS))
    {
        DEB_ALWAYS() << DEB_VAR2(_exposing, status);
    }
}





void Camera::setStatus(HwInterface::StatusType::Basic status, bool force)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    AutoMutex aLock(m_cond.mutex());

    if (force || m_status != HwInterface::StatusType::Basic::Fault)
        m_status = status;

    m_cond.broadcast();
}


//=================================================================================================
//=================================================================================================
//================================= ROLLING SHUTTER
//===============================================
//=================================================================================================
//=================================================================================================
void Camera::_get_shutter_rolling_edge(DWORD &dwRolling, int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    DWORD m_dwSetup[10];
    WORD m_wLen = 10;
    WORD m_wType;

    m_wType = 0;
    _pco_GetCameraSetup(m_wType, m_dwSetup[0], m_wLen, error);
    PCO_PRINT_ERR(error, "_pco_GetCameraSetup");

    dwRolling = error ? 0 : m_dwSetup[0];

    return;
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_set_shutter_rolling_edge(int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    char *msg=(char*)"none";
    const char *cmsg;
    char msgBuff[MSG_SIZE + 1];

    DWORD dwRollingShRequested, dwRollingShNow;
    DWORD m_dwSetup[10];
    WORD m_wLen = 10;
    WORD m_wType = 0;

    // PCO recommended timing values
    int ts[3] = {2000, 3000, 250}; // command, image, channel timeout
    DWORD sleepMs = 10000;         // sleep time after reboot

    if (!_isCameraType(Edge))
    {
        return;
    }

    DEB_TRACE() << fnId << " [entry - edge] ";

    m_config = true;

    // DWORD m_dwSetup[10];
    // WORD m_wLen = 10;
    // WORD m_wType;
    // int ts[3] = { 2000, 3000, 250}; // command, image, channel timeout
    // PCO_OpenCamera(&m_hCam,0);
    // PCO_GetCameraSetup(m_hCam, &m_wType, &m_dwSetup[0], &m_wLen);

    // m_dwSetup[0] = PCO_EDGE_SETUP_GLOBAL_SHUTTER;
    // PCO_SetTimeouts(m_hCam, &ts[0], sizeof(ts));
    // PCO_SetCameraSetup(m_hCam, m_wType, &m_dwSetup[0], m_wLen);
    // PCO_RebootCamera(m_hCam);
    // PCO_CloseCamera(m_hCam);
    // Camera setup parameter for pco.edge:
    // #define PCO_EDGE_SETUP_ROLLING_SHUTTER 0x00000001         // rolling
    // shutter #define PCO_EDGE_SETUP_GLOBAL_SHUTTER  0x00000002         //
    // global shutter

    dwRollingShRequested = m_pcoData->dwRollingShutter;

    m_wType = 0;
    _pco_GetCameraSetup(m_wType, m_dwSetup[0], m_wLen, error);
    PCO_PRINT_ERR(error, "_pco_GetCameraSetup");
    if (error)
    {
        DEB_ALWAYS() << fnId << " [ERROR PCO_GetCameraSetup] " << msg;
        m_config = false;
        return;
    }

    dwRollingShNow = m_dwSetup[0];

    if (m_dwSetup[0] == dwRollingShRequested)
    {
        DEB_TRACE() << "exit NO Change in ROLLING SHUTTER "
                    << DEB_VAR2(dwRollingShNow, dwRollingShRequested);
        m_config = false;
        return;
    }

    msg = msgBuff;
    __sprintfSExt(msg, MSG_SIZE, "[Change ROLLING SHUTTER from [%d] to [%d]]",
                  m_dwSetup[0] == PCO_EDGE_SETUP_ROLLING_SHUTTER,
                  dwRollingShRequested == PCO_EDGE_SETUP_ROLLING_SHUTTER);

    DEB_TRACE() << "Change in ROLLING SHUTTER "
                << DEB_VAR2(dwRollingShNow, dwRollingShRequested);

    m_dwSetup[0] = dwRollingShRequested;

    _pco_SetTimeouts(&ts[0], sizeof(ts), error);
    PCO_PRINT_ERR(error, msg);
    if (error)
        return;

    cmsg = "[PCO_SetCameraSetup]";
    DEB_TRACE() << fnId << " " << cmsg;
    _pco_SetCameraSetup(m_wType, m_dwSetup[0], m_wLen, error);
    PCO_PRINT_ERR(error, cmsg);
    if (error)
    {
        DEB_ALWAYS() << fnId << " [ERROR PCO_SetCameraSetup] " << cmsg;
        m_config = false;
        return;
    }

    cmsg = "[PCO_RebootCamera]";
    DEB_TRACE() << fnId << " " << cmsg;
    _pco_RebootCamera(error);
    PCO_PRINT_ERR(error, cmsg);
    if (error)
        return;

    // m_sync->_getBufferCtrlObj()->_pcoAllocBuffersFree();
    m_buffer->_pcoAllocBuffersFree();

    cmsg = "[PCO_CloseCamera]";
    DEB_TRACE() << fnId << " " << cmsg;
    _pco_CloseCamera(error);
    PCO_PRINT_ERR(error, cmsg);
    if (error)
        return;
    m_handle = 0;

    msg = msgBuff;
    __sprintfSExt(msg, MSG_SIZE, "[Sleep %d ms]", sleepMs);
    DEB_TRACE() << fnId << " " << msg;
    ::Sleep(sleepMs);

    _init();

    DEB_TRACE() << fnId << " [exit] ";

    m_config = false;
    return;
}

//=================================================================================================
//=================================================================================================
bool Camera::_isValid_rollingShutter(DWORD dwRolling)
{
    switch (dwRolling)
    {
        case PCO_EDGE_SETUP_ROLLING_SHUTTER:
            return _isCapsDesc(capsRollingShutter); // 1
        case PCO_EDGE_SETUP_GLOBAL_SHUTTER:
            return _isCapsDesc(capsGlobalShutter); // 2
        case PCO_EDGE_SETUP_GLOBAL_RESET:
            return _isCapsDesc(capsGlobalResetShutter); // 4
        default:
            return false;
    }
}
//================================================================================================
//================================= ROLLING SHUTTER / end
//========================================
//================================================================================================
#    include "PcoCameraLin.cpp"
