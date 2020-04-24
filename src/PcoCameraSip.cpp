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

#include "PcoSdkVersion.h"

#include "PcoCamera.h"
#include "PcoCameraUtils.h"
#include "PcoSyncCtrlObj.h"
#include "PcoBufferCtrlObj.h"

using namespace lima;
using namespace lima::Pco;

#define BUFF_INFO_SIZE 10000
static char buff[BUFF_INFO_SIZE + 16];

//====================================================================
// SIP - attributes
//====================================================================

void Camera::getPixelRate(int &val)
{
    DWORD pixRate, pixRateNext;
    int error;

    _pco_GetPixelRate(pixRate, pixRateNext, error);
    val = pixRate;
}

void Camera::setPixelRate(int val)
{
    int error;

    DWORD pixRate = val;
    _presetPixelRate(pixRate, error);
}

//====================================================================
//====================================================================
void Camera::getAcqTimeoutRetry(int &val)
{
    val = m_pcoData->acqTimeoutRetry;
}

void Camera::setAcqTimeoutRetry(int val)
{
    m_pcoData->acqTimeoutRetry = val < 0 ? 0 : val;
}
//====================================================================
//====================================================================
void Camera::getAdc(int &adc)
{
    int adc_working, adc_max;

    // int error = _pco_GetADCOperation(adc_working, adc_max);
    _pco_GetADCOperation(adc_working, adc_max);
    adc = adc_working;
}

void Camera::setAdc(int adc_new)
{
    int adc_working;

    // int error = _pco_SetADCOperation(adc_new, adc_working);
    _pco_SetADCOperation(adc_new, adc_working);
}

void Camera::getAdcMax(int &adc)
{
    int adc_working, adc_max;

    // int error = _pco_GetADCOperation(adc_working, adc_max);
    _pco_GetADCOperation(adc_working, adc_max);

    adc = adc_max;
}

//====================================================================
//====================================================================
void Camera::getCocRunTime(double &coc)
{
    coc = m_pcoData->cocRunTime;
}

void Camera::getFrameRate(double &framerate)
{
    framerate = m_pcoData->frameRate;
}

//====================================================================
//====================================================================
void Camera::getLastImgRecorded(unsigned long &img)
{
    img = m_pcoData->traceAcq.nrImgRecorded;
}

void Camera::getLastImgAcquired(unsigned long &img)
{
    img = m_pcoData->traceAcq.nrImgAcquired;
}
//====================================================================
//====================================================================
void Camera::getMaxNbImages(unsigned long &nr)
{
    int err;
    WORD wActSeg;

    if (!_isCameraType(Dimax | Pco2k | Pco4k))
    {
        nr = 0;
        return;
    }

    _pco_GetActiveRamSegment(wActSeg, err);

    if (err)
    {
        nr = 0;
        return;
    }

    nr = _pco_GetNumberOfImagesInSegment_MaxCalc(wActSeg);

    return;
}
//====================================================================
//====================================================================

void Camera::getPcoLogsEnabled(int &enabled)
{
    enabled = m_pcoData->pcoLogActive;
}

//====================================================================
// SIP - attrib
//====================================================================
void Camera::getCamType(std::string &o_sn)
{
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);
    _camInfo(ptr, ptrMax, CAMINFO_CAMERATYPE);
    o_sn = buff;
}

void Camera::getCamInfo(std::string &o_sn)
{
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);
    _camInfo(ptr, ptrMax, CAMINFO_ALL);
    o_sn = buff;
}

void Camera::getVersion(std::string &o_sn)
{
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);
    _camInfo(ptr, ptrMax, CAMINFO_VERSION);
    o_sn = buff;
}

//====================================================================
//====================================================================

void Camera::getClTransferParam(std::string &o_sn)
{
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);
    _camInfo(ptr, ptrMax, CAMINFO_CAMERALINK);
    o_sn = buff;
}

void Camera::getPixelRateInfo(std::string &o_sn)
{
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);
    _camInfo(ptr, ptrMax, CAMINFO_PIXELRATE);
    o_sn = buff;
}

void Camera::getLastError(std::string &o_sn)
{
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);
    __sprintfSExt(ptr, ptrMax - ptr, "[x%08x] [%s]\n", m_pcoData->pcoError,
                  m_pcoData->pcoErrorMsg);
    o_sn = buff;
}

