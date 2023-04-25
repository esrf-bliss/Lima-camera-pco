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
#include <sstream>
#include "lima/Exceptions.h"
#include "Pco.h"
#include "PcoCamera.h"
#include "PcoSyncCtrlObj.h"
#include "PcoBufferCtrlObj.h"

using namespace lima;
using namespace lima::Pco;

//=========================================================================================================
const char *_timestamp_pcosyncctrlobj()
{
    return ID_FILE_TIMESTAMP;
}
//=========================================================================================================

//=========================================================================================================
//=========================================================================================================
SyncCtrlObj::SyncCtrlObj(Camera *cam, BufferCtrlObj *buffer)
    : m_exp_time(0.), m_lat_time(0.),

      m_cam(cam), m_handle(cam->getHandle()), m_trig_mode(IntTrig),
      m_buffer(buffer), m_nb_frames(1), m_nb_acq_frames(0),
      m_pcoData(cam->_getPcoData())
{
    DEB_CONSTRUCTOR();
    cam->setRequestStop(stopNone);
    bool ret;
    char *value;

    cam->m_exposing=pcoAcqIdle;
    cam->m_started=false;
    ret = cam->getProperty("trigSingleMulti", value);
    m_extTrigSingle_eq_Multi = ret && (!!atoi(value));
}

//=========================================================================================================
//=========================================================================================================
SyncCtrlObj::~SyncCtrlObj()
{
    DEB_DESTRUCTOR();
}

//=========================================================================================================
//=========================================================================================================

/******************************************************************************
enum TrigMode {
        IntTrig,IntTrigMult,
        ExtTrigSingle, ExtTrigMult,
        ExtGate, ExtStartStop, ExtTrigReadout,
};
******************************************************************************/

bool SyncCtrlObj::checkTrigMode(TrigMode trig_mode)
{
    DEB_MEMBER_FUNCT();
    DEB_PARAM() << DEB_VAR1(trig_mode);

    switch (trig_mode)
    {
        case IntTrig:
        case ExtTrigMult:
        case ExtGate:
            return true;

        case ExtTrigSingle:
            if ((m_cam->_isCameraType(Dimax)) || m_extTrigSingle_eq_Multi)
                return true;
            break;

        case IntTrigMult:
        case ExtStartStop:
        case ExtTrigReadout:
        default:
            break;
    }

    // DEB_TRACE() << "<Trig mode not allowed>  " << DEB_VAR1(trig_mode);
    return false;
}

//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::setTrigMode(TrigMode trig_mode)
{
    DEB_MEMBER_FUNCT();
    DEB_PARAM() << DEB_VAR1(trig_mode);

    if (!checkTrigMode(trig_mode))
    {
        throw LIMA_HW_EXC(NotSupported, "Trigger type not supported");
    }

    m_trig_mode = trig_mode;
}

//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::getTrigMode(TrigMode &trig_mode)
{
    trig_mode = m_trig_mode;
}

//=========================================================================================================
//=========================================================================================================
/******************************************************************************
enum TrigMode {
        IntTrig,IntTrigMult,
        ExtTrigSingle, ExtTrigMult,
        ExtGate, ExtStartStop, ExtTrigReadout,
};
******************************************************************************/

