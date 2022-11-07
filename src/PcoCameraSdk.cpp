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
#include <cstdlib>


#include <sys/stat.h>
#include <sys/timeb.h>
#include <time.h>

#include "lima/Exceptions.h"
#include "lima/HwSyncCtrlObj.h"

#include "PcoCamera.h"
#include "PcoCameraSdk.h"
#include "PcoSyncCtrlObj.h"

using namespace lima;
using namespace lima::Pco;

//=================================================================================================
//=================================================================================================

// 4294967295.0 = double(DWORD(0xFFFFFFFF))
#define DWORD_MAX_FLOAT 4294967295.0

#define MAX_DWORD_MS (double(4294967295.0e-3))
#define MAX_DWORD_US (double(4294967295.0e-6))
#define MAX_DWORD_NS (double(4294967295.0e-9))

void Camera::_pco_time2dwbase(double exp_time, DWORD &dwExp, WORD &wBase)
{
    // conversion time(s) to PCO standard DWORD + UNIT(ms, us, ns)
    // exp & lat time is saved in seconds (LIMA).
    // PCO requires them expressed in DWORD as ms(base=2), us(base=1) or
    // ns(base=0) max DWORD 0xFFFFFFFF = 4294967295.0 find the lowest unit (ns
    // -> us -> ms) which does not overflow DWORD

    if (exp_time <= MAX_DWORD_NS)
    {
        dwExp = DWORD(exp_time * 1.0e9);
        wBase = 0; // ns
    }
    else if (exp_time <= MAX_DWORD_US)
    {
        dwExp = DWORD(exp_time * 1.0e6);
        wBase = 1; // us
    }
    else
    {
        dwExp = DWORD(exp_time * 1.0e3);
        wBase = 2; // ms
    }

    DWORD mask = 0x7;
    DWORD min = 0x1000;

    if (dwExp > min)
    {
        dwExp |= mask;
        dwExp ^= mask;
    }

    return;
}


void Camera::_pco_time2dwbase_bis(double exp_time, DWORD &dwExp, WORD &wBase)
{
    // conversion time(s) to PCO standard DWORD + UNIT(ms, us, ns)
    // exp & lat time is saved in seconds (LIMA).
    // PCO requires them expressed in DWORD as ms(base=2), us(base=1) or
    // ns(base=0) max DWORD 0xFFFFFFFF = 4 294 967 295.0 find the lowest unit (ns
    // -> us -> ms) which does not overflow DWORD

	double factor = 1000.0;
    DWORD dwExpMask = 0x0007;
    DWORD dwExpMin  = 0x1000;
	
	for(int base = 2; base >= 0; base--) 
	{
		dwExp = DWORD(exp_time * factor);
		wBase = base;

		if(dwExp > dwExpMin)
		{
			// set to 0 the 3 LSBits ... XXXX X000
			dwExp |= dwExpMask;
			dwExp ^= dwExpMask;
			break;
		}

		factor *= 1000.0;
	}


    return;
}




//====================================================================
//====================================================================
// extract image number and time from the image
// timeStampMode must be enabled
//====================================================================
//====================================================================

CheckImgNr::CheckImgNr(Camera *cam)
{
    DEF_FNID;
    //printf("--- %s> [ENTRY]\n", fnId);

    m_cam = cam;
}

CheckImgNr::~CheckImgNr()
{
    DEF_FNID;
    //printf("--- %s> [ENTRY]\n", fnId);
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
void CheckImgNr::update(int iLimaFrame, void *ptrImage)
{
    DEF_FNID;

    //printf("--- %s> [ENTRY]\n", fnId);

    int pcoImgNr, diff;
    int iImgNrLima;

    if (!checkImgNr)
    {
        //printf("--- %s> --- BYPASSED - NOT CHECKED\n", fnId);
        return;
    }
    
    pcoImgNr = _get_imageNr_from_imageTimestamp(ptrImage, alignmentShift);
    if (pcoImgNr <= pcoImgNrLast)
        pcoImgNrOrder++;
    diff = pcoImgNr - iLimaFrame;
    m_traceAcq->checkImgNrLima = iImgNrLima = iLimaFrame + 1;
    m_traceAcq->checkImgNrPco = pcoImgNr;
    m_traceAcq->checkImgNrOrder = pcoImgNrOrder;

    if (diff > pcoImgNrDiff)
    {
        pcoImgNrDiff = diff;
    }
    pcoImgNrLast = pcoImgNr;

    //printf("--- %s> alignment[%d] iImgNrLima[%d], pcoImgNr[%d], diff[%d], pcoImgNrDiff[%d]\n", 
    //        fnId, alignmentShift, iImgNrLima, pcoImgNr, diff, pcoImgNrDiff);


}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
void CheckImgNr::init(STC_traceAcq *traceAcq)
{
    DEF_FNID;
    
    m_traceAcq = traceAcq;

    pcoImgNrDiff = 1;
    alignmentShift = 0;

    int err;
    WORD wTimeStampMode;
    m_cam->_pco_GetTimestampMode(wTimeStampMode, err);

    bNoTimestamp = err || !wTimeStampMode;
    checkImgNr = !bNoTimestamp;
    m_traceAcq->checkImgNr = checkImgNr; 

    //printf("--- %s> checkImgNr[%d], err[%d], wTimeStampMode[%d]\n", fnId, checkImgNr, err, wTimeStampMode);
    
    pcoImgNrLast = -1;
    pcoImgNrOrder = 0;

    int alignment;

    m_cam->_pco_GetBitAlignment(alignment);

    if (alignment == 0)
        alignmentShift = (16 - m_cam->m_pcoData->stcPcoDescription.wDynResDESC);
    else
        alignmentShift = 0;


    return;
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------

//*************************************************************************
//#define TIMESTAMP_MODE_BINARY           1
//#define TIMESTAMP_MODE_BINARYANDASCII   2
//
//SYSTEMTIME st;
//int act_timestamp;
//int shift;
//
//if(camval.strImage.wBitAlignment==0)
//  shift = (16-camval.strSensor.strDescription.wDynResDESC);
//else
//  shift = 0;
//
//err=PCO_SetTimestampMode(hdriver,TIMESTAMP_MODE_BINARY); //Binary+ASCII
//
//grab image to adr
//
//time_from_timestamp(adr,shift,&st);
//act_timestamp=image_nr_from_timestamp(adr,shift);
//*************************************************************************

int CheckImgNr::_get_imageNr_from_imageTimestamp(void *buf, int _shift)
{
    DEF_FNID;
    //printf("--- %s> [ENTRY]\n", fnId);

    unsigned short *wData;  // 2 bytes word
    unsigned short wBcd;
    int iDecPlaces;
    int image_nr = 0;

    if (bNoTimestamp) return -1;
    
    wData = (unsigned short *)(buf);
    

    // 100 * 100 * 100  1st word  MS BCD   XX......
    // 100 * 100        2nd word  .. BCD   ..XX....
    // 100              3rd word  .. BCD   ....XX..
    // 1                4th word  LS BCD   ......XX
    
    //printf("--- [%04x] [%04x] [%04x] [%04x] \n", *wData, *(wData+1),*(wData+2),*(wData+3)); 
    
    int shift = 0;
    
    for (iDecPlaces = 100 * 100 * 100; iDecPlaces > 0; iDecPlaces /= 100)
    {
        wBcd = *wData >> shift;   // shift - aligment  0 | 8

        image_nr += (((wBcd & 0x00F0) >> 4) * 10 + (wBcd & 0x000F)) * iDecPlaces;
        //                        X.                           .X
        //                    MS decDig               LS decDig
    
        wData++;    // next word
     }

    return image_nr;
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------

//=================================================================================================
//=================================================================================================
void Camera::_pco_GetAcqEnblSignalStatus(WORD &wAcquEnableState, int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    WORD _wAcquEnableState;

    error = camera->PCO_GetAcqEnblSignalStatus(&_wAcquEnableState);
    PCO_CHECK_ERROR(error, "PCO_GetAcqEnblSignalStatus");


    if (error)
        return;

    wAcquEnableState = _wAcquEnableState;
}

//=================================================================================================
//=================================================================================================

double Camera::pcoGetCocRunTime()
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    return m_pcoData->cocRunTime;
}

double Camera::pcoGetFrameRate()
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    return m_pcoData->frameRate;
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_GetCOCRuntime(int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    const char *msg;

    //====================================== get the coc runtime
    //---- only valid if it was used PCO_SetDelayExposureTime
    //---- and AFTER armed the cam

    // Get and split the 'camera operation code' runtime into two DWORD. One
    // will hold the longer part, in seconds, and the other will hold the
    // shorter part, in nanoseconds. This function can be used to calculate the
    // FPS. The sum of dwTime_s and dwTime_ns covers the delay, exposure and
    // readout time. If external exposure is active, it returns only the readout
    // time.

    DWORD dwTime_s, dwTime_ns;
    double runTime;


    error = camera->PCO_GetCOCRuntime(&dwTime_s, &dwTime_ns);
    msg = "PCO_GetCOCRuntime";
    PCO_CHECK_ERROR(error, msg);
    if (error)
        return;

    m_pcoData->cocRunTime = runTime =
        ((double)dwTime_ns * NANO) + (double)dwTime_s;
    m_pcoData->frameRate = (dwTime_ns | dwTime_s) ? 1.0 / runTime : 0.0;

    DEB_TRACE() << DEB_VAR2(m_pcoData->frameRate, m_pcoData->cocRunTime);

    return;
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_SetTimestampMode(WORD mode, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    WORD modeNew, modeOld;


    err = camera->PCO_GetTimestampMode(&modeOld);
    PCO_CHECK_ERROR(err, "PCO_GetTimestampMode");

    err = camera->PCO_SetTimestampMode(mode);
    PCO_CHECK_ERROR(err, "PCO_SetTimestampMode");

    err = camera->PCO_GetTimestampMode(&modeNew);
    PCO_CHECK_ERROR(err, "PCO_GetTimestampMode");


    DEB_TRACE() << "\n   " << DEB_VAR3(mode, modeOld, modeNew);

    return;
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_GetTimestampMode(WORD &mode, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;


    err = camera->PCO_GetTimestampMode(&mode);
    PCO_CHECK_ERROR(err, "PCO_GetTimestampMode");


    DEB_TRACE() << "\n   " << DEB_VAR1(mode);
    return;
}
//=================================================================================================
//=================================================================================================
WORD Camera::_pco_GetRecordingState(int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    WORD wRecState_actual;

    err = camera->PCO_GetRecordingState(&wRecState_actual);
    PCO_CHECK_ERROR(err, "PCO_GetRecordingState");

    return wRecState_actual;
}
//=================================================================================================
//=================================================================================================

/**************************************************************************************************
        If a set recording status = [stop] command is sent and the current
status is already [stop]’ped, nothing will happen (only warning, error message).

        If the camera is in
        [run]’ing state, it will last some time (system delay + last image
readout), until the camera is stopped. The system delay depends on the PC and
the image readout depends on the image size transferred. The SetRecordingState =
[stop] checks for a stable stop state by calling GetRecordingState.  --- 165 ms

        Please call PCO_CancelImages to remove pending buffers from the driver.
--- 1.5 s
**************************************************************************************************/

const char *Camera::_pco_SetRecordingState(int state, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    long long usStart;

    WORD wRecState_new, wRecState_actual;

    wRecState_new = state ? 0x0001 : 0x0000; // 0x0001 => START acquisition

    usElapsedTimeSet(usStart);

    wRecState_actual = _pco_GetRecordingState(err);

    //_setCameraState(CAMSTATE_RECORD_STATE, !!(wRecState_actual));

    m_pcoData->traceAcq.usTicks[traceAcq_GetRecordingState].value =
        usElapsedTime(usStart);
    m_pcoData->traceAcq.usTicks[traceAcq_GetRecordingState].desc =
        "PCO_GetRecordingState execTime";
    usElapsedTimeSet(usStart);

    if (wRecState_new == wRecState_actual)
    {
        DEB_TRACE() << "  SetRecordingState - BYPASSED "
                    << DEB_VAR2(wRecState_new, wRecState_actual);
        err = 0;
        return fnId;
    }

    // ------------------------------------------ cancel images
    if (wRecState_new == 0)
    {
        int count = 1;

        _setCameraState(CAMSTATE_RECORD_STATE, false);

        if (count && !_isCameraType(Edge))
        {
            DEB_TRACE() << fnId << ": PCO_CancelImages";

            err = camera->PCO_CancelImage();
            PCO_CHECK_ERROR(err, "PCO_CancelImage");

            err = camera->PCO_CancelImageTransfer();
            PCO_CHECK_ERROR(err, "PCO_CancelImageTransfer");
        }
    }

    err = camera->PCO_SetRecordingState(wRecState_new);
    PCO_CHECK_ERROR(err, "PCO_SetRecordingState");


    wRecState_actual = _pco_GetRecordingState(err);
    _setCameraState(CAMSTATE_RECORD_STATE, !!(wRecState_actual));

    m_pcoData->traceAcq.usTicks[traceAcq_SetRecordingState].value =
        usElapsedTime(usStart);
    m_pcoData->traceAcq.usTicks[traceAcq_SetRecordingState].desc =
        "PCO_SetRecordingState execTime";
    usElapsedTimeSet(usStart);


    m_pcoData->traceAcq.usTicks[traceAcq_CancelImages].value =
        usElapsedTime(usStart);
    m_pcoData->traceAcq.usTicks[traceAcq_CancelImages].desc =
        "PCO_CancelImages execTime";
    usElapsedTimeSet(usStart);

    // DEB_TRACE() << fnId << ": " << DEB_VAR4(error, state, wRecState_actual,
    // wRecState_new);
    return fnId;
}
//=================================================================================================
//=================================================================================================

//=================================================================================================
//=================================================================================================
int Camera::_pco_GetBitAlignment(int &alignment)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    int error = 0;
    WORD wBitAlignment;

    error = camera->PCO_GetBitAlignment(&wBitAlignment);
    PCO_CHECK_ERROR(error, "PCO_GetBitAlignment");
    PCO_THROW_OR_TRACE(error, "PCO_GetBitAlignment");

    if (error)
        return error;

    alignment = m_pcoData->wBitAlignment = wBitAlignment;

    return error;
}

//=================================================================================================
//=================================================================================================

// wBitAlignment:
// - 0x0000 = [MSB aligned]; all raw image data will be aligned to the MSB. This
// is thedefault setting.
// - 0x0001 = [LSB aligned]; all raw image data will be aligned to the LSB.

int Camera::_pco_SetBitAlignment(int alignment)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    int error = 0;
    WORD wBitAlignment;

    if ((alignment < 0) || (alignment > 1))
    {
        DEB_ALWAYS() << "ERROR - invalid value " << DEB_VAR1(alignment);
        return -1;
    }

    wBitAlignment = int(alignment);

    error = camera->PCO_SetBitAlignment(wBitAlignment);
    const char *msg = "PCO_SetBitAlignment";
    PCO_CHECK_ERROR(error, msg);
    PCO_THROW_OR_TRACE(error, msg);

    return error;
}

//=================================================================================================
//=================================================================================================
//-------------------------------------------------------------------------------------------------
// PCO_SetADCOperation
// Set analog-digital-converter (ADC) operation for reading the image sensor
// data. Pixel data can be read out using one ADC (better linearity) or in
// parallel using two ADCs (faster). This option is only available for some
// camera models. If the user sets 2ADCs he must center and adapt the ROI to
// symmetrical values, e.g. pco.1600: x1,y1,x2,y2=701,1,900,500 (100,1,200,500
// is not possible).
//
// DIMAX -> 1 adc
//-------------------------------------------------------------------------------------------------
int Camera::_pco_GetADCOperation(int &adc_working, int &adc_max)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    int error = 0;
    WORD wADCOperation;

    adc_max =
        m_pcoData->stcPcoDescription.wNumADCsDESC; // nr of ADC in the system

    if (adc_max == 2)
    {
        error = camera->PCO_GetADCOperation(&wADCOperation);
        PCO_CHECK_ERROR(error, "PCO_GetADCOperation");

        if (error)
            wADCOperation = (WORD)1;
    }
    else
    {
        adc_max = 1;
        wADCOperation = (WORD)1;
    }

    adc_working = wADCOperation;
    m_pcoData->wNowADC = wADCOperation;

    return error;
}

//=================================================================================================
//=================================================================================================

int Camera::_pco_SetADCOperation(int adc_new, int &adc_working)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    int error, adc_max;

    error = _pco_GetADCOperation(adc_working, adc_max);

    DEB_TRACE() << fnId << ": " DEB_VAR2(adc_max, adc_working);

    if (error)
        return error;

    if ((adc_new >= 1) && (adc_new <= adc_max) && (adc_new != adc_working))
    {
        error = camera->PCO_SetADCOperation((WORD)adc_new);
        PCO_CHECK_ERROR(error, "PCO_SetADCOperation");
        _pco_GetADCOperation(adc_working, adc_max);
    }
    m_pcoData->wNowADC = adc_working;
    return error;
}
//=================================================================================================
//=================================================================================================

