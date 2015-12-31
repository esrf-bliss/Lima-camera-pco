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
#include <process.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <time.h>

#include "lima/Exceptions.h"
#include "lima/HwSyncCtrlObj.h"

#include "PcoCamera.h"
#include "PcoSyncCtrlObj.h"
#include "PcoBufferCtrlObj.h"
#include "PcoCameraUtils.h"


using namespace lima;
using namespace lima::Pco;

static char *timebaseUnits[] = {"ns", "us", "ms"};

char *_checkLogFiles();
void _pco_acq_thread_dimax(void *argin);
void _pco_acq_thread_dimax_live(void *argin);
void _pco_acq_thread_ringBuffer(void *argin);

void _pco_acq_thread_edge(void *argin);
void _pco_shutter_thread_edge(void *argin);
void _pco_time2dwbase(double exp_time, DWORD &dwExp, WORD &wBase);

char * _timestamp_pcosyncctrlobj();
char * _timestamp_pcointerface();
char * _timestamp_pcobufferctrlobj();
char * _timestamp_pcodetinfoctrlobj();
char * _timestamp_pcocamerautils();
char * _timestamp_pcoroictrlobj();
char *_split_date(char *s);

char *str_trim_left(char *s);
char *str_trim_right(char *s);
char *str_trim(char *s) ;
char *str_toupper(char *s);




//=========================================================================================================
char* _timestamp_pcocamera() {return ID_TIMESTAMP ;}


#ifdef WITH_GIT_VERSION
#include "PcoGitVersion.h"
char * _timestamp_gitversion(char *buffVersion, int len)
{
	sprintf_s(buffVersion, len, "%s\n%s\n%s\n%s\n%s\n%s\n", 
				 PCO_GIT_VERSION,
				 PROCLIB_GIT_VERSION,
				 LIBCONFIG_GIT_VERSION,
				 LIMA_GIT_VERSION,
				 TANGO_GIT_VERSION,
				 SPEC_GIT_VERSION
				 );
	return buffVersion;
}
#endif

char * _getComputerName(char *infoBuff, DWORD  bufCharCount);
char * _getUserName(char *infoBuff, DWORD  bufCharCount);
char * _getVSconfiguration(char *infoBuff, DWORD  bufCharCount);

//=========================================================================================================

//=========================================================================================================
//=========================================================================================================
char *xlatCode2Str(int code, struct stcXlatCode2Str *stc) {

	char *type;

	while( (type = stc->str) != NULL) {
		if(stc->code == code) return type;
		stc++;
	}

	return NULL;

}

//=========================================================================================================
//=========================================================================================================

enum tblXlatCode2Str {ModelType, InterfaceType};

char *xlatPcoCode2Str(int code, tblXlatCode2Str table, int &err) {
	struct stcXlatCode2Str modelType[] = {
		{CAMERATYPE_PCO1200HS, "PCO 1200 HS"},
		{CAMERATYPE_PCO1300, "PCO 1300"},
		{CAMERATYPE_PCO1600, "PCO 1600"},
		{CAMERATYPE_PCO2000, "PCO 2000"},
		{CAMERATYPE_PCO4000, "PCO 4000"},
		{CAMERATYPE_PCO_DIMAX_STD, "PCO DIMAX STD"},
		{CAMERATYPE_PCO_DIMAX_TV, "PCO DIMAX TV"},
		{CAMERATYPE_PCO_DIMAX_AUTOMOTIVE, "PCO DIMAX AUTOMOTIVE"},
		{CAMERATYPE_PCO_EDGE, "PCO EDGE"},
		{CAMERATYPE_PCO_EDGE_GL, "PCO EDGE GL"},
		{0, NULL}
	};

  struct stcXlatCode2Str interfaceType[] = {
		{INTERFACE_FIREWIRE, "FIREWIRE"},
		{INTERFACE_CAMERALINK, "CAMERALINK"},
		{INTERFACE_USB, "USB"},
		{INTERFACE_ETHERNET, "ETHERNET"},
		{INTERFACE_SERIAL, "SERIAL"},
		{0, NULL}
	};

  struct stcXlatCode2Str *stc;
	char *ptr;
	static char buff[BUFF_XLAT_SIZE+1];

  switch(table) {
    case ModelType: stc = modelType; break;
    case InterfaceType: stc = interfaceType; break;
    default:
  		sprintf_s(buff, BUFF_XLAT_SIZE, "UNKNOWN XLAT TABLE [%d]", table);
  		err = 1;
	  	return buff;
  }

	if((ptr = xlatCode2Str(code, stc)) != NULL) {
		err = 0;
		return ptr;
	} else {
		sprintf_s(buff, BUFF_XLAT_SIZE, "UNKNOWN %s code [0x%04x]", (table == ModelType) ? "MODEL" : "INTERFACE", code);
		err = 1;
		return buff;
	}
}



//=========================================================================================================
//=========================================================================================================

#define BUFFER_LEN 256
#define BUFFVERSION_LEN 2048
stcPcoData::stcPcoData(){

	char *ptr, *ptrMax;
	int i;
	char buff[BUFFER_LEN+1];
	
	memset(this, 0, sizeof(struct stcPcoData));

	ptr = version; *ptr = 0;
	ptrMax = ptr + sizeof(version) - 1;

	ptr += sprintf_s(ptr, ptrMax - ptr, "\n");
	ptr += sprintf_s(ptr, ptrMax - ptr, "%s\n", _split_date(_timestamp_pcocamera()));
	ptr += sprintf_s(ptr, ptrMax - ptr, "%s\n", _split_date(_timestamp_pcosyncctrlobj()));
	ptr += sprintf_s(ptr, ptrMax - ptr, "%s\n", _split_date(_timestamp_pcointerface()));
	ptr += sprintf_s(ptr, ptrMax - ptr, "%s\n", _split_date(_timestamp_pcobufferctrlobj()));
	ptr += sprintf_s(ptr, ptrMax - ptr, "%s\n", _split_date(_timestamp_pcodetinfoctrlobj()));
	ptr += sprintf_s(ptr, ptrMax - ptr, "%s\n", _split_date(_timestamp_pcocamerautils()));
	ptr += sprintf_s(ptr, ptrMax - ptr, "%s\n", _split_date(_timestamp_pcoroictrlobj()));

#ifdef WITH_GIT_VERSION
	char buffVersion[BUFFVERSION_LEN+1];
	ptr += sprintf_s(ptr, ptrMax - ptr, "%s\n", _timestamp_gitversion(buffVersion, BUFFVERSION_LEN));
#endif

	ptr += sprintf_s(ptr, ptrMax - ptr, "       timestamp: %s\n", getTimestamp(Iso));
	ptr += sprintf_s(ptr, ptrMax - ptr, "   computer name: %s\n", _getComputerName(buff, BUFFER_LEN));
	ptr += sprintf_s(ptr, ptrMax - ptr, "       user name: %s\n", _getUserName(buff, BUFFER_LEN));
	ptr += sprintf_s(ptr, ptrMax - ptr, "VS configuration: %s\n", _getVSconfiguration(buff, BUFFER_LEN));

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

	for(i=0; i < SIZEARR_stcPcoHWIOSignal; i++) {
		stcPcoHWIOSignal[i].wSize = sizeof(stcPcoHWIOSignal[i]);
		stcPcoHWIOSignalDesc[i].wSize = sizeof(stcPcoHWIOSignalDesc[i]);
	}

	bAllocatedBufferDone = 
		false;
	
	msAcqRecTimestamp = msAcqXferTimestamp =
			getTimestamp();

}

//=========================================================================================================
//=========================================================================================================
bool Camera::paramsGet(const char *key, char *&value) {
	DEF_FNID;
	DEB_CONSTRUCTOR();
	bool ret;

	for(int i = 0; i < m_pcoData->params.nr; i++) {
		if(_stricmp(key, m_pcoData->params.ptrKey[i]) == 0) {
			ret = true;
			value = m_pcoData->params.ptrValue[i];
			DEB_ALWAYS() << DEB_VAR3(key, ret, value);
			return ret;
		}
	}
	ret = false;
	value = ""; 
	DEB_ALWAYS() << DEB_VAR3(key, ret, value);
	return ret;

}
//=========================================================================================================
//=========================================================================================================
void Camera::paramsInit(const char *str) 
{
	DEF_FNID;
	DEB_CONSTRUCTOR();

	int i;
	char *tokNext = NULL;
	char *buff = m_pcoData->params.buff;
	int &nrList = m_pcoData->params.nr;
	int nr;
	
	// --- split of params string
	strcpy_s(buff, PARAMS_LEN_BUFF , str);
	char *ptr = buff;
	
	for(nr = i = 0; i < PARAMS_NR; i++) {
		if( (m_pcoData->params.ptrKey[i] = strtok_s(ptr, ";", &tokNext)) == NULL) break;
		ptr = NULL;
		nr = i+1;
	}

	nrList = 0;

	for(i = 0; i < nr; i++) {
		char *key, *value;
		bool found;

		ptr = str_trim(m_pcoData->params.ptrKey[i]);
		key = strtok_s(ptr, "=", &tokNext);
		value = strtok_s(NULL, "=", &tokNext);
		str_toupper(key);	
		m_pcoData->params.ptrKey[i] = key = str_trim(key);	
		value = str_trim(value);	
		if(value == NULL) value = "";
		m_pcoData->params.ptrValue[i] = value;	
		
		found = false;
		for(int j = 0; j < nrList; j++) {
			if(_stricmp(m_pcoData->params.ptrKey[j], m_pcoData->params.ptrKey[i]) == 0){
				m_pcoData->params.ptrValue[j] = m_pcoData->params.ptrValue[i];
				found = true;
				break;
			}
		}
		if(!found) {
			key = m_pcoData->params.ptrKey[nrList] = m_pcoData->params.ptrKey[i]; 
			value = m_pcoData->params.ptrValue[nrList] = m_pcoData->params.ptrValue[i] ;
			nrList++;
		}
	}

	for(int j = 0; j < nrList; j++) {
		char *key, *value;
		key = m_pcoData->params.ptrKey[j];
		value = m_pcoData->params.ptrValue[j];
		DEB_ALWAYS() << DEB_VAR2(key, value);
	}
};

//=========================================================================================================
//=========================================================================================================

Camera::Camera(const char *params) :
	m_cam_connected(false),
	m_acq_frame_nb(1),
	m_sync(NULL),
	m_buffer(NULL),
	m_handle(NULL)
{
	DEF_FNID;
	DEB_CONSTRUCTOR();

	int error=0;
	m_config = TRUE;
	DebParams::checkInit();

	
	m_msgLog = new ringLog(300) ;
	m_tmpLog = new ringLog(300) ;
	if(m_msgLog == NULL)
		throw LIMA_HW_EXC(Error, "m_msgLog > creation error");
	if(m_tmpLog == NULL)
		throw LIMA_HW_EXC(Error, "m_tmpLog > creation error");

	m_pcoData =new stcPcoData();
	if(m_pcoData == NULL)
		throw LIMA_HW_EXC(Error, "m_pcoData > creation error");

	// properties: params 
	paramsInit(params);

	char *value, *key;
	bool ret;

	/***
	key = "test";
	key = "withConfig";
	key = "testMode";
	key = "debugPco";
	***/
	key = "extValue";
	ret = paramsGet(key, value);

	DEB_ALWAYS()
		<< ALWAYS_NL << DEB_VAR1(m_pcoData->version) 
		<< ALWAYS_NL << _checkLogFiles();

	m_bin.changed = Invalid;
	
	_init();
	m_config = FALSE;
}