void SyncCtrlObj::xlatLimaTrigMode2Pco(lima::TrigMode limaTrigMode,
                                       WORD &pcoTrigMode, WORD &pcoAcqMode,
                                       bool &extTrig, int &err)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    char const *sLimaTriggerMode = "invalid";

    char const *sPcoTriggerMode = "invalid";
    WORD _pcoTrigMode = 0;

    char const *sPcoAcqMode = "invalid";
    WORD _pcoAcqMode = 0;
    bool ext_trig;
    err = -1;

    if (!checkTrigMode(limaTrigMode))
    {
        throw LIMA_HW_EXC(NotSupported, "Trigger type not supported");
    }

    // acquire mode to be selected:
    // - 0x0000 = [auto] - all images taken are stored

    // - 0x0001 = [external] - the external control input <acq enbl> is a static
    // enable signal of
    //            images. If this input is true (level depending on the DIP
    //            switch), exposure triggers are accepted and images are taken.
    //            If this signal is set false, all exposure triggers are ignored
    //            and the sensor readout is stopped.

    // - 0x0002 = [external] - the external control input <acq enbl> is a
    // dynamic frame start
    //            signal. If this input has got a rising edge true (level
    //            depending on the DIP switch), a frame will be started with
    //            modulation mode. This is only available with modulation mode
    //            enabled (see camera description).

    switch (limaTrigMode)
    {
            //  TRIG MODE PCO = 0x0000
            // A new image exposure is automatically started best possible
            // compared to the readout of an image. If a CCD is used and the
            // images are taken in a sequence, then exposures and sensor readout
            // are started simultaneously. Signals at the trigger input (<exp
            // trig>) are irrelevant.

        case IntTrig: // 0 SOFT (spec)
            sLimaTriggerMode = "IntTrig";

            sPcoTriggerMode = "auto";
            ext_trig = false;
            _pcoTrigMode = 0x0000; // 0 = SOFT (spec)

            sPcoAcqMode = "acqEnbl_Ignored";
            _pcoAcqMode = 0x0000;
            break;

        case ExtTrigSingle:
            // trig (at ACQ ENABLE) starts a sequence of int trigger (dimax
            // only)
            //   StorageMode 0 - record mode
            //   RecorderSubmode 1 - ring buffer
            //   Triggermode 0 - auto
            //   Acquiremode 0 - auto / ignored

            sLimaTriggerMode = "ExtTrigSingle";

            sPcoTriggerMode = "auto";
            ext_trig = true;
            _pcoTrigMode = 0x0000;

            sPcoAcqMode = "acqEnbl_Ignored";
            _pcoAcqMode = 0x0000;
            break;

            // TRIG MODE PCO = 0x0002
            // A delay / exposure sequence is started at the RISING or FALLING
            // edge (depending on the DIP switch setting) of the trigger input
            // (<exp trig>).
        // case IntTrigMult: return 0x0002;   // 1 = START (spec)
        case ExtTrigMult:
            sLimaTriggerMode = "ExtTrigMult";

            sPcoTriggerMode = "startExposure";
            ext_trig = true;
            _pcoTrigMode = 0x0002; // 1 = START (spec)

            sPcoAcqMode = "acqEnbl_Ignored";
            _pcoAcqMode = 0x0000;
            break;

            // TRIG MODE PCO = 0x0003
            // The exposure time is defined by the pulse length at the trigger
            // input(<exp trig>). The delay and exposure time values defined by
            // the set/request delay and exposure command are ineffective.
            // (Exposure time length control is also possible for double image
            // mode; exposure time of the second image is given by the readout
            // time of the first image.)
        case ExtGate: // 2 GATE (spec)
            sLimaTriggerMode = "ExtGate";

            sPcoTriggerMode = "extGate";
            ext_trig = true;
            _pcoTrigMode = 0x0003; // 2 = GATE (spec)

            // case IntTrigMult: // 1 START (spec)

#ifdef DISABLE_ACQ_ENBL_SIGNAL
            sPcoAcqMode = "acqEnbl_Ignored";
            _pcoAcqMode = 0x0000;
#else
            _pcoAcqMode = 0x0001;
            sPcoAcqMode = "acqEnbl_TrigAccepted";
#endif
            break;

            //			pcoAcqMode= 0x0002;
            //			sPcoAcqMode = "acqEnbl_startModulationMode";

        default:
            throw LIMA_HW_EXC(NotSupported, "Invalid value");
    }

    
    err = 0;
    pcoTrigMode = _pcoTrigMode;
    pcoAcqMode = _pcoAcqMode;
    extTrig = ext_trig;

    m_pcoData->traceAcq.sLimaTriggerMode = sLimaTriggerMode;
    m_pcoData->traceAcq.bExtTrig = extTrig;

    m_pcoData->traceAcq.sPcoTriggerMode = sPcoTriggerMode;
    m_pcoData->traceAcq.iPcoTriggerMode = pcoTrigMode;

    m_pcoData->traceAcq.sPcoAcqMode = sPcoAcqMode;
    m_pcoData->traceAcq.iPcoAcqMode = pcoAcqMode;

    DEB_ALWAYS() << "\n ... " << DEB_VAR2(*sLimaTriggerMode, extTrig) << "\n ... "
                 << DEB_VAR2(*sPcoTriggerMode, pcoTrigMode) << "\n ... "
                 << DEB_VAR2(*sPcoAcqMode, pcoAcqMode);

    return;
}