//====================================================================
// SIP - attrib
//====================================================================
void Camera::getTraceAcq(std::string &o_sn)
{
    
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);

    //time_t _timet;

    if (0 && !(_isCameraType(Dimax | Pco2k | Pco4k)))
    {
        ptr +=
            __sprintfSExt(ptr, ptrMax - ptr, "* ERROR - only for DIMAX / 2K");
        o_sn = buff;
        return;
    }

    ptr += __sprintfSExt(ptr, ptrMax - ptr,
            "\n"
            "* fnId[%s] nrEvents[%d]\n"
            "* ... fnIdXfer[%s]\n",
            m_pcoData->traceAcq.fnId, m_pco_buffer_nrevents,
            m_pcoData->traceAcq.fnIdXfer);

    ptr += __sprintfSExt(ptr, ptrMax - ptr, 
            "* ... testCmdMode [0x%llx]\n",
            m_pcoData->testCmdMode);

    ptr += __sprintfSExt(ptr, ptrMax - ptr, 
            "* msExposure[%g] msDelay[%g]\n",
            m_pcoData->traceAcq.sExposure * 1000.,
            m_pcoData->traceAcq.sDelay * 1000.);

    ptr += __sprintfSExt(ptr, ptrMax - ptr,
            "* ... msLimaExposure[%g] Pco(exp[%d] base[%d])\n",
            m_pcoData->traceAcq.dLimaExposure * 1000.,
            m_pcoData->traceAcq.iPcoExposure,
            m_pcoData->traceAcq.iPcoExposureBase);

    ptr += __sprintfSExt(ptr, ptrMax - ptr, 
            "* ... msLimaDelay[%g] Pco(delay[%d] base[%d])\n",
            m_pcoData->traceAcq.dLimaDelay * 1000., m_pcoData->traceAcq.iPcoDelay,
            m_pcoData->traceAcq.iPcoDelayBase);

    ptr += __sprintfSExt(ptr, ptrMax - ptr, 
            "* pcoBin horz[%d] vert[%d]\n",
            m_pcoData->traceAcq.iPcoBinHorz,
            m_pcoData->traceAcq.iPcoBinVert);

    Roi limaRoi;
    int error;
    _pco_GetROI(limaRoi, error);

    Point top_left = limaRoi.getTopLeft();
    Point bot_right = limaRoi.getBottomRight();
    Size size = limaRoi.getSize();
    unsigned int bytesPerPix;
    getBytesPerPixel(bytesPerPix);

    ptr += __sprintfSExt(ptr, ptrMax - ptr,
            "* limaRoi xy0[%d,%d] xy1[%d,%d] size[%d,%d]\n",
            top_left.x, top_left.y, bot_right.x, bot_right.y,
            size.getWidth(), size.getHeight());
    ptr += __sprintfSExt(ptr, ptrMax - ptr, 
            "* ... pcoRoi x[%d,%d] y[%d,%d]\n",
            m_pcoData->traceAcq.iPcoRoiX0, m_pcoData->traceAcq.iPcoRoiX1,
            m_pcoData->traceAcq.iPcoRoiY0, m_pcoData->traceAcq.iPcoRoiY1);

    long long imgSize = size.getWidth() * size.getHeight() * bytesPerPix;
    long long totSize = imgSize * m_pcoData->traceAcq.nrImgRequested;
    double mbTotSize = totSize / (1024. * 1024.);
    double totTime = m_pcoData->traceAcq.msXfer / 1000.;
    double xferSpeed = mbTotSize / totTime;
    double framesPerSec = m_pcoData->traceAcq.nrImgRequested / totTime;
    
    ptr += __sprintfSExt(ptr, ptrMax - ptr, 
            "* nrImgRequested[%d] nrImgAcquired[%d]\n",
            m_pcoData->traceAcq.nrImgRequested, 
            m_pcoData->traceAcq.nrImgAcquired);
    ptr += __sprintfSExt(ptr, ptrMax - ptr,
            "* ... nrImgRequested0[%d] nrImgRecorded[%d] maxImgCount[%d]\n",
            m_pcoData->traceAcq.nrImgRequested0, 
            m_pcoData->traceAcq.nrImgRecorded,
            m_pcoData->traceAcq.maxImgCount);
    ptr += __sprintfSExt(ptr, ptrMax - ptr,
            "* ... imgSize[%lld B] totSize[%lld B][%g MB]\n",
            imgSize, totSize, mbTotSize);

    ptr += __sprintfSExt(ptr, ptrMax - ptr, 
            "* limaTriggerMode[%s]  extTrig[%d]\n",
            m_pcoData->traceAcq.sLimaTriggerMode,
            m_pcoData->traceAcq.bExtTrig);
    ptr += __sprintfSExt(ptr, ptrMax - ptr, 
            "* ... pcoTriggerMode[%s] [%d]\n",
            m_pcoData->traceAcq.sPcoTriggerMode,
            m_pcoData->traceAcq.iPcoTriggerMode);
    ptr += __sprintfSExt(ptr, ptrMax - ptr, 
            "* ... pcoAcqMode[%s] [%d]\n",
            m_pcoData->traceAcq.sPcoAcqMode,
            m_pcoData->traceAcq.iPcoAcqMode);

    ptr += __sprintfSExt(ptr, ptrMax - ptr, 
            "* msStartAcqStart[%ld]  msStartAcqEnd[%ld]\n",
            m_pcoData->traceAcq.msStartAcqStart, 
            m_pcoData->traceAcq.msStartAcqEnd);

    for (int _i = 0; _i < LEN_TRACEACQ_TRHEAD; _i++)
    {
        const char *desc = m_pcoData->traceAcq.usTicks[_i].desc;
        if (desc != NULL)
        {
            ptr += __sprintfSExt(ptr, ptrMax - ptr, 
                    "* ... usTicks[%d][%5.3f] (ms)   (%s)\n", _i,
                    m_pcoData->traceAcq.usTicks[_i].value / 1000., desc);
        }
    }


    ptr += __sprintfSExt(ptr, ptrMax - ptr,
            "* msImgCoc[%.3g] fps[%.3g] msTotal[%ld]\n",
            m_pcoData->traceAcq.msImgCoc, 
            1000. / m_pcoData->traceAcq.msImgCoc,
            m_pcoData->traceAcq.msTotal);

    time_t ts;

    ts = m_pcoData->traceAcq.startRecordTimestamp;
    ptr += __sprintfSExt(ptr, ptrMax - ptr,
            "* ... msRecLoop[%ld] msRec[%ld] msRecTimeOut[%ld] startRec[%s]",
            m_pcoData->traceAcq.msRecordLoop, 
            m_pcoData->traceAcq.msRecord,
            m_pcoData->traceAcq.msTout, 
            ts ? getTimestamp(Iso, ts) : "noSet" );

    ts = m_pcoData->traceAcq.endRecordTimestamp;
    ptr += __sprintfSExt(ptr, ptrMax - ptr,
            " endRec[%s]\n",
            ts ? getTimestamp(Iso, ts) : "noSet" );

    ts = m_pcoData->traceAcq.startXferTimestamp;
    ptr += __sprintfSExt(ptr, ptrMax - ptr, 
            "* ... msXfer[%ld] startXfer[%s]",
            m_pcoData->traceAcq.msXfer,
            ts ? getTimestamp(Iso, ts) : "noSet" );

    ts = m_pcoData->traceAcq.endXferTimestamp;
    ptr += __sprintfSExt(ptr, ptrMax - ptr, 
            " endXfer[%s]\n",
            ts ? getTimestamp(Iso, ts) : "noSet" );

    ptr += __sprintfSExt(ptr, ptrMax - ptr,
            "* ... xferTimeTot[%g s] xferSpeed[%g MB/s][%g fps]\n",
            totTime, xferSpeed, framesPerSec);

    ptr += __sprintfSExt(ptr, ptrMax - ptr,
            "* ... checkImgNr[%s] pco[%d] lima[%d] diff[%d] order[%d]\n",
            m_pcoData->traceAcq.checkImgNr ? "true" : "false",
            m_pcoData->traceAcq.checkImgNrPco, 
            m_pcoData->traceAcq.checkImgNrLima,
            m_pcoData->traceAcq.checkImgNrPco - m_pcoData->traceAcq.checkImgNrLima,
            m_pcoData->traceAcq.checkImgNrOrder);

    ptr += __sprintfSExt(ptr, ptrMax - ptr, 
            "%s\n", 
            m_pcoData->traceAcq.msg);

    o_sn = buff;
}
//====================================================================
//====================================================================
void Camera::getPixelRateValidValues(std::string &o_sn)
{
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);
    DWORD dwPixRate, dwPixRateNext;
    int error, i, nr;

    _pco_GetPixelRate(dwPixRate, dwPixRateNext, error);

    for (nr = i = 0; i < 4; i++)
    {
        dwPixRate = m_pcoData->stcPcoDescription.dwPixelRateDESC[i];
        if (dwPixRate)
        {
            nr++;
            ptr += __sprintfSExt(ptr, ptrMax - ptr, "%d  ", dwPixRate);
        }
    }

    if (nr == 0)
        ptr += __sprintfSExt(ptr, ptrMax - ptr, "%d  ", nr);

    o_sn = buff;
}