bool Camera::_isCapsDesc(int caps)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    DWORD _dwCaps1;

    _dwCaps1 = m_pcoData->stcPcoDescription.dwGeneralCaps1;

    switch (caps)
    {
        case capsCDI:
            return !!(_dwCaps1 & GENERALCAPS1_CDI_MODE);

        case capsDoubleImage:
            return !!(m_pcoData->stcPcoDescription.wDoubleImageDESC);

        case capsRollingShutter:
            return _isCameraType(Edge);

        case capsGlobalShutter:
            //#define GENERALCAPS1_NO_GLOBAL_SHUTTER                 0x00080000
            //// Camera does not support global shutter
            return _isCameraType(Edge) &&
                   !(_dwCaps1 & GENERALCAPS1_NO_GLOBAL_SHUTTER);

        case capsGlobalResetShutter:
            //#define GENERALCAPS1_GLOBAL_RESET_MODE                 0x00100000
            //// Camera supports global reset rolling readout
            return _isCameraType(Edge) &&
                   !(_dwCaps1 & GENERALCAPS1_NO_GLOBAL_SHUTTER) &&
                   !!(_dwCaps1 & GENERALCAPS1_GLOBAL_RESET_MODE);

        case capsHWIO:
            return !!(_dwCaps1 & GENERALCAPS1_HW_IO_SIGNAL_DESCRIPTOR);

        case capsCamRam:
            return ((_dwCaps1 & GENERALCAPS1_NO_RECORDER) == 0);

        case capsMetadata:
            return !!(_dwCaps1 & GENERALCAPS1_METADATA);

            //----------------------------------------------------------------------------------------------------------
            // dwGeneralCapsDESC1;      // General capabilities:
            //		Bit 3: Timestamp ASCII only available (Timestamp mode 3
            // enabled) 		Bit 8: Timestamp not available
            // m_pcoData->stcPcoDescription.dwGeneralCapsDESC1 & BIT3 / BIT8

        case capsTimestamp3:
            return (!(_dwCaps1 & BIT8)) && (_dwCaps1 & BIT3);

        case capsTimestamp:
            return !(_dwCaps1 & BIT8);

        default:
            return false;
    }
}

//=================================================================================================
//=================================================================================================