//=========================================================================================================
//=========================================================================================================
void Camera::_init(){
	DEB_CONSTRUCTOR();
	DEF_FNID;

	DEB_ALWAYS() << fnId << " [entry]";

	char msg[MSG_SIZE + 1];
	int error=0;
	char *errMsg;
	char *pcoFn;

	_armRequired(true);

	m_log.clear();
	sprintf_s(msg, MSG_SIZE, "*** Pco log %s\n", getTimestamp(Iso));
	m_log.append(msg);


		// --- Open Camera - close before if it is open
	if(m_handle) {
		char *pcoFn;
		DEB_ALWAYS() << fnId << " [closing opened camera]";
		PCO_FN1(error, pcoFn,PCO_CloseCamera, m_handle);
		PCO_THROW_OR_TRACE(error, "_init(): PCO_CloseCamera - closing opened cam") ;
		m_handle = NULL;
	}

	int retrySleep =15;
	int retryMax = 0;
	int retry = retryMax;
	while(true) {
		PCO_FN2(error, pcoFn,PCO_OpenCamera, &m_handle, 0);
		if(error)
		{
			if(retry--<=0) break;
			DEB_ALWAYS() << "\n... ERROR - PCO_OpenCamera / retry ... " << DEB_VAR2(retry, retrySleep);
			::Sleep(retrySleep * 1000);
		} else {break;}
	} 

	if(error)
	{ 
		DEB_ALWAYS() << "\n... ERROR - PCO_OpenCamera / abort" ; 
		THROW_HW_ERROR(Error) ;
	}
		
	PCO_THROW_OR_TRACE(error, "_init(): PCO_OpenCamera") ;
	
	errMsg = _pco_GetCameraType(error);
	PCO_THROW_OR_TRACE(error, errMsg) ;

	DEB_ALWAYS() << fnId << " [camera opened] " << DEB_VAR1(m_handle);

	// -- Initialise ADC
	//-------------------------------------------------------------------------------------------------
	// PCO_SetADCOperation
    // Set analog-digital-converter (ADC) operation for reading the image sensor data. Pixel data can be
    // read out using one ADC (better linearity) or in parallel using two ADCs (faster). This option is
    // only available for some camera models. If the user sets 2ADCs he must center and adapt the ROI
    // to symmetrical values, e.g. pco.1600: x1,y1,x2,y2=701,1,900,500 (100,1,200,500 is not possible).
    //
	// DIMAX -> 1 adc
	//-------------------------------------------------------------------------------------------------
	
	// set ADC = 1 for better linearity (when it is configurable ...)
	int adc_working;
	_pco_SetADCOperation(1, adc_working);
	m_pcoData->wNowADC= (WORD) adc_working;

		// -- Initialise size, bin, roi
	unsigned int maxWidth, maxHeight,maxwidth_step, maxheight_step; 
	getMaxWidthHeight(maxWidth, maxHeight);
	getXYsteps(maxwidth_step, maxheight_step);


	_get_MaxRoi(m_RoiLima);
	_get_MaxRoi(m_RoiLimaRequested);
	
	WORD bitsPerPix;
	getBitsPerPixel(bitsPerPix);

	sprintf_s(msg, MSG_SIZE, "* CCD Size = X[%d] * Y[%d] (%d bits)\n", maxWidth, maxHeight, bitsPerPix);
	DEB_TRACE() <<   msg;
	m_log.append(msg);
	
	sprintf_s(msg, MSG_SIZE, "* ROI Steps = x:%d, y:%d\n", maxwidth_step, maxheight_step);
	DEB_TRACE() <<   msg;
	m_log.append(msg);

	errMsg = _pco_GetTemperatureInfo(error);
	PCO_THROW_OR_TRACE(error, errMsg) ;

	_pcoSet_RecordingState(0, error);
	
	if(_isCameraType(Dimax)) _init_dimax();
	else if(_isCameraType(Pco2k)) _init_dimax();
	else if(_isCameraType(Pco4k)) _init_dimax();
	else if(_isCameraType(Edge)) _init_edge();
	else {
		char msg[MSG_SIZE+1];
		sprintf_s(msg, MSG_SIZE, "Camera type not supported! [x%04x]", _getCameraType());
		DEB_ALWAYS() <<  msg;

		throw LIMA_HW_EXC(Error, msg);
	}


	m_cam_connected = true;
	error = 0;

	if(!m_cam_connected)
		throw LIMA_HW_EXC(Error, "Camera not found!");

	_pco_initHWIOSignal(0, error);

  DEB_TRACE() << m_log;
  DEB_TRACE() << "END OF CAMERA";
	DEB_ALWAYS() << fnId << " [exit]";

}

//=========================================================================================================
//=========================================================================================================
void  Camera::_init_dimax() {

	DEB_CONSTRUCTOR();
	char msg[MSG_SIZE + 1];
	char *pcoFn;

	int error=0;
	DWORD _dwValidImageCnt, _dwMaxImageCnt;

	
	// block #1 -- Get RAM size
	{
		int segmentPco, segmentArr;

		DWORD ramSize;
		WORD pageSize;

		WORD bitsPerPix;
		getBitsPerPixel(bitsPerPix);

		PCO_FN3(error, pcoFn,PCO_GetCameraRamSize, m_handle, &ramSize, &pageSize);
		PCO_THROW_OR_TRACE(error, pcoFn) ;

		m_pcoData->dwRamSize = ramSize;     // nr of pages of the ram
		m_pcoData->wPixPerPage = pageSize;    // nr of pixels of the page

		sprintf_s(msg, MSG_SIZE, "* ramPages[%ld] pixPerPage[%d] bitsPerPix[%d]\n",  
				m_pcoData->dwRamSize, m_pcoData->wPixPerPage, bitsPerPix);
		DEB_TRACE() <<   msg;
		m_log.append(msg);
		
		double nrBytes = (double) m_pcoData->dwRamSize  * (double) m_pcoData->wPixPerPage * 
			(double)bitsPerPix / 9.; // 8 bits data + 1 bit CRC -> 9
		
		sprintf_s(msg, MSG_SIZE, "* camMemorySize [%lld B] [%g GB]\n",  
				(long long int) nrBytes, nrBytes/GIGABYTE);
		DEB_TRACE() <<   msg;
		m_log.append(msg);

		// ----------------- get initial seg Size - images & print

		// ---- get the size in pages of each of the 4 segments

		DWORD   segSize[4];
		PCO_FN2(error, pcoFn,PCO_GetCameraRamSegmentSize, m_handle, segSize);
		PCO_THROW_OR_TRACE(error, pcoFn) ;

		for(segmentArr=0; segmentArr < PCO_MAXSEGMENTS ; segmentArr++) {
			segmentPco = segmentArr +1;		// PCO segment (1 ... 4)
			m_pcoData->dwSegmentSize[segmentArr] = segSize[segmentArr];

			sprintf_s(msg, MSG_SIZE, "* segment[%d] number of pages[%ld]\n", segmentPco,m_pcoData->dwSegmentSize[segmentArr]);
			DEB_TRACE() <<   msg;
			m_log.append(msg);

		}

		//---- get nr de images in each segment & nr max of img on each segmente
		for(segmentArr=0;  segmentArr< PCO_MAXSEGMENTS ; segmentArr++) {
			segmentPco = segmentArr +1;

			PCO_FN4(error, pcoFn,PCO_GetNumberOfImagesInSegment, m_handle, segmentPco, &_dwValidImageCnt, &_dwMaxImageCnt);
			PCO_THROW_OR_TRACE(error, pcoFn) ;

			m_pcoData->dwValidImageCnt[segmentArr] = _dwValidImageCnt;
			m_pcoData->dwMaxImageCnt[segmentArr] = _dwMaxImageCnt;

			sprintf_s(msg, MSG_SIZE, "* segment[%d] nr images [%ld]  max imag [%ld]\n", segmentPco, _dwValidImageCnt,  _dwMaxImageCnt);
			DEB_TRACE() <<   msg;
			m_log.append(msg);

		} // for	


		// set the first segment to the max ram size, the others = 0
		// This function will result in all segments being cleared. 
		// All previously recorded images will be lost!

		//m_pcoData->dwSegmentSize[0] = m_pcoData->dwRamSize;
		for(segmentArr=1;  segmentArr < PCO_MAXSEGMENTS ; segmentArr++) {
			m_pcoData->dwSegmentSize[0] += m_pcoData->dwSegmentSize[segmentArr];
			m_pcoData->dwSegmentSize[segmentArr] = 0;
		}
		sprintf_s(msg, MSG_SIZE, "* m_pcoData->dwSegmentSize0 [%ld]  m_pcoData->dwRamSize [%ld]\n", m_pcoData->dwSegmentSize[0], m_pcoData->dwRamSize);
		DEB_TRACE() <<   msg;
		m_log.append(msg);


		PCO_FN2(error, pcoFn,PCO_SetCameraRamSegmentSize, m_handle, &m_pcoData->dwSegmentSize[0]);
		PCO_THROW_OR_TRACE(error, pcoFn) ;
	}  // block #1 

	DEB_TRACE() <<  "end block 1 / get initial seg Size - images";

	{
		int segmentPco, segmentArr;
		
		unsigned int maxWidth, maxHeight; 
		getMaxWidthHeight(maxWidth, maxHeight);

		
		DWORD pages_per_image = maxWidth * maxHeight / m_pcoData->wPixPerPage;

		///------------------------------------------------------------------------TODO ?????
		for(segmentArr=0; segmentArr < PCO_MAXSEGMENTS ; segmentArr++) {
			segmentPco = segmentArr +1;
			if(m_pcoData->dwMaxImageCnt[segmentArr] == 0){
				m_pcoData->dwMaxImageCnt[segmentArr] = m_pcoData->dwSegmentSize[segmentArr] / pages_per_image;
				if(m_pcoData->dwMaxImageCnt[segmentArr] > 4) m_pcoData->dwMaxImageCnt[segmentArr] -= 2;
			}
		}	
	} // block


	// -- Get Active RAM segment 

		PCO_FN2(error, pcoFn,PCO_GetActiveRamSegment, m_handle, &m_pcoData->activeRamSegment);
		PCO_THROW_OR_TRACE(error, pcoFn) ;

		PCO_FN4(error, pcoFn,PCO_GetNumberOfImagesInSegment, m_handle, m_pcoData->activeRamSegment, &_dwValidImageCnt, &_dwMaxImageCnt);
		PCO_THROW_OR_TRACE(error, pcoFn) ;



	DEB_TRACE() <<  "original DONE";


}


//=========================================================================================================
//=========================================================================================================
void Camera::_init_edge() {

	m_pcoData->fTransferRateMHzMax = 550.;



}



//=========================================================================================================
//=========================================================================================================
Camera::~Camera()
{
  DEB_DESTRUCTOR();
  DEB_TRACE() << "~Camera";
	int error;
	char *msg ;

  if(m_cam_connected){
		//m_sync->_getBufferCtrlObj()->_pcoAllocBuffersFree();
		//m_buffer->_pcoAllocBuffersFree();

	}
	m_cam_connected = false;

	//m_sync->_getBufferCtrlObj()->_pcoAllocBuffersFree();
	m_buffer->_pcoAllocBuffersFree();
	PCO_FN1(error, msg,PCO_CloseCamera, m_handle);
	PCO_PRINT_ERR(error, msg); 

}



//=================================================================================================
//=================================================================================================
void Camera::getCameraName(std::string& name)
{
  DEB_MEMBER_FUNCT();
  DEB_RETURN() << DEB_VAR1(m_pcoData->camera_name);

  name = m_pcoData->camera_name;
}