//====================================================================
//====================================================================
void Camera::getLastFixedRoi(std::string &o_sn)
{
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);

    //------
    unsigned int x0, x1, y0, y1;
    Roi new_roi;
    int error;

    Roi limaRoiRequested, limaRoiFixed, limaRoi;
    time_t dt;

    _get_logLastFixedRoi(limaRoiRequested, limaRoiFixed, dt);

    _pco_GetROI(limaRoi, error);
    x0 = limaRoi.getTopLeft().x - 1;
    x1 = limaRoi.getBottomRight().x - 1;
    y0 = limaRoi.getTopLeft().y - 1;
    y1 = limaRoi.getBottomRight().y - 1;

    ptr += __sprintfSExt(ptr, ptrMax - ptr,
                         "* roi PCO ACTUAL X(%d,%d) Y(%d,%d) size(%d,%d)\n", x0,
                         x1, y0, y1, x1 - x0 + 1, y1 - y0 + 1);

    if (dt)
    {
        limaRoi = limaRoiRequested;
        x0 = limaRoi.getTopLeft().x - 1;
        x1 = limaRoi.getBottomRight().x - 1;
        y0 = limaRoi.getTopLeft().y - 1;
        y1 = limaRoi.getBottomRight().y - 1;

        ptr += __sprintfSExt(
            ptr, ptrMax - ptr,
            "* roi PCO REQUESTED X(%d,%d) Y(%d,%d) size(%d,%d) [%s]\n", x0, x1,
            y0, y1, x1 - x0 + 1, y1 - y0 + 1, getTimestamp(Iso, dt));

        limaRoi = limaRoiFixed;
        x0 = limaRoi.getTopLeft().x - 1;
        x1 = limaRoi.getBottomRight().x - 1;
        y0 = limaRoi.getTopLeft().y - 1;
        y1 = limaRoi.getBottomRight().y - 1;

        ptr += __sprintfSExt(ptr, ptrMax - ptr,
                             "* roi PCO FIXED X(%d,%d) Y(%d,%d) size(%d,%d)\n",
                             x0, x1, y0, y1, x1 - x0 + 1, y1 - y0 + 1);
    }
    else
    {
        ptr += __sprintfSExt(ptr, ptrMax - ptr,
                             "* roi PCO FIXED - no roi fixed yet!\n");
    }

    bool bSymX, bSymY;
    unsigned int xMax, yMax, xSteps, ySteps, xMinSize, yMinSize;
    getXYdescription(xSteps, ySteps, xMax, yMax, xMinSize, yMinSize);
    getRoiSymetrie(bSymX, bSymY);

    ptr += __sprintfSExt(
        ptr, ptrMax - ptr,
        "* xySteps[%d,%d] xyMinSize[%d,%d] xyMaxSize[%d,%d] xySym[%d,%d]\n",
        xSteps, ySteps, xMinSize, yMinSize, xMax, yMax, bSymX, bSymY);

    //------

    o_sn = buff;
}

