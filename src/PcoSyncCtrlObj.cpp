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

#define THROW_LIMA_HW_EXC(Error, x)  { \
	printf("========*** LIMA_HW_EXC %s\n", x ); \
			throw LIMA_HW_EXC(Error, x); \
} 


using namespace lima;
using namespace lima::Pco;

//=========================================================================================================
char* _timestamp_pcosyncctrlobj() {return ID_TIMESTAMP ;}
//=========================================================================================================

//=========================================================================================================
//=========================================================================================================
SyncCtrlObj::SyncCtrlObj(Camera *cam,BufferCtrlObj *buffer) :
  m_cam(cam),
  m_handle(cam->getHandle()),
  m_pcoData(cam->_getPcoData()),
  m_trig_mode(IntTrig),
  m_buffer(buffer),
  m_lat_time(0.), m_exp_time(0.),
  m_nb_frames(1),
  m_nb_acq_frames(0),
  m_exposing(pcoAcqIdle),
  m_started(false)
{
	// DONE
  DEB_CONSTRUCTOR();
    _setRequestStop(stopNone);

}

//=========================================================================================================
//=========================================================================================================
SyncCtrlObj::~SyncCtrlObj()
{
	// DONE
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

	switch(trig_mode){

		case IntTrig:
		case ExtTrigMult:
		case ExtGate:
			return true;

		case IntTrigMult:
		case ExtTrigSingle:
		case ExtStartStop:
		case ExtTrigReadout:
		default: 
			break;
	}

	//DEB_ALWAYS() << "<Trig mode not allowed>  " << DEB_VAR1(trig_mode);
	return false;
}

//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::setTrigMode(TrigMode trig_mode)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(trig_mode);

	// DONE

	if(!checkTrigMode(trig_mode)){
		throw LIMA_HW_EXC(NotSupported,"Trigger type not supported");
	}

	m_trig_mode = trig_mode;
}

