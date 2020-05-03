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
#ifdef __linux__

#    include <cstdlib>

#    include <sys/stat.h>
#    include <sys/timeb.h>
#    include <time.h>

#    include "lima/Exceptions.h"
#    include "lima/HwSyncCtrlObj.h"

#    include "PcoCamera.h"
#    include "PcoCameraSdk.h"
#    include "PcoSyncCtrlObj.h"

using namespace lima;
using namespace lima::Pco;

void _pco_time2dwbase(double exp_time, DWORD &dwExp, WORD &wBase);

//=================================================================================================
//=================================================================================================
void Camera::_pco_GetCameraInfo(int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    // int errTot=0;
    const char *msg;
    char buff[MSG4K + 1];

    // const char *ptr;
    SC2_Camera_Description_Response dummy;
    dummy.wSize = sizeof(dummy);

    m_pcoData->frames_per_buffer = 1; // for PCO DIMAX

    // -- Get camera description
    // m_pcoData->stcPcoDescription.wSize= sizeof(m_pcoData->stcPcoDescription);

    // error = camera->PCO_GetCameraDescriptor(&m_pcoData->stcPcoDescription);
    // msg = "PCO_GetCameraDescriptor(1)"; PCO_CHECK_ERROR(error, msg);

    error = camera->PCO_GetCameraDescription(&m_pcoData->stcPcoDescription);
    msg = "PCO_GetCameraDescription(1)";
    PCO_CHECK_ERROR(error, msg);
    __sprintfSExt(buff, sizeof(buff), "PCO_GetCameraDescription(1) err[0x%08x]\n", error);
	m_log.append(buff);

    // ERROR!!!!!!!!!
    // error = camera->PCO_GetCameraDescriptionEx(&dummy,
    // &m_pcoData->stcPcoDesc2, 1);

    // not used, only to test the function!
    // error = camera->PCO_GetCameraDescription(&m_pcoData->stcPcoDesc2);
    // msg = "PCO_GetCameraDescription(2)"; PCO_CHECK_ERROR(error, msg);

    double min_exp_time0, max_exp_time0;
    double min_lat_time0, max_lat_time0;
    double min_exp_time, max_exp_time;
    double min_lat_time, max_lat_time;
    double step_exp_time, step_lat_time;

    step_exp_time = m_pcoData->step_exp_time =
        (m_pcoData->stcPcoDescription.dwMinExposureStepDESC) *
        NANO; // step exposure time in ns

    min_exp_time0 = m_pcoData->min_exp_time =
        (m_pcoData->stcPcoDescription.dwMinExposureDESC) *
        NANO; // Minimum exposure time in ns
    min_exp_time = m_pcoData->min_exp_time_err =
        m_pcoData->min_exp_time - m_pcoData->step_exp_time;

    max_exp_time0 = m_pcoData->max_exp_time =
        (m_pcoData->stcPcoDescription.dwMaxExposureDESC) *
        MILI; // Maximum exposure time in ms
    max_exp_time = m_pcoData->max_exp_time_err =
        m_pcoData->max_exp_time + m_pcoData->step_exp_time;

    step_lat_time = m_pcoData->step_lat_time =
        (m_pcoData->stcPcoDescription.dwMinDelayStepDESC) *
        NANO; // step delay time in ns

    min_lat_time0 = m_pcoData->min_lat_time =
        (m_pcoData->stcPcoDescription.dwMinDelayDESC) *
        NANO; // Minimum delay time in ns
    min_lat_time = m_pcoData->min_lat_time_err =
        (m_pcoData->min_lat_time < m_pcoData->step_lat_time)
            ? m_pcoData->min_lat_time
            : m_pcoData->min_lat_time - m_pcoData->step_lat_time;

    max_lat_time0 = m_pcoData->max_lat_time =
        (m_pcoData->stcPcoDescription.dwMaxDelayDESC) *
        MILI; // Maximum delay time in ms
    max_lat_time = m_pcoData->max_lat_time_err =
        m_pcoData->max_lat_time + m_pcoData->step_lat_time;

    DEB_ALWAYS() << "\n   " << DEB_VAR2(step_exp_time, step_lat_time) << "\n   "
                 << DEB_VAR2(min_exp_time0, max_exp_time0) << "\n   "
                 << DEB_VAR2(min_lat_time0, max_lat_time0) << "\n   "
                 << DEB_VAR2(min_exp_time, max_exp_time) << "\n   "
                 << DEB_VAR2(min_lat_time, max_lat_time);

    __sprintfSExt(buff, sizeof(buff), 
		"       Step (ms): exp_time[%g] lat_time[%g]\n"
		"  exp_time0 (ms): min[%g] max[%g]\n"
		"  lat_time0 (ms): min[%g] max[%g]\n"
		"   exp_time (ms): min[%g] max[%g]\n"
		"   lat_time (ms): min[%g] max[%g]\n",
		step_exp_time*1000., step_lat_time*1000.,
		min_exp_time0*1000., max_exp_time0*1000.,
		min_lat_time0*1000., max_lat_time0*1000.,
		min_exp_time*1000., max_exp_time*1000.,
		min_lat_time*1000., max_lat_time*1000.);
	m_log.append(buff);



    // callback to update in lima the valid_ranges from the last
    // stcPcoDescription read
    if (m_sync)
    {
        HwSyncCtrlObj::ValidRangesType valid_ranges;
        m_sync->getValidRanges(valid_ranges);     // from stcPcoDescription
        m_sync->validRangesChanged(valid_ranges); // callback
        DEB_ALWAYS() << fnId << ": callback - new valid_ranges: "
                     << DEB_VAR1(valid_ranges);
    }

    m_pcoData->bMetaDataAllowed =
        !!(m_pcoData->stcPcoDescription.dwGeneralCaps1 & GENERALCAPS1_METADATA);


    __sprintfSExt(buff, sizeof(buff), "Metadata allowed[%d]\n", m_pcoData->bMetaDataAllowed);
    m_log.append(buff);



    if (m_pcoData->bMetaDataAllowed)
    {
        error = camera->PCO_GetMetadataMode(&m_pcoData->wMetaDataMode,
                                            &m_pcoData->wMetaDataSize,
                                            &m_pcoData->wMetaDataVersion);
        msg = "PCO_GetMetadataMode";
        PCO_CHECK_ERROR(error, msg);

		__sprintfSExt(buff, sizeof(buff), "PCO_GetMetadataMode err[0x%08x]\n"
			"  metadataMode[%d] metadataSize[%d] metadataVersion[%d]\n",
			error, m_pcoData->wMetaDataMode, m_pcoData->wMetaDataSize,m_pcoData->wMetaDataVersion);
			m_log.append(buff);
    }
    else
    {
        m_pcoData->wMetaDataMode = m_pcoData->wMetaDataSize =
            m_pcoData->wMetaDataVersion = 0;
    }


    {
        // get the max CAMERA pixel rate (Hz) from the description structure

        DWORD _dwPixRate;
        dwPixelRateMax = m_pcoData->dwPixelRateMax = 0;
        iPixelRateValidNr = 0;

        char validPixelRate[128];
        char *ptr = validPixelRate;
        char *ptrMax = ptr + sizeof(validPixelRate);

        ptr += __sprintfSExt(ptr, ptrMax - ptr, "validRates:");
        for (int i = 0; i < 4; i++)
        {
            _dwPixRate = m_pcoData->stcPcoDescription.dwPixelRateDESC[i];
            if (_dwPixRate > 0)
            {
                dwPixelRateValid[iPixelRateValidNr++] = _dwPixRate;
                ptr += __sprintfSExt(ptr, ptrMax - ptr, "  [%d]", _dwPixRate);
            }
            if (dwPixelRateMax < _dwPixRate)
            {
                dwPixelRateMax = m_pcoData->dwPixelRateMax = _dwPixRate;
            }
        }

        DEB_ALWAYS() << "\n   " << DEB_VAR1(iPixelRateValidNr) << "\n   "
                     << DEB_VAR1(validPixelRate);


		__sprintfSExt(buff, sizeof(buff), "Pixel rates (Hz) from description structure:\n"
			"  nr[%d] max[%d] %s\n", iPixelRateValidNr, dwPixelRateMax, validPixelRate);
			m_log.append(buff);

    }

    m_pcoData->bMetaDataAllowed =
        !!(m_pcoData->stcPcoDescription.dwGeneralCaps1 & GENERALCAPS1_METADATA);

    // -- Get General
    // m_pcoData->stcPcoGeneral.wSize= sizeof(m_pcoData->stcPcoGeneral);
    // m_pcoData->stcPcoGeneral.strCamType.wSize=
    // sizeof(m_pcoData->stcPcoGeneral.strCamType);

    // PCO2(error, msg,PCO_GetGeneral,m_handle, &m_pcoData->stcPcoGeneral);
    // PCO_CHECK_ERROR(error, msg); 	if(error);

    // -- Get Sensor struct
    // m_pcoData->stcPcoSensor.wSize= sizeof(m_pcoData->stcPcoSensor);
    // m_pcoData->stcPcoSensor.strDescription.wSize=
    // sizeof(m_pcoData->stcPcoSensor.strDescription);
    // m_pcoData->stcPcoSensor.strDescription2.wSize=
    // sizeof(m_pcoData->stcPcoSensor.strDescription2);

    // PCO2(error, msg,PCO_GetSensorStruct, m_handle, &m_pcoData->stcPcoSensor);
    // PCO_CHECK_ERROR(error, msg); 	if(error) return;

    // -- Get timing struct
    // m_pcoData->stcPcoTiming.wSize= sizeof(m_pcoData->stcPcoTiming);

    // PCO2(error, msg,PCO_GetTimingStruct, m_handle, &m_pcoData->stcPcoTiming);
    // PCO_CHECK_ERROR(error, msg); 	if(error) return;

    // -- Get recording struct
    // m_pcoData->stcPcoRecording.wSize= sizeof(m_pcoData->stcPcoRecording);

    // PCO2(error, msg,PCO_GetRecordingStruct,m_handle,
    // &m_pcoData->stcPcoRecording);
    // PCO_CHECK_ERROR(error, msg); 	if(error) return;

    // -- Get storage struct
    // m_pcoData->stcPcoStorage.wSize= sizeof(m_pcoData->stcPcoStorage);

    // PCO2(error, msg,PCO_GetStorageStruct, m_handle,
    // &m_pcoData->stcPcoStorage);  PCO_CHECK_ERROR(error, msg); if(error)
    // return;

    return;
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_GetLut(int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    err = camera->PCO_GetLut(&wLutActive, &wLutParam);
    PCO_CHECK_ERROR(err, "PCO_GetLut");
    if (err)
        return;

    DEB_ALWAYS() << "\n   " << DEB_VAR1(wLutActive) << "\n   "
                 << DEB_VAR1(wLutParam);

    return;
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_Open_Cam(int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    DEB_ALWAYS() << "creating the camera";
    char *value;

    // SDK call -> depends on the interface!
    // camera = new CPco_com_cl_me4();
    // camera= new CPco_com_clhs();
    // camera= new CPco_com_usb();
    

    if(!getProperty("camera_if", value))
    {
        THROW_FATAL(Hardware, Error) << "PCO property camera_if is REQUIRED!!!";
    }

    camera = NULL;
    
    if (_stricmp(value, "ME4") == 0)
    {
        //camera = new CPco_com_cl_me4();
    }
    else if (_stricmp(value, "CLHS") == 0)
    {
        camera = new CPco_com_clhs();
    } 
    else 
    {
        THROW_FATAL(Hardware, Error) << "interface is NOT IMPLEMENTED!!! " << DEB_VAR1(value);
    }

    if (camera == NULL)
    {
        THROW_FATAL(Hardware, Error) << "could not create interface!!! " << DEB_VAR1(value);
    }


    camera->SetLog(mylog);
    unsigned long debugSdk_get = mylog->get_logbits();
    DEB_ALWAYS() << "debugSdk_get" << DEB_VAR1(debugSdk_get);
    
    DEB_ALWAYS() << "BEFORE Try to open Camera";
    err = camera->Open_Cam(0);
    DEB_ALWAYS() << "AFTER Try to open Camera" << DEB_VAR1(err);
    PCO_CHECK_ERROR(err, "Open_Cam close application");
    if (err)
    {
        delete camera;
        camera = NULL;
        THROW_FATAL(Hardware, Error) << "can NOT open the camera!!!";
    }

    DEB_ALWAYS() << "After open Camera";

    return;
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_Open_Grab(int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    // int iErr;

    const char *sCamType = _getCameraTypeStr();
    const char *sCamSubType = _getCameraSubTypeStr();
    const char *sInterfaceType = _getInterfaceTypeStr();
    WORD wInterfaceType = _getInterfaceType();
    camtype = _getCameraType();
	const char *msg = "NONE";

	int PicTimeOutMs = 10*1000;
	
    grabber_me4 = NULL;
	grabber_clhs = NULL;
	
    DEB_ALWAYS() << "Open Grabber" << DEB_VAR2(wInterfaceType, camtype) << "=" << DEB_HEX(camtype)
				<< ", " << DEB_VAR3(sCamType, sCamSubType, sInterfaceType);



    if ((camtype == CAMERATYPE_PCO_EDGE_HS) && (wInterfaceType == INTERFACE_CAMERALINKHS))
    {
		grabber_clhs = new CPco_grab_clhs((CPco_com_clhs*)camera);
		msg = "CLHS";
		
	}
    else if ((camtype == CAMERATYPE_PCO_EDGE) || (camtype == CAMERATYPE_PCO_EDGE_42))
    {
        DEB_ALWAYS() << "Grabber is CPco_grab_cl_me4_edge";

		switch(wInterfaceType)
		{
			case INTERFACE_CAMERALINK:
#ifdef ME4
				grabber_me4 = new CPco_grab_cl_me4_edge((CPco_com_cl_me4 *)camera);
#endif
				msg = "ME4";
				break;
			case INTERFACE_CAMERALINKHS:
				grabber_clhs = new CPco_grab_clhs((CPco_com_clhs*)camera);
				msg = "CLHS";
				break;
			default:
				THROW_FATAL(Hardware, Error) << "grabber NOT implemented!!! " << DEB_VAR2(camtype, wInterfaceType);
		}
    }
    else if (_isCameraType(Dimax))
    {
		msg = "ME4";
#ifdef ME4
        //grabber_me4 = new CPco_grab_cl_me4_camera((CPco_com_cl_me4 *)camera);
#endif
    }
    else
    {
        camera->Close_Cam();
        delete camera;
        camera = NULL;
        grabber_me4 = NULL;
        grabber_clhs = NULL;

        THROW_FATAL(Hardware, Error) << "Camera is not supported";
    }

    DEB_ALWAYS() << "Grabber is " << msg;

    if ((grabber_me4 == NULL) && (grabber_clhs == NULL))
    {
        camera->Close_Cam();
        delete camera;
        camera = NULL;

        THROW_FATAL(Hardware, Error) << "can not create the grabber";
    }

	printf("\n+++++++++++++++++++=Logging set to 0x%x\n", mylog->get_logbits());
	DEB_ALWAYS() << "Try to open Grabber";

	if(grabber_clhs)
    {
		grabber_clhs->SetLog(mylog);
		err = grabber_clhs->Open_Grabber(board);
		if(!err)
		{
			err=grabber_clhs->Set_Grabber_Timeout(PicTimeOutMs);
		}
	}
	else if(grabber_me4)
	{
#ifdef ME4
		grabber_me4->SetLog(mylog);
		err = grabber_me4->Open_Grabber(board);
#endif
	}
	else
	{
        THROW_FATAL(Hardware, Error) << "any grabber is opened";
	}
	
    
    if (err)
    {
		PCO_CHECK_ERROR(err, "grabber creation");
        delete grabber_me4;
        delete grabber_clhs;
        grabber_me4 = NULL;
        grabber_clhs = NULL;

        camera->Close_Cam();
        delete camera;
        camera = NULL;

        THROW_FATAL(Hardware, Error) << "Open_Grabber, close application";
    }

    return;
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_ResetSettingsToDefault(int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    int iErr = 0;

    // -- Reset to default settings

    _pco_SetRecordingState(0, iErr);

    error = camera->PCO_ResetSettingsToDefault();
    PCO_CHECK_ERROR(error, "PCO_ResetSettingsToDefault");

    error |= iErr;

    return;
}

#endif