//====================================================================
// SIP - attrib
//====================================================================
void Camera::getCDIMode(int &cdi)
{
    int error;
    WORD wCdi;

    _pco_GetCDIMode(wCdi, error);

    cdi = error ? -1 : wCdi;
}

void Camera::setCDIMode(int cdi)
{
    int error;
    WORD wCdi = (WORD)cdi;

    _pco_SetCDIMode(wCdi, error);
}

//=================================================================================================
//=================================================================================================

void Camera::getRoiSymetrie(bool &bSymX, bool &bSymY)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    bSymY = bSymX = false;
    if (_isCameraType(Dimax))
    {
        bSymX = bSymY = true;
    }
    if (_isCameraType(Edge))
    {
        bSymY = true;
    }

    int adc_working, adc_max;
    _pco_GetADCOperation(adc_working, adc_max);
    if (adc_working != 1)
    {
        bSymX = true;
    }
}
//====================================================================
// SIP - attrib
//====================================================================
void Camera::getTemperatureInfo(std::string &o_sn)
{
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);
    int error;

    _pco_GetTemperatureInfo(ptr, ptrMax, error);

    o_sn = buff;
}

//====================================================================
// SIP - attributes
//====================================================================

void Camera::getCoolingTemperature(int &val)
{
    int error;
    _pco_GetCoolingSetpointTemperature(val, error);
}

void Camera::setCoolingTemperature(int val)
{
    int error;
    _pco_SetCoolingSetpointTemperature(val, error);
}

//====================================================================
// SIP - attributes
//====================================================================

void Camera::getSdkRelease(std::string &o_sn)
{
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);
    const char *release;

#ifdef __linux__
    release = PCO_SDK_LIN_RELEASE;
#else
    release = PCO_SDK_WIN_RELEASE;
#endif

    ptr += __sprintfSExt(ptr, ptrMax - ptr, release);

    o_sn = buff;
}