//=================================================================================================
//=================================================================================================
void Camera::startAcq()
{
    DEB_MEMBER_FUNCT();

	m_acq_frame_nb = -1;
	m_pcoData->pcoError = 0;
	m_pcoData->pcoErrorMsg[0] = 0;

	m_pcoData->traceAcqClean();

	struct __timeb64 tStart;
	msElapsedTimeSet(tStart);

//=====================================================================
	DEF_FNID;
    WORD state;
    HANDLE hEvent= NULL;

	DEB_ALWAYS() << fnId << " [ENTRY]" ;

	int error;
	char *msg;

    int iRequestedFrames;

			// live video requested frames = 0
    m_sync->getNbFrames(iRequestedFrames);

	//SetBinning, SetROI, ARM, GetSizes, AllocateBuffer.
    //------------------------------------------------- set binning if needed
    WORD wBinHorz, wBinVert, wBinHorzNow, wBinVertNow;
    if (m_bin.changed == Changed) {
		wBinHorz = (WORD)m_bin.x;
		wBinVert = (WORD)m_bin.y;

		PCO_FN3(error, msg,PCO_GetBinning, m_handle, &wBinHorzNow, &wBinVertNow);
		PCO_THROW_OR_TRACE(error, msg) ;
		
		if((wBinHorz != wBinHorzNow) || (wBinVert != wBinVertNow)) {
			PCO_FN3(error, msg,PCO_SetBinning, m_handle, wBinHorz, wBinVert);
			PCO_THROW_OR_TRACE(error, msg) ;
			_armRequired(true);

			PCO_FN3(error, msg,PCO_GetBinning,m_handle, &wBinHorzNow, &wBinVertNow);
			PCO_THROW_OR_TRACE(error, msg) ;
		}
		m_bin.changed= Valid;
		DEB_TRACE() << DEB_VAR4(wBinHorz, wBinVert, wBinHorzNow, wBinVertNow);
    }


    //------------------------------------------------- set roi if needed
    WORD &wRoiX0Now = m_pcoData->wRoiX0Now;
	WORD &wRoiY0Now = m_pcoData->wRoiY0Now;
    WORD &wRoiX1Now = m_pcoData->wRoiX1Now;
	WORD &wRoiY1Now = m_pcoData->wRoiY1Now;

    WORD wRoiX0, wRoiY0; // Roi upper left x y
    WORD wRoiX1, wRoiY1; // Roi lower right x y
	unsigned int x0, x1, y0, y1;

	_get_Roi(x0, x1, y0, y1);
    wRoiX0 = (WORD) x0; wRoiX1 = (WORD) x1;
    wRoiY0 = (WORD) y0; wRoiY1 = (WORD) y1;

	PCO_FN5(error, msg,PCO_GetROI, m_handle, &wRoiX0Now, &wRoiY0Now, &wRoiX1Now, &wRoiY1Now);
	PCO_THROW_OR_TRACE(error, msg) ;

	bool test;
	test = ((wRoiX0Now != wRoiX0) ||	(wRoiX1Now != wRoiX1) || (wRoiY0Now != wRoiY0) || (wRoiY1Now != wRoiY1));
	DEB_ALWAYS() 
		<< "\n> " << DEB_VAR1(test)
		<< "\n   _get_Roi> " << DEB_VAR4(x0, x1, y0, y1)
		<< "\n   _get_Roi> " << DEB_VAR4(wRoiX0, wRoiX1, wRoiY0, wRoiY1)
		<< "\n   PCO_GetROI> " << DEB_VAR4(wRoiX0Now, wRoiY0Now, wRoiX1Now, wRoiY1Now);
	test = true;
	if(test) {
		
		DEB_ALWAYS() 
			<< "\n   PCO_SetROI> " << DEB_VAR5(m_RoiLima, wRoiX0, wRoiY0, wRoiX1, wRoiY1);

		PCO_FN5(error, msg,PCO_SetROI, m_handle, wRoiX0, wRoiY0, wRoiX1, wRoiY1);
		PCO_THROW_OR_TRACE(error, msg) ;

		_armRequired(true);

		PCO_FN5(error, msg,PCO_GetROI, m_handle, &wRoiX0Now, &wRoiY0Now, &wRoiX1Now, &wRoiY1Now);
		PCO_THROW_OR_TRACE(error, msg) ;
		DEB_ALWAYS() 
			<< "\n   PCO_GetROI> " << DEB_VAR4(wRoiX0Now, wRoiY0Now, wRoiX1Now, wRoiY1Now)
			<< "\n   PCO_GetROI> " << DEB_VAR4(m_pcoData->wRoiX0Now, m_pcoData->wRoiY0Now, m_pcoData->wRoiX1Now, m_pcoData->wRoiY1Now);

	}

	//------------------------------------------------- triggering mode 
    //------------------------------------- acquire mode : ignore or not ext. signal
	msg = _pco_SetTriggerMode_SetAcquireMode(error);
    PCO_THROW_OR_TRACE(error, msg) ;

    // ----------------------------------------- storage mode (recorder + sequence)
    if(_isCameraType(Dimax)) {
		
			// live video requested frames = 0
		enumPcoStorageMode mode = (iRequestedFrames > 0) ? RecSeq : Fifo;

		msg = _pco_SetStorageMode_SetRecorderSubmode(mode, error);
		PCO_THROW_OR_TRACE(error, msg) ;
	}

	if(_isCameraType(Pco4k | Pco2k)) {
			// live video requested frames = 0
		enumPcoStorageMode mode = Fifo;
		DEB_ALWAYS() << "PcoStorageMode mode - PCO2K / 4K: " << DEB_VAR1(mode);

		msg = _pco_SetStorageMode_SetRecorderSubmode(mode, error);
		PCO_THROW_OR_TRACE(error, msg) ;
	}
//----------------------------------- set exposure time & delay time
	msg = _pco_SetDelayExposureTime(error,0);   // initial set of delay (phase = 0)
	PCO_THROW_OR_TRACE(error, msg) ;


    //------------------------------------------------- check recording state
    PCO_FN2(error, msg,PCO_GetRecordingState, m_handle, &state);
    PCO_THROW_OR_TRACE(error, msg) ;

    if (state>0) {
        DEB_TRACE() << "Force recording state to 0x0000" ;

		_pcoSet_RecordingState(0, error);
        PCO_THROW_OR_TRACE(error, "PCO_SetRecordingState") ;
	}

//-----------------------------------------------------------------------------------------------
//	5. Arm the camera.
//	6. Get the sizes and allocate a buffer:
//		PCO_GETSIZES(hCam, &actualsizex, &actualsizey, &ccdsizex, &ccdsizey)
//		PCO_ALLOCATEBUFFER(hCam, &bufferNr, actualsizex * actualsizey * sizeof(WORD), &data, &hEvent)
//		In case of CamLink and GigE interface: PCO_CamLinkSetImageParameters(actualsizex, actualsizey)
//		PCO_ArmCamera(hCam)
//-----------------------------------------------------------------------------------------------
	
	msg = _pco_SetMetaDataMode(0, error); PCO_THROW_OR_TRACE(error, msg) ;
  
	//--------------------------- PREPARE / pixel rate - ARM required     
	msg = _pco_SetPixelRate(error); PCO_THROW_OR_TRACE(error, msg) ;
		
	//--------------------------- PREPARE / clXferParam, LUT - ARM required 
	msg = _pco_SetTransferParameter_SetActiveLookupTable(error); PCO_THROW_OR_TRACE(error, msg) ;

	//--------------------------- PREPARE / ARM  
	DEB_ALWAYS() << "\n   ARM the camera / PCO_ArmCamera (1)";
	PCO_FN1(error, msg,PCO_ArmCamera, m_handle); 
	PCO_THROW_OR_TRACE(error, msg) ;
	
	
	//--------------------------- PREPARE / getSizes (valid after ARM) alloc buffer
	PCO_FN5(error, msg,PCO_GetSizes , m_handle, &m_pcoData->wXResActual, &m_pcoData->wYResActual, &m_pcoData->wXResMax, &m_pcoData->wYResMax) ;
	PCO_THROW_OR_TRACE(error, msg) ;

	m_buffer->_pcoAllocBuffers(false);

	//--------------------------- PREPARE / img parameters
	msg = _pco_SetCamLinkSetImageParameters(error); PCO_THROW_OR_TRACE(error, msg) ;

	//--------------------------- PREPARE / cocruntime (valid after PCO_SetDelayExposureTime and ARM)
	msg = _pco_GetCOCRuntime(error); PCO_THROW_OR_TRACE(error, msg) ;


    //------------------------------------------------- checking nr of frames
    if(_isCameraType(Dimax)){
        unsigned long framesMax;
        framesMax = pcoGetFramesMax(m_pcoData->activeRamSegment);

        if ((((unsigned long) iRequestedFrames) > framesMax)) {
            throw LIMA_HW_EXC(Error, "frames OUT OF RANGE");
        }
    } 
	
	//------------------------------------------------- start acquisition

	m_pcoData->traceAcq.msStartAcqStart = msElapsedTime(tStart);

	m_sync->setStarted(true);
	m_sync->setExposing(pcoAcqRecordStart);

	if(_isCameraType(Edge)){
		_beginthread( _pco_acq_thread_edge, 0, (void*) this);
		m_pcoData->traceAcq.msStartAcqEnd = msElapsedTime(tStart);
		return;
	}

	if(_isCameraType(Pco2k | Pco4k)){
		_pcoSet_RecordingState(1, error);
		_beginthread( _pco_acq_thread_ringBuffer, 0, (void*) this);
		m_pcoData->traceAcq.msStartAcqEnd = msElapsedTime(tStart);
		return;
	}

	if(_isCameraType(Dimax)){
		_pcoSet_RecordingState(1, error);
		if(iRequestedFrames > 0 ) {
			_beginthread( _pco_acq_thread_dimax, 0, (void*) this);
		} else {
			_beginthread( _pco_acq_thread_dimax_live, 0, (void*) this);
		}
		m_pcoData->traceAcq.msStartAcqEnd = msElapsedTime(tStart);
		return;
	}

	throw LIMA_HW_EXC(Error, "unkown camera type");
	return;
}


//==========================================================================================================
//==========================================================================================================

long msElapsedTime(struct __timeb64 &t0) {
	struct __timeb64 tNow;
	_ftime64_s(&tNow);

	return (long)((tNow.time - t0.time)*1000) + (tNow.millitm - t0.millitm);
}

void msElapsedTimeSet(struct __timeb64 &t0) {
	_ftime64_s(&t0);
}


void usElapsedTimeSet(LARGE_INTEGER &tick0) {

	QueryPerformanceCounter(&tick0);

}

long long usElapsedTime(LARGE_INTEGER &tick0) {
	LARGE_INTEGER ticksPerSecond;
	LARGE_INTEGER tick;   // A point in time
	long long uS, uS0;

	QueryPerformanceFrequency(&ticksPerSecond); 
	QueryPerformanceCounter(&tick);

	double ticsPerUSecond = ticksPerSecond.QuadPart/1.0e6;
	uS = (long long) (tick.QuadPart/ticsPerUSecond);
	uS0 = (long long) (tick0.QuadPart/ticsPerUSecond);

	return uS - uS0;

}

double usElapsedTimeTicsPerSec() {
	LARGE_INTEGER ticksPerSecond;

	QueryPerformanceFrequency(&ticksPerSecond); 
	return (double) ticksPerSecond.QuadPart;

}

//==========================================================================================================
//==========================================================================================================

void _pco_acq_thread_dimax(void *argin) {
	DEF_FNID;
	printf("=== %s [%d]> %s ENTRY\n",  fnId, __LINE__,getTimestamp(Iso));

	static char msgErr[LEN_ERROR_MSG+1];

	int error;
	int _nrStop;
	DWORD _dwValidImageCnt, _dwMaxImageCnt;

	Camera* m_cam = (Camera *) argin;
	SyncCtrlObj* m_sync = m_cam->_getSyncCtrlObj();
	//BufferCtrlObj* m_buffer = m_sync->_getBufferCtrlObj();
	BufferCtrlObj* m_buffer = m_cam->_getBufferCtrlObj();

	struct stcPcoData *m_pcoData = m_cam->_getPcoData();
	//m_pcoData->traceAcqClean();
	m_pcoData->traceAcq.fnId = fnId;

	char *msg;
	struct __timeb64 tStart, tStart0;
	msElapsedTimeSet(tStart);
	tStart0 = tStart;

	long timeout, timeout0, msNowRecordLoop, msRecord, msXfer, msTotal;
	int nb_acq_frames;
	int requestStop = stopNone;

	HANDLE m_handle = m_cam->getHandle();

	WORD wSegment = m_cam->pcoGetActiveRamSegment(); 
	double msPerFrame = (m_cam->pcoGetCocRunTime() * 1000.);
	m_pcoData->traceAcq.msImgCoc = msPerFrame;

	//DWORD dwMsSleepOneFrame = (DWORD) (msPerFrame + 0.5);	// 4/5 rounding
	DWORD dwMsSleepOneFrame = (DWORD) (msPerFrame/5.0);	// 4/5 rounding
	if(dwMsSleepOneFrame == 0) dwMsSleepOneFrame = 1;		// min sleep

	bool nb_frames_fixed = false;
	int nb_frames; 	m_sync->getNbFrames(nb_frames);
	//m_pcoData->traceAcq.nrImgRequested = nb_frames;
	m_pcoData->traceAcq.nrImgRequested0 = nb_frames;

	m_sync->setAcqFrames(0);

	timeout = timeout0 = (long) (msPerFrame * (nb_frames * 1.3));	// 30% guard
	if(timeout < TOUT_MIN_DIMAX) timeout = TOUT_MIN_DIMAX;
    
	m_pcoData->traceAcq.msTout = m_pcoData->msAcqTout = timeout;
	_dwValidImageCnt = 0;

	m_sync->getExpTime(m_pcoData->traceAcq.sExposure);
	m_sync->getLatTime(m_pcoData->traceAcq.sDelay);

	m_sync->setExposing(pcoAcqRecordStart);

	while(true) {
		msg = m_cam->_PcoCheckError(__LINE__, __FILE__, 
					PCO_GetNumberOfImagesInSegment(m_handle, wSegment, &_dwValidImageCnt, &_dwMaxImageCnt), error, "PCO_GetNumberOfImagesInSegment");
		if(error) {
			printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__, msg);
			throw LIMA_HW_EXC(Error, "PCO_GetNumberOfImagesInSegment");
		}

		m_pcoData->dwValidImageCnt[wSegment-1] = 
			m_pcoData->traceAcq.nrImgRecorded = _dwValidImageCnt;
		m_pcoData->dwMaxImageCnt[wSegment-1] =
			m_pcoData->traceAcq.maxImgCount = _dwMaxImageCnt;

		m_pcoData->msAcqTnow = msNowRecordLoop = msElapsedTime(tStart);
		m_pcoData->traceAcq.msRecordLoop = msNowRecordLoop;
		
		if( ((DWORD) nb_frames > _dwMaxImageCnt) ){
			nb_frames_fixed = true;
			
			sprintf_s(msgErr,LEN_ERROR_MSG, 
				"=== %s [%d]> ERROR INVALID NR FRAMES fixed nb_frames[%d] _dwMaxImageCnt[%d]", 
				fnId, __LINE__, nb_frames, _dwMaxImageCnt);
			printf("%s\n", msgErr);

			m_sync->setExposing(pcoAcqError);
			break;
		}

		if(  (_dwValidImageCnt >= (DWORD) nb_frames)) break;

		if((timeout < msNowRecordLoop) && !m_pcoData->bExtTrigEnabled) { 
			//m_sync->setExposing(pcoAcqRecordTimeout);
			m_sync->stopAcq();
			m_sync->setExposing(pcoAcqStop);
			printf("=== %s [%d]> TIMEOUT!!! tout[(%ld) 0(%ld)] recLoopTime[%ld ms] lastImgRecorded[%ld] nrImgRequested[%d]\n", 
				fnId, __LINE__, timeout, timeout0, msNowRecordLoop, _dwValidImageCnt, nb_frames);
			break;
		}
	
		if((requestStop = m_sync->_getRequestStop(_nrStop))  == stopRequest) {
			m_sync->_setRequestStop(stopNone);
		
			char msg[LEN_TRACEACQ_MSG+1];
				//m_buffer->_setRequestStop(stopProcessing);
				//m_sync->setExposing(pcoAcqStop);
				
			snprintf(msg,LEN_TRACEACQ_MSG, "=== %s> STOP REQ (recording). lastImgRec[%d]\n", fnId, _dwValidImageCnt);
				printf(msg);
				m_pcoData->traceMsg(msg);
				break;
		}
		Sleep(dwMsSleepOneFrame);	// sleep 1 frame
	} // while(true)

	m_pcoData->msAcqTnow = msNowRecordLoop = msElapsedTime(tStart);
	m_pcoData->traceAcq.msRecordLoop = msNowRecordLoop;

	msg = m_cam->_pcoSet_RecordingState(0, error);
	if(error) {
		printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__, msg);
		throw LIMA_HW_EXC(Error, "_pcoSet_RecordingState");
	}

	if(!nb_frames_fixed) {
		if(m_sync->getExposing() == pcoAcqRecordStart) m_sync->setExposing(pcoAcqRecordEnd);

		msg = m_cam->_PcoCheckError(__LINE__, __FILE__, 
			PCO_GetNumberOfImagesInSegment(m_handle, wSegment, &_dwValidImageCnt, &_dwMaxImageCnt), error);
		if(error) {
			printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__, msg);
			throw LIMA_HW_EXC(Error, "PCO_GetNumberOfImagesInSegment");
		}

		m_pcoData->dwValidImageCnt[wSegment-1] = 
			m_pcoData->traceAcq.nrImgRecorded = _dwValidImageCnt;

		nb_acq_frames = (_dwValidImageCnt < (DWORD) nb_frames) ? _dwValidImageCnt : nb_frames;
		//m_sync->setAcqFrames(nb_acq_frames);

		// dimax recording time
		m_pcoData->msAcqRec = msRecord = msElapsedTime(tStart);
		m_pcoData->traceAcq.msRecord = msRecord;    // loop & stop record
		
		m_pcoData->traceAcq.endRecordTimestamp = m_pcoData->msAcqRecTimestamp = getTimestamp();
		
		m_pcoData->traceAcq.nrImgAcquired = nb_acq_frames;
		m_pcoData->traceAcq.nrImgRequested = nb_frames;

		msElapsedTimeSet(tStart);  // reset for xfer


		if(nb_acq_frames < nb_frames) m_sync->setNbFrames(nb_acq_frames);