//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::getTrigMode(TrigMode &trig_mode)
{
	// DONE
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


WORD SyncCtrlObj::xlatLimaTrigMode2PcoAcqMode()
{
	DEB_MEMBER_FUNCT();
    DEF_FNID;

	WORD pcoAcqMode;


	if(!checkTrigMode(m_trig_mode)){
		throw LIMA_HW_EXC(NotSupported,"Trigger type not supported");
	}


// · acquire mode to be selected:
// - 0x0000 = [auto] - all images taken are stored

// - 0x0001 = [external] - the external control input <acq enbl> is a static enable signal of
//            images. If this input is TRUE (level depending on the DIP switch), exposure triggers are
//            accepted and images are taken. If this signal is set FALSE, all exposure triggers are
//            ignored and the sensor readout is stopped.

// - 0x0002 = [external] - the external control input <acq enbl> is a dynamic frame start
//            signal. If this input has got a rising edge TRUE (level depending on the DIP switch), a
//            frame will be started with modulation mode. This is only available with modulation mode
//            enabled (see camera description).


    switch( m_trig_mode)	{
	  case IntTrig: // 0 SOFT (spec)
    	  pcoAcqMode= 0x0000;
			break;

		case ExtTrigMult:
		//case IntTrigMult: // 1 START (spec)
      case ExtGate:  // 2 GATE (spec)
#ifdef DISABLE_ACQ_ENBL_SIGNAL
    	  pcoAcqMode= 0x0000;
#else
		  pcoAcqMode= 0x0001;
#endif
		  break;

	  default:
		 throw LIMA_HW_EXC(NotSupported,"Invalid value");

	}

	DEB_ALWAYS() << fnId << ": " << DEB_VAR2(pcoAcqMode, m_trig_mode);
	return pcoAcqMode;

}

//=========================================================================================================
//=========================================================================================================

WORD SyncCtrlObj::xlatLimaTrigMode2PcoTrigMode(bool &ext_trig){
	DEB_MEMBER_FUNCT();
    DEF_FNID;

	WORD pcoTrigMode;

	if(!checkTrigMode(m_trig_mode)){
		throw LIMA_HW_EXC(NotSupported,"Trigger type not supported");
	}


	// xlat from lima trig mode to PCO trig mode
  	//------------------------------------------------- triggering mode 
	switch (m_trig_mode) {  // trig mode in spec
			//  PCO = 0x0000
			// A new image exposure is automatically started best possible compared to
			// the readout of an image. If a CCD is used and the images are taken in a
			// sequence, then exposures and sensor readout are started simultaneously.
			// Signals at the trigger input (<exp trig>) are irrelevant.
	    default: 
		case IntTrig: 
			ext_trig = false;
			pcoTrigMode= 0x0000;  // 0 = SOFT (spec)
			break;

			// PCO = 0x0002
			// A delay / exposure sequence is started at the RISING or FALLING edge
			// (depending on the DIP switch setting) of the trigger input (<exp trig>).
		//case IntTrigMult: return 0x0002;   // 1 = START (spec)
		case ExtTrigMult: 
			ext_trig = true;
			pcoTrigMode= 0x0002;   // 1 = START (spec)
			break;

			// PCO = 0x0003
			// The exposure time is defined by the pulse length at the trigger
			// input(<exp trig>). The delay and exposure time values defined by the
			// set/request delay and exposure command are ineffective. (Exposure
			// time length control is also possible for double image mode; exposure
			// time of the second image is given by the readout time of the first image.)
		case ExtGate: 
			ext_trig = true;
			pcoTrigMode= 0x0003;  // 2 = GATE (spec)
			break;

	
	}


	DEB_ALWAYS() << fnId <<": " << DEB_VAR3(pcoTrigMode, m_trig_mode, ext_trig);

	return pcoTrigMode;
}


//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::setExpTime(double exp_time)
{
	// DONE
	DEB_MEMBER_FUNCT();

	ValidRangesType valid_ranges;
  getValidRanges(valid_ranges);
  double diff;

#if 0
  char buff[LEN_DUMP];

  _hex_dump_bytes(&exp_time, sizeof(exp_time), buff, LEN_DUMP);
	DEB_ALWAYS() << DEB_VAR2(exp_time, buff);

	_hex_dump_bytes(&valid_ranges.min_exp_time, sizeof(valid_ranges.min_exp_time), buff, LEN_DUMP);
	DEB_ALWAYS() << DEB_VAR2(valid_ranges.min_exp_time, buff);

	_hex_dump_bytes(&m_pcoData->min_exp_time, sizeof(m_pcoData->min_exp_time), buff, LEN_DUMP);
	DEB_ALWAYS() << DEB_VAR2(m_pcoData->min_exp_time, buff);

	_hex_dump_bytes(&m_pcoData->min_exp_time_err, sizeof(m_pcoData->min_exp_time_err), buff, LEN_DUMP);
	DEB_ALWAYS() << DEB_VAR2(m_pcoData->min_exp_time_err, buff);

#endif


	if ((exp_time >= m_pcoData->min_exp_time) && (exp_time <= m_pcoData->max_exp_time))	{
		m_exp_time = exp_time;
		return;
	}

	if (exp_time < m_pcoData->min_exp_time_err){
	  diff = exp_time - valid_ranges.min_exp_time;
	  DEB_ALWAYS() << "Exposure time out of range (exp < min): "
		<< DEB_VAR3(diff, exp_time, valid_ranges.min_exp_time);
	  THROW_HW_ERROR(NotSupported) << "Exposure time out of range" ;
	}

	if (exp_time > m_pcoData->max_exp_time_err){ 
		diff = exp_time - valid_ranges.max_exp_time;
		DEB_ALWAYS() << "Exposure time out of range (exp > max): "
			<< DEB_VAR3(diff, exp_time, valid_ranges.max_exp_time);
		THROW_HW_ERROR(NotSupported) << "Exposure time out of range" ;
	}

	if (exp_time < m_pcoData->min_exp_time){
		m_exp_time = m_pcoData->min_exp_time;
		DEB_ALWAYS() << "Exp time fixed " << DEB_VAR2(m_exp_time, exp_time);
		return;
	}

	m_exp_time = m_pcoData->max_exp_time;
	DEB_ALWAYS() << "Exp time fixed " << DEB_VAR2(m_exp_time, exp_time);
	return;
}

//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::getExpTime(double &exp_time)
{
  DEB_MEMBER_FUNCT();

  ValidRangesType valid_ranges;
  getValidRanges(valid_ranges);
	// DONE
  
  if (m_exp_time < m_pcoData->min_exp_time) m_exp_time = m_pcoData->min_exp_time;
  else if (m_exp_time > m_pcoData->max_exp_time) m_exp_time = m_pcoData->max_exp_time;
  

  exp_time = m_exp_time;
  DEB_RETURN() << DEB_VAR1(exp_time);
}

//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::setLatTime(double  lat_time)
{
	// DONE
  DEB_MEMBER_FUNCT();
  //delay ???

  m_lat_time = lat_time;
  DEB_PARAM() << DEB_VAR2(m_lat_time, lat_time);
}

//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::getLatTime(double& lat_time)
{
	// DONE
  DEB_MEMBER_FUNCT();

  //m_lat_time = 0.;
  lat_time = m_lat_time;		// Don't know - delay????
  DEB_PARAM() << DEB_VAR2(m_lat_time, lat_time);
}

//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::setNbFrames(int  nb_frames)
{
	// DONE
  DEB_MEMBER_FUNCT();
  DEB_PARAM() << DEB_VAR1(nb_frames);

  m_nb_frames = nb_frames;
}

//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::getNbFrames(int& nb_frames)
{
	// DONE
  nb_frames = m_nb_frames;
}

//=========================================================================================================
//=========================================================================================================
// these two functions calls the upper ones get/setNbFrames
void SyncCtrlObj::setNbHwFrames(int  nb_frames)
{
	// DONE
  setNbFrames(nb_frames);
}

//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::getNbHwFrames(int& nb_frames)
{
	// DONE
  getNbFrames(nb_frames);
}

//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::getValidRanges(ValidRangesType& valid_ranges)
{
	DEF_FNID;
	// DONE

	

	m_pcoData->step_exp_time = (m_pcoData->stcPcoDescription.dwMinExposureStepDESC) * NANO ;	//step exposure time in ns
	
	m_pcoData->min_exp_time = (m_pcoData->stcPcoDescription.dwMinExposureDESC) * NANO ;	//Minimum exposure time in ns
	valid_ranges.min_exp_time = m_pcoData->min_exp_time_err = m_pcoData->min_exp_time - m_pcoData->step_exp_time ;	

	m_pcoData->max_exp_time = (m_pcoData->stcPcoDescription.dwMaxExposureDESC) * MILI ;   // Maximum exposure time in ms  
	valid_ranges.max_exp_time = m_pcoData->max_exp_time_err = m_pcoData->max_exp_time + m_pcoData->step_exp_time ;	


	m_pcoData->step_lat_time = (m_pcoData->stcPcoDescription.dwMinDelayStepDESC) * NANO ;	//step delay time in ns

	m_pcoData->min_lat_time = (m_pcoData->stcPcoDescription.dwMinDelayDESC) * NANO ; // Minimum delay time in ns
	valid_ranges.min_lat_time = m_pcoData->min_lat_time_err = 
		(m_pcoData->min_lat_time < m_pcoData->step_lat_time) ? m_pcoData->min_lat_time : m_pcoData->min_lat_time - m_pcoData->step_lat_time ;

	m_pcoData->max_lat_time = (m_pcoData->stcPcoDescription.dwMaxDelayDESC) * MILI ; // Maximum delay time in ms
	valid_ranges.max_lat_time = m_pcoData->max_lat_time_err = m_pcoData->max_lat_time + m_pcoData->step_lat_time ;	


}

//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::startAcq()
{
  DEB_MEMBER_FUNCT();
  
  bool _started = getStarted();

  m_cam->msgLog("startAcq");

  DEB_ALWAYS() << ": SyncCtrlObj::startAcq() " << DEB_VAR1(_started);

  if(!_started)
    {
  
		if(m_buffer) {
			m_buffer->startAcq();
		}

		_setRequestStop(stopNone);
		setExposing(pcoAcqStart);
		m_cam->startAcq();
		setStarted(true);
    }
}



//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::setStarted(bool started) {

	DEB_MEMBER_FUNCT();
	DEF_FNID;
	AutoMutex lock(m_cond.mutex());

	
	m_started = started;
	m_cond.broadcast();

	DEB_ALWAYS() << fnId << "[exit]" << ": " << DEB_VAR2(m_started, started);
}