//=================================================================================================
// SIP - attributes
//=================================================================================================
void Camera::getCameraName(std::string &o_sn)
{
    DEB_MEMBER_FUNCT();
    DEB_TRACE() << DEB_VAR1(m_pcoData->camera_name);

    o_sn = m_pcoData->camera_name;
}

void Camera::getCameraNameBase(std::string &o_sn)
{
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);
    int error;

    _pco_GetInfoString(1, ptr, (int)(ptrMax - ptr), error);

    o_sn = buff;
}

void Camera::getCameraNameEx(std::string &o_sn)
{
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);
    int error;

    _pco_GetInfoString(0, ptr, (int)(ptrMax - ptr), error);
    ptr += strlen(ptr);

    ptr += __sprintfSExt(ptr, ptrMax - ptr, "\nCamera name: ");
    _pco_GetInfoString(1, ptr, (int)(ptrMax - ptr), error);
    ptr += strlen(ptr);

    ptr += __sprintfSExt(ptr, ptrMax - ptr, "\nSensor: ");
    _pco_GetInfoString(2, ptr, (int)(ptrMax - ptr), error);
    ptr += strlen(ptr);

    o_sn = buff;
}
//=================================================================================================
// SIP - attributes
//=================================================================================================
void Camera::getBinningInfo(std::string &o_sn)
{
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);
    int error;

    _pco_GetBinningInfo(ptr, (int)(ptrMax - ptr), error);

    o_sn = buff;
}
void Camera::getFirmwareInfo(std::string &o_sn)
{
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);
    int error;

    _pco_GetFirmwareInfo(ptr, (int)(ptrMax - ptr), error);

    o_sn = buff;
}
void Camera::getRoiInfo(std::string &o_sn)
{
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);
    int error;

    _pco_GetRoiInfo(ptr, (int)(ptrMax - ptr), error);

    o_sn = buff;
}

//=================================================================================================
// SIP - msgLog
//=================================================================================================
void Camera::getMsgLog(std::string &o_sn)
{
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);

    // 0 -> decreasing / older - last
    // 1 -> increasing / newer - last
    m_msgLog->dump(ptr, (int)(ptrMax - ptr), 1);

    o_sn = buff;
}

//=================================================================================================
// SIP - msgLog
//=================================================================================================
void Camera::getCamerasFound(std::string &o_sn)
{
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);

    ptr += __sprintfSExt(ptr, ptrMax - ptr, "search:\n%s",
                         m_pcoData->camerasFound);
    ptr += __sprintfSExt(ptr, ptrMax - ptr, "opened:\n%s", _getCameraIdn());

    o_sn = buff;
}
//====================================================================
// SIP - attrib
//====================================================================
void Camera::getDoubleImageMode(int &mode)
{
    int error;
    WORD wMode;

    _pco_GetDoubleImageMode(wMode, error);

    mode = error ? -1 : wMode;
}

void Camera::setDoubleImageMode(int mode)
{
    int error;
    WORD wMode = (WORD)mode;

    _pco_SetDoubleImageMode(wMode, error);
}

//====================================================================
// SIP - attrib
//====================================================================
void Camera::getDebugInt(std::string &o_sn)
{
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);
    unsigned long long debugLevel;

    debugLevel = m_pcoData->debugLevel;
    ptr += __sprintfSExt(ptr, ptrMax - ptr, "0x%llx (%lld)", debugLevel,
                         debugLevel);

    o_sn = buff;
}

void Camera::setDebugInt(std::string &i_sn)
{
    DEB_MEMBER_FUNCT();

    unsigned long long debugLevel;
    const char *strIn = i_sn.c_str();

#ifdef __linux__
    debugLevel = atoll(strIn);
#else
    debugLevel = _atoi64(strIn);
#endif
    m_pcoData->debugLevel = debugLevel;
}

void Camera::getDebugIntTypes(std::string &o_sn)
{
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);

#define _PRINT_DBG(x)                                                          \
    ptr += __sprintfSExt(ptr, ptrMax - ptr, "%15s  0x%08x\n", #x, x)

    ptr += __sprintfSExt(ptr, ptrMax - ptr, "\n");
    _PRINT_DBG(DBG_BUFF);
    _PRINT_DBG(DBG_XFER2LIMA);
    _PRINT_DBG(DBG_LIMABUFF);
    _PRINT_DBG(DBG_EXP);
    _PRINT_DBG(DBG_XFERMULT);
    _PRINT_DBG(DBG_XFERMULT1);
    _PRINT_DBG(DBG_ASSIGN_BUFF);
    _PRINT_DBG(DBG_DUMMY_IMG);
    _PRINT_DBG(DBG_WAITOBJ);
    _PRINT_DBG(DBG_XFER_IMG);
    _PRINT_DBG(DBG_TRACE_FIFO);
    _PRINT_DBG(DBG_ROI);
    _PRINT_DBG(DBG_FN1);
    _PRINT_DBG(DBG_FN2);
    _PRINT_DBG(DBG_FN3);
    _PRINT_DBG(DBG_FN4);

    o_sn = buff;
}