//		if(m_buffer->_getRequestStop()) {
//			m_sync->setExposing(pcoAcqStop);
//		} else 
		
		// --- in case of stop request during the record phase, the transfer
		// --- is made to avoid lose the image recorded
		{
			pcoAcqStatus status;

			if(m_cam->_isCameraType(Pco2k | Pco4k)){
				if(m_pcoData->testCmdMode & TESTCMDMODE_DIMAX_XFERMULTI) {
					status = (pcoAcqStatus) m_buffer->_xferImag();
				} else {
					status = (pcoAcqStatus) m_buffer->_xferImagMult();  //  <------------- default NO waitobj
				}
			}else{
				if(m_pcoData->testCmdMode & TESTCMDMODE_DIMAX_XFERMULTI) {
					status = (pcoAcqStatus) m_buffer->_xferImagMult();
				} else {
					status = (pcoAcqStatus) m_buffer->_xferImag(); //  <------------- default YES waitobj
				}

			}
			
			if(nb_frames_fixed) status = pcoAcqError;
			m_sync->setExposing(status);

		}

	} // if nb_frames_fixed
	
	
	
	//m_sync->setExposing(status);
	m_pcoData->dwMaxImageCnt[wSegment-1] =
			m_pcoData->traceAcq.maxImgCount = _dwMaxImageCnt;

	// traceAcq info - dimax xfer time
	m_pcoData->msAcqXfer = msXfer = msElapsedTime(tStart);
	m_pcoData->traceAcq.msXfer = msXfer;

	m_pcoData->msAcqAll = msTotal = msElapsedTime(tStart0);
	m_pcoData->traceAcq.msTotal= msTotal;

	m_pcoData->traceAcq.endXferTimestamp = m_pcoData->msAcqXferTimestamp = getTimestamp();


	printf("=== %s [%d]> EXIT imgRecorded[%d] coc[%g] recLoopTime[%ld] "
			"tout[(%ld) 0(%ld)] rec[%ld] xfer[%ld] all[%ld](ms)\n", 
			fnId, __LINE__, _dwValidImageCnt, msPerFrame, msNowRecordLoop, timeout, timeout0, msRecord, msXfer, msTotal);

	// included in 34a8fb6723594919f08cf66759fe5dbd6dc4287e only for dimax (to check for others)
	m_sync->setStarted(false);

	_endthread();
}

//==========================================================================================================
//==========================================================================================================

char *sPcoAcqStatus[] ={
	"pcoAcqIdle", 
	"pcoAcqStart", 
	"pcoAcqRecordStart", 
	"pcoAcqRecordEnd",  
	"pcoAcqTransferStart", 
	"pcoAcqTransferEnd", 
	"pcoAcqStop", 
	"pcoAcqTransferStop", 
	"pcoAcqRecordTimeout",
	"pcoAcqWaitTimeout",
	"pcoAcqWaitError",
	"pcoAcqError",
	"pcoAcqPcoError",
};

//=====================================================================
//=====================================================================
void _pco_shutter_thread_edge(void *argin) {
	DEF_FNID;
	int error;

	printf("=== %s %s> ENTRY\n", fnId, getTimestamp(Iso));

	Camera* m_cam = (Camera *) argin;
	SyncCtrlObj* m_sync = m_cam->_getSyncCtrlObj();
	m_cam->_pco_set_shutter_rolling_edge(error);

	printf("=== %s> EXIT\n", fnId);

	m_sync->setStarted(false); // to test

	_endthread();
}


//=====================================================================
//=====================================================================

void _pco_acq_thread_edge(void *argin) {
	DEF_FNID;


	printf("=== %s> ENTRY\n", fnId);

	Camera* m_cam = (Camera *) argin;
	SyncCtrlObj* m_sync = m_cam->_getSyncCtrlObj();
	//BufferCtrlObj* m_buffer = m_sync->_getBufferCtrlObj();
	BufferCtrlObj* m_buffer = m_cam->_getBufferCtrlObj();

	struct stcPcoData *m_pcoData = m_cam->_getPcoData();

	struct __timeb64 tStart;
	msElapsedTimeSet(tStart);
	int error;
	long msXfer;
	int requestStop = stopNone;

	HANDLE m_handle = m_cam->getHandle();

	m_sync->setAcqFrames(0);


	pcoAcqStatus status = (pcoAcqStatus) m_buffer->_xferImag();
	//pcoAcqStatus status = (pcoAcqStatus) m_buffer->_xferImagMult();

	m_sync->setExposing(status);
	//m_sync->stopAcq();
	char *msg = m_cam->_pcoSet_RecordingState(0, error);
	if(error) {
		printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__, msg);
		//throw LIMA_HW_EXC(Error, "_pcoSet_RecordingState");
	}

	//m_pcoData->traceAcqClean();
	m_pcoData->traceAcq.fnId = fnId;

	m_pcoData->msAcqXfer = msXfer = msElapsedTime(tStart);
	printf("=== %s> EXIT xfer[%ld] (ms) status[%s]\n", 
			fnId, msXfer, sPcoAcqStatus[status]);

	
	m_sync->setStarted(false); // updated

	_endthread();
}

//=====================================================================
//=====================================================================

void _pco_acq_thread_dimax_live(void *argin) {
	DEF_FNID;

	printf("=== %s> ENTRY\n", fnId);

	Camera* m_cam = (Camera *) argin;
	SyncCtrlObj* m_sync = m_cam->_getSyncCtrlObj();
	//BufferCtrlObj* m_buffer = m_sync->_getBufferCtrlObj();
	BufferCtrlObj* m_buffer = m_cam->_getBufferCtrlObj();

	struct stcPcoData *m_pcoData = m_cam->_getPcoData();

	struct __timeb64 tStart;
	msElapsedTimeSet(tStart);
	int error;
	long msXfer;
	int requestStop = stopNone;

	HANDLE m_handle = m_cam->getHandle();

	m_sync->setAcqFrames(0);

	// dimax recording time -> live NO record
	m_pcoData->msAcqRec  = 0;
	m_pcoData->msAcqRecTimestamp = getTimestamp();


	pcoAcqStatus status = (pcoAcqStatus) m_buffer->_xferImag();
	m_sync->setExposing(status);
	m_sync->stopAcq();
	char *msg = m_cam->_pcoSet_RecordingState(0, error);
	if(error) {
		printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__, msg);
		//throw LIMA_HW_EXC(Error, "_pcoSet_RecordingState");
	}

	// dimax xfer time
	m_pcoData->msAcqXfer = msXfer = msElapsedTime(tStart);
	m_pcoData->msAcqXferTimestamp = getTimestamp();
	printf("=== %s> EXIT xfer[%ld] (ms) status[%s]\n", 
			fnId, msXfer, sPcoAcqStatus[status]);

	m_sync->setStarted(false); // to test

	_endthread();
}

//=====================================================================
//=====================================================================
void _pco_acq_thread_ringBuffer(void *argin) {
	DEF_FNID;
	char _msg[LEN_MSG + 1];

	Camera* m_cam = (Camera *) argin;
	SyncCtrlObj* m_sync = m_cam->_getSyncCtrlObj();
	BufferCtrlObj* m_buffer = m_cam->_getBufferCtrlObj();

	sprintf_s(_msg, LEN_MSG, "%s> [ENTRY]", fnId);
	m_cam->_traceMsg(_msg);

	struct stcPcoData *m_pcoData = m_cam->_getPcoData();

	struct __timeb64 tStart;
	msElapsedTimeSet(tStart);

	LARGE_INTEGER usStart;
	usElapsedTimeSet(usStart);

	int error;
	long msXfer;
	int requestStop = stopNone;
	pcoAcqStatus status;

	HANDLE m_handle = m_cam->getHandle();

	m_sync->setAcqFrames(0);

	// traceAcq
	//m_pcoData->traceAcqClean();
	m_pcoData->traceAcq.fnId = fnId;
	double msPerFrame = (m_cam->pcoGetCocRunTime() * 1000.);
	m_pcoData->traceAcq.msImgCoc = msPerFrame;
	m_sync->getExpTime(m_pcoData->traceAcq.sExposure);
	m_sync->getLatTime(m_pcoData->traceAcq.sDelay);


	m_pcoData->msAcqRec  = 0;
	m_pcoData->msAcqRecTimestamp = getTimestamp();



	m_pcoData->traceAcq.usTicks[0].value = usElapsedTime(usStart);
	m_pcoData->traceAcq.usTicks[0].desc = "before xferImag execTime";
	
	usElapsedTimeSet(usStart);

	if(m_pcoData->testCmdMode & TESTCMDMODE_PCO2K_XFER_WAITOBJ) {
		status = (pcoAcqStatus) m_buffer->_xferImag();      //  <------------- uses WAITOBJ
	} else {
		status = (pcoAcqStatus) m_buffer->_xferImagMult();  //  <------------- USES PCO_GetImageEx (NO waitobj)   0x20
	}

	m_pcoData->traceAcq.usTicks[1].value = usElapsedTime(usStart);
	m_pcoData->traceAcq.usTicks[1].desc = "xferImag execTime";
	usElapsedTimeSet(usStart);

	
	m_sync->setExposing(status);

	m_pcoData->traceAcq.usTicks[2].value = usElapsedTime(usStart);
	m_pcoData->traceAcq.usTicks[2].desc = "sync->setExposing(status) execTime";
	usElapsedTimeSet(usStart);

	//m_sync->stopAcq();

	m_pcoData->traceAcq.usTicks[3].value = usElapsedTime(usStart);
	m_pcoData->traceAcq.usTicks[3].desc = "sync->stopAcq execTime";
	usElapsedTimeSet(usStart);

	char *msg = m_cam->_pcoSet_RecordingState(0, error);
	m_pcoData->traceAcq.usTicks[4].value = usElapsedTime(usStart);
	m_pcoData->traceAcq.usTicks[4].desc = "_pcoSet_RecordingState execTime";
	usElapsedTimeSet(usStart);

	if(error) {
		sprintf_s(_msg, LEN_MSG, "%s> [%d]> ERROR %s", fnId, __LINE__, msg);
		m_cam->_traceMsg(_msg);
		//throw LIMA_HW_EXC(Error, "_pcoSet_RecordingState");
	}

	// xfer time
	m_pcoData->msAcqXfer =
		m_pcoData->traceAcq.msXfer = 
		m_pcoData->traceAcq.msTotal = 
		msXfer =
		msElapsedTime(tStart);

	m_pcoData->traceAcq.endXferTimestamp =
		m_pcoData->msAcqXferTimestamp = 
		getTimestamp();

	sprintf_s(_msg, LEN_MSG, "%s> EXIT xfer[%ld] (ms) status[%s]", 
			fnId, msXfer, sPcoAcqStatus[status]);
	m_cam->_traceMsg(_msg);


	m_pcoData->traceAcq.usTicks[5].desc = "up to _endtrhead execTime";
	m_pcoData->traceAcq.usTicks[5].value = usElapsedTime(usStart);

	m_sync->setStarted(false); // to test

	_endthread();
}

//=====================================================================
//=====================================================================
void Camera::reset()
{
  DEB_MEMBER_FUNCT();

  DEB_ALWAYS() << "\n   RESET -----------\n";
  //_init();
}



//=========================================================================================================
//=========================================================================================================
#define LEN_TMP_MSG 256
int Camera::PcoCheckError(int line, char *file, int err, char *fn) {
	DEB_MEMBER_FUNCT();
	DEF_FNID;


	static char lastErrorMsg[500];
	static char tmpMsg[LEN_TMP_MSG+1];
	char *msg;
	size_t lg;

	sprintf_s(tmpMsg,LEN_TMP_MSG,"%s (%d)", fn, line);
	m_msgLog->add(tmpMsg);
	if (err != 0) {
		DWORD dwErr = err;
		m_pcoData->pcoError = err;
		msg = m_pcoData->pcoErrorMsg;

		PCO_GetErrorText(dwErr, msg, ERR_SIZE-14);
        
		lg = strlen(msg);
		sprintf_s(msg+lg,ERR_SIZE - lg, " [%s][%d]", file, line);

		if(err & PCO_ERROR_IS_WARNING) {
			DEB_WARNING() << fnId << ": --- WARNING - IGNORED --- " << DEB_VAR1(m_pcoData->pcoErrorMsg);
			//DEB_ALWAYS() << fnId << ": --- WARNING - IGNORED --- " << DEB_VAR1(m_pcoData->pcoErrorMsg);
			return 0;
		}
		DEB_ALWAYS() << fnId << ":\n   " << DEB_VAR1(msg);
		return (err);
	}
	return (err);
}


//=========================================================================================================
//=========================================================================================================
char* Camera::_PcoCheckError(int line, char *file, int err, int &error, char *fn) {
	static char lastErrorMsg[ERR_SIZE];
	static char tmpMsg[LEN_TMP_MSG+1];
	char *msg;
	size_t lg;

	sprintf_s(tmpMsg,LEN_TMP_MSG,"%s (%d)", fn, line);
	m_msgLog->add(tmpMsg);

	error = m_pcoData->pcoError = err;
	msg = m_pcoData->pcoErrorMsg;

	if (err != 0) {
		PCO_GetErrorText(err, lastErrorMsg, ERR_SIZE-14);
		strncpy_s(msg, ERR_SIZE, lastErrorMsg, _TRUNCATE); 

		lg = strlen(msg);
		sprintf_s(msg+lg,ERR_SIZE - lg, " [%s][%d]", file, line);

		return lastErrorMsg;
	}
	return NULL;
}