//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::stopAcq(bool clearQueue)
{
	DEB_MEMBER_FUNCT();
	DEF_FNID;


	int _stopRequestIn, _stopRequestOut, _nrStop;
	bool _started;
	bool resWait;

	AutoMutex lock(m_cond.mutex());

	m_cam->msgLog("stopAcq");

	_stopRequestIn = _getRequestStop(_nrStop);

	while(_started = getStarted()) {
		_setRequestStop(stopRequest);
        resWait = m_cond.wait(5.);
	}
	lock.unlock();
	_stopRequestOut = _getRequestStop(_nrStop);

	DEB_ALWAYS() << fnId << " [exit]" << ": " << DEB_VAR5(_started, _stopRequestIn, _stopRequestOut, _nrStop, resWait);
}
//=========================================================================================================
//=========================================================================================================
void SyncCtrlObj::getStatus(HwInterface::StatusType& status)
{
	// DONE
	bool _started = getStarted();
  DEB_MEMBER_FUNCT();
DEB_TRACE() << DEB_VAR3(_started, m_buffer, m_exposing);
 DEF_FNID;
  if(_started){
      if(m_buffer){

		  switch(m_exposing) {

			case pcoAcqStart: 
			case pcoAcqRecordStart: 
			  status.acq = AcqRunning;
			  status.det = DetExposure;
			  break;

			case pcoAcqStop: 
			case pcoAcqTransferStop: 
			case pcoAcqIdle: 
			case pcoAcqTransferEnd: 
			case pcoAcqRecordEnd:  
			case pcoAcqTransferStart: 
			  status.acq = AcqRunning;
			  status.det = DetIdle;
			  break;

			case pcoAcqRecordTimeout:
			case pcoAcqWaitTimeout:
			case pcoAcqWaitError:
			case pcoAcqError:
			case pcoAcqPcoError:
			  status.acq = AcqFault;
			  status.det = DetFault;
			  break;


			default:
					THROW_HW_ERROR(NotSupported) << "Undefined value";
		  } // sw
		} // m_buffer
    } else { // not started
      status.acq = AcqReady;
      status.det = DetIdle;
    }

  if(m_cam->_getDebug(DBG_STATUS)) {DEB_ALWAYS() << DEB_VAR2(m_exposing, status);}	

  DEB_RETURN() << DEB_VAR1(status);
}

//=========================================================================================================
//=========================================================================================================
int SyncCtrlObj::_getRequestStop(int &nrStop)
{ 
	nrStop = m_requestStopRetry;
	return m_requestStop;
}


void SyncCtrlObj::_setRequestStop(int requestStop) 
{ 
	DEB_MEMBER_FUNCT();
	DEF_FNID;

	int m_requestStop0 = m_requestStop;

	switch(requestStop) {
			case stopNone:
				m_requestStopRetry = 0;
				m_requestStop = requestStop;
				break;

			case stopRequest:
				m_requestStopRetry++;
				m_requestStop = requestStop;
				break;

	}
	//DEB_ALWAYS() <<  fnId << " [exit]: "  << DEB_VAR4(m_requestStop0, m_requestStop, m_requestStopRetry, requestStop);

}
//=========================================================================================================
//=========================================================================================================