//====================================================================
// SIP - attrib
//====================================================================
void Camera::setTimestampMode(int val)
{
    DEB_MEMBER_FUNCT();
    int err;

    WORD mode = val;

    _pco_SetTimestampMode(mode, err);
}

void Camera::getTimestampMode(int &val)
{
    DEB_MEMBER_FUNCT();
    int err;
    WORD mode;
    val = 0;
    _pco_GetTimestampMode(mode, err);

    if (!err)
        val = mode;
}

//====================================================================
// SIP - attrib
//====================================================================
void Camera::getBitAlignment(std::string &o_sn)
{
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);
    int alignment, error;

    error = _pco_GetBitAlignment(alignment);

    if (error)
    {
        ptr += __sprintfSExt(ptr, ptrMax - ptr, "ERROR");
    }
    else
    {
        ptr += __sprintfSExt(ptr, ptrMax - ptr, "%s(%d)",
                             (alignment ? "LSB" : "MSB"), alignment);
    }

    o_sn = buff;
}

void Camera::setBitAlignment(std::string &i_sn)
{
    DEB_MEMBER_FUNCT();

    const char *strIn = i_sn.c_str();
    int _val;

    if ((_stricmp(strIn, "0") == 0) || (_stricmp(strIn, "MSB") == 0))
    {
        _val = 0;
    }
    else
    {
        if ((_stricmp(strIn, "1") == 0) || (_stricmp(strIn, "LSB") == 0))
        {
            _val = 1;
        }
        else
        {
            DEB_ALWAYS() << "ERROR - invalid value: " << strIn;
            return;
        }
    }

    _pco_SetBitAlignment(_val);
    return;
}
//====================================================================
// SIP - attrib
//====================================================================
void Camera::setRecorderForcedFifo(int val)
{
    DEB_MEMBER_FUNCT();

    m_bRecorderForcedFifo = !!val;
}

void Camera::getRecorderForcedFifo(int &val)
{
    DEB_MEMBER_FUNCT();
    val = m_bRecorderForcedFifo;
}
//====================================================================
// SIP - attrib
//====================================================================
void Camera::setNrEvents(int val)
{
    DEB_MEMBER_FUNCT();

    if ((val < 1) || (val > PCO_BUFFER_NREVENTS))
        return;
    m_pco_buffer_nrevents = val;
}

void Camera::getNrEvents(int &val)
{
    DEB_MEMBER_FUNCT();
    val = m_pco_buffer_nrevents;
}

//====================================================================
// SIP - attrib
//====================================================================

void Camera::setRecorderStopRequest(int val)
{
    DEB_MEMBER_FUNCT();

    HwInterface::StatusType status;
    getStatus(status);

    if (_getRecorderStopRequest() || !_isCameraType(camRAM) ||
        !(_pco_GetStorageMode_GetRecorderSubmode() != Fifo) ||
        !(status.acq != AcqRunning))
        return;

    _setRecorderStopRequest(1);

    DEB_ALWAYS() << "INFO - setRecorderStopRequest event generated";

    Event *ev = new Event(Hardware, Event::Error, Event::Camera,
                          Event::CamFault, "EVENT setRecorderStopRequest");
    _getPcoHwEventCtrlObj()->reportEvent(ev);
    return;
}

void Camera::getRecorderStopRequest(int &val)
{
    DEB_MEMBER_FUNCT();
    val = _getRecorderStopRequest();
}

int Camera::_getRecorderStopRequest()
{
    DEB_MEMBER_FUNCT();
    return m_iRecorderStopRequest;
}

int Camera::_setRecorderStopRequest(int val)
{
    DEB_MEMBER_FUNCT();

    return (m_iRecorderStopRequest = val);
}
//====================================================================
// SIP - attrib
//====================================================================

void Camera::getLastImgFifo(int &val)
{
    DEB_MEMBER_FUNCT();
    val = m_pcoData->traceAcq.lastImgFifo;
}

//====================================================================
// SIP - attrib / EDGE - rolling shutter
//====================================================================