//=========================================================================================================
//=========================================================================================================
unsigned long Camera::pcoGetFramesMax(int segmentPco){
	DEF_FNID;

		int segmentArr = segmentPco-1;
		unsigned long framesMax;
		unsigned long xroisize,yroisize;
		unsigned long long pixPerFrame, pagesPerFrame;

		if(_isCameraType(Edge)) {
			return LONG_MAX;
		}



		if(!_isCameraType(Dimax | Pco2k | Pco4k)) {
			printf("=== %s> unknown camera type [%d]\n", fnId, _getCameraType());
			return -1;
		}

		if((segmentPco <1) ||(segmentPco > PCO_MAXSEGMENTS)) {
			printf("=== %s> ERROR segmentPco[%d]\n", fnId, segmentPco);
			return -1;
		}

		xroisize = m_RoiLima.getSize().getWidth();
		yroisize = m_RoiLima.getSize().getHeight();

		//xroisize = m_roi.x[1] - m_roi.x[0] + 1;
		//yroisize = m_roi.y[1] - m_roi.y[0] + 1;

		pixPerFrame = (unsigned long long)xroisize * (unsigned long long)yroisize;

		if(pixPerFrame <0) {
			printf("=== %s> ERROR pixPerFrame[%lld]\n", fnId, pixPerFrame);
			return -1;
		}

		if(m_pcoData->wPixPerPage < 1) {
			printf("=== %s> ERROR m_pcoData->wPixPerPage[%d]\n", fnId, m_pcoData->wPixPerPage);
			return -1;
		}
		pagesPerFrame = (pixPerFrame / m_pcoData->wPixPerPage) + 1;
		if(pixPerFrame % m_pcoData->wPixPerPage) pagesPerFrame++;

		framesMax = m_pcoData->dwMaxFramesInSegment[segmentArr] = (unsigned long)(((long long) m_pcoData->dwSegmentSize[segmentArr] ) / pagesPerFrame);

		return framesMax;
	}


//=================================================================================================
//=================================================================================================
char * Camera::_pco_SetTriggerMode_SetAcquireMode(int &error){
	DEB_MEMBER_FUNCT();
	char *msg;
	
	DEF_FNID;
	//------------------------------------------------- triggering mode 
	WORD trigmode = m_sync->xlatLimaTrigMode2PcoTrigMode(m_pcoData->bExtTrigEnabled);
    PCO_FN2(error, msg,PCO_SetTriggerMode , m_handle, trigmode);
	if(error) return msg;
	//PCO_THROW_OR_TRACE(error, "PCO_SetTriggerMode") ;
	//DEB_TRACE() << DEB_VAR1(trigmode);

    //------------------------------------- acquire mode : ignore or not ext. signal

	WORD acqmode = m_sync->xlatLimaTrigMode2PcoAcqMode();
	PCO_FN2(error, msg,PCO_SetAcquireMode , m_handle, acqmode);
	if(error) return msg;
   //PCO_THROW_OR_TRACE(error, "PCO_SetAcquireMode") ;
	return fnId;
}



//=================================================================================================
// ----------------------------------------- storage mode (recorder + sequence)
// current storage mode
//
// case RecSeq
// case RecRing
// - 0x0000 = [recorder] mode
//		. images are recorded and stored within the internal camera memory (camRAM)
//      . Live View transfers the most recent image to the PC (for viewing / monitoring)
//      . indexed or total image readout after the recording has been stopped
//
// case Fifo
// - 0x0001 = [FIFO buffer] mode
//      . all images taken are transferred to the PC in chronological order
//      . camera memory (camRAM) is used as huge FIFO buffer to bypass short bottlenecks in data transmission
//      . if buffer overflows, the oldest images are overwritten
//      . if Set Recorder = [stop] is sent, recording is stopped and the transfer of the current image to the PC is finished.
//      . Images not read are stored within the segment and can be read with the Read Image From Segment command.
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
//      . recording is stopped by software or disabling acquire signal (<acq enbl>)
//=================================================================================================
char * Camera::_pco_SetStorageMode_SetRecorderSubmode(enumPcoStorageMode mode, int &error){
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	char *msg;

	switch(mode) {
		case RecSeq:  m_pcoData->storage_mode = 0; m_pcoData->recorder_submode = 0; break;
		case RecRing: m_pcoData->storage_mode = 0; m_pcoData->recorder_submode = 1; break;
		case Fifo:    m_pcoData->storage_mode = 1; m_pcoData->recorder_submode = 0; break;
		default: 
			throw LIMA_HW_EXC(Error,"FATAL - invalid storage mode!" );
	}

    PCO_FN2(error, msg,PCO_SetStorageMode, m_handle, m_pcoData->storage_mode);
	if(error) return msg;
    //PCO_THROW_OR_TRACE(error, msg) ;

    PCO_FN2(error, msg,PCO_SetRecorderSubmode, m_handle, m_pcoData->recorder_submode);
	if(error) return msg;
    //PCO_THROW_OR_TRACE(error, msg) ;

	return fnId;
}

int Camera::_pco_GetStorageMode_GetRecorderSubmode(){
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	char *msg;

	WORD wStorageMode, wRecSubmode;
	int error;

    PCO_FN2(error, msg,PCO_GetStorageMode, m_handle, &wStorageMode);
	if(error){ 
		    PCO_THROW_OR_TRACE(error, msg) ;
	}


    PCO_FN2(error, msg,PCO_GetRecorderSubmode, m_handle, &wRecSubmode);
	if(error) {
		    PCO_THROW_OR_TRACE(error, msg) ;
	}

	if((wStorageMode == 0) && (wRecSubmode == 0)) return RecSeq;
	if((wStorageMode == 0) && (wRecSubmode == 1)) return RecRing;
	if((wStorageMode == 1) && (wRecSubmode == 0)) return Fifo;

	return RecInvalid;
}

//=================================================================================================
//=================================================================================================

// 4294967295.0 = double(DWORD(0xFFFFFFFF)) 
#define DWORD_MAX_FLOAT 4294967295.0

#define	MAX_DWORD_MS (double(4294967295.0e-3))
#define	MAX_DWORD_US (double(4294967295.0e-6))
#define	MAX_DWORD_NS (double(4294967295.0e-9))


void _pco_time2dwbase(double exp_time, DWORD &dwExp, WORD &wBase) {
	// conversion time(s) to PCO standard DWORD + UNIT(ms, us, ns)
		// exp & lat time is saved in seconds (LIMA). 
		// PCO requires them expressed in DWORD as ms(base=2), us(base=1) or ns(base=0)
		// max DWORD 0xFFFFFFFF = 4294967295.0
		// find the lowest unit (ns -> us -> ms) which does not overflow DWORD
    
	if(exp_time <= MAX_DWORD_NS) {   
		dwExp = DWORD(exp_time * 1.0e9); wBase = 0; // ns
	} else 	if(exp_time <= MAX_DWORD_US) {  
		dwExp = DWORD(exp_time * 1.0e6); wBase = 1; // us
	} else {  
		dwExp = DWORD(exp_time * 1.0e3); wBase = 2; // ms
	}

	DWORD mask = 0x7;
	DWORD min = 0x1000;

	if(dwExp > min){
		dwExp |= mask;
		dwExp ^= mask;
	}

	return;
}


//=================================================================================================
//=================================================================================================
char* Camera::_pco_SetDelayExposureTime(int &error, int ph){
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	bool doIt;
	char *msg;


    DWORD dwExposure, dwDelay;
	WORD wExposure_base, wDelay_base;
    double _exposure, _delay;
    m_sync->getExpTime(_exposure);
    m_sync->getLatTime(_delay);
	double _delay0 = _delay;

	doIt = TRUE;
	
	if(ph != 0){ 
		doIt = FALSE;

		if((_isCameraType(Edge)) && (m_pcoData->dwPixelRate >= PCO_EDGE_PIXEL_RATE_HIGH) ) {
			double pixels = ((double) m_pcoData->wXResActual)* ((double) m_pcoData->wYResActual);
			double bytes = (m_pcoData->wLUT_Identifier == PCO_EDGE_LUT_SQRT) ? 1.5 : 2.0;
			double period = bytes * pixels / (m_pcoData->fTransferRateMHzMax * 1000000.);

			printf("--- %s>period[%g] -> cocRunTime[%g]\n", fnId, period , m_pcoData->cocRunTime);
			if(period > m_pcoData->cocRunTime) {
				_delay += period - m_pcoData->cocRunTime;
				doIt = TRUE;
				printf("--- %s> delay forced [%g] -> [%g]\n", fnId, _delay0, _delay);
			}
		}
	}

	if(!doIt) return fnId;

	_pco_time2dwbase(_exposure, dwExposure, wExposure_base);
	_pco_time2dwbase(_delay,  dwDelay, wDelay_base);

	PCO_FN5(error, msg,PCO_SetDelayExposureTime, m_handle, dwDelay, dwExposure, wDelay_base, wExposure_base);

	if(error || _getDebug(DBG_EXP)) {
		DEB_ALWAYS() << DEB_VAR3(_exposure, dwExposure, wExposure_base);
		DEB_ALWAYS() << DEB_VAR3(_delay,  dwDelay, wDelay_base);
	}

	if(error) {
		return msg;
	}

	return fnId;
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

  DWORD  TriggerSystemJitter_ns;       // Max. possible trigger jitter -0/+ ... ns

  DWORD  TriggerDelay_ns;              // Resulting trigger delay = system delay
  DWORD  TriggerDelay_s;               // + delay of SetDelayExposureTime ... // 9

} PCO_ImageTiming;
******************************************************************************************/


int Camera::_pco_GetImageTiming(double &frameTime, double &expTime, double &sysDelay, double &sysJitter, double &trigDelay ){
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	char *msg;


	int error;

	PCO_ImageTiming pstrImageTiming;

	PCO_FN2(error, msg,PCO_GetImageTiming, m_handle, &pstrImageTiming);

	frameTime = (pstrImageTiming.FrameTime_ns * NANO) + pstrImageTiming.FrameTime_s ;
	expTime = (pstrImageTiming.ExposureTime_ns * NANO) + pstrImageTiming.ExposureTime_s ;
	sysDelay = (pstrImageTiming.TriggerSystemDelay_ns * NANO) ;
	sysJitter = (pstrImageTiming.TriggerSystemJitter_ns * NANO) ;
	trigDelay = (pstrImageTiming.TriggerDelay_ns * NANO) + pstrImageTiming.TriggerDelay_s ;



	return error;
}
//=================================================================================================
//=================================================================================================
char *Camera::_pco_SetCamLinkSetImageParameters(int &error){
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	char *pcoFn;

	// camLink -> imgPar
	// GigE    -> imgPar 
	switch (m_pcoData->stcPcoCamType.wInterfaceType) {
        case INTERFACE_CAMERALINK: 
		case INTERFACE_ETHERNET:
		    WORD wXres, wYres;

            wXres= m_pcoData->wXResActual;
            wYres= m_pcoData->wYResActual;
			
			DEB_ALWAYS() << "PCO_CamLinkSetImageParameters: " <<  DEB_VAR2(wXres, wYres);

			PCO_FN3(error, pcoFn,PCO_CamLinkSetImageParameters, m_handle, wXres, wYres);
			if(error) { throw LIMA_HW_EXC(Error, pcoFn); }

        default: break;
    } // switch


	return fnId;
}