void Camera::_pco_GetCDIMode(WORD &wCDIMode, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    // char *pcoFn;
    int pcoErr;
    err = 0;

    if (!_isCapsDesc(capsCDI))
    {
        wCDIMode = 0;
        err = 1;
        DEB_TRACE() << "WARNING / CDI mode is NOT ALLOWED!";
        return;
    }

    err = 0;

    wCDIMode = 0;
    DEB_ALWAYS() << NOT_IMPLEMENTED;
    pcoErr = -1;

    err |= PCO_CHECK_ERROR(pcoErr, "PCO_GetCDIMode");

    return;
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_SetCDIMode(WORD wCDIMode, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    // char *pcoFn;
    int pcoErr;
    err = 0;

    if (!_isCapsDesc(capsCDI))
    {
        err = 1;
        DEB_TRACE() << "WARNING / CDI mode is NOT ALLOWED!";
        return;
    }

    if (wCDIMode)
    {
        wCDIMode = 1;
        _pco_SetDoubleImageMode(0, err); // cdi and double image are exclusive
    }

    err = 0;

    DEB_ALWAYS() << NOT_IMPLEMENTED;
    pcoErr = -1;

    err |= PCO_CHECK_ERROR(pcoErr, "PCO_SetCDIMode");

    return;
}
//=================================================================================================
//=================================================================================================
void Camera::_pco_GetDoubleImageMode(WORD &wDoubleImage, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    // char *pcoFn;
    int pcoErr;
    err = 0;

    if (!_isCapsDesc(capsDoubleImage))
    {
        wDoubleImage = 0;
        err = 1;
        DEB_TRACE() << "WARNING / DoubleImage mode is NOT ALLOWED!";
        return;
    }

    err = 0;

    wDoubleImage = 0;
    DEB_ALWAYS() << NOT_IMPLEMENTED;
    pcoErr = -1;

    err |= PCO_CHECK_ERROR(pcoErr, "PCO_GetDoubleImageMode");

    return;
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_SetDoubleImageMode(WORD wDoubleImage, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    // char *pcoFn;
    int pcoErr;
    err = 0;

    if (!_isCapsDesc(capsDoubleImage))
    {
        err = 1;
        DEB_TRACE() << "WARNING / DoubleImage mode is NOT ALLOWED!";
        return;
    }

    if (wDoubleImage)
    {
        wDoubleImage = 1;
        _pco_SetCDIMode(0, err); // cdi and double image are exclusive
    }

    err = 0;

    DEB_ALWAYS() << NOT_IMPLEMENTED;
    pcoErr = -1;

    err |= PCO_CHECK_ERROR(pcoErr, "PCO_SetDoubleImageMode");

    return;
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_GetGeneralCapsDESC(DWORD &capsDesc1, int &err)
{
    err = 0;
    capsDesc1 = m_pcoData->stcPcoDescription.dwGeneralCaps1;

    return;
}

//=================================================================================================
//=================================================================================================

void Camera::_pco_GetSegmentInfo(int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    // char *pcoFn;
    int pcoErr;
    err = 0;

    err = 0;
    if (!_isCameraType(Dimax | Pco2k | Pco4k))
    {
        err = 1;
        return;
    }

    DEB_ALWAYS() << NOT_IMPLEMENTED;
    pcoErr = -1;
    err |= pcoErr;
    return;
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_CloseCamera(int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    err = 0;

#ifdef ME4
    if (grabber_me4)
    {
        delete grabber_me4;
        grabber_me4 = NULL;
    }
#endif

#ifdef CLHS
    if (grabber_clhs)
    {
        delete grabber_clhs;
        grabber_clhs = NULL;
    }
#endif

    if (camera)
    {
        err = camera->Close_Cam();
        PCO_CHECK_ERROR(err, "Close_Cam");

        delete camera;
        camera = NULL;
    }
    return;
}
//=================================================================================================
//=================================================================================================

bool Camera::_isCooledCamera()
{
    return !((m_pcoData->stcPcoDescription.sMinCoolSetDESC == 0) &&
             (m_pcoData->stcPcoDescription.sMaxCoolSetDESC == 0));
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_GetCoolingSetpointTemperature(int &val, int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    error = -1;

    DEB_ALWAYS() << NOT_IMPLEMENTED;

    val = 0;
    return;

}

//=================================================================================================
//=================================================================================================
void Camera::_pco_SetCoolingSetpointTemperature(int val, int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    error = -1;

    DEB_ALWAYS() << NOT_IMPLEMENTED;

    return;

}

//=================================================================================================
//=================================================================================================

void Camera::_pco_GetInfoString(int infotype, char *buf_in, int size_in,
                                int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    error = -1;
    *buf_in = 0;
    DEB_ALWAYS() << NOT_IMPLEMENTED;

}

//=================================================================================================
// only in linux - to merge
//=================================================================================================
void Camera::_pco_SetCameraToCurrentTime(int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    char buff[MSG4K + 1];
	const char *cmsg;

    // Sets the camera time to current system time.
    //
    // The date and time is updated automatically, as long as the camera is
    // supplied with power. Camera time is used for the timestamp and metadata.
    // When powering up the camera, then this command or PCO_SetDateTime should
    // be done once. return: Error code

    cmsg = "PCO_SetCameraToCurrentTime";
    err = camera->PCO_SetCameraToCurrentTime();
    PCO_CHECK_ERROR(err, cmsg);
    __sprintfSExt(buff, sizeof(buff), "%s err[0x%08x]\n", cmsg, err);
	m_log.append(buff);



    PCO_CHECK_ERROR(err, "PCO_SetCameraToCurrentTime");

    return;
}

//=================================================================================================
//=================================================================================================

void Camera::_pco_SetBinning(Bin binNew, Bin &binActual, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    WORD wBinHorz, wBinVert;
    Bin binOld;
    int err0;

    _pco_GetBinning(binOld, err);

    if (binOld == binNew)
    {
        binActual = binOld;
        err = 0;
        return;
    }
    wBinHorz = binNew.getX();
    wBinVert = binNew.getY();

    err = camera->PCO_SetBinning(wBinHorz, wBinVert);
    PCO_CHECK_ERROR(err, "PCO_SetBinning");
    PCO_THROW_OR_TRACE(err, "PCO_SetBinning");


    _pco_GetBinning(binActual, err0);
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_SetCamLinkSetImageParameters(int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    DEB_ALWAYS() << NOT_IMPLEMENTED;
    error = -1;
    return;
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_GetHWIOSignalAll(int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    if (!_isCapsDesc(capsHWIO))
    {
        error = -1;
        return;
    }

// linux
    int errorTot;
    errorTot = 0;
    char buff[MSG4K + 1];

    const char *msg __attribute__((unused));

    int iSignal, iSignalMax;

    if (!(_isCameraType(Dimax | Edge | Pco2k | Pco4k)))
    {
        errorTot = -1;
        return;
    }

    error = camera->PCO_GetHWIOSignalCount(&m_pcoData->wNrPcoHWIOSignal0);
    msg = "PCO_GetHWIOSignalCount";
    PCO_CHECK_ERROR(error, msg);


    errorTot |= error;

    iSignalMax = m_pcoData->wNrPcoHWIOSignal =
        (m_pcoData->wNrPcoHWIOSignal0 <= SIZEARR_stcPcoHWIOSignal)
            ? m_pcoData->wNrPcoHWIOSignal0
            : SIZEARR_stcPcoHWIOSignal;

	__sprintfSExt(buff, sizeof(buff), "========================\n"
		"PCO_GetHWIOSignalCount err[0x%08x]\n"
		"  nrSignals: sdk[%d] saved[%d]\n",
		error, 
		m_pcoData->wNrPcoHWIOSignal0, 
		iSignalMax);
	m_log.append(buff);

    WORD wEnabled, wType, wPolarity, wFilterSetting, wSelected;

    for (iSignal = 0; iSignal < iSignalMax; iSignal++)
    {
        int sizeName = SIZESTR_PcoHWIOSignal;
        char *ptrName = &(m_pcoData->sPcoHWIOSignalDesc[iSignal][0]);


		__sprintfSExt(buff, sizeof(buff), "=====  descriptor / signal[%d] size[%d] [BEGIN]\n",
			iSignal,
			m_pcoData->stcPcoHWIOSignalDesc[iSignal].wSize);
		m_log.append(buff);



        // telegram structure 4 signals * 24 char
        // memset(&m_pcoData->stcPcoHWIOSignalDesc[iSignal].szSignalName[0][0],0,24*4);

        error = camera->PCO_GetHWIOSignalDescriptor(
            iSignal, (SC2_Get_HW_IO_Signal_Descriptor_Response *)&m_pcoData
                         ->stcPcoHWIOSignalDesc[iSignal]);
        msg = "PCO_GetHWIOSignalDescriptor (struct)";
        PCO_CHECK_ERROR(error, msg);
        errorTot |= error;

		__sprintfSExt(buff, sizeof(buff), "--- PCO_GetHWIOSignalDescriptor (struct) signal[%d] err[0x%08x]\n",
			iSignal, error);
		m_log.append(buff);

        // DEB_ALWAYS()  << "---  signal" << DEB_VAR2(iSignal,
        // m_pcoData->stcPcoHWIOSignal[iSignal].wSize) ;

        // DWORD PCO_GetHWIOSignalDescriptor ( WORD SignalNum, char &outbuf, int
        // &size )

        // Gets the signal descriptor of the requested signal number as a string
        // for console output.
        *ptrName = 0;
        error =
            camera->PCO_GetHWIOSignalDescriptor(iSignal, ptrName, &sizeName);
        msg = "PCO_GetHWIOSignalDescriptor (name)";
        PCO_CHECK_ERROR(error, msg);
        errorTot |= error;

		__sprintfSExt(buff, sizeof(buff), "--- PCO_GetHWIOSignalDescriptor (name) signal[%d] err[0x%08x]\n"
			"  nameSize[%d] name[%s]\n",
			iSignal, error,
			sizeName, ptrName);
		m_log.append(buff);

        // m_pcoData->stcPcoHWIOSignal[iSignal].wSignalNum = iSignal;

        if (_isCameraType(Dimax | Edge))
        {
            error = camera->PCO_GetHWIOSignal(iSignal, &wEnabled, &wType,
                                              &wPolarity, &wFilterSetting,
                                              &wSelected);
            msg = "PCO_GetHWIOSignal";
            PCO_CHECK_ERROR(error, msg);

			__sprintfSExt(buff, sizeof(buff), "--- PCO_GetHWIOSignal signal[%d] err[0x%08x]\n"
				"  enabled[%d] type[%d] polarity[%d] filter[%d] selected[%d]\n",
				iSignal, error,
				wEnabled, wType, wPolarity,
				wFilterSetting, wSelected);
		m_log.append(buff);

            // PCO3(error, msg,PCO_GetHWIOSignal, m_handle, iSignal,
            // &m_pcoData->stcPcoHWIOSignal[i]);
            errorTot |= error;

            m_pcoData->stcPcoHWIOSignal[iSignal].wEnabled = wEnabled;
            m_pcoData->stcPcoHWIOSignal[iSignal].wType = wType;
            m_pcoData->stcPcoHWIOSignal[iSignal].wPolarity = wPolarity;
            m_pcoData->stcPcoHWIOSignal[iSignal].wFilterSetting =
                wFilterSetting;
            m_pcoData->stcPcoHWIOSignal[iSignal].wSelected = wSelected;

        } // if dimax

		__sprintfSExt(buff, sizeof(buff), "=====  descriptor / signal[%d] [END]\n",
			iSignal);
		m_log.append(buff);

    } // for iSignal

    error = errorTot;

}

//=================================================================================================
//=================================================================================================
void Camera::_pco_GetNumberOfImagesInSegment(WORD wSegment,
                                             DWORD &dwValidImageCnt,
                                             DWORD &dwMaxImageCnt, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    err = 0;

    if ((!_isCameraType(Dimax | Pco2k | Pco4k)) ||
        ((wSegment > PCO_MAXSEGMENTS) || (wSegment < 1)))
    {
        err = -1;
        dwValidImageCnt = dwMaxImageCnt = 0;
        return;
    }

    // DWORD PCO_GetNumberOfImagesInSegment ( WORD wSegment, DWORD *dwValid,
    // DWORD *dwMax )

    err = camera->PCO_GetNumberOfImagesInSegment(wSegment, &dwValidImageCnt,
                                                 &dwMaxImageCnt);
    PCO_CHECK_ERROR(err, "PCO_GetNumberOfImagesInSegment");


    if (err)
    {
        dwValidImageCnt = dwMaxImageCnt = 0;
    }

    return;
}
//=========================================================================================================
//=========================================================================================================
unsigned long Camera::_pco_GetNumberOfImagesInSegment_MaxCalc(int segmentPco)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    unsigned long framesMax;

    // if(_isCameraType(Edge)) {return LONG_MAX;}

    if (!_isCameraType(Dimax | Pco2k | Pco4k))
    {
        // DEB_WARNING() << "ERROR camera not valid for this function: " <<
        // _getCameraType();
        return 0;
    }

    if ((segmentPco < 1) || (segmentPco > PCO_MAXSEGMENTS))
    {
        DEB_ALWAYS() << "ERROR invalid segment: " << DEB_VAR1(segmentPco);
        return 0;
    }


    int error;
    DWORD dwValid, dwMax;

    _pco_GetNumberOfImagesInSegment((WORD)segmentPco, dwValid, dwMax, error);

    if (error)
    {
        framesMax = 0;
        DEB_ALWAYS() << "ERROR _pco_GetNumberOfImagesInSegment()";
    }
    else
    {
        framesMax = dwMax;
    }


    return framesMax;
}
//=================================================================================================
//=================================================================================================
void Camera::_pco_GetTemperatureInfo(int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    char buff[MSG4K + 1];


    sCoolingSetpoint = sTempCcd = sTempCam = sTempPS = -273;
    
    error = camera->PCO_GetTemperature(&sTempCcd, &sTempCam, &sTempPS);
    PCO_CHECK_ERROR(error, "PCO_GetTemperature");

	__sprintfSExt(buff, sizeof(buff), "PCO_GetTemperature err[0x%08x]\n",
		"  Temp (C): ccd[%d] cam[%d] PS[%d]\n",
		error, sTempCcd, sTempCam, sTempPS);
	m_log.append(buff);



    error = camera->PCO_GetCoolingSetpointTemperature(&sCoolingSetpoint);
    PCO_CHECK_ERROR(error, "PCO_GetCoolingSetpointTemperature");

	__sprintfSExt(buff, sizeof(buff), "PCO_GetCoolingSetpointTemperature err[0x%08x]\n",
		"  Temp (C): cooling setpoint[%d]\n",
		error, sCoolingSetpoint);
	m_log.append(buff);

    DEB_ALWAYS() 	<< "\nTemperature (C):" 
					<< "\n               CCD  " << DEB_VAR1(sTempCcd)
					<< "\n            Camera   " << DEB_VAR1(sTempCam) 
					<< "\n      Power Supply   " << DEB_VAR1(sTempPS)
					<< "\n   Cooling Setpoint  " << DEB_VAR1(sCoolingSetpoint);

    m_pcoData->temperature.sCcd = sTempCcd;
    m_pcoData->temperature.sCam = sTempCam;
    m_pcoData->temperature.sPower = sTempPS;

    return;
}
//=================================================================================================
//=================================================================================================
void Camera::_pco_GetTemperatureInfo(char *ptr, char *ptrMax, int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    error = -1;
    ptr += __sprintfSExt(ptr, ptrMax - ptr, NOT_IMPLEMENTED);

    return;
}
//=================================================================================================
//=================================================================================================
void Camera::_pco_GetCameraType(int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    //linux
    char buff[MSG4K + 1];

    const char *ptr;
    int errTot = 0;

    //------------------ GigE
    //------------------ PCO_GetCameraType ---> camtype, serialnumber, iftype
    //------------------ PCO_GetInfo idem

    WORD camtype;
    WORD iftype = 0;
    DWORD serialnumber;

    m_pcoData->frames_per_buffer = 1; // for PCO DIMAX

    DEB_ALWAYS() << fnId;

    m_pcoData->ipField[0] = m_pcoData->ipField[1] = m_pcoData->ipField[2] =
        m_pcoData->ipField[3] = 0;

    int err1;
    err1=error = camera->PCO_GetCameraType(&camtype, &serialnumber, &iftype);
    PCO_CHECK_ERROR(error, "PCO_GetCameraType");

    m_pcoData->wCamType = camtype;
    m_pcoData->wCamSubType = 0;
    m_pcoData->dwSerialNumber =
        serialnumber;
    m_pcoData->wIfType = iftype;

    ptr = _xlatPcoCode2Str(camtype, ModelType, error);
    strcpy_s(m_pcoData->model, sizeof(m_pcoData->model), ptr);
    errTot |= error;
    if(error)
    {
        DEB_ALWAYS() << "WARNING ERROR in xlatPcoCode2Str " << DEB_VAR1(camtype);
    }

    ptr = _xlatPcoCode2Str(iftype, InterfaceType, error);
    strcpy_s(m_pcoData->iface, INTERFACE_TYPE_SIZE, ptr);
    errTot |= error;

    if(error)
    {
        DEB_ALWAYS() << "WARNING ERROR in xlatPcoCode2Str " << DEB_VAR1(iftype);
    }

    __sprintfSExt(m_pcoData->camera_name, sizeof(m_pcoData->camera_name),
                  "%s (IF %s) (SN %u)", m_pcoData->model, m_pcoData->iface,
                  m_pcoData->dwSerialNumber);


	__sprintfSExt(buff, sizeof(buff), "PCO_GetCameraType err[0x%08x]\n"
			"  camType[%d][0x%04x][%s]\n"
			"  s/n[%d]\n"
			"  interface[%d][%s]\n",
			err1, 
			camtype, camtype, m_pcoData->model,
			serialnumber,
			iftype, m_pcoData->iface);
	m_log.append(buff);



/// #define INFO_STRING_CAMERA              1   // Camera name
/// #define INFO_STRING_SENSOR              2   // Sensor name
/// #define INFO_STRING_PCO_MATERIALNUMBER  3   // get PCO material number
/// #define INFO_STRING_BUILD               4   // Build number and date
/// #define INFO_STRING_PCO_INCLUDE         5   // PCO_Include rev used for building

    
	m_pcoData->infoCamName[0] = 0;
    error = camera->PCO_GetInfo(1, &m_pcoData->infoCamName,
                                sizeof(m_pcoData->infoCamName) - 1);
    PCO_CHECK_ERROR(error, "PCO_GetInfo(1)");
	if(!error)
	{
		__sprintfSExt(buff, sizeof(buff), "PCO_GetInfo(1) err[0x%08x]\n"
				"  infoCamName[%s]\n",
				error, 
				m_pcoData->infoCamName);
		m_log.append(buff);
	}
	
	m_pcoData->infoSensorName[0] = 0;
    error = camera->PCO_GetInfo(2, &m_pcoData->infoSensorName,
                                sizeof(m_pcoData->infoSensorName) - 1);
    PCO_CHECK_ERROR(error, "PCO_GetInfo(2)");
	if(!error)
	{
		__sprintfSExt(buff, sizeof(buff), "PCO_GetInfo(2) err[0x%08x]\n"
				"  infoSensorName[%s]\n",
				error, 
				m_pcoData->infoSensorName);
		m_log.append(buff);
	}



	m_pcoData->infoMaterialNr[0] = 0;
    error = camera->PCO_GetInfo(3, &m_pcoData->infoMaterialNr,
                                sizeof(m_pcoData->infoMaterialNr) - 1);
    PCO_CHECK_ERROR(error, "PCO_GetInfo(3)");
	if(!error)
	{
		__sprintfSExt(buff, sizeof(buff), "PCO_GetInfo(3) err[0x%08x]\n"
				"  infoMaterialNr[%s]\n",
				error, 
				m_pcoData->infoMaterialNr);
		m_log.append(buff);
	}


	m_pcoData->infoBuildNr[0] = 0;
    error = camera->PCO_GetInfo(4, &m_pcoData->infoBuildNr,
                                sizeof(m_pcoData->infoBuildNr) - 1);
    PCO_CHECK_ERROR(error, "PCO_GetInfo(4)");
	if(!error)
	{

		__sprintfSExt(buff, sizeof(buff), "PCO_GetInfo(4) err[0x%08x]\n"
				"  infoBuildNr[%s]\n",
				error, 
				m_pcoData->infoBuildNr);
		m_log.append(buff);
	}


	m_pcoData->infoIncludeRev[0] = 0;
    error = camera->PCO_GetInfo(5, &m_pcoData->infoIncludeRev,
                                sizeof(m_pcoData->infoIncludeRev) - 1);
    PCO_CHECK_ERROR(error, "PCO_GetInfo(5)");
	if(!error)
	{
		__sprintfSExt(buff, sizeof(buff), "PCO_GetInfo(5) err[0x%08x]\n"
				"  infoIncludeRev[%s]\n",
				error, 
				m_pcoData->infoIncludeRev);
		m_log.append(buff);
	}



    DEB_ALWAYS() << "\n   " 
 
		
				<< "GetCameraType: " << DEB_VAR1(camtype) << " = " << DEB_HEX(camtype) << ", "
										<<DEB_VAR1(m_pcoData->model) << "\n   "
				<< "               " << DEB_VAR1(serialnumber) << "\n   "
				<< "               " << DEB_VAR2(iftype, m_pcoData->iface) << "\n   "
				<< "               " << DEB_VAR1(m_pcoData->camera_name) << "\n   "
                << "   GetInfo(1): " << DEB_VAR1(m_pcoData->infoCamName) << "\n   "
                << "   GetInfo(2): " << DEB_VAR1(m_pcoData->infoSensorName) << "\n   "
                << "   GetInfo(3): " << DEB_VAR1(m_pcoData->infoMaterialNr) << "\n   "
                << "   GetInfo(4): " << DEB_VAR1(m_pcoData->infoBuildNr) << "\n   "
                << "   GetInfo(5): " << DEB_VAR1(m_pcoData->infoIncludeRev)
                ;


    return;
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_SetMetaDataMode(WORD wMetaDataMode, int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    if (!_isCameraType(Dimax))
    {
        error = 0;
        return;
    }
    const char *msg;

    m_pcoData->wMetaDataSize = m_pcoData->wMetaDataVersion = 0;
    if (_isCameraType(Dimax))
    {
        m_pcoData->wMetaDataMode = wMetaDataMode;

        error = camera->PCO_SetMetadataMode(wMetaDataMode,
                                            &m_pcoData->wMetaDataSize,
                                            &m_pcoData->wMetaDataVersion);
        msg = "PCO_SetMetadataMode";
        PCO_CHECK_ERROR(error, msg);
        if (error)
            return;
    }

    return;
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_SetPixelRate(int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    error = 0;
    const char *msg;
    DWORD _dwPixelRate, _dwPixelRateOld, _dwPixelRateReq;
    DWORD _dwPixelRateMax;

    DWORD _dwPixelRateNext;

    if (_isCameraType(Edge))
    {
        _pco_GetPixelRate(m_pcoData->dwPixelRate, _dwPixelRateNext, error);
        PCO_THROW_OR_TRACE(error, "_pco_GetPixelRate");

        _dwPixelRateOld = m_pcoData->dwPixelRate;
        _dwPixelRateReq = m_pcoData->dwPixelRateRequested;
        // DEB_ALWAYS() << "PIXEL rate (actual/req): " <<
        // DEB_VAR2(_dwPixelRateOld, _dwPixelRateReq) ;

        if (_isValid_pixelRate(_dwPixelRateReq) &&
            (_dwPixelRateOld != _dwPixelRateReq))
        {
            error = camera->PCO_SetPixelRate(_dwPixelRateReq);
            msg = "PCO_SetPixelRate";
            PCO_CHECK_ERROR(error, msg);
            PCO_THROW_OR_TRACE(error, msg);

            _pco_GetPixelRate(m_pcoData->dwPixelRate, _dwPixelRateNext, error);
            PCO_THROW_OR_TRACE(error, "_pco_GetPixelRate");

            _dwPixelRate = m_pcoData->dwPixelRate;
            // DEB_ALWAYS() << "PIXEL rate SET (old/new): "  <<
            // DEB_VAR2(_dwPixelRateOld, _dwPixelRate) ;

        }
        m_pcoData->dwPixelRateRequested = 0;
        return;
    }

    if (_isCameraType(Pco2k | Pco4k))
    {
        _pco_GetPixelRate(m_pcoData->dwPixelRate, _dwPixelRateNext, error);
        PCO_THROW_OR_TRACE(error, "_pco_GetPixelRate");

        _dwPixelRateOld = m_pcoData->dwPixelRate;
        _dwPixelRateMax = m_pcoData->dwPixelRateMax;
        _dwPixelRateReq = m_pcoData->dwPixelRateRequested;

        DEB_TRACE() << "PIXEL rate (requested/actual/max): "
                    << DEB_VAR3(_dwPixelRateReq, _dwPixelRateOld,
                                _dwPixelRateMax);

        if (_isValid_pixelRate(_dwPixelRateReq) &&
            (_dwPixelRateOld != _dwPixelRateReq))
        {
            error = camera->PCO_SetPixelRate(_dwPixelRateReq);
            msg = "PCO_SetPixelRate";
            PCO_CHECK_ERROR(error, msg);
            PCO_THROW_OR_TRACE(error, msg);

            _pco_GetPixelRate(m_pcoData->dwPixelRate, _dwPixelRateNext, error);
            PCO_THROW_OR_TRACE(error, "_pco_GetPixelRate");

            _dwPixelRate = m_pcoData->dwPixelRate;
            DEB_TRACE() << "PIXEL rate SET (old/new): "
                        << DEB_VAR2(_dwPixelRateOld, _dwPixelRate);

        }
        return;
    }
    return;
}
//=================================================================================================
//=================================================================================================
void Camera::_pco_GetPixelRate(DWORD &pixRateActual, DWORD &pixRateNext,
                               int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    const char *msg;

    DWORD _dwPixRate;

    err = camera->PCO_GetPixelRate(&_dwPixRate);
    msg = "PCO_GetPixelRate";
    PCO_CHECK_ERROR(err, msg);
    if (err)
        _dwPixRate = 0;

    m_pcoData->dwPixelRate = pixRateActual = _dwPixRate;

    pixRateNext = ((m_pcoData->dwPixelRateRequested != 0) &&
                   (pixRateActual != m_pcoData->dwPixelRateRequested))
                      ? m_pcoData->dwPixelRateRequested
                      : pixRateActual;

    DEB_ALWAYS() << "\n   " << DEB_VAR1(pixRateActual) << "\n   "
                 << DEB_VAR1(pixRateNext);

}
//=================================================================================================
// ----------------------------------------- storage mode (recorder + sequence)
// current storage mode
//
// case RecSeq
// case RecRing
// - 0x0000 = [recorder] mode
//		. images are recorded and stored within the internal camera
// memory (camRAM)
//      . Live View transfers the most recent image to the PC (for viewing /
//      monitoring) . indexed or total image readout after the recording has
//      been stopped
//
// case Fifo
// - 0x0001 = [FIFO buffer] mode
//      . all images taken are transferred to the PC in chronological order
//      . camera memory (camRAM) is used as huge FIFO buffer to bypass short
//      bottlenecks in data transmission . if buffer overflows, the oldest
//      images are overwritten . if Set Recorder = [stop] is sent, recording is
//      stopped and the transfer of the current image to the PC is finished. .
//      Images not read are stored within the segment and can be read with the
//      Read Image From Segment command.
//
// current recorder submode:
//
// case RecSeq
// - 0x0000 = [sequence]
//      . recording is stopped when the allocated buffer is full
//
// case RecRing
// - 0x0001 = [ring buffer].
//      . camera records continuously into ring buffer
//      . if the allocated buffer overflows, the oldest images are overwritten
//      . recording is stopped by software or disabling acquire signal (<acq
//      enbl>)
//
// for the case of ExtTrigSingle (dimax) we use RecRing
//    case RecRing
//       StorageMode 0 - record mode
//       RecorderSubmode 1 - ring buffer
//  Triggermode 0 - auto
//  Acquiremode 0 - auto / ignored
//=================================================================================================
//=================================================================================================
// sets storage mode and subrecord mode
//    PCO_SetStorageMode
//    PCO_SetRecorderSubmode
//=================================================================================================
void Camera::_pco_SetStorageMode_SetRecorderSubmode(enumPcoStorageMode mode,
                                                    int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    const char *msg, *sMode;

    sMode = "invalid";
    switch (mode)
    {
        case RecSeq:
            m_pcoData->storage_mode = 0;
            m_pcoData->recorder_submode = 0;
            sMode = "RecSeq";
            break;
        case RecRing:
            m_pcoData->storage_mode = 0;
            m_pcoData->recorder_submode = 1;
            sMode = "RecRing";
            break;
        case Fifo:
            m_pcoData->storage_mode = 1;
            m_pcoData->recorder_submode = 0;
            sMode = "Fifo";
            break;
        default:
            throw LIMA_HW_EXC(Error, "FATAL - invalid storage mode!");
    }
    DEB_TRACE() << "\n>>> storage/recorder mode: " << DEB_VAR2(sMode, mode);

    m_pcoData->traceAcq.sPcoStorageRecorderMode = sMode;
    m_pcoData->traceAcq.iPcoStorageMode = m_pcoData->storage_mode;
    m_pcoData->traceAcq.iPcoRecorderSubmode = m_pcoData->recorder_submode;

    error = camera->PCO_SetStorageMode(m_pcoData->storage_mode);
    msg = "PCO_SetStorageMode";
    PCO_CHECK_ERROR(error, msg);
    // PCO_THROW_OR_TRACE(error, msg) ;

    error = camera->PCO_SetRecorderSubmode(m_pcoData->recorder_submode);
    msg = "PCO_SetRecorderSubmode";
    PCO_CHECK_ERROR(error, msg);
    // PCO_THROW_OR_TRACE(error, msg) ;
    return;
}

//=================================================================================================
//=================================================================================================
int Camera::_pco_GetStorageMode_GetRecorderSubmode()
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    const char *msg;

    WORD wStorageMode, wRecSubmode;
    int error;

    error = camera->PCO_GetStorageMode(&wStorageMode);
    msg = "PCO_GetStorageMode";
    PCO_CHECK_ERROR(error, msg);
    if (error)
    {
        PCO_THROW_OR_TRACE(error, msg);
    }
    error = camera->PCO_GetRecorderSubmode(&wRecSubmode);
    msg = "PCO_getRecorderSubmode";
    PCO_CHECK_ERROR(error, msg);
    if (error)
    {
        PCO_THROW_OR_TRACE(error, msg);
    }
    m_pcoData->storage_mode = wStorageMode;
    m_pcoData->recorder_submode = wRecSubmode;

    if ((wStorageMode == 0) && (wRecSubmode == 0))
    {
        m_pcoData->storage_str = "RecSeq";
        return RecSeq;
    }
    if ((wStorageMode == 0) && (wRecSubmode == 1))
    {
        m_pcoData->storage_str = "RecRing";
        return RecRing;
    }
    if ((wStorageMode == 1) && (wRecSubmode == 0))
    {
        m_pcoData->storage_str = "Fifo";
        return Fifo;
    }

    m_pcoData->storage_str = "INVALID";
    return RecInvalid;
}
//=================================================================================================
//=================================================================================================

/******************************************************************************************
typedef struct
{
  DWORD  FrameTime_ns;                 // Frametime replaces COC_Runtime
  DWORD  FrameTime_s;

  DWORD  ExposureTime_ns;
  DWORD  ExposureTime_s;               // 5

  DWORD  TriggerSystemDelay_ns;        // System internal min. trigger delay

  DWORD  TriggerSystemJitter_ns;       // Max. possible trigger jitter -0/+ ...
ns

  DWORD  TriggerDelay_ns;              // Resulting trigger delay = system delay
  DWORD  TriggerDelay_s;               // + delay of SetDelayExposureTime ... //
9

} PCO_ImageTiming;
******************************************************************************************/

int Camera::_pco_GetImageTiming(double &frameTime, double &expTime,
                                double &sysDelay, double &sysJitter,
                                double &trigDelay)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    int error;
    const char *msg;

    SC2_Get_Image_Timing_Response pstrImageTiming;
    error = camera->PCO_GetImageTiming(&pstrImageTiming);
    msg = "PCO_GetImageTiming";
    PCO_CHECK_ERROR(error, msg);

    frameTime =
        (pstrImageTiming.FrameTime_ns * NANO) + pstrImageTiming.FrameTime_s;
    expTime = (pstrImageTiming.ExposureTime_ns * NANO) +
              pstrImageTiming.ExposureTime_s;
    sysDelay = (pstrImageTiming.TriggerSystemDelay_ns * NANO);
    sysJitter = (pstrImageTiming.TriggerSystemJitter_ns * NANO);
    trigDelay = (pstrImageTiming.TriggerDelay_ns * NANO) +
                pstrImageTiming.TriggerDelay_s;

    return error;
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_SetTriggerMode_SetAcquireMode(int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    char buff[MSG4K + 1];

	const char *cmsg;
    const char *msg;
    const char *sLimaTriggerMode, *sPcoTriggerMode, *sPcoAcqMode; 

    WORD wPcoTriggerMode, wPcoAcqMode;

    lima::TrigMode limaTrigMode;
    m_sync->getTrigMode(limaTrigMode);
    m_sync->xlatLimaTrigMode2Pco(limaTrigMode, wPcoTriggerMode, wPcoAcqMode,
                                &sLimaTriggerMode, &sPcoTriggerMode, &sPcoAcqMode,
                                m_pcoData->bExtTrigEnabled, error);


    cmsg = "_pco_SetTriggerMode_SetAcquireMode";
    __sprintfSExt(buff, sizeof(buff), "%s err[0x%08x]\n"
		"  limaTrigMode[%d][%s]\n"
        "   pcoTrigMode[%d][%s]\n"
        "    pcoAcqMode[%d][%s]\n",
		cmsg, error, 
        limaTrigMode, sLimaTriggerMode,
        wPcoTriggerMode, sPcoTriggerMode,
        wPcoAcqMode, sPcoAcqMode);
	m_log.append(buff);

    
    //------------------------------------------------- triggering mode
    error = camera->PCO_SetTriggerMode(wPcoTriggerMode);
    msg = "PCO_SetTriggerMode";
    PCO_CHECK_ERROR(error, msg);
    if (error)
    {
        DEB_ALWAYS() << "ERROR PCO_SetTriggerMode" << DEB_VAR1(wPcoTriggerMode);
        return;
    }

    //------------------------------------- acquire mode : ignore or not
    // ext. signal

    error = camera->PCO_SetAcquireMode(wPcoAcqMode);
    msg = "PCO_SetAcquireMode";
    PCO_CHECK_ERROR(error, msg);
    if (error)
    {
        DEB_ALWAYS() << "ERROR PCO_SetAcquireMode" << DEB_VAR1(wPcoAcqMode);
        return;
    }

    return;
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_SetHWIOSignal(int sigNum, int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    const char *msg;
    error = 0;

    if (!_isCapsDesc(capsHWIO) || (sigNum < 0) ||
        (sigNum >= m_pcoData->wNrPcoHWIOSignal))
    {
        error = -1;
        return;
    }

    // error=camera->PCO_SetHWIOSignal(wSignalNum, wEnabled, wType, wPolarity,
    // wFilterSetting, wSelected);
    error = camera->PCO_SetHWIOSignal(
        sigNum, m_pcoData->stcPcoHWIOSignal[sigNum].wEnabled,
        m_pcoData->stcPcoHWIOSignal[sigNum].wType,
        m_pcoData->stcPcoHWIOSignal[sigNum].wPolarity,
        m_pcoData->stcPcoHWIOSignal[sigNum].wFilterSetting,
        m_pcoData->stcPcoHWIOSignal[sigNum].wSelected);

    msg = "PCO_SetHWIOSignal";
    PCO_CHECK_ERROR(error, msg);
}
//=================================================================================================
//=================================================================================================
// PCO_CL_DATAFORMAT_5x12   0x07     //extract data to 12bit
// PCO_CL_DATAFORMAT_5x12L  0x09     //extract data to 16Bit
// PCO_CL_DATAFORMAT_5x12R  0x0A     //without extract

// transfer dataformat must be changed depending on pixelrate and horizontal
// resolution

// SC2_SDKAddendum.h:#define PCO_CL_DATAFORMAT_5x12   0x07     //extract data to
// 12bit SC2_SDKAddendum.h:#define PCO_CL_DATAFORMAT_5x12L  0x09     //extract
// data to 16Bit SC2_SDKAddendum.h:#define PCO_CL_DATAFORMAT_5x12R  0x0A
// //without extract

// DWORD   baudrate;         // serial baudrate: 9600, 19200, 38400, 56400,
// 115200 DWORD   ClockFrequency;   // Pixelclock in Hz:
// 40000000,66000000,80000000 DWORD   CCline;           // Usage of CameraLink
// CC1-CC4 lines, use value returned by Get DWORD   DataFormat;       // see
// defines below, use value returned by Get DWORD   Transmit;         // single
// or continuous transmitting images, 0-single, 1-continuous

void Camera::_pco_SetTransferParameter_SetActiveLookupTable(int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    if (!_isInterfaceType(ifCameralinkAll))
    {
        DEB_ALWAYS() << "PCO_SetTransferParameter (clTransferParam) NOT DONE!";
        return;
    }

    const char *info = "[none]";
    bool doLut = false;

    WORD width, height, wXResMax, wYResMax;
    _pco_GetSizes(&width, &height, &wXResMax, &wYResMax, error);
    WORD wXResActual = width;

// linux prep

    struct stcPcoData _pcoData;
    char *pbla = mybla;
    const char *msg;
    DWORD pixelrate, pixRateNext;
    WORD actlut;
    // WORD lutparam;
    int pcoBuffNr = 10;

    WORD lut;

    _pco_GetPixelRate(pixelrate, pixRateNext, error);
    error = camera->PCO_GetPixelRate(&pixelrate);
    msg = "PCO_GetPixelRate";
    PCO_CHECK_ERROR(error, msg);

    error = camera->PCO_GetTransferParameter(&clpar, sizeof(clpar));
    if (error != PCO_NOERROR)
    {
        DEB_ALWAYS() << "ERROR - PCO_GetTransferParameter " << DEB_VAR1(error);
    }

    m_pcoData->clTransferParam.baudrate = clpar.baudrate;
    m_pcoData->clTransferParam.ClockFrequency = clpar.ClockFrequency;
    m_pcoData->clTransferParam.CCline = clpar.CCline;
    m_pcoData->clTransferParam.DataFormat = clpar.DataFormat;
    m_pcoData->clTransferParam.Transmit = clpar.Transmit;



    //---------------------------------------------------------------------------
    // set of parameters
    //---------------------------------------------------------------------------
    if (_isCameraType(Dimax))
    {
        // m_pcoData->clTransferParam.Transmit = 1;
        //_pcoData.clTransferParam.Transmit =
        // m_pcoData->clTransferParam.Transmit;
        m_pcoData->clTransferParam.DataFormat = PCO_CL_DATAFORMAT_2x12; //=2
        info = "DIMAX / 2x12 / LUT notValid";
        doLut = false;
    }
    else if (_isCameraType(EdgeGL))
    {
        m_pcoData->clTransferParam.Transmit = 1;
        m_pcoData->clTransferParam.DataFormat =
            PCO_CL_DATAFORMAT_5x12 | SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER;
        // SCCMOS_FORMAT_TOP_BOTTOM;
        m_pcoData->wLUT_Identifier = 0; // Switch LUT->off
        doLut = true;
        info = "EDGE GL / 5x12 topCenter bottomCenter / LUT off";
    }
    else if (_isCameraType(EdgeHS))
    {
        m_pcoData->clTransferParam.Transmit = 1;
        m_pcoData->clTransferParam.DataFormat =
            PCO_CL_DATAFORMAT_5x16 | SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER;
        m_pcoData->wLUT_Identifier = PCO_EDGE_LUT_NONE; // Switch LUT->off
        doLut = true;
        info = "EDGE HS / 5x16 topCenter bottomCenter / LUT off";
    }
    else if (_isCameraType(EdgeRolling))
    {
        m_pcoData->clTransferParam.Transmit = 1;

        if (m_pcoData->dwPixelRate <= PCO_EDGE_PIXEL_RATE_LOW)
        {
            m_pcoData->clTransferParam.DataFormat =
                PCO_CL_DATAFORMAT_5x16 | SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER;
            m_pcoData->wLUT_Identifier = PCO_EDGE_LUT_NONE; // Switch LUT->off
            info = "EDGE Rolling / 5x16 topCenter bottomCenter / LUT off";
        }
        else if (((m_pcoData->dwPixelRate >= PCO_EDGE_PIXEL_RATE_HIGH) &
                  (wXResActual > PCO_EDGE_WIDTH_HIGH)))
        {
            m_pcoData->clTransferParam.DataFormat =
                PCO_CL_DATAFORMAT_5x12L |
                SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER;
            m_pcoData->wLUT_Identifier = PCO_EDGE_LUT_SQRT; // Switch LUT->sqrt
            info = "EDGE Rolling / 5x12L topCenter bottomCenter / LUT SQRT";
        }
        else
        {
            m_pcoData->clTransferParam.DataFormat =
                PCO_CL_DATAFORMAT_5x16 | SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER;
            m_pcoData->wLUT_Identifier = PCO_EDGE_LUT_NONE; // Switch LUT->off
            info = "EDGE Rolling / 5x16 topCenter bottomCenter / LUT off";
        }
        doLut = true;
    }
	DEB_ALWAYS() << "ClTransferParameterSettings " << DEB_VAR2(info, doLut) ;

    //---------------------------------------------------------------------------

// linux pcoSet


    // m_pcoData->clTransferParam.baudrate = PCO_CL_BAUDRATE_115K2;

    clpar.DataFormat = m_pcoData->clTransferParam.DataFormat;
    m_pcoData->sClTransferParameterSettings = info;
    lut = m_pcoData->wLUT_Identifier;

    pbla +=
        __sprintfSExt(pbla, myblamax - pbla, " / width[%d][%d] height[%d][%d]",
                      width, wXResMax, height, wYResMax);

    DEB_TRACE() << mybla;
    // mylog->writelog(INFO_M, "%s", bla);
    mylog->writelog(INFO_M, mybla);

    // See http://wikiserv.esrf.fr/bliss/index.php/PCO_-_NOTES_on_PCO_EDGE_-_lookup_table_LUT_1612_/_square_root_LUT_SQRT_compression
    if (doLut)
    {
        actlut = lut;
		DEB_ALWAYS() << "camera->PCO_SetLut() " << DEB_VAR1(actlut) ;
        error = camera->PCO_SetLut(actlut, 0);
        msg = "PCO_SetLut";
        PCO_CHECK_ERROR(error, msg);
    }
    else
    {
		DEB_ALWAYS() << "camera->PCO_SetLut() BYPASSED!!!" ;
	}

	DEB_ALWAYS() << "camera->PCO_SetTransferParameter()" ;
    error = camera->PCO_SetTransferParameter(&clpar, sizeof(clpar));
    if (error != PCO_NOERROR)
    {
        DEB_ALWAYS() << "ERROR - PCO_SetTransferParameter " << DEB_VAR1(error);
    }

	DEB_ALWAYS() << "camera->PCO_ArmCamera()" ;
    error = camera->PCO_ArmCamera();
    msg = "PCO_ArmCamera()";
    PCO_CHECK_ERROR(error, msg);
    if (error != PCO_NOERROR)
    {
        DEB_ALWAYS() << "ERROR - PCO_ArmCamera() " << DEB_VAR1(error);
    }

#ifdef ME4
	if((grabber_me4 == NULL))
	{
        THROW_FATAL(Hardware, Error) << "any grabber is opened";
	}
#endif

#ifdef CLHS
	if((grabber_clhs == NULL))
	{
        THROW_FATAL(Hardware, Error) << "any grabber is opened";
	}
#endif


#ifdef ME4
	if(grabber_me4)
	{	
		DEB_ALWAYS() << "grabber_me4->Set_DataFormat() " << DEB_VAR1(clpar.DataFormat);
		error = grabber_me4->Set_DataFormat(clpar.DataFormat);
        msg = "Set_DataFormat";
        PCO_CHECK_ERROR(error, msg);
        if (error != PCO_NOERROR)
        {
            DEB_ALWAYS() << "ERROR - Set_DataFormat " << DEB_VAR1(error);
        }

		DEB_ALWAYS() << "grabber_me4->Set_Grabber_Size() " << DEB_VAR2(width, height);
		error = grabber_me4->Set_Grabber_Size(width, height);
    	msg = "Set_Grabber_Size";
        PCO_CHECK_ERROR(error, msg);

		DEB_ALWAYS() << "grabber_me4->PostArm(1)" ;
		error = grabber_me4->PostArm(1);
        msg = "grabber->PostArm(1)";
        PCO_CHECK_ERROR(error, msg);

		DEB_ALWAYS() << "grabber_me4->Allocate_Framebuffer() " << DEB_VAR1(pcoBuffNr);
		error = grabber_me4->Allocate_Framebuffer(pcoBuffNr);
		msg = "Allocate_Framebuffer";
        PCO_CHECK_ERROR(error, msg);
        error = 0;
	}
#endif

#ifdef CLHS
	if(grabber_clhs)
	{
		DEB_ALWAYS() << "grabber_clhs->Set_DataFormat() " << DEB_VAR1(clpar.DataFormat);
		error = grabber_clhs->Set_DataFormat(clpar.DataFormat);
        msg = "Set_DataFormat";
        PCO_CHECK_ERROR(error, msg);
        if (error != PCO_NOERROR)
        {
            DEB_ALWAYS() << "ERROR - Set_DataFormat " << DEB_VAR1(error);
        }

		DEB_ALWAYS() << "grabber_clhs->Set_Grabber_Size() " << DEB_VAR2(width, height);
		error = grabber_clhs->Set_Grabber_Size(width, height);
        msg = "Set_Grabber_Size";
        PCO_CHECK_ERROR(error, msg);

		DEB_ALWAYS() << "grabber_clhs->PostArm(1)" ;
		error = grabber_clhs->PostArm(1);
        msg = "grabber->PostArm(1)";
        PCO_CHECK_ERROR(error, msg);

		DEB_ALWAYS() << "grabber_clhs->Allocate_Framebuffer() " << DEB_VAR1(pcoBuffNr);
		error = grabber_clhs->Allocate_Framebuffer(pcoBuffNr);
		msg = "Allocate_Framebuffer";
        PCO_CHECK_ERROR(error, msg);
        error = 0;
	}
#endif


    return;
}
//=================================================================================================
//=================================================================================================
#define LEN_ERRSTR 127

void Camera::_pco_GetActiveRamSegment(WORD &wActSeg, int &err)
{
    DEB_MEMBER_FUNCT();

    char errstr[LEN_ERRSTR + 1];

    // if((m_pcoData->stcPcoDescription.dwGeneralCaps1&GENERALCAPS1_NO_RECORDER)==0)
    if (_isCapsDesc(capsCamRam))
    {
        // DWORD PCO_GetActiveRamSegment ( WORD & wActSeg )
        err = camera->PCO_GetActiveRamSegment(&wActSeg);

        if (err != PCO_NOERROR)
        {
            PCO_GetErrorText(err, errstr, LEN_ERRSTR);
            DEB_ALWAYS() << "ERROR: " << DEB_VAR2(err, errstr);
            wActSeg = 1;
        }
    }
    else
        wActSeg = 1;

    m_pcoData->wActiveRamSegment = wActSeg;

    return;
}
//=================================================================================================
//=================================================================================================

/**************************************************************************************************
        name[Acquire Enable] idx[0] num[0]
        -def:     def[0x1] type[0xf] pol[0x3] filt[0x7]
        -sig:    enab[0x1] type[0x1] pol[0x1] filt[0x1] sel[0x0]

        name[Exposure Trigger] idx[1] num[1]
        -def:     def[0x1] type[0xf] pol[0xc] filt[0x7]
        -sig:    enab[0x1] type[0x1] pol[0x4] filt[0x1] sel[0x0]

        name[Status Expos] idx[2] num[2]
        -def:     def[0x3] type[0x1] pol[0x3] filt[0x0]
        -sig:    enab[0x1] type[0x1] pol[0x1] filt[0x0] sel[0x0]

        name[Ready Status] idx[3] num[3]
        -def:     def[0x3] type[0x1] pol[0x3] filt[0x0]
        -sig:    enab[0x1] type[0x1] pol[0x1] filt[0x0] sel[0x0]

        name[Set Ready] idx[4] num[4]
        -def:     def[0x1] type[0xf] pol[0x3] filt[0x7]
        -sig:    enab[0x1] type[0x1] pol[0x1] filt[0x1] sel[0x0]
**************************************************************************************************/

void Camera::_pco_initHWIOSignal(int mode, WORD wVal, int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    int _err, sigNum;
    error = 0;
    char *name;
    WORD wSelected;

    if (!_isCapsDesc(capsHWIO))
    {
        DEB_WARNING()
            << "WARNING - camera does not supoort HWIO signals - IGNORED";
        error = -1;
        return;
    }

    _pco_GetHWIOSignalAll(_err);
    error |= _err;

    if (mode == 0)
    {
        const char *sSignalPolarity;
        switch (wVal)
        {
            case 0x01:
                sSignalPolarity = "Low level active";
                break;
            case 0x02:
                sSignalPolarity = "High Level active";
                break;
            case 0x04:
                sSignalPolarity = "Rising edge active";
                break;
            case 0x08:
                sSignalPolarity = "Falling edge active";
                break;
            default:
                sSignalPolarity = "UNKNOWN";
        }

        /***************************************************************
        wSignalPolarity: Flags showing which signal polarity can be selected:
        - 0x01: Low level active
        - 0x02: High Level active
        - 0x04: Rising edge active
        - 0x08: Falling edge active
        ***************************************************************/

        //	name[Acquire Enable] idx[0] num[0]
        sigNum = 0; // descriptor
        wSelected = 0;
        WORD wPolarity = wVal;

        name = m_pcoData->stcPcoHWIOSignalDesc[sigNum].szSignalName[wSelected];
        m_pcoData->stcPcoHWIOSignal[sigNum].wPolarity = wVal;
        m_pcoData->stcPcoHWIOSignal[sigNum].wSelected = wSelected;

        _pco_SetHWIOSignal(sigNum, _err);
        error |= _err;

        DEB_ALWAYS() << "set PcoHWIOSignal polarity "
                     << DEB_VAR5(name, sigNum, wSelected, wPolarity,
                                 sSignalPolarity);
    }
}
//=================================================================================================
//=================================================================================================

void Camera::_pco_GetROI(Roi &roi, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    WORD wX0, wY0, wX1, wY1;

    // DWORD PCO_GetROI ( WORD *RoiX0, WORD * RoiY0, WORD *RoiX1, WORD *RoiY1 )
    err = camera->PCO_GetROI(&wX0, &wY0, &wX1, &wY1);
    PCO_CHECK_ERROR(err, "PCO_GetROI");
    if (err)
    {
        DEB_ALWAYS() << "ERROR - PCO_GetROI";
        wX0 = wY0 = wX1 = wY1 = 0;
    }

    _xlatRoi_pco2lima(roi, wX0, wX1, wY0, wY1);
}
//=================================================================================================
//=================================================================================================
void Camera::_pco_SetROI(Roi roi, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    unsigned int uiX0, uiY0, uiX1, uiY1;

    _xlatRoi_lima2pco(roi, uiX0, uiX1, uiY0, uiY1);

    err = camera->PCO_SetROI(uiX0, uiY0, uiX1, uiY1);

    PCO_CHECK_ERROR(err, "PCO_SetROI");
    if (err)
    {
        DEB_ALWAYS() << "ERROR - PCO_SetROI";
    }

}

//=================================================================================================
//=================================================================================================
void Camera::_pco_GetRoiInfo(char *buf_in, int size_in, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    char *ptr = buf_in;
    char *ptrMax = ptr + size_in;

    Roi roi;

    unsigned int x0, x1, y0, y1;
    _pco_GetROI(roi, err);

    _xlatRoi_lima2pco(roi, x0, x1, y0, y1);

    ptr += __sprintfSExt(ptr, ptrMax - ptr,
                         "pco[x<%d,%d> y<%d,%d>] lima[<%d,%d>-<%dx%d>]", x0, x1,
                         y0, y1, roi.getTopLeft().x, roi.getTopLeft().y,
                         roi.getSize().getWidth(), roi.getSize().getHeight());
}

//=================================================================================================
//=================================================================================================
int Camera::_binning_fit(int binRequested, int binMax, int binMode)
{
    int binLast, bin;

    if (binRequested < 1)
        return 1;
    if (binRequested >= binMax)
        return binMax;

    binLast = bin = 1;

    while (true)
    {
        if (bin == binRequested)
            return bin;
        if (bin > binRequested)
            return binLast;
        binLast = bin;
        bin = binMode ? bin + 1 : bin * 2;
        if (bin > binMax)
            return binLast;
    }
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_GetBinningInfo(char *buf_in, int size_in, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    char *ptr = buf_in;
    char *ptrMax = ptr + size_in;

    Bin aBin;

    int binX, binY, binMaxX, binModeX, binMaxY, binModeY;
    _pco_GetBinning(aBin, err);

    binX = aBin.getX();
    binY = aBin.getY();

    // TOCHECK - the same?
    binMaxX = m_pcoData->stcPcoDescription.wMaxBinHorzDESC;
    binModeX = m_pcoData->stcPcoDescription.wBinHorzSteppingDESC;

    binMaxY = m_pcoData->stcPcoDescription.wMaxBinVertDESC;
    binModeY = m_pcoData->stcPcoDescription.wBinVertSteppingDESC;

    ptr += __sprintfSExt(ptr, ptrMax - ptr,
                         "bin[%d,%d] binMax[%d,%d] binStepMode[%d,%d][%s,%s]",
                         binX, binY, binMaxX, binMaxY, binModeX, binModeY,
                         binModeX ? "lin" : "bin", binModeY ? "lin" : "bin");
}

//=================================================================================================
//=================================================================================================

void Camera::_pco_GetBinning(Bin &bin, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    WORD wBinHorz, wBinVert;

    err = camera->PCO_GetBinning(&wBinHorz, &wBinVert);
    PCO_CHECK_ERROR(err, "PCO_GetBinning");
    if (err)
    {
        wBinHorz = wBinVert = 1;
        DEB_ALWAYS() << "ERROR - PCO_GetBinning";
    }


    bin = Bin(wBinHorz, wBinVert);
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_GetFirmwareInfo(char *buf_in, int size_in, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    char *ptr = buf_in;
    char *ptrMax = ptr + size_in;

    int lg = ptrMax - ptr;

    err = camera->PCO_GetHardwareVersion(ptr, &lg);
    PCO_CHECK_ERROR(err, "PCO_GetROI");
    if (err)
        return;

    lg = strlen(buf_in);
    ptr = buf_in + lg;
    lg = ptrMax - ptr;

    if (lg)
    {
        err = camera->PCO_GetFirmwareVersion(ptr, &lg);
        PCO_CHECK_ERROR(err, "PCO_GetROI");
        if (err)
            return;
    }

    return;
}

//=================================================================================================
//=================================================================================================

void Camera::getXYdescription(unsigned int &xSteps, unsigned int &ySteps,
                              unsigned int &xMax, unsigned int &yMax,
                              unsigned int &xMinSize, unsigned int &yMinSize)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    xSteps = m_pcoData->stcPcoDescription.wRoiHorStepsDESC;
    ySteps = m_pcoData->stcPcoDescription.wRoiVertStepsDESC;

    xMax = m_pcoData->stcPcoDescription.wMaxHorzResStdDESC;
    yMax = m_pcoData->stcPcoDescription.wMaxVertResStdDESC;

    // xMinSize = xMinSize0 = m_pcoData->stcPcoDescription.wRoiHorStepsDESC;
    // yMinSize = m_pcoData->stcPcoDescription.wRoiVertStepsDESC;
    WORD wXMinSize, wYMinSize;
    _pco_GetCameraMinSizeCalc(wXMinSize, wYMinSize);
    xMinSize = wXMinSize;
    yMinSize = wYMinSize;
}

void Camera::getXYsteps(unsigned int &xSteps, unsigned int &ySteps)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    xSteps = m_pcoData->stcPcoDescription.wRoiHorStepsDESC;
    ySteps = m_pcoData->stcPcoDescription.wRoiVertStepsDESC;
}

void Camera::getMaxWidthHeight(unsigned int &xMax, unsigned int &yMax)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    xMax = m_pcoData->stcPcoDescription.wMaxHorzResStdDESC;
    yMax = m_pcoData->stcPcoDescription.wMaxVertResStdDESC;
}


void Camera::getBytesPerPixel(unsigned int &pixbytes)
{
    pixbytes = (m_pcoData->stcPcoDescription.wDynResDESC <= 8) ? 1 : 2;
}

void Camera::getBitsPerPixel(WORD &pixbits)
{
    pixbits = m_pcoData->stcPcoDescription.wDynResDESC;
    m_pcoData->m_wBitPerPixel = pixbits;
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_GetSizes(WORD *wXResActual, WORD *wYResActual, WORD *wXResMax,
                           WORD *wYResMax, int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    error = 0;

    const char *msg;
    char *bla = mybla;
    DWORD dwXResActual, dwYResActual;

    error = camera->PCO_GetActualSize(&dwXResActual, &dwYResActual);
    msg = "PCO_GetActualSize";
    PCO_CHECK_ERROR(error, msg);

    if (error)
        return;

    *wXResActual = (WORD)dwXResActual;
    *wYResActual = (WORD)dwYResActual;
    *wXResMax = m_pcoData->stcPcoDescription.wMaxHorzResStdDESC;
    *wYResMax = m_pcoData->stcPcoDescription.wMaxVertResStdDESC;

    bla +=
        __sprintfSExt(bla, myblamax - bla,
                      "%s> resAct[%d][%d] resStdMax[%d][%d] resExtMax[%d][%d]",
                      fnId, *wXResActual, *wYResActual, *wXResMax, *wYResMax,
                      m_pcoData->stcPcoDescription.wMaxHorzResExtDESC,
                      m_pcoData->stcPcoDescription.wMaxVertResExtDESC);
    mylog->writelog(INFO_M, mybla);


    m_pcoData->m_wArmWidth = *wXResActual;
    m_pcoData->m_wArmHeight = *wYResActual;
    m_pcoData->m_wMaxWidth = *wXResMax;
    m_pcoData->m_wMaxHeight = *wYResMax;


    return;
}

//=================================================================================================
//=================================================================================================

void Camera::_pco_SetRecordStopEvent(WORD wRecordStopEventMode,
                                     DWORD dwRecordStopDelayImages, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    err = 0;

    err = camera->PCO_SetRecordStopEvent(wRecordStopEventMode,
                                         dwRecordStopDelayImages);

    PCO_CHECK_ERROR(err, "PCO_SetRecordStopEvent");
    if (err)
    {
        PCO_THROW_OR_TRACE(err, "PCO_SetRecordStopEvent");
    }
    return;
}
//=================================================================================================
//=================================================================================================
void Camera::_pco_SetDelayExposureTime(int &error, int ph)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    bool doIt;
    // const char *msg;

    DWORD dwExposure, dwDelay;
    WORD wExposure_base, wDelay_base;
    double _exposure, _delay;
    m_sync->getExpTime(_exposure);
    m_sync->getLatTime(_delay);
    double _delay0 = _delay;

    doIt = true;

    ph = 0;

    if (ph != 0)
    {
        doIt = false;

        WORD _wArmWidth, _wArmHeight, _wMaxWidth, _wMaxHeight;
        _pco_GetSizes(&_wArmWidth, &_wArmHeight, &_wMaxWidth, &_wMaxHeight,
                      error);

        if ((_isCameraType(Edge)) &&
            (m_pcoData->dwPixelRate >= PCO_EDGE_PIXEL_RATE_HIGH))
        {
            double pixels = ((double)_wArmWidth) * ((double)_wArmHeight);
            double bytes =
                (m_pcoData->wLUT_Identifier == PCO_EDGE_LUT_SQRT) ? 1.5 : 2.0;
            double period =
                bytes * pixels / (m_pcoData->fTransferRateMHzMax * 1000000.);

            printf("--- %s>period[%g] -> cocRunTime[%g]\n", fnId, period,
                   m_pcoData->cocRunTime);
            if (period > m_pcoData->cocRunTime)
            {
                _delay += period - m_pcoData->cocRunTime;
                doIt = true;
                printf("--- %s> delay forced [%g] -> [%g]\n", fnId, _delay0,
                       _delay);
            }
        }
    }

    if (!doIt)
        return;

    error = 0;

    _pco_time2dwbase(_exposure, dwExposure, wExposure_base);
    _pco_time2dwbase(_delay, dwDelay, wDelay_base);

    int err;
    DWORD exp_time,delay_time, exp_time0,delay_time0;
    WORD exp_timebase,del_timebase, exp_timebase0,del_timebase0;

    camera->PCO_SetRecordingState(0);
    err=camera->PCO_GetDelayExposure(&delay_time0,&exp_time0);
    PCO_CHECK_ERROR(err, "PCO_GetDelayExposure");
    err=camera->PCO_GetTimebase(&del_timebase0,&exp_timebase0);
    PCO_CHECK_ERROR(err, "PCO_GetTimebase");

    err = camera->PCO_SetTimebase(wDelay_base, wExposure_base);
    PCO_CHECK_ERROR(err, "PCO_SetTimebase");
    error |= err;

    err = camera->PCO_SetDelayExposure(dwDelay, dwExposure);
    PCO_CHECK_ERROR(err, "PCO_SetDelayExposure");
    error |= err;
    
    err=camera->PCO_ArmCamera();
    PCO_CHECK_ERROR(err, "PCO_ArmCamera");
    
    err=camera->PCO_GetDelayExposure(&delay_time,&exp_time);
    PCO_CHECK_ERROR(err, "PCO_GetDelayExposure");
    err=camera->PCO_GetTimebase(&del_timebase,&exp_timebase);
    PCO_CHECK_ERROR(err, "PCO_GetTimebase");


    DEB_ALWAYS() << 
        "\n... timebase: ns(0), us(1) ms(2)" << 
        "\n... get " << DEB_VAR4(delay_time0,exp_time, del_timebase0, exp_timebase0) <<
        "\n... set " << DEB_VAR3(_exposure, dwExposure, wExposure_base) <<
        "\n... set " << DEB_VAR3(_delay, dwDelay, wDelay_base) <<
        "\n... get " << DEB_VAR4(delay_time,exp_time, del_timebase, exp_timebase);

    m_pcoData->traceAcq.dLimaExposure = _exposure;
    m_pcoData->traceAcq.dLimaDelay = _delay;

    m_pcoData->traceAcq.iPcoExposure = dwExposure;
    m_pcoData->traceAcq.iPcoDelay = dwDelay;
    m_pcoData->traceAcq.iPcoExposureBase = wExposure_base;
    m_pcoData->traceAcq.iPcoDelayBase = wDelay_base;

    if (error || _getDebug(DBG_EXP))
    {
        DEB_ALWAYS() << DEB_VAR3(_exposure, dwExposure, wExposure_base);
        DEB_ALWAYS() << DEB_VAR3(_delay, dwDelay, wDelay_base);
    }

    return;
}

//=================================================================================================
//=================================================================================================

void Camera::_pco_FreeBuffer(int bufIdx, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    err = 0;

    DEB_ALWAYS() << NOT_IMPLEMENTED;
    return;

}

//=================================================================================================
//=================================================================================================

void Camera::_pco_AllocateBuffer(SHORT *sBufNr, DWORD dwSize, WORD **wBuf,
                                 void **hEvent, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    err = 0;

    DEB_ALWAYS() << NOT_IMPLEMENTED;
    return;
}

//=================================================================================================
//=================================================================================================

void Camera::_pco_GetImageEx(WORD wSegment, DWORD dw1stImage, DWORD dwLastImage,
                             SHORT sBufNr, WORD wXRes, WORD wYRes,
                             WORD wBitPerPixel, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    err = 0;

    DEB_ALWAYS() << NOT_IMPLEMENTED;
    return;

}

//=================================================================================================
//=================================================================================================

void Camera::_pco_GetBufferStatus(SHORT sBufNr, DWORD *dwStatusDll,
                                  DWORD *dwStatusDrv, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    err = 0;

    DEB_ALWAYS() << NOT_IMPLEMENTED;
    return;
}

//=================================================================================================
//=================================================================================================

void Camera::_pco_AddBufferExtern(HANDLE hEvent, WORD wActSeg, DWORD dw1stImage,
                                  DWORD dwLastImage, DWORD dwSynch, void *pBuf,
                                  DWORD dwLen, DWORD *dwStatus, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    err = 0;

    DEB_ALWAYS() << NOT_IMPLEMENTED;
    return;

}

//=================================================================================================
//=================================================================================================

void Camera::_pco_AddBufferEx(DWORD dw1stImage, DWORD dwLastImage, SHORT sBufNr,
                              WORD wXRes, WORD wYRes, WORD wBitPerPixel,
                              int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    err = 0;

    DEB_ALWAYS() << NOT_IMPLEMENTED;
    return;

}

//=================================================================================================
//=================================================================================================

void Camera::_pco_RebootCamera(int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    err = 0;


    err = camera->PCO_RebootCamera();
    PCO_CHECK_ERROR(err, "PCO_RebootCamera");

    // DEB_ALWAYS() <<  NOT_IMPLEMENTED ;
    return;

}

//=================================================================================================
//=================================================================================================

void Camera::_pco_OpenCamera(int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    err = 0;

    DEB_ALWAYS() << NOT_IMPLEMENTED;
    return;
}

//=================================================================================================
//=================================================================================================

void Camera::_pco_GetCameraRamSize(DWORD &dwRamSize, WORD &wPageSizeint,
                                   int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    err = 0;

    DEB_ALWAYS() << NOT_IMPLEMENTED;
    return;
}

//=================================================================================================
//=================================================================================================

void Camera::_pco_ResetLib(int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    err = 0;

    DEB_ALWAYS() << "INFO: function does NOT exists in linux [" << fnId << "]";
    return;
}

//=================================================================================================
//=================================================================================================

void Camera::_pco_GetCameraSetup(WORD &wType, DWORD &dwSetup, WORD &wLen,
                                 int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    err = 0;


    /// \brief Request the current camera setup
    /// \anchor PCO_GetCameraSetup
    ///
    /// This function is used to query the current operation mode of the camera.
    /// Some cameras can work at different operation modes with different
    /// descriptor settings.\n pco.edge:\n To get the current shutter mode input
    /// index setup_id must be set to 0.\n current shutter mode is returned in
    /// setup_flag[0]
    ///  - 0x00000001 = Rolling Shutter
    ///  - 0x00000002 = Global Shutter
    ///  - 0x00000004 = Global Reset
    ///
    /// \param setup_id Identification code for selected setup type.
    /// \param setup_flag Pointer to a DWORD array to get the current setup
    /// flags. If set to NULL in input only the array length is returned.
    /// - On input this variable can be set to NULL, then only array length is
    /// filled with correct value.
    /// - On output the array is filled with the available information for the
    /// selected setup_id \param length Pointer to a WORD variable
    /// - On input to indicate the length of the Setup_flag array in DWORDs.
    /// - On output the length of the setup_flag array in DWORDS
    /// \return Error code or PCO_NOERROR on success
    ///
    DWORD PCO_GetCameraSetup(WORD setup_id, DWORD * setup_flag, WORD * length);

    // DWORD PCO_GetCameraSetup(WORD *setup_id, DWORD *setup_flag, WORD
    // *length); DWORD PCO_SetCameraSetup(WORD setup_id, DWORD *setup_flag,WORD
    // length);
    //  setup_flag[0]=PCO_EDGE_SETUP_ROLLING_SHUTTER;
    //    setup_flag[1]=0;
    //    setup_flag[2]=0;
    //    setup_flag[3]=0;

    // WORD setup_id=0;
    // int err = 0;
    // DWORD setup_flag[4];
    // WORD  length=sizeof(setup_flag);

    DWORD dwErr = PCO_NOERROR;
    wType = 0;

    // dwErr = camera->PCO_SetCameraSetup(setup_id, setup_flag, length);    WORD
    // setup_id=0;
    dwErr = camera->PCO_GetCameraSetup(wType, &dwSetup, &wLen);
    err = dwErr;
    PCO_CHECK_ERROR(err, "PCO_GetCameraSetup");

    DEB_ALWAYS() << fnId << " " << DEB_VAR2(wType, wLen);
    ;

    return;

}

//=================================================================================================
//=================================================================================================

void Camera::_pco_SetCameraSetup(WORD wType, DWORD &dwSetup, WORD wLen,
                                 int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    err = 0;

    /// \brief Sets the camera setup structure (see camera specific structures)
    /// \anchor PCO_SetCameraSetup
    ///
    /// pco.edge:\n
    /// To get the current shutter mode input index setup_id must be set to 0.\n
    /// current shutter mode is returned in setup_flag[0]
    ///  - 0x00000001 = Rolling Shutter
    ///  - 0x00000002 = Global Shutter
    ///  - 0x00000004 = Global Reset
    /// When camera is set to a new shuttermode uit must be reinitialized by
    /// calling one of the reboot functions. After rebooting, camera description
    /// must be read again see \ref  PCO_GetCameraDescription.
    ///
    /// \param setup_id Identification code for selected setup type.
    /// \param setup_flag Flags to be set for the selected setup type.
    /// \param length Number of valid DWORDs in setup_flag array.
    /// \return Error code or PCO_NOERROR on success
    ///
    // DWORD PCO_SetCameraSetup(WORD setup_id, DWORD *setup_flag,WORD length);

    DWORD dwErr = PCO_NOERROR;
    wType = 0;

    dwErr = camera->PCO_SetCameraSetup(wType, &dwSetup, wLen);
    err = dwErr;
    PCO_CHECK_ERROR(err, "PCO_SetCameraSetup");

    DEB_ALWAYS() << fnId << " " << DEB_VAR2(wType, wLen);
    ;
    return;

}

//=================================================================================================
//=================================================================================================

void Camera::_pco_SetTimeouts(void *buf_in, unsigned int size_in, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    err = 0;

    DEB_ALWAYS() << NOT_IMPLEMENTED;
    return;
}

//=================================================================================================
//=================================================================================================

void Camera::_pco_GetCameraRamSegmentSize(DWORD *dwRamSegSize, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    err = 0;

    DEB_ALWAYS() << NOT_IMPLEMENTED;
    return;
}

//=================================================================================================
//=================================================================================================

void Camera::_pco_SetCameraRamSegmentSize(DWORD *dwRamSegSize, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    err = 0;

    DEB_ALWAYS() << NOT_IMPLEMENTED;
    return;
}

//=================================================================================================
//=================================================================================================

void Camera::_pco_ArmCamera(int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    err = 0;

    err = camera->PCO_ArmCamera();

    PCO_CHECK_ERROR(err, "PCO_ArmCamera()");

    if (err != PCO_NOERROR)
    {
        DEB_ALWAYS() << "ERROR - PCO_ArmCamera() " << DEB_VAR1(err);
    }

    PCO_THROW_OR_TRACE(err, "PCO_ArmCamera");
    return;
}

//=================================================================================================
//=================================================================================================

/// typedef struct _PCO_SC2_CL_TRANSFER_PARAMS
/// {
///    DWORD   baudrate;         // serial baudrate: 9600, 19200, 38400, 56400, 115200
///    DWORD   ClockFrequency;   // Pixelclock in Hz: 40000000,66000000,80000000
///    DWORD   CCline;           // Usage of CameraLink CC1-CC4 lines, use value returned by Get 
///    DWORD   DataFormat;       // see defines below, use value returned by Get
///    DWORD   Transmit;         // single or continuous transmitting images, 0-single, 1-continuous
/// }PCO_SC2_CL_TRANSFER_PARAM;

/// #define PCO_CL_DEFAULT_BAUDRATE 9600
/// #define PCO_CL_PIXELCLOCK_40MHZ 40000000
/// #define PCO_CL_PIXELCLOCK_66MHZ 66000000
/// #define PCO_CL_PIXELCLOCK_80MHZ 80000000
/// #define PCO_CL_PIXELCLOCK_32MHZ 32000000
/// #define PCO_CL_PIXELCLOCK_64MHZ 64000000

/// #define PCO_CL_CCLINE_LINE1_TRIGGER           0x01
/// #define PCO_CL_CCLINE_LINE2_ACQUIRE           0x02
/// #define PCO_CL_CCLINE_LINE3_HANDSHAKE         0x04
/// #define PCO_CL_CCLINE_LINE4_TRANSMIT_ENABLE   0x08

/// #define PCO_CL_DATAFORMAT_MASK   0x0F
/// #define PCO_CL_DATAFORMAT_1x16   0x01
/// #define PCO_CL_DATAFORMAT_2x12   0x02
/// #define PCO_CL_DATAFORMAT_3x8    0x03
/// #define PCO_CL_DATAFORMAT_4x16   0x04
/// #define PCO_CL_DATAFORMAT_5x16   0x05
/// #define PCO_CL_DATAFORMAT_5x12   0x07     //extract data to 12bit
/// #define PCO_CL_DATAFORMAT_10x8   0x08
/// #define PCO_CL_DATAFORMAT_5x12L  0x09     //extract data to 16Bit
/// #define PCO_CL_DATAFORMAT_5x12R  0x0A     //without extract

void Camera::_pco_GetTransferParameter(int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    err = 0;

    char buff[MSG4K + 1];
	const char *cmsg;
    cmsg = "PCO_GetTransferParameter";

	if(_isInterfaceType(ifCameralinkHS))
	{
		__sprintfSExt(buff, sizeof(buff), "%s BYPASSED not valid in CLHS\n", cmsg);
		m_log.append(buff);
		return;
	}

	memset((void *) &clpar, 0, sizeof (clpar));
    err = camera->PCO_GetTransferParameter(&clpar, sizeof(clpar));
    PCO_CHECK_ERROR(err, cmsg);
    __sprintfSExt(buff, sizeof(buff), "%s err[0x%08x]\n"
		"  baudrate[%ld] ClockFrequency[%ld]\n"
		"  CCline[%ld] DataFormat[%ld] Transmit[%ld]\n",
		cmsg, err,
		clpar.baudrate, clpar.ClockFrequency, 
		clpar.CCline,clpar.DataFormat, clpar.Transmit);
	m_log.append(buff);


    if (err)
        return;

    DEB_ALWAYS() << "\n   " << DEB_VAR1(clpar.baudrate) << "\n   "
                 << DEB_VAR1(clpar.ClockFrequency) << "\n   "
                 << DEB_VAR1(clpar.DataFormat) << "\n   "
                 << DEB_VAR1(clpar.Transmit);


    return;
}

//=================================================================================================
//=================================================================================================

void Camera::_pco_FillStructures(int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    // char *pcoFn;
    err = 0;

    DEB_ALWAYS() << NOT_IMPLEMENTED;
    err = -1;

    return;
}

//=================================================================================================
//=================================================================================================

#define HANDLE_LIST_DIM 30

void Camera::_pco_OpenCameraSn(DWORD snRequested, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    err = 0;

    DEB_ALWAYS() << NOT_IMPLEMENTED;
    return;
}
//=================================================================================================
//=================================================================================================

void Camera::_pco_GetCameraTypeOnly(int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    err = 0;

    DEB_ALWAYS() << NOT_IMPLEMENTED;
    return;
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_GetCameraMinSizeCalc(WORD &wMinSizeHorz, WORD &wMinSizeVert)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    unsigned int uiHorzSteps, uiVertSteps;
    getXYsteps(uiHorzSteps, uiVertSteps);
    WORD wHorzSteps = uiHorzSteps;
    WORD wVertSteps = uiVertSteps;

    WORD wh = _getCameraType();
    if ((wh == CAMERATYPE_PCO_EDGE) || (wh == CAMERATYPE_PCO_EDGE_GL) ||
        (wh == CAMERATYPE_PCO_EDGE_42) || (wh == CAMERATYPE_PCO_EDGE_USB3) ||
        (wh == CAMERATYPE_PCO_EDGE_HS))
    {
        wMinSizeHorz = wHorzSteps;
        wMinSizeVert = wVertSteps;

        if (wMinSizeHorz < 160)
            wMinSizeHorz = 160; // due to camera limitation
        if (wMinSizeVert < 16)
            wMinSizeVert = 16;
        if (wh == CAMERATYPE_PCO_EDGE_42)
        {
            wMinSizeHorz = 40; // due to camera limitation
            wMinSizeVert = 16;
        }
        if ((wh == CAMERATYPE_PCO_EDGE_USB3) || (wh == CAMERATYPE_PCO_EDGE_HS))
        {
            wMinSizeVert = 16;
            wMinSizeHorz = 64; // due to camera limitation
        }
    }
    else
    {
        wMinSizeHorz = wHorzSteps * 2;
        wMinSizeVert = wVertSteps * 2;

        if (wMinSizeHorz < 8)
            wMinSizeHorz = 8;
        if (wMinSizeVert < 8)
            wMinSizeVert = 8;
    }
    if (wHorzSteps > 1)
    {
        int k = (wMinSizeHorz) % wHorzSteps;
        if (k > 0)
        {
            k++;
            wMinSizeHorz = wHorzSteps * k;
        }
    }

    if (wVertSteps > 1)
    {
        int k = (wMinSizeVert) % wVertSteps;
        if (k > 0)
        {
            k++;
            wMinSizeVert = wVertSteps * k;
        }
    }

    DEB_ALWAYS() << DEB_VAR4(wMinSizeHorz, wMinSizeVert, wHorzSteps,
                             wVertSteps);
}