void Camera::getRollingShutter(int &val)
{
    int error;
    DWORD dwRolling;

    if (!_isCameraType(Edge))
    {
        val = -1;
        return;
    }
    _get_shutter_rolling_edge(dwRolling, error);
    val = dwRolling;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
// GeneralCaps1-Bits
//    NO_GLOBAL_SHUTTER 0x00080000 Global shutter operation mode not available
//    GLOBAL_RESET_MODE 0x00100000 Global reset operation mode not available

void Camera::setRollingShutter(int val)
{
    DEB_MEMBER_FUNCT();

    int error;
    DWORD dwRolling, dwRollingNew;

    dwRollingNew = (DWORD)val;

    if (!_isValid_rollingShutter(dwRollingNew))
    {
        DEB_ALWAYS() << "ERROR requested Rolling Shutter not allowed "
                     << DEB_VAR1(dwRollingNew);
        error = -1;
        return;
    }

    _get_shutter_rolling_edge(dwRolling, error);

    // different modules for lin & win
    //   lin -> activate thread Camera::_AcqThread::threadFunction_SwitchEdge()
    //   win -> activate thread _pco_shutter_thread_edge
    //          -> m_cam->_pco_set_shutter_rolling_edge
    if (dwRollingNew != dwRolling)
    {
        _set_shutter_rolling_edge(dwRollingNew, error);
    }
}

void Camera::setRollingShutterStr(std::string &i_sn)
{
    DEB_MEMBER_FUNCT();

    const char *strIn = i_sn.c_str();
    int _val;

    if ((_stricmp(strIn, "1") == 0) || (_stricmp(strIn, "RS") == 0))
    {
        _val = 1;
    }
    else if ((_stricmp(strIn, "2") == 0) || (_stricmp(strIn, "GS") == 0))
    {
        _val = 2;
    }
    else if ((_stricmp(strIn, "4") == 0) || (_stricmp(strIn, "GR") == 0))
    {
        _val = 4;
    }
    else
    {
        DEB_ALWAYS() << "ERROR requested Rolling Shutter not allowed "
                     << DEB_VAR1(strIn);
        return;
    }

    setRollingShutter(_val);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------

void Camera::getRollingShutterInfo(std::string &o_sn)
{
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);
    const char *sId;

    int val;

    bool bRS = _isCapsDesc(capsRollingShutter);
    bool bGL = _isCapsDesc(capsGlobalShutter);
    bool bGR = _isCapsDesc(capsGlobalResetShutter);

    if (!(bRS || bGL || bGR))
    {
        ptr +=
            __sprintfSExt(ptr, ptrMax - ptr, "Rolling Shutter is not allowed");
        o_sn = buff;
        return;
    }

    getRollingShutter(val);
    switch (val)
    {
        case 1:
            sId = "RS";
            break;
        case 2:
            sId = "GS";
            break;
        case 4:
            sId = "GR";
            break;
        default:
            sId = "ERROR";
            break;
    }

    ptr += __sprintfSExt(ptr, ptrMax - ptr, "actual[%d][%s] validValues: ", val,
                         sId);

    if (bRS)
        ptr += __sprintfSExt(ptr, ptrMax - ptr, "rolling[%d][RS] ",
                             (int)PCO_EDGE_SETUP_ROLLING_SHUTTER);
    if (bGL)
        ptr += __sprintfSExt(ptr, ptrMax - ptr, "global[%d][GS] ",
                             (int)PCO_EDGE_SETUP_GLOBAL_SHUTTER);
    if (bGR)
        ptr += __sprintfSExt(ptr, ptrMax - ptr, "globalReset[%d][GR] ",
                             (int)PCO_EDGE_SETUP_GLOBAL_RESET);

    o_sn = buff;
    return;
}

//=================================================================================================
//=================================================================================================

void Camera::getGeneralCAPS1(std::string &o_sn)
{
    // error = camera->PCO_GetCameraDescription(&m_pcoData->stcPcoDescription);
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);

    DWORD dwGeneralCaps1;
    DWORD dwVal;

#ifndef __linux__
    dwGeneralCaps1 = m_pcoData->stcPcoDescription.dwGeneralCapsDESC1;
#else
    dwGeneralCaps1 = m_pcoData->stcPcoDescription.dwGeneralCaps1;
#endif

    int nib[8];
    int i, j;

    dwVal = dwGeneralCaps1;

    for (i = 0; i < 8; i++)
    {
        nib[i] = (dwVal & 0xF);
        dwVal >>= 4;
    }

    ptr += __sprintfSExt(ptr, ptrMax - ptr, "CAPS1: %08X\n", dwGeneralCaps1);
    ptr += __sprintfSExt(ptr, ptrMax - ptr, "(hex): ");
    for (i = 7; i >= 0; i--)
    {
        ptr += __sprintfSExt(ptr, ptrMax - ptr, "%5X", nib[i]);
    }
    ptr += __sprintfSExt(ptr, ptrMax - ptr, "\n");

    ptr += __sprintfSExt(ptr, ptrMax - ptr, "(bin): ");
    for (i = 7; i >= 0; i--)
    {
        ptr += __sprintfSExt(ptr, ptrMax - ptr, " ");
        for (j = 3; j >= 0; j--)
        {
            ptr += __sprintfSExt(ptr, ptrMax - ptr, "%1d",
                                 ((1 << j) & nib[i]) ? 1 : 0);
        }
    }
    ptr += __sprintfSExt(ptr, ptrMax - ptr, "\n");

    o_sn = buff;
}