//=================================================================================================
//=================================================================================================
char *Camera::_pco_SetTransferParameter_SetActiveLookupTable(int &error){
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	struct stcPcoData _pcoData;
	char msg[ERRMSG_SIZE + 1];
	char *pcoFn;
	bool doLut = false;

	// sizes are only updated AFTER arm, so i will use the last roi settings
	// to get the size. after arm, the size is updated with this value
	WORD wXResActual = m_pcoData->wRoiX1Now - m_pcoData->wRoiX0Now +1;

	//================================================================================================
	// PCO_SetTransferParameter
	//================================================================================================

	if (m_pcoData->stcPcoCamType.wInterfaceType==INTERFACE_CAMERALINK) 
	{
		PCO_FN3(error, pcoFn,PCO_GetTransferParameter, m_handle, &m_pcoData->clTransferParam, sizeof(PCO_SC2_CL_TRANSFER_PARAM));
		PCO_THROW_OR_TRACE(error, pcoFn) ;
	
		memcpy(&_pcoData.clTransferParam, &m_pcoData->clTransferParam,sizeof(PCO_SC2_CL_TRANSFER_PARAM));
	
		m_pcoData->clTransferParam.baudrate = PCO_CL_BAUDRATE_115K2;

		if(_isCameraType(Dimax)){
				//m_pcoData->clTransferParam.Transmit = 1;
				//_pcoData.clTransferParam.Transmit = m_pcoData->clTransferParam.Transmit;
				m_pcoData->clTransferParam.DataFormat=PCO_CL_DATAFORMAT_2x12; //=2
		} else 

		if(_isCameraType(EdgeGL)) {
			m_pcoData->clTransferParam.Transmit = 1;
			m_pcoData->clTransferParam.DataFormat=PCO_CL_DATAFORMAT_5x12 | 
				SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER;
				//SCCMOS_FORMAT_TOP_BOTTOM;
			m_pcoData->wLUT_Identifier = 0; //Switch LUT->off
			doLut = true;
		} else 
			
		if(_isCameraType(EdgeRolling)){
				m_pcoData->clTransferParam.Transmit = 1;

				if(m_pcoData->dwPixelRate <= PCO_EDGE_PIXEL_RATE_LOW){
					m_pcoData->clTransferParam.DataFormat=PCO_CL_DATAFORMAT_5x16 | 
						SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER;
					m_pcoData->wLUT_Identifier = PCO_EDGE_LUT_NONE; // Switch LUT->off
				} else 
				if( ((m_pcoData->dwPixelRate >= PCO_EDGE_PIXEL_RATE_HIGH) & 
						(wXResActual > PCO_EDGE_WIDTH_HIGH))) {
					m_pcoData->clTransferParam.DataFormat=PCO_CL_DATAFORMAT_5x12L | 
						SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER;
					m_pcoData->wLUT_Identifier = PCO_EDGE_LUT_SQRT; //Switch LUT->sqrt
				} else {
					m_pcoData->clTransferParam.DataFormat = PCO_CL_DATAFORMAT_5x16 | 
						SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER;
					m_pcoData->wLUT_Identifier = PCO_EDGE_LUT_NONE; // Switch LUT->off
				}
				doLut = true;
		} 

		if((_pcoData.clTransferParam.baudrate != m_pcoData->clTransferParam.baudrate) ||
			(_pcoData.clTransferParam.DataFormat != m_pcoData->clTransferParam.DataFormat) ||
			(_pcoData.clTransferParam.Transmit != m_pcoData->clTransferParam.Transmit)	)
		{
			DEB_ALWAYS() << "PCO_SetTransferParameter (ckTransferParam)" ;
			PCO_FN3(error, pcoFn,PCO_SetTransferParameter,m_handle, &m_pcoData->clTransferParam, sizeof(m_pcoData->clTransferParam));
			if(error){
				sprintf_s(msg,ERRMSG_SIZE, "PCO_SetTransferParameter - baudrate[%d][%d] dataFormat[x%08x][x%08x] trasmit[%d][%d]",
					_pcoData.clTransferParam.baudrate, m_pcoData->clTransferParam.baudrate,
					_pcoData.clTransferParam.DataFormat, m_pcoData->clTransferParam.DataFormat,
					_pcoData.clTransferParam.Transmit, m_pcoData->clTransferParam.Transmit);
				throw LIMA_HW_EXC(Error, msg); 
			} 
			_armRequired(true);
		}
	} // if cameralink



	//================================================================================================
	// PCO_SetActiveLookupTable
	//================================================================================================

	if(doLut) {
		WORD _wLUT_Identifier, _wLUT_Parameter;

		PCO_FN3(error, pcoFn,PCO_GetActiveLookupTable, m_handle, &_wLUT_Identifier, &_wLUT_Parameter);
	    PCO_THROW_OR_TRACE(error, pcoFn) ;

		if(_wLUT_Identifier != m_pcoData->wLUT_Identifier) {
			PCO_FN3(error, pcoFn,PCO_SetActiveLookupTable, m_handle, &m_pcoData->wLUT_Identifier, &m_pcoData->wLUT_Parameter);
		    PCO_THROW_OR_TRACE(error, pcoFn) ;
			_armRequired(true);

			PCO_FN3(error, pcoFn,PCO_GetActiveLookupTable, m_handle, &m_pcoData->wLUT_Identifier, &m_pcoData->wLUT_Parameter);
		    PCO_THROW_OR_TRACE(error, pcoFn) ;
		}
	}



	return fnId;
}

//=================================================================================================
//=================================================================================================
void Camera::getArmWidthHeight(WORD& width,WORD& height)
{
	DEB_MEMBER_FUNCT();
	DEF_FNID;

	width = m_pcoData->wXResActual;
	height = m_pcoData->wYResActual;
}



//=================================================================================================
//=================================================================================================
char *Camera::_pco_GetCameraType(int &error){
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	char *msg;
	m_pcoData->frames_per_buffer = 1; // for PCO DIMAX

	// --- Get camera type
	{
		char *ptr;
		int errTot = 0;

		//m_pcoData->stcPcoCamType.wSize= sizeof(m_pcoData->stcPcoCamType);

		error = PCO_GetGigEIPAddress(m_handle, &m_pcoData->ipField[0], &m_pcoData->ipField[1], &m_pcoData->ipField[2], &m_pcoData->ipField[3]);
		if(error) {m_pcoData->ipField[0] = m_pcoData->ipField[1] = m_pcoData->ipField[2] =m_pcoData->ipField[3]= 0;}

		PCO_FN2(error, msg,PCO_GetCameraType, m_handle, &m_pcoData->stcPcoCamType);
		PCO_PRINT_ERR(error, msg); 	if(error) return msg;

		ptr = xlatPcoCode2Str(_getCameraType(), ModelType, error);
		strcpy_s(m_pcoData->model, MODEL_TYPE_SIZE, ptr);
		errTot |= error;

		ptr = xlatPcoCode2Str(m_pcoData->stcPcoCamType.wInterfaceType, InterfaceType, error);
		strcpy_s(m_pcoData->iface, INTERFACE_TYPE_SIZE, ptr);
		errTot |= error;

		sprintf_s(m_pcoData->camera_name, CAMERA_NAME_SIZE, "%s %s (SN %d)", 
			m_pcoData->model, m_pcoData->iface, m_pcoData->stcPcoCamType.dwSerialNumber);
		DEB_ALWAYS() <<  DEB_VAR3(m_pcoData->model, m_pcoData->iface, m_pcoData->camera_name);

		if(errTot) return m_pcoData->camera_name;

	}

	// -- Reset to default settings

	PCO_FN2(error, msg,PCO_SetRecordingState, m_handle, 0);
	if(error) return msg;


	PCO_FN1(error, msg,PCO_ResetSettingsToDefault, m_handle);
	PCO_PRINT_ERR(error, msg); 	if(error) return msg;
	

	// -- Get camera description
	//m_pcoData->stcPcoDescription.wSize= sizeof(m_pcoData->stcPcoDescription);

	PCO_FN2(error, msg,PCO_GetCameraDescription, m_handle, &m_pcoData->stcPcoDescription);
	PCO_PRINT_ERR(error, msg); 	if(error) return msg;

	// callback to update in lima the valid_ranges from the last stcPcoDescription read
	if(m_sync) {
		HwSyncCtrlObj::ValidRangesType valid_ranges;
		m_sync->getValidRanges(valid_ranges);		// from stcPcoDescription
		m_sync->validRangesChanged(valid_ranges);	// callback
		DEB_ALWAYS() << fnId << ": callback - new valid_ranges: " << DEB_VAR1(valid_ranges);
	}
	
    // get the max CAMERA pixel rate (Hz) from the description structure
	m_pcoData->dwPixelRateMax = 0;
	for(int i=0; i<4; i++) {
		if(m_pcoData->dwPixelRateMax < m_pcoData->stcPcoDescription.dwPixelRateDESC[i])
					m_pcoData->dwPixelRateMax = m_pcoData->stcPcoDescription.dwPixelRateDESC[i];
	}	

	m_pcoData->bMetaDataAllowed = !!(m_pcoData->stcPcoDescription.dwGeneralCapsDESC1 & GENERALCAPS1_METADATA) ;


	// -- Get General
	//m_pcoData->stcPcoGeneral.wSize= sizeof(m_pcoData->stcPcoGeneral);
	//m_pcoData->stcPcoGeneral.strCamType.wSize= sizeof(m_pcoData->stcPcoGeneral.strCamType);

	PCO_FN2(error, msg,PCO_GetGeneral,m_handle, &m_pcoData->stcPcoGeneral);
	PCO_PRINT_ERR(error, msg); 	if(error) return msg;


	// -- Get Sensor struct
	//m_pcoData->stcPcoSensor.wSize= sizeof(m_pcoData->stcPcoSensor);
	//m_pcoData->stcPcoSensor.strDescription.wSize= sizeof(m_pcoData->stcPcoSensor.strDescription);
	//m_pcoData->stcPcoSensor.strDescription2.wSize= sizeof(m_pcoData->stcPcoSensor.strDescription2);

	PCO_FN2(error, msg,PCO_GetSensorStruct, m_handle, &m_pcoData->stcPcoSensor);
	PCO_PRINT_ERR(error, msg); 	if(error) return msg;

	// -- Get timing struct
	//m_pcoData->stcPcoTiming.wSize= sizeof(m_pcoData->stcPcoTiming);

	PCO_FN2(error, msg,PCO_GetTimingStruct, m_handle, &m_pcoData->stcPcoTiming);
	PCO_PRINT_ERR(error, msg); 	if(error) return msg;


	// -- Get recording struct
	//m_pcoData->stcPcoRecording.wSize= sizeof(m_pcoData->stcPcoRecording);

	PCO_FN2(error, msg,PCO_GetRecordingStruct,m_handle, &m_pcoData->stcPcoRecording);
	PCO_PRINT_ERR(error, msg); 	if(error) return msg;


	// -- Get storage struct
	//m_pcoData->stcPcoStorage.wSize= sizeof(m_pcoData->stcPcoStorage);

	PCO_FN2(error, msg,PCO_GetStorageStruct, m_handle, &m_pcoData->stcPcoStorage);
	PCO_PRINT_ERR(error, msg); 	if(error) return msg;



	return fnId;
}



//=================================================================================================
//=================================================================================================
char *Camera::_pco_GetTemperatureInfo(int &error){
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	char msg[MSG_SIZE + 1];
	char *pcoFn;



	// -- Print out current temperatures
	PCO_FN4(error, pcoFn,PCO_GetTemperature, m_handle, &m_pcoData->temperature.wCcd, &m_pcoData->temperature.wCam, &m_pcoData->temperature.wPower);
	if(error) return pcoFn;
	//PCO_THROW_OR_TRACE(error, "PCO_GetTemperature") ;

	sprintf_s(msg, MSG_SIZE, "* temperature: CCD[%.1f]  CAM[%d]  PS[%d]\n", m_pcoData->temperature.wCcd/10., m_pcoData->temperature.wCam, m_pcoData->temperature.wPower);
	//DEB_TRACE() <<   msg;
	m_log.append(msg);


	m_pcoData->temperature.wMinCoolSet = m_pcoData->stcPcoDescription.sMinCoolSetDESC;
	m_pcoData->temperature.wMaxCoolSet = m_pcoData->stcPcoDescription.sMaxCoolSetDESC;

	sprintf_s(msg, MSG_SIZE, "* cooling temperature: MIN [%d]  Max [%d]\n",  m_pcoData->temperature.wMinCoolSet, m_pcoData->temperature.wMaxCoolSet);
	//DEB_TRACE() <<   msg;
	m_log.append(msg);

	// -- Set/Get cooling temperature
	if (m_pcoData->temperature.wSetpoint != -1) {
		if (m_pcoData->temperature.wSetpoint < m_pcoData->temperature.wMinCoolSet)	m_pcoData->temperature.wSetpoint = m_pcoData->temperature.wMinCoolSet;
		if (m_pcoData->temperature.wSetpoint > m_pcoData->temperature.wMaxCoolSet)	m_pcoData->temperature.wSetpoint= m_pcoData->temperature.wMaxCoolSet;
	} else {
		PCO_FN2(error, pcoFn,PCO_GetCoolingSetpointTemperature, m_handle, &m_pcoData->temperature.wSetpoint);
		if(error) return pcoFn;
		//PCO_THROW_OR_TRACE(error, "PCO_GetCoolingSetpointTemperature") ;
	}
	sprintf_s(msg, MSG_SIZE, "* Cooling Setpoint = %d\n", m_pcoData->temperature.wSetpoint);
	//DEB_TRACE() <<   msg;
	m_log.append(msg);


	return fnId;
}


//=================================================================================================
//=================================================================================================

/**************************************************************************************************
	If a set recording status = [stop] command is sent and the current status is already
	[stop]’ped, nothing will happen (only warning, error message). 
	
	If the camera is in
	[run]’ing state, it will last some time (system delay + last image readout), until the
	camera is stopped. The system delay depends on the PC and the image readout
	depends on the image size transferred. The SetRecordingState = [stop] checks for a
	stable stop state by calling GetRecordingState.  --- 165 ms 
	
	Please call PCO_CancelImages to remove pending buffers from the driver.   --- 1.5 s
**************************************************************************************************/

char * Camera::_pcoSet_RecordingState(int state, int &error){
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	char *msg;
	LARGE_INTEGER usStart;


	WORD wRecState_new, wRecState_actual;

	wRecState_new = state ? 0x0001 : 0x0000 ; // 0x0001 => START acquisition

	usElapsedTimeSet(usStart);

	PCO_FN2(error, msg,PCO_GetRecordingState, m_handle, &wRecState_actual);
	PCO_PRINT_ERR(error, msg); 	if(error) return msg;

	m_pcoData->traceAcq.usTicks[8].value = usElapsedTime(usStart);
	m_pcoData->traceAcq.usTicks[8].desc = "PCO_GetRecordingState execTime";
	usElapsedTimeSet(usStart);

	//if(wRecState_new == wRecState_actual) {error = 0; return fnId; }

	PCO_FN2(error, msg,PCO_SetRecordingState, m_handle, wRecState_new);
	PCO_PRINT_ERR(error, msg); 	if(error) return msg;

	m_pcoData->traceAcq.usTicks[9].value = usElapsedTime(usStart);
	m_pcoData->traceAcq.usTicks[9].desc = "PCO_SetRecordingState execTime";
	usElapsedTimeSet(usStart);

	_armRequired(true);

	if(wRecState_new == 0) {
		int count;

		PCO_FN2(error, msg,PCO_GetPendingBuffer, m_handle, &count);
		PCO_PRINT_ERR(error, msg); 	if(error) return msg;

		if(count) {
			PCO_FN1(error, msg,PCO_CancelImages, m_handle);
			PCO_PRINT_ERR(error, msg); 	if(error) return msg;
		}
	}

	m_pcoData->traceAcq.usTicks[10].value = usElapsedTime(usStart);
	m_pcoData->traceAcq.usTicks[10].desc = "PCO_CancelImages execTime";
	usElapsedTimeSet(usStart);

	DEB_ALWAYS() << fnId << ": " << DEB_VAR4(error, state, wRecState_actual, wRecState_new);
	return fnId;

}