//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::setExpTime(double exp_time)
{
    DEB_MEMBER_FUNCT();

    ValidRangesType valid_ranges;
    getValidRanges(valid_ranges);
    double diff;


    if ((exp_time >= m_pcoData->min_exp_time) &&
        (exp_time <= m_pcoData->max_exp_time))
    {
        m_exp_time = exp_time;
        return;
    }

    if (exp_time < m_pcoData->min_exp_time_err)
    {
        diff = exp_time - valid_ranges.min_exp_time;
        DEB_ALWAYS() << "Exposure time out of range (exp < min): "
                     << DEB_VAR3(diff, exp_time, valid_ranges.min_exp_time);
        THROW_HW_ERROR(NotSupported) << "Exposure time out of range";
    }

    if (exp_time > m_pcoData->max_exp_time_err)
    {
        diff = exp_time - valid_ranges.max_exp_time;
        DEB_ALWAYS() << "Exposure time out of range (exp > max): "
                     << DEB_VAR3(diff, exp_time, valid_ranges.max_exp_time);
        THROW_HW_ERROR(NotSupported) << "Exposure time out of range";
    }

    if (exp_time < m_pcoData->min_exp_time)
    {
        m_exp_time = m_pcoData->min_exp_time;
        DEB_TRACE() << "Exp time fixed " << DEB_VAR2(m_exp_time, exp_time);
        return;
    }

    m_exp_time = m_pcoData->max_exp_time;
    DEB_TRACE() << "Exp time fixed " << DEB_VAR2(m_exp_time, exp_time);
    return;
}

//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::getExpTime(double &exp_time)
{
    DEB_MEMBER_FUNCT();

    ValidRangesType valid_ranges;
    getValidRanges(valid_ranges);

    if (m_exp_time < m_pcoData->min_exp_time)
        m_exp_time = m_pcoData->min_exp_time;
    else if (m_exp_time > m_pcoData->max_exp_time)
        m_exp_time = m_pcoData->max_exp_time;

    exp_time = m_exp_time;
    DEB_RETURN() << DEB_VAR1(exp_time);
}

//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::setLatTime(double lat_time)
{
    DEB_MEMBER_FUNCT();
    // latency time -> delay

    m_lat_time = lat_time;
    DEB_PARAM() << DEB_VAR2(m_lat_time, lat_time);
}

//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::getLatTime(double &lat_time)
{
    DEB_MEMBER_FUNCT();

    lat_time = m_lat_time; // latency time -> delay
    DEB_PARAM() << DEB_VAR2(m_lat_time, lat_time);
}

//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::setNbFrames(int nb_frames)
{
    DEB_MEMBER_FUNCT();
    DEB_PARAM() << DEB_VAR1(nb_frames);

    m_nb_frames = nb_frames;
}

//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::getNbFrames(int &nb_frames)
{
    nb_frames = m_nb_frames;
}

//=========================================================================================================
//=========================================================================================================
// these two functions calls the upper ones get/setNbFrames
void SyncCtrlObj::setNbHwFrames(int nb_frames)
{
    setNbFrames(nb_frames);
}

//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::getNbHwFrames(int &nb_frames)
{
    getNbFrames(nb_frames);
}

//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::getValidRanges(ValidRangesType &valid_ranges)
{
    m_pcoData->step_exp_time =
        (m_pcoData->stcPcoDescription.dwMinExposureStepDESC) *
        NANO; // step exposure time in ns

    m_pcoData->min_exp_time = (m_pcoData->stcPcoDescription.dwMinExposureDESC) *
                              NANO; // Minimum exposure time in ns
    valid_ranges.min_exp_time = m_pcoData->min_exp_time_err =
        m_pcoData->min_exp_time - m_pcoData->step_exp_time;

    m_pcoData->max_exp_time = (m_pcoData->stcPcoDescription.dwMaxExposureDESC) *
                              MILI; // Maximum exposure time in ms
    valid_ranges.max_exp_time = m_pcoData->max_exp_time_err =
        m_pcoData->max_exp_time + m_pcoData->step_exp_time;

    m_pcoData->step_lat_time =
        (m_pcoData->stcPcoDescription.dwMinDelayStepDESC) *
        NANO; // step delay time in ns

    m_pcoData->min_lat_time = (m_pcoData->stcPcoDescription.dwMinDelayDESC) *
                              NANO; // Minimum delay time in ns
    valid_ranges.min_lat_time = m_pcoData->min_lat_time_err =
        (m_pcoData->min_lat_time < m_pcoData->step_lat_time)
            ? m_pcoData->min_lat_time
            : m_pcoData->min_lat_time - m_pcoData->step_lat_time;

    m_pcoData->max_lat_time = (m_pcoData->stcPcoDescription.dwMaxDelayDESC) *
                              MILI; // Maximum delay time in ms
    valid_ranges.max_lat_time = m_pcoData->max_lat_time_err =
        m_pcoData->max_lat_time + m_pcoData->step_lat_time;
}