//=================================================================================================
//=================================================================================================

void Camera::getParamsInfo(std::string &o_sn)
{
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);

    struct lima::Pco::stcPcoData::stcParams *par = &m_pcoData->params;

    int nr = par->nr;
    int nrMax = sizeof(par->ptrKey) / sizeof(par->ptrKey[0]);
    int lenTok = sizeof(par->buff) / nrMax;

    ptr += __sprintfSExt(ptr, ptrMax - ptr, "initial values:\n");
    ptr += __sprintfSExt(ptr, ptrMax - ptr, "keys: %d/%d lenMaxToken: %d\n", nr,
                         nrMax, lenTok);
    for (int i = 0; i < nr; i++)
    {
        ptr += __sprintfSExt(ptr, ptrMax - ptr, "[%s] [%s]\n", par->ptrKey[i],
                             par->ptrValue[i]);
    }

    ptr += __sprintfSExt(ptr, ptrMax - ptr, "working values:\n");
    ptr +=
        __sprintfSExt(ptr, ptrMax - ptr, "logBits:  [0x%08x]\n", par->logBits);
    ptr += __sprintfSExt(ptr, ptrMax - ptr, "logPath:  [%s]\n", par->logPath);
    ptr +=
        __sprintfSExt(ptr, ptrMax - ptr, "testMode: [0x%llx]\n", par->testMode);

    o_sn = buff;
}

//====================================================================
// SIP - attrib
//====================================================================
#define NRTOK 10
void Camera::setTest(std::string &i_sn)
{
    DEB_MEMBER_FUNCT();

    const char *cmd, *cmd0;
    char msg[MSG1K];

    int ikey = 0;
    const char *tok[NRTOK];
    int tokNr;

    char *ptr = msg;
    char *ptrMax = msg + sizeof(msg);

    cmd0 = i_sn.c_str();
    strncpy_s(buff, sizeof(buff), cmd0, sizeof(buff) - 1);
    cmd = str_trim(buff);

    tokNr = -1;
    if (*cmd)
    {
        char *tokContext;
        for (int i = 0; i < NRTOK; i++)
        {
            if ((tok[i] = strtok_s((char *)cmd, " ", &tokContext)) == NULL)
                break;
            cmd = NULL;
            tokNr = i;
        }
        cmd = tok[0];
    }
    tokNr++;

    // DEB_ALWAYS() << DEB_VAR2(cmd0, tokNr);

    if (tokNr == 0)
        return;

    //------------------------------------------------
    // syntax:  mode  0x123
    // mode 0x1234
    //------------------------------------------------
    if (_stricmp(cmd, "mode") == 0)
    {
        ptr += __sprintfSExt(ptr, ptrMax - ptr, "testCmdMode [0x%llx]",
                             m_pcoData->testCmdMode);
        if (tokNr > 1)
        {
            int nr;
            unsigned long long _testCmdMode;

            nr = sscanf_s(tok[1], "0x%llx", &_testCmdMode);

            if (nr == 1)
            {
                m_pcoData->testCmdMode = _testCmdMode;
                ptr += __sprintfSExt(ptr, ptrMax - ptr, "   changed OK>  ");
            }
            else
            {
                ptr += __sprintfSExt(ptr, ptrMax - ptr,
                                     "   ERROR - NOT changed>  ");
            }
            ptr += __sprintfSExt(ptr, ptrMax - ptr, "testCmdMode [0x%llx]",
                                 m_pcoData->testCmdMode);
        }
        DEB_ALWAYS() << msg;
        return;
    }

    //------------------------------------------------
    //------------------------------------------------

    //------------------------------------------------
    // NOT FOUND
    //------------------------------------------------
    DEB_ALWAYS() << " command NOT FOUND " << cmd0;
}

void Camera::getTest(std::string &o_sn)
{
    DEB_MEMBER_FUNCT();
    char *ptr = buff;
    char *ptrMax = buff + sizeof(buff);

    ptr += __sprintfSExt(ptr, ptrMax - ptr, "valid commands:\n");

    o_sn = buff;
}

//====================================================================
//====================================================================