//=================================================================================================
//=================================================================================================
int Camera::dumpRecordedImages(int &nrImages, int &error){
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	char *msg;

	HANDLE m_handle = getHandle();
	WORD wSegment = pcoGetActiveRamSegment(); 
	DWORD _dwValidImageCnt, _dwMaxImageCnt;


	WORD wRecState_actual;

	nrImages = -1;

	if(!_isCameraType(Dimax)) return -2;

	PCO_FN2(error, msg,PCO_GetRecordingState, m_handle, &wRecState_actual);
	PCO_PRINT_ERR(error, msg); 	
	
	if (error) return -100;
	if(wRecState_actual != 0) return -1;


	msg = _PcoCheckError(__LINE__, __FILE__, PCO_GetNumberOfImagesInSegment(m_handle, wSegment, &_dwValidImageCnt, &_dwMaxImageCnt), error);
	if(error) {
		printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__, msg);
		throw LIMA_HW_EXC(Error, "PCO_GetNumberOfImagesInSegment");
	}

	nrImages = _dwValidImageCnt;

	return 0;

}


//=================================================================================================
//=================================================================================================
char *Camera::_pco_GetCOCRuntime(int &error){
		
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	char *msg;

	//====================================== get the coc runtime 
    //---- only valid if it was used PCO_SetDelayExposureTime
	//---- and AFTER armed the cam

	// Get and split the 'camera operation code' runtime into two DWORD. One will hold the longer
	// part, in seconds, and the other will hold the shorter part, in nanoseconds. This function can be
	// used to calculate the FPS. The sum of dwTime_s and dwTime_ns covers the delay, exposure and
	// readout time. If external exposure is active, it returns only the readout time.

	DWORD dwTime_s, dwTime_ns;
    double runTime;

    PCO_FN3(error, msg,PCO_GetCOCRuntime, m_handle, &dwTime_s, &dwTime_ns);
    PCO_PRINT_ERR(error, msg); 	if(error) return msg;

    m_pcoData->cocRunTime = runTime = ((double) dwTime_ns * NANO) + (double) dwTime_s;
    m_pcoData->frameRate = (dwTime_ns | dwTime_s) ? 1.0 / runTime : 0.0;

    DEB_TRACE() << DEB_VAR2(m_pcoData->frameRate, m_pcoData->cocRunTime);

	return fnId;

}

//=================================================================================================
//=================================================================================================
char *Camera::_pco_SetMetaDataMode(WORD wMetaDataMode, int &error){
		
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	char *msg;

	m_pcoData->wMetaDataSize = m_pcoData->wMetaDataVersion = 0;
	if(_isCameraType(Dimax)) {
		m_pcoData->wMetaDataMode = wMetaDataMode;
		PCO_FN4(error, msg,PCO_SetMetaDataMode, m_handle, wMetaDataMode, &m_pcoData->wMetaDataSize, &m_pcoData->wMetaDataVersion);
		PCO_PRINT_ERR(error, msg); 	if(error) return msg;
	}
	return fnId;
}





void Camera::_set_shutter_rolling_edge(bool rolling, int &error){
		
	DEB_MEMBER_FUNCT();


	error = 0;

	if(!_isCameraType(Edge)) {
		error = -1;
		return ;
	}

	m_pcoData->bRollingShutter = rolling;


	_beginthread( _pco_shutter_thread_edge, 0, (void*) this);

	return;

}

//=================================================================================================
//=================================================================================================
void Camera::_pco_set_shutter_rolling_edge(int &error){
		
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	char *msg;
	char msgBuff[MSG_SIZE+1];

	DWORD _dwSetup;
	DWORD m_dwSetup[10];
	WORD m_wLen = 10;
	WORD m_wType;

	// PCO recommended timing values
	int ts[3] = {2000, 3000, 250}; // command, image, channel timeout
	DWORD sleepMs = 10000;  // sleep time after reboot

	if(!_isCameraType(Edge)) {
		return ;
	}

	DEB_ALWAYS() << fnId << " [entry - edge] ";

	m_config = TRUE;

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
	// #define PCO_EDGE_SETUP_ROLLING_SHUTTER 0x00000001         // rolling shutter
	// #define PCO_EDGE_SETUP_GLOBAL_SHUTTER  0x00000002         // global shutter

	
	_dwSetup = m_pcoData->bRollingShutter ? PCO_EDGE_SETUP_ROLLING_SHUTTER : PCO_EDGE_SETUP_GLOBAL_SHUTTER;

    PCO_FN4(error, msg,PCO_GetCameraSetup, m_handle, &m_wType, &m_dwSetup[0], &m_wLen);
    PCO_PRINT_ERR(error, msg); 	if(error) return;

	if(m_dwSetup[0] == _dwSetup) { 
		DEB_ALWAYS() << fnId << " [exit - no change] ";
		m_config = FALSE;
		return;
	}

	msg = msgBuff;
	sprintf_s(msg, MSG_SIZE, "[Change ROLLING SHUTTER from [%d] to [%d]]", 
		m_dwSetup[0]==PCO_EDGE_SETUP_ROLLING_SHUTTER, _dwSetup==PCO_EDGE_SETUP_ROLLING_SHUTTER);
	DEB_ALWAYS() << fnId << " " << msg;

	m_dwSetup[0] = _dwSetup;

    PCO_FN3(error, msg,PCO_SetTimeouts, m_handle, &ts[0], sizeof(ts));
    PCO_PRINT_ERR(error, msg); 	if(error) return;

	msg = "[PCO_SetCameraSetup]";
	DEB_ALWAYS() << fnId << " " << msg;
    PCO_FN4(error, msg,PCO_SetCameraSetup, m_handle, m_wType, &m_dwSetup[0], m_wLen);
    PCO_PRINT_ERR(error, msg); 	if(error) return;

	msg = "[PCO_RebootCamera]";
	DEB_ALWAYS() << fnId << " " << msg;
    PCO_FN1(error, msg,PCO_RebootCamera, m_handle);
    PCO_PRINT_ERR(error, msg); 	if(error) return;

	//m_sync->_getBufferCtrlObj()->_pcoAllocBuffersFree();
	m_buffer->_pcoAllocBuffersFree();

	msg = "[PCO_CloseCamera]";
	DEB_ALWAYS() << fnId << " " << msg;
    PCO_FN1(error, msg,PCO_CloseCamera, m_handle);
    PCO_PRINT_ERR(error, msg); 	if(error) return;
	m_handle = NULL;
	
	msg = msgBuff;
	sprintf_s(msg, MSG_SIZE, "[Sleep %d ms]", sleepMs);
	DEB_ALWAYS() << fnId << " " << msg;
	::Sleep(sleepMs);

	_init();

	DEB_ALWAYS() << fnId << " [exit] ";

	m_config = FALSE;
	return;

}

//=================================================================================================
//=================================================================================================
bool Camera::_get_shutter_rolling_edge(int &error){
		
	DEB_MEMBER_FUNCT();
	DEF_FNID;

	DWORD m_dwSetup[10];
	WORD m_wLen = 10;
	WORD m_wType;
	char *msg;


    PCO_FN4(error, msg,PCO_GetCameraSetup, m_handle, &m_wType, &m_dwSetup[0], &m_wLen);
    PCO_PRINT_ERR(error, msg); 	if(error) return FALSE;

	return (m_dwSetup[0] == PCO_EDGE_SETUP_ROLLING_SHUTTER);

}
//=================================================================================================
//=================================================================================================
bool Camera::_isValid_pixelRate(DWORD dwPixelRate){
		
	DEB_MEMBER_FUNCT();
	DEF_FNID;

	// pixelrate 1     (long word; frequency in Hz)
	// pixelrate 2,3,4 (long word; frequency in Hz; if not available, then value = 0)

	if(dwPixelRate > 0) 
		for(int i = 0; i < 4; i++) {			
			if(dwPixelRate == m_pcoData->stcPcoDescription.dwPixelRateDESC[i]) return TRUE;
		}

	return FALSE;
}



//=================================================================================================
//=================================================================================================

void Camera::getXYsteps(unsigned int &xSteps, unsigned int &ySteps){
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	
	xSteps = m_pcoData->stcPcoDescription.wRoiHorStepsDESC;
	ySteps = m_pcoData->stcPcoDescription.wRoiVertStepsDESC;
}
        
void Camera::getMaxWidthHeight(unsigned int &xMax, unsigned int &yMax){
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	xMax = m_pcoData->stcPcoDescription.wMaxHorzResStdDESC;
	yMax = m_pcoData->stcPcoDescription.wMaxVertResStdDESC;
}
	
void Camera::getMaxWidthHeight(DWORD &xMax, DWORD &yMax){
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	xMax = m_pcoData->stcPcoDescription.wMaxHorzResStdDESC;
	yMax = m_pcoData->stcPcoDescription.wMaxVertResStdDESC;
}


void Camera::getBytesPerPixel(unsigned int& pixbytes){
	pixbytes = (m_pcoData->stcPcoDescription.wDynResDESC <= 8)?1:2;
}

void Camera::getBitsPerPixel(WORD& pixbits){
	pixbits = m_pcoData->stcPcoDescription.wDynResDESC;
}


/****************************************************************************************
 Some sensors have a ROI stepping. See the camera description and check the parameters
 wRoiHorStepsDESC and/or wRoiVertStepsDESC.

 For dual ADC mode the horizontal ROI must be symmetrical. For a pco.dimax the horizontal and
 vertical ROI must be symmetrical. For a pco.edge the vertical ROI must be symmetrical.
****************************************************************************************/

int Camera::_checkValidRoi(const Roi &roi_new, Roi &roi_fixed){
		
	DEB_MEMBER_FUNCT();
	DEF_FNID;

	int iInvalid;
	unsigned int x0, x1, y0, y1;
	unsigned int x0org, x1org, y0org, y1org;
	unsigned int diff0, diff1, tmp;

	unsigned int xMax, yMax, xSteps, ySteps;
	getMaxWidthHeight(xMax, yMax);
	getXYsteps(xSteps, ySteps);

	x0org = x0 = roi_new.getTopLeft().x+1;
	x1org = x1 = roi_new.getBottomRight().x+1;
	y0org = y0 = roi_new.getTopLeft().y+1;
	y1org = y1 = roi_new.getBottomRight().y+1;

	// lima roi [0,2047]
	//  pco roi [1,2048]

	iInvalid = 0;

	if(x0 < 1) {x0 = 1 ; iInvalid |= Xrange;}
	if(x1 > xMax) {x1 = xMax ; iInvalid |= Xrange;}
	if(x0 > x1) { tmp = x0 ; x0 = x1 ; x1 = tmp;  iInvalid |= Xrange; }

	if ( (diff0 = (x0 - 1) % xSteps) != 0 ) { x0 -= diff0; iInvalid |= Xsteps; }
	if ( (diff1 = x1 % xSteps) != 0 ) { x1 += xSteps - diff1; iInvalid |= Xsteps; }

	if(y0 < 1) {y0 = 1 ; iInvalid |= Yrange;}
	if(y1 > yMax) {y1 = yMax ; iInvalid |= Yrange;}
	if(y0 > y1) { tmp = y0 ; y0 = y1 ; y1 = tmp;  iInvalid |= Yrange; }

	if ( (diff0 = (y0 - 1) % ySteps) != 0 ) { y0 -= diff0; iInvalid |= Ysteps; }
	if ( (diff1 = y1 % ySteps) != 0 ) { y1 += ySteps - diff1; iInvalid |= Ysteps; }


	bool bSymX = false, bSymY = false;
	if(_isCameraType(Dimax)){ bSymX = bSymY = true; }
	if(_isCameraType(Edge)) { bSymY = true; }

	int adc_working, adc_max;
	_pco_GetADCOperation(adc_working, adc_max);
	if(adc_working != 1) { bSymX = true; }

	if(bSymY){
		if( (diff0 = y0 - 1) != (diff1 = yMax - y1) ){
			if(diff0 > diff1) 
				y0 -= diff0 - diff1;
			else
				y1 += diff1 - diff0;

			iInvalid |= Ysym;
		}
	}

	if(bSymX){
		if( (diff0 = x0 - 1) != (diff1 = xMax - x1) ){
			if(diff0 > diff1) 
				x0 -= diff0 - diff1;
			else
				x1 += diff1 - diff0;

			iInvalid |= Xsym;
		}
	}

	roi_fixed.setTopLeft(Point(x0-1, y0-1));
	roi_fixed.setSize(Size(x1 -x0+1, y1-y0+1));

	if(_getDebug(DBG_ROI) || iInvalid) {
		DEB_ALWAYS()  
			<< "\nREQUESTED roiX " << DEB_VAR4(x0org, x1org, xSteps, xMax)   
			<< "\nREQUESTED roiY " << DEB_VAR4(y0org, y1org, ySteps, yMax) 
			<< "\n     FIXED roi " << DEB_VAR4(x0, x1, y0, y1)
			<< "\n       STATUS  " << DEB_VAR3(iInvalid, bSymX, bSymY);
	}

	return iInvalid ;

}


//=================================================================================================
//=================================================================================================
void Camera::_set_Roi(const Roi &new_roi, const Roi &requested_roi, int &error){
	
	Size roi_size;
	Roi fixed_roi;
	DEB_MEMBER_FUNCT();
	DEF_FNID;

	if(_checkValidRoi(new_roi, fixed_roi)){
		error = -1;
		return;
	}

	    // pco roi [1,max] ---- lima Roi [0, max-1]


		m_RoiLima = new_roi;
		m_RoiLimaRequested = requested_roi;

	if(_getDebug(DBG_ROI)) {
		DEB_ALWAYS() << DEB_VAR1(m_RoiLima);
	}	
		
	error = 0;
	return ;
}

//=================================================================================================
//=================================================================================================


void Camera::_get_Roi(Roi &roi){
		
	DEB_MEMBER_FUNCT();
	DEF_FNID;

	roi = m_RoiLima;


	if(_getDebug(DBG_ROI)) {
		DEB_ALWAYS() << DEB_VAR1(m_RoiLima);
	}	
}

void Camera::_get_Roi(unsigned int &x0, unsigned int &x1, unsigned int &y0, unsigned int &y1){
		
	DEB_MEMBER_FUNCT();
	DEF_FNID;

	Point top_left = m_RoiLima.getTopLeft();
	Point bot_right = m_RoiLima.getBottomRight();
	Size size = m_RoiLima.getSize();

	x0 = top_left.x + 1;
	y0 = top_left.y + 1;
	x1 = bot_right.x + 1; 
	y1 = bot_right.y + 1;

	if(_getDebug(DBG_ROI)) {
		DEB_ALWAYS() << DEB_VAR5(m_RoiLima, x0, x1, y0, y1);
	}	
}

void Camera::_get_MaxRoi(Roi &roi){
		
	DEB_MEMBER_FUNCT();
	DEF_FNID;

	unsigned int xMax, yMax;

	getMaxWidthHeight(xMax, yMax);

	roi.setTopLeft(Point(0, 0));
	roi.setSize(Size(xMax, yMax));
}


//=========================================================================================================
//=========================================================================================================
void Camera::_get_RoiSize(Size& roi_size)
{

	roi_size = m_RoiLima.getSize();
}

//=========================================================================================================
//=========================================================================================================
void Camera::_get_ImageType(ImageType& image_type)
{
  unsigned int pixbytes;
  getBytesPerPixel(pixbytes);
  image_type = (pixbytes == 2) ? Bpp16 : Bpp8;
}

//=================================================================================================
//=================================================================================================

// 31/10/2013 PCO Support Team <support@pco.de>
// Pixelsize is not implemented in the complete SW- and HW-stack.

void Camera::_get_PixelSize(double& x_size,double &y_size)
{  

	// pixel size in micrometer 

	if( _isCameraType(Pco2k)) {
		x_size = y_size = 7.4;	// um / BR_pco_2000_105.pdf	
		return;
	}

	if( _isCameraType(Pco4k)) {
		x_size = y_size = 9.0;	// um / BR_pco_4000_105.pdf	
		return;
	}

	if( _isCameraType(Edge)) {
		x_size = y_size = 6.5;	// um / pco.edge User Manual V1.01, page 34	
		return;
	}

	if( _isCameraType(Dimax)) {
		x_size = y_size = 11;	// um / pco.dimax User’s Manual V1.01	
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
  switch(curr_image_type)
    {
    case Bpp16:
    case Bpp8:
      break;

    default:
      throw LIMA_HW_EXC(InvalidValue,"This image type is not Managed");
    }

}
//=========================================================================================================
//=========================================================================================================
void Camera::_get_DetectorType(std::string& det_type)
{
    // ---- DONE
   det_type = "Pco";
}

//=========================================================================================================
//=========================================================================================================
void Camera::_get_MaxImageSize(Size& max_image_size)
{

  // ---- DONE
  DWORD width,height;

  getMaxWidthHeight(width,height);
  max_image_size = Size(int(width),int(height));

}

//=================================================================================================
//=================================================================================================
bool Camera::_isCameraType(int tp){
		
	DEB_MEMBER_FUNCT();
	DEF_FNID;

	switch(_getCameraType()) {
		case CAMERATYPE_PCO_DIMAX_STD: 
			return !!(tp & Dimax) ;
		
		case CAMERATYPE_PCO_EDGE_GL:
			return !!(tp & (EdgeGL | Edge));

		case CAMERATYPE_PCO_EDGE:
			return !!(tp & (EdgeRolling | Edge));

		case CAMERATYPE_PCO2000:
			return !!(tp & Pco2k) ;

		case CAMERATYPE_PCO4000:
			return !!(tp & Pco4k) ;


		default:
			return FALSE;

	}
		
}

//=================================================================================================
//=================================================================================================
void Camera::_pco_GetPixelRate(DWORD &pixRate, DWORD &pixRateNext, int &error){
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	char *msg;

#if 0
		if(!_isCameraType(Edge)) {
			pixRate = 0;
			error = -1;
			return;
		}
#endif

		PCO_FN2(error, msg,PCO_GetPixelRate, m_handle, &m_pcoData->dwPixelRate);
		PCO_THROW_OR_TRACE(error, msg) ;

		pixRate = m_pcoData->dwPixelRate;

		pixRateNext = ((m_pcoData->dwPixelRateRequested != 0) && (pixRate != m_pcoData->dwPixelRateRequested)) ?
			m_pcoData->dwPixelRateRequested : pixRate;
}



//=================================================================================================
//=================================================================================================
void Camera::_pco_GetHWIOSignal(int &errorTot){
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	int error;
	errorTot = 0;
	char *msg;

	int i, imax;
	
	if(!( _isCameraType(Dimax | Edge | Pco2k | Pco4k))  ) {
		errorTot = -1;
		return;
	}

	PCO_FN2(error, msg, PCO_GetHWIOSignalCount, m_handle, &m_pcoData->wNrPcoHWIOSignal0);
	errorTot |= error;

	imax = m_pcoData->wNrPcoHWIOSignal = 
		(m_pcoData->wNrPcoHWIOSignal0 <= SIZEARR_stcPcoHWIOSignal) ? m_pcoData->wNrPcoHWIOSignal0 : SIZEARR_stcPcoHWIOSignal;

	//DEB_ALWAYS()  << "--- size" << DEB_VAR3(imax, m_pcoData->wNrPcoHWIOSignal0 , m_pcoData->wNrPcoHWIOSignal ) ;

	for(i=0; i< imax; i++) {
		//DEB_ALWAYS()  << "---  descriptor" << DEB_VAR2(i, m_pcoData->stcPcoHWIOSignalDesc[i].wSize) ;
		PCO_FN3(error, msg,PCO_GetHWIOSignalDescriptor, m_handle, i, &m_pcoData->stcPcoHWIOSignalDesc[i]);
		errorTot |= error;
		//DEB_ALWAYS()  << "---  signal" << DEB_VAR2(i, m_pcoData->stcPcoHWIOSignal[i].wSize) ;
		PCO_FN3(error, msg,PCO_GetHWIOSignal, m_handle, i, &m_pcoData->stcPcoHWIOSignal[i]);
		errorTot |= error;
	}

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


void Camera::_pco_initHWIOSignal(int mode, int &error){
	DEB_MEMBER_FUNCT();
	DEF_FNID;

	int  _err, idx;
	error = 0;
	char *name;
	WORD val;


	if(!( _isCameraType(Dimax |Edge))  ) {
		error = -1;
		return;
	}

	_pco_GetHWIOSignal(_err); error |= _err;

	//	name[Acquire Enable] idx[0] num[0]
	idx = 0; val = 2;
	name = m_pcoData->stcPcoHWIOSignalDesc[idx].strSignalName[0];
	m_pcoData->stcPcoHWIOSignal[idx].wPolarity = 2;

	_pco_SetHWIOSignal(idx, _err); error |= _err;

	DEB_ALWAYS() << "set PcoHWIOSignal polarity "  << DEB_VAR3(name, idx, val) ;


}



//=================================================================================================
//=================================================================================================
void Camera::_pco_SetHWIOSignal(int sigNum, int &error){
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	char *msg;


		if(!( _isCameraType(Dimax |Edge))  || 
			(sigNum < 0) || (sigNum >= m_pcoData->wNrPcoHWIOSignal) ) {
			error = -1;
			return;
		}

		PCO_FN3(error, msg,PCO_SetHWIOSignal, m_handle, sigNum, &m_pcoData->stcPcoHWIOSignal[sigNum]);
		
}


//=================================================================================================
//=================================================================================================
void Camera::_get_XYsteps(Point &xy_steps){
	DEB_MEMBER_FUNCT();
	DEF_FNID;

		unsigned int xSteps, ySteps;

		getXYsteps(xSteps, ySteps);

		xy_steps.x = xSteps;
		xy_steps.y = ySteps;
}

//=================================================================================================
//=================================================================================================
void Camera::_presetPixelRate(DWORD &pixRate, int &error){
	DEB_MEMBER_FUNCT();
	DEF_FNID;
#ifndef WITH_MAVEN_COMPILATION       
    if(!_isCameraType(Edge) || !_isValid_pixelRate(pixRate)) {
        pixRate = 0;
        error = -1;
        return;
    }
#else
    if(!_isCameraType(Edge | Pco2k | Pco4k)|| !_isValid_pixelRate(pixRate)) {
        pixRate = 0;
        error = -1;
        return;
    }    
#endif    

		m_pcoData->dwPixelRateRequested = pixRate;
		error = 0;
}


//=================================================================================================
//=================================================================================================
void Camera::msgLog(char *s) {
	m_msgLog->add(s); 
}



//=================================================================================================
//=================================================================================================
	//-------------------------------------------------------------------------------------------------
	// PCO_SetADCOperation
    // Set analog-digital-converter (ADC) operation for reading the image sensor data. Pixel data can be
    // read out using one ADC (better linearity) or in parallel using two ADCs (faster). This option is
    // only available for some camera models. If the user sets 2ADCs he must center and adapt the ROI
    // to symmetrical values, e.g. pco.1600: x1,y1,x2,y2=701,1,900,500 (100,1,200,500 is not possible).
    //
	// DIMAX -> 1 adc
	//-------------------------------------------------------------------------------------------------
int Camera::_pco_GetADCOperation(int &adc_working, int &adc_max)
{
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	char *msg;

	int error;
	WORD wADCOperation;

	adc_max = m_pcoData->stcPcoDescription.wNumADCsDESC; // nr of ADC in the system

	if(adc_max == 2) {
		PCO_FN2(error, msg,PCO_GetADCOperation, m_handle, &wADCOperation);
		if(error) wADCOperation = (WORD) 1;
	} else {
		adc_max = 1;
		wADCOperation = (WORD) 1;
	}

	adc_working = wADCOperation;
	m_pcoData->wNowADC= wADCOperation;

	return error;
}

//=================================================================================================
//=================================================================================================

int Camera::_pco_SetADCOperation(int adc_new, int &adc_working)
{
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	char *msg;

	int error, adc_max;

	error = _pco_GetADCOperation(adc_working, adc_max);

	DEB_ALWAYS() << fnId << ": " DEB_VAR2(adc_max, adc_working);

	if(error) return error;

	if((adc_new >=1) && (adc_new <= adc_max) && (adc_new != adc_working) ){
		PCO_FN2(error, msg,PCO_SetADCOperation, m_handle, (WORD) adc_new);
		_pco_GetADCOperation(adc_working, adc_max);
	}
	m_pcoData->wNowADC = adc_working;
	return error;
}


//=================================================================================================
//=================================================================================================
char *Camera::_pco_SetPixelRate(int &error){
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	error = 0;
	char *msg;

	if(_isCameraType(Edge)) {
		DWORD _dwPixelRate, _dwPixelRateOld, _dwPixelRateReq;

		PCO_FN2(error, msg,PCO_GetPixelRate, m_handle, &m_pcoData->dwPixelRate);
		PCO_THROW_OR_TRACE(error, msg) ;

		_dwPixelRateOld = m_pcoData->dwPixelRate;
		_dwPixelRateReq = m_pcoData->dwPixelRateRequested;
		DEB_ALWAYS() << "PIXEL rate (actual/req): " << DEB_VAR2(_dwPixelRateOld, _dwPixelRateReq) ;

		if(_isValid_pixelRate(_dwPixelRateReq) && (_dwPixelRateOld != _dwPixelRateReq)) {

			PCO_FN2(error, msg,PCO_SetPixelRate, m_handle, _dwPixelRateReq);
			PCO_THROW_OR_TRACE(error, msg) ;

			PCO_FN2(error, msg,PCO_GetPixelRate, m_handle, &m_pcoData->dwPixelRate);
			PCO_THROW_OR_TRACE(error, msg) ;

			_dwPixelRate = m_pcoData->dwPixelRate;
			DEB_ALWAYS() << "PIXEL rate SET (old/new): "  << DEB_VAR2(_dwPixelRateOld, _dwPixelRate) ;

			_armRequired(true);
		}
		m_pcoData->dwPixelRateRequested = 0;
		return fnId;
	}

	if(_isCameraType(Pco2k | Pco4k)) {
		DWORD _dwPixelRate, _dwPixelRateOld, _dwPixelRateMax;

		PCO_FN2(error, msg,PCO_GetPixelRate, m_handle, &m_pcoData->dwPixelRate);
		PCO_THROW_OR_TRACE(error, msg) ;

		_dwPixelRateOld = m_pcoData->dwPixelRate;
		_dwPixelRateMax = m_pcoData->dwPixelRateMax;
		DEB_ALWAYS() << "PIXEL rate (actual/max): " << DEB_VAR2(_dwPixelRateOld, _dwPixelRateMax) ;

		if(_dwPixelRateMax > _dwPixelRateOld) {

			PCO_FN2(error, msg,PCO_SetPixelRate, m_handle, _dwPixelRateMax);
			PCO_THROW_OR_TRACE(error, msg) ;

			PCO_FN2(error, msg,PCO_GetPixelRate, m_handle, &m_pcoData->dwPixelRate);
			PCO_THROW_OR_TRACE(error, msg) ;
			
			_dwPixelRate = m_pcoData->dwPixelRate;
			DEB_ALWAYS() << "PIXEL rate SET (old/new): "  << DEB_VAR2(_dwPixelRateOld, _dwPixelRate) ;

			_armRequired(true);
		}
		return fnId;
	}
return fnId;
}
//=================================================================================================
//=================================================================================================
