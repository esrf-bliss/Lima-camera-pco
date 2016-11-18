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

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#include <cstdlib>
#include <process.h>

#include <sys/stat.h>

#include <sys/timeb.h>
#include <time.h>

#include "lima/HwSyncCtrlObj.h"

#include "lima/Exceptions.h"

#include "PcoCameraUtils.h"
#include "PcoCamera.h"
#include "PcoSyncCtrlObj.h"
#include "PcoBufferCtrlObj.h"

using namespace lima;
using namespace lima::Pco;

static char *timebaseUnits[] = {"ns", "us", "ms"};

#define BUFF_INFO_SIZE 10000


void print_hex_dump_buff(void *ptr_buff, size_t len);
int __xlat_date(char *s1, char &ptrTo, int lenTo) ;
char *_xlat_date(char *s1, char *s2, char *s3) ;

//=========================================================================================================
char* _timestamp_pcocamerautils() {return ID_TIMESTAMP ;}
//=========================================================================================================

//=========================================================================================================
// dummy comments for test 02ccc
//=========================================================================================================


//=========================================================================================================
//=========================================================================================================
char *getTimestamp(timestampFmt fmtIdx, time_t xtime) {
   static char timeline[128];
   errno_t err;
	time_t ltime;
	struct tm today;
	char *fmt;

  switch(fmtIdx) {
    case Iso: fmt = "%Y/%m/%d %H:%M:%S"; break;
    case IsoHMS: fmt = "%H:%M:%S"; break;
    case FnDate: fmt = "%Y-%m-%d"; break;

    default:
    case FnFull: fmt = "%Y-%m-%d-%H%M%S"; break;
  }

	if(xtime == 0) 
		time( &ltime );
	else
		ltime = xtime;
	err = localtime_s( &today, &ltime );
	strftime(timeline, 128, fmt, &today );
      
	return timeline;
}

time_t getTimestamp() { return time(NULL); }

//====================================================================
//====================================================================
//$Id: [Oct  8 2013 15:21:07] [Tue Oct  8 15:21:07 2013] (..\..\..\..\src\PcoCamera.cpp) $

#define LEN_BUFF_DATE 128
#define TOKNR_DT 5


int __xlat_date(char *s1, char &ptrTo, int lenTo) {
	char *tok[TOKNR_DT];
	char *tokNext = NULL;
	int tokNr, iM, iD, iY, i;
	char *ptr;
	char *months = "Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec";
	char *sM, *sT;
	char buff[LEN_BUFF_DATE+1];

	strcpy_s(buff, LEN_BUFF_DATE, s1);
	ptr = buff;

	for(tokNr = i = 0; i < TOKNR_DT; i++) {
		if( (tok[i] = strtok_s(ptr, " ", &tokNext)) == NULL) break;
		ptr = NULL;
		tokNr = i+1;
	}

	if(tokNr == 4) {
		// Oct  8 2013 15:21:07
		sM = tok[0]; iD = atoi(tok[1]); iY = atoi(tok[2]); sT = tok[3]; 
	} else if(tokNr == 5) {
		// Tue Oct  8 15:21:07 2013
		sM = tok[1]; iD = atoi(tok[2]); iY = atoi(tok[4]); sT = tok[3]; 
	} else {
		sM = "xxx"; iD = 99; iY = 9999; sT = "99:99:99"; 
	}
	
	ptr = strstr(months,sM);
	iM = (ptr != NULL) ? ( int(ptr - months) / 4) + 1 : 99;


	return sprintf_s(&ptrTo, lenTo, "%04d/%02d/%02d %s", iY, iM, iD, sT);
}

char *_xlat_date(char *s1, char *s2, char *s3) {
	static char buff[LEN_BUFF_DATE+1];
	char *ptr = buff;
	char *ptrMax = buff + LEN_BUFF_DATE;

	ptr += sprintf_s(ptr, ptrMax - ptr, "$Id: comp[");
	ptr += __xlat_date(s1, *ptr, (int) (ptrMax - ptr));
	ptr += sprintf_s(ptr, ptrMax - ptr, "] file[");
	ptr += __xlat_date(s2, *ptr, (int) (ptrMax - ptr));
	ptr += sprintf_s(ptr, ptrMax - ptr, "] [%s] $", s3);
	return buff;
	
}

char *_split_date(char *s) {
	static char s1[LEN_BUFF_DATE+1];
	static char s2[LEN_BUFF_DATE+1];
	static char s3[LEN_BUFF_DATE+1];
	char *ptr1, *ptr2;

	ptr1 = strchr(s,'[');
	ptr2 = strchr(ptr1,']');
	strncpy_s(s1, LEN_BUFF_DATE, ptr1+1, ptr2-ptr1-1);

	ptr1 = strchr(ptr2,'[');
	ptr2 = strchr(ptr1,']');
	strncpy_s(s2, LEN_BUFF_DATE, ptr1+1, ptr2-ptr1-1);

	ptr1 = strchr(ptr2,'[');
	ptr2 = strchr(ptr1,']');
	strncpy_s(s3, LEN_BUFF_DATE, ptr1+1, ptr2-ptr1-1);

	return _xlat_date(s1, s2, s3);
}
//====================================================================
//====================================================================

char *str_trim_left(char *s) {
	char c;
	if(s == NULL) return NULL;
	while((c = *s) != 0) {
		if(!isspace(c)) break;
		s++;		
	}
	return s;
}

char *str_trim_right(char *s) {
	char *ptr;
	if(s == NULL) return NULL;
	ptr = s + strlen(s) - 1;
	while(s <= ptr) {
		if(!isspace(*ptr)) break;
		*ptr-- = 0;
	}
	return s;
}
char *str_trim(char *s) {
	return str_trim_left(str_trim_right(s));
}

char *str_toupper(char *s) {
	char *ptr = s;
	while(*ptr) { 
		*ptr = toupper(*ptr);
		ptr++;
	}
	return s;
}


//=========================================================================================================
//=========================================================================================================

void stcPcoData::traceAcqClean(){
	void *ptr = &this->traceAcq;
	memset(ptr, 0, sizeof(struct stcTraceAcq));
}

void stcPcoData::traceMsg(char *s){
	char *ptr = traceAcq.msg;
	char *dt = getTimestamp(IsoHMS);
	size_t lg = strlen(ptr);
	ptr += lg;
	snprintf(ptr, LEN_TRACEACQ_MSG - lg,"%s> %s", dt, s);
}

char *Camera::talk(char *cmd){
	DEB_MEMBER_FUNCT();

	static char buff[BUFF_INFO_SIZE +16];
	sprintf_s(buff, BUFF_INFO_SIZE, "talk> %s", cmd);
	m_msgLog->add(buff);

	return _talk(cmd, buff, BUFF_INFO_SIZE);
}

#define NRTOK 5
#define NRCMDS 50
char *Camera::_talk(char *_cmd, char *output, int lg){
	DEB_MEMBER_FUNCT();
		char cmdBuff[BUFF_INFO_SIZE +1];
		char cmdBuffAux[BUFF_INFO_SIZE +1];
		char *cmd, *key, *keys[NRCMDS], *keys_desc[NRCMDS];
		int ikey = 0;
		char *tok[NRTOK];
		int tokNr;
		char *ptr, *ptrMax;
		int segmentPco = m_pcoData->wActiveRamSegment;
		int segmentArr = segmentPco -1;
		
		ptr = output; *ptr = 0;
		ptrMax = ptr + lg;

		int width = +20;

		strncpy_s(cmdBuff, BUFF_INFO_SIZE, _cmd, BUFF_INFO_SIZE);
		cmd = str_trim(cmdBuff);
		strncpy_s(cmdBuffAux, BUFF_INFO_SIZE, cmd, BUFF_INFO_SIZE);

		if(*cmd){
			char *tokContext;
			for(int i=0; i < NRTOK; i++) {
				if( (tok[i] = strtok_s(cmd, " ", &tokContext)) == NULL) break;
				cmd = NULL;
				tokNr = i;
			}
			cmd = tok[0];
		} else {
			cmd = "camInfo";
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "camInfo";     
		keys_desc[ikey++] = "(R) detailed cam info (type, if, sn, hw & fw ver, ...)";     
		if(_stricmp(cmd, key) == 0){
			_camInfo(ptr, ptrMax, CAMINFO_ALL);
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "getVersionFile";     
		keys_desc[ikey++] = "(R) reads INSTALL_VERSION.txt";     
		if(_stricmp(cmd, key) == 0){
			_getDllPath(FILE_PCO_DLL, ptr, ptrMax -ptr);
			return output;
		}
		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "expDelayTime";     
		keys_desc[ikey++] = "(R) exposure & delay time (actual & valid ranges)";
		if(_stricmp(cmd, key) == 0){
			_camInfo(ptr, ptrMax, CAMINFO_EXP);
			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "timingInfo";     
		keys_desc[ikey++] = "(R) timing information (exp, trig delay, ...)";
		if(_stricmp(cmd, key) == 0){
			double frameTime, expTime, sysDelay, sysJitter, trigDelay;

			_pco_GetImageTiming(frameTime, expTime, sysDelay, sysJitter, trigDelay );


			ptr += sprintf_s(ptr, ptrMax - ptr, "frameTime %g  ",  frameTime);
			ptr += sprintf_s(ptr, ptrMax - ptr, "expTime %g  ",  expTime);
			ptr += sprintf_s(ptr, ptrMax - ptr, "sysDelay %g  ",  sysDelay);
			ptr += sprintf_s(ptr, ptrMax - ptr, "sysJitter %g  ",  sysJitter);
			ptr += sprintf_s(ptr, ptrMax - ptr, "trigDelay %g  ",  trigDelay);
			ptr += sprintf_s(ptr, ptrMax - ptr, "\n");

			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "cocRunTime";     
		keys_desc[ikey++] = "(R) Camera Operation Code runtime covers the delay, exposure and readout time";
		if(_stricmp(cmd, key) == 0){
			ptr += sprintf_s(ptr, ptrMax - ptr, "%g",  m_pcoData->cocRunTime);
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "frameRate";     
		keys_desc[ikey++] = "(R) max frame rate (calculated as 1/cocRunTime)";   
		if(_stricmp(cmd, key) == 0){
			ptr += sprintf_s(ptr, ptrMax - ptr, "%g", m_pcoData->frameRate);
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "timestamp";     
		keys_desc[ikey++] = "(R) timestamp of compiled modules";     
		if(_stricmp(cmd, key) == 0){
			_camInfo(ptr, ptrMax, CAMINFO_VERSION);
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "actionTimestamp";     
		keys_desc[ikey++] = "(R) action timestamps";     
		if(_stricmp(cmd, key) == 0){
			ptr += sprintf_s(ptr, ptrMax - ptr,  "  constructor [%s]\n", getTimestamp(Iso, _getActionTimestamp(tsConstructor)));
			ptr += sprintf_s(ptr, ptrMax - ptr,  "last startAcq [%s]\n", getTimestamp(Iso, _getActionTimestamp(tsStartAcq)));
			ptr += sprintf_s(ptr, ptrMax - ptr,  " last stopAcq [%s]\n", getTimestamp(Iso, _getActionTimestamp(tsStartAcq)));

			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "clTransferParam";     
		keys_desc[ikey++] = "(R) CameraLink transfer parameters";     
		if(_stricmp(cmd, key) == 0){
			_camInfo(ptr, ptrMax, CAMINFO_CAMERALINK);
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "maxNbImages";     
		keys_desc[ikey++] = "(R) for DIMAX 2K 4K only / max number of images in the active ram segment";     
		if(_stricmp(cmd, key) == 0){
			if(!_isCameraType(Dimax | Pco2k | Pco4k )) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "%d", -1);
				return output;
			}

			ptr += sprintf_s(ptr, ptrMax - ptr, "%ld", pcoGetFramesMax(m_pcoData->wActiveRamSegment));
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "dumpRecordedImg";     
		keys_desc[ikey++] = "(R) for DIMAX only / TODO";     
		if(_stricmp(cmd, key) == 0){
			int res, nrImages, error;

			res = dumpRecordedImages(nrImages, error);


			ptr += sprintf_s(ptr, ptrMax - ptr, "res[%d] nrImages[%d] error[0x%x] ", res, nrImages, error);
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "allocatedBuffer";     
		keys_desc[ikey++] = "(R) TODO";     
		if(_stricmp(cmd, key) == 0){
			unsigned int bytesPerPix; getBytesPerPixel(bytesPerPix);

			int sizeBytes = m_pcoData->wXResActual * m_pcoData->wYResActual * bytesPerPix;
			ptr += sprintf_s(ptr, ptrMax - ptr, "IMAGE info:\n"
			                                    "    X=[%d] Y=[%d] bytesPerPix=[%d] size=[%d B]\n",  
				m_pcoData->wXResActual,  m_pcoData->wYResActual, bytesPerPix, sizeBytes);
			
			ptr += sprintf_s(ptr, ptrMax - ptr, "PCO API allocated buffers:\n"
												"    allocated=[%s] nrBuff=[%d] size=[%ld B][%g MB] imgPerBuff[%d]\n", 
				m_pcoData->bAllocatedBufferDone ? "TRUE" : "FALSE", 
				m_pcoData->iAllocatedBufferNumber, 
				m_pcoData->dwAllocatedBufferSize, m_pcoData->dwAllocatedBufferSize/1000000.,
				m_pcoData->dwAllocatedBufferSize/sizeBytes);

			ptr += sprintf_s(ptr, ptrMax - ptr, "LIMA allocated buffers: \n"
												"    nr of buffers=[%d] \n", 
				m_pcoData->iAllocatedBufferNumberLima);


			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "timeDimax";     
		keys_desc[ikey++] = "(R) for DIMAX only / acq time details (record and transfer time)";     
		if(_stricmp(cmd, key) == 0){

			if(!(_isCameraType(Dimax | Pco2k | Pco4k))) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "* ERROR - only for DIMAX / 2K");
				return output;
			}

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* Acq: rec[%ld] xfer[%ld] recLoopTime[%ld] recLoopTout[%ld] acqAll[%ld] (ms) [%s]\n",
				m_pcoData->msAcqRec, m_pcoData->msAcqXfer,  
				m_pcoData->msAcqTnow, m_pcoData->msAcqTout, 
				m_pcoData->msAcqAll, 
				getTimestamp(Iso, m_pcoData->msAcqRecTimestamp));


			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "lastImgRecorded";     
		keys_desc[ikey++] = "last image recorded";     
		if(_stricmp(cmd, key) == 0){
			DWORD lastImgRecorded = m_pcoData->traceAcq.nrImgRecorded;

			if(!(_isCameraType(Dimax | Pco2k | Pco4k))) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "%ld", -1);
			} else {
				ptr += sprintf_s(ptr, ptrMax - ptr, "%ld", lastImgRecorded);
			}
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "lastImgAcquired";     
		keys_desc[ikey++] = "last image acquired";     
		if(_stricmp(cmd, key) == 0){
			DWORD lastImgAcquired = m_pcoData->traceAcq.nrImgAcquired;


			ptr += sprintf_s(ptr, ptrMax - ptr, "%ld", lastImgAcquired);
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "traceAcq";     
		keys_desc[ikey++] = "(R) trace details (not all records are filled!)";     
		if(_stricmp(cmd, key) == 0){
			time_t _timet;

			if(0 && !(_isCameraType(Dimax | Pco2k | Pco4k))) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "* ERROR - only for DIMAX / 2K");
				return output;
			}

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"\n"
				"* fnId[%s] nrEvents[%d]\n"
				"* ... fnIdXfer[%s]\n",
				m_pcoData->traceAcq.fnId,
				PCO_BUFFER_NREVENTS,
				m_pcoData->traceAcq.fnIdXfer);

			ptr += sprintf_s(ptr, ptrMax - ptr, "* ... testCmdMode [0x%llx]\n",  m_pcoData->testCmdMode);

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* msExposure[%g] msDelay[%g]\n",
				m_pcoData->traceAcq.sExposure * 1000.,
				m_pcoData->traceAcq.sDelay * 1000.);

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* ... msLimaExposure[%g] Pco exposure[%d] base[%d]\n",
				m_pcoData->traceAcq.dLimaExposure * 1000.,
				m_pcoData->traceAcq.iPcoExposure, 
				m_pcoData->traceAcq.iPcoExposureBase);

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* ... msLimaDelay[%g] Pco delay[%d] base[%d]\n",
				m_pcoData->traceAcq.dLimaDelay * 1000.,
				m_pcoData->traceAcq.iPcoDelay, 
				m_pcoData->traceAcq.iPcoDelayBase);

			ptr += sprintf_s(ptr, ptrMax - ptr, "* pcoBin horz[%d] vert[%d]\n",  
					m_pcoData->traceAcq.iPcoBinHorz, 
					m_pcoData->traceAcq.iPcoBinVert);


			Point top_left = m_RoiLima.getTopLeft();
			Point bot_right = m_RoiLima.getBottomRight();
			Size size = m_RoiLima.getSize();			
			unsigned int bytesPerPix; getBytesPerPixel(bytesPerPix);

			ptr += sprintf_s(ptr, ptrMax - ptr, "* limaRoi xy0[%d,%d] xy1[%d,%d] size[%d,%d]\n",  
					top_left.x, top_left.y,
					bot_right.x, bot_right.y,
					size.getWidth(), size.getHeight());


			ptr += sprintf_s(ptr, ptrMax - ptr, "* ... pcoRoi x[%d,%d] y[%d,%d]\n",  
					m_pcoData->traceAcq.iPcoRoiX0, 
					m_pcoData->traceAcq.iPcoRoiX1, 
					m_pcoData->traceAcq.iPcoRoiY0, 
					m_pcoData->traceAcq.iPcoRoiY1);


			long long imgSize = size.getWidth()* size.getHeight() * bytesPerPix;
			long long totSize = imgSize * m_pcoData->traceAcq.nrImgRequested;
			double mbTotSize =  totSize/(1024.*1024.);
			double totTime = m_pcoData->traceAcq.msXfer / 1000.;
			double xferSpeed = mbTotSize / totTime;
			double framesPerSec = m_pcoData->traceAcq.nrImgRequested / totTime;
			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* ... imgSize[%lld B] totSize[%lld B][%g MB]\n",  
				imgSize, totSize, mbTotSize);

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* nrImgRequested[%d] nrImgAcquired[%d]\n",
				m_pcoData->traceAcq.nrImgRequested,
				m_pcoData->traceAcq.nrImgAcquired);


			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* ... nrImgRequested0[%d] nrImgRecorded[%d] maxImgCount[%d]\n",
				m_pcoData->traceAcq.nrImgRequested0,
				m_pcoData->traceAcq.nrImgRecorded,
				m_pcoData->traceAcq.maxImgCount);

			ptr += sprintf_s(ptr, ptrMax - ptr,	
				"* limaTriggerMode[%s]\n",
				m_pcoData->traceAcq.sLimaTriggerMode);
			ptr += sprintf_s(ptr, ptrMax - ptr,	
				"* ... pcoTriggerMode[%s] [%d]\n",
				m_pcoData->traceAcq.sPcoTriggerMode,
				m_pcoData->traceAcq.iPcoTriggerMode);
			ptr += sprintf_s(ptr, ptrMax - ptr,	
				"* ... pcoAcqMode[%s] [%d]\n",
				m_pcoData->traceAcq.sPcoAcqMode,
				m_pcoData->traceAcq.iPcoAcqMode);


			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* msStartAcqStart[%ld]  msStartAcqEnd[%ld]\n",
				m_pcoData->traceAcq.msStartAcqStart, m_pcoData->traceAcq.msStartAcqEnd);
			

			for(int _i = 0; _i < LEN_TRACEACQ_TRHEAD; _i++){
				char *desc = m_pcoData->traceAcq.usTicks[_i].desc;
				if(desc != NULL) {
					ptr += sprintf_s(ptr, ptrMax - ptr, 
						"* ... usTicks[%d][%5.3f] (ms)   (%s)\n", 
						_i, m_pcoData->traceAcq.usTicks[_i].value/1000.,
						desc);
			
				}
			}

			_timet = m_pcoData->traceAcq.endRecordTimestamp;

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* msImgCoc[%.3g] fps[%.3g] msTout[%ld] msTotal[%ld]\n",
				m_pcoData->traceAcq.msImgCoc, 
				1000. / m_pcoData->traceAcq.msImgCoc,
				m_pcoData->traceAcq.msTout,
				m_pcoData->traceAcq.msTotal);

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* ... msRecordLoop[%ld] msRecord[%ld] endRecord[%s]\n",
				m_pcoData->traceAcq.msRecordLoop,
				m_pcoData->traceAcq.msRecord,
				_timet ? getTimestamp(Iso, _timet) : "");

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* ... msXfer[%ld] endXfer[%s]\n",
				m_pcoData->traceAcq.msXfer,
				getTimestamp(Iso, m_pcoData->traceAcq.endXferTimestamp));

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* ... xferTimeTot[%g s] xferSpeed[%g MB/s][%g fps]\n",  
				totTime, xferSpeed, framesPerSec);

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* ... checkImgNr pco[%d] lima[%d] diff[%d]\n",  
				m_pcoData->traceAcq.checkImgNrPco,
				m_pcoData->traceAcq.checkImgNrLima,
				m_pcoData->traceAcq.checkImgNrPco -	m_pcoData->traceAcq.checkImgNrLima);

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"%s\n", m_pcoData->traceAcq.msg);

			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "testCmd";     
		keys_desc[ikey++] = "DISABLED / debug tool";     
		if(_stricmp(cmd, key) == 0){


			if((tokNr >= 1) &&  (_stricmp(tok[1], "mode")==0)){
				ptr += sprintf_s(ptr, ptrMax - ptr, "testCmdMode [0x%llx]",  m_pcoData->testCmdMode);
				if(tokNr >= 2){
					int nr;
					unsigned long long _testCmdMode;

					nr = sscanf_s(tok[2], "0x%llx", &_testCmdMode);
					
					if(nr == 1) {
						m_pcoData->testCmdMode = _testCmdMode;
						ptr += sprintf_s(ptr, ptrMax - ptr, "   changed OK>  ");
					} else {
						ptr += sprintf_s(ptr, ptrMax - ptr, "   ERROR - NOT changed>  ");
					}
					ptr += sprintf_s(ptr, ptrMax - ptr, "testCmdMode [0x%llx]",  m_pcoData->testCmdMode);
				}
				return output;
			}
			
			
			//--- test of close
			if( (_stricmp(tok[1], "cb")==0)){
				int error;
				char *msg;

				m_cam_connected = false;

				//m_sync->_getBufferCtrlObj()->_pcoAllocBuffersFree();
				m_buffer->_pcoAllocBuffersFree();
				PCO_FN1(error, msg,PCO_CloseCamera, m_handle);
				PCO_PRINT_ERR(error, msg); 
				m_handle = NULL;

				ptr += sprintf_s(ptr, ptrMax - ptr, "%s> closed cam\n", tok[1]);
				return output;
			}

			
			//--- test of callback
			if( (_stricmp(tok[1], "cb")==0)){
				Event *ev = new Event(Hardware,Event::Error,Event::Camera,Event::Default, "test cb");
				m_HwEventCtrlObj->reportEvent(ev);
				ptr += sprintf_s(ptr, ptrMax - ptr, "%s> done\n", tok[1]);
				return output;
			}

			//--- test of sleep
			if((tokNr == 2) &&  (_stricmp(tok[1], "time")==0)){
				long long us;

				LARGE_INTEGER usStart;

				ptr += sprintf_s(ptr, ptrMax - ptr, "sleeping ...\n"); 

				usElapsedTimeSet(usStart);

				::Sleep(atoi(tok[2])*1000);


				us = usElapsedTime(usStart);
				double ticksPerSec = usElapsedTimeTicsPerSec();
				ptr += sprintf_s(ptr, ptrMax - ptr, "us[%lld] ms[%5.3f] tics/us[%g]\n", us, us/1000., ticksPerSec/1.0e6);

				return output;
			}





			if(tokNr == 0) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "tokNr [%d] cmd [%s] No parameters", tokNr, cmd); 
			} else {
				ptr += sprintf_s(ptr, ptrMax - ptr, "tokNr [%d] cmd [%s]\n", tokNr, cmd); 
				for(int i = 1; i<= tokNr; i++) {
					ptr += sprintf_s(ptr, ptrMax - ptr, "tok [%d] [%s]\n", i, tok[i]); 
				}
			}
			return output;
		}



		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "rollingShutter";
		keys_desc[ikey++] = "(RW) for EDGE only / rolling shutter mode";
		if(_stricmp(cmd, key) == 0){
			int error;
			bool rolling, rollingNew;

			if(!_isCameraType(Edge)) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "%d", -1);
				return output;
			}
			
			rolling = _get_shutter_rolling_edge(error);
			if(tokNr == 0) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "%d", rolling);
				return output;
			}

			if((tokNr != 1) || ((strcmp(tok[1],"0") != 0) && (strcmp(tok[1],"1") != 0))){
				ptr += sprintf_s(ptr, ptrMax - ptr, "syntax ERROR - %s <0 | 1>", cmd);
				return output;
			}
			
			rollingNew = atoi(tok[1]) != 0;

			if(rollingNew != rolling){
				_set_shutter_rolling_edge(rollingNew, error);
			}
			ptr += sprintf_s(ptr, ptrMax - ptr, "%d", rollingNew);
			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "bitAlignment";     
		keys_desc[ikey++] = "(RW) bit alignment (LSB = 1, MSB = 0)";
		if(_stricmp(cmd, key) == 0){
			int alignment;
			bool syntax = false;
			char *res;
			
			if( (tokNr < 0) || (tokNr > 1)) {
				syntax = true;
			}
			
			if(tokNr == 1) {
				alignment = atoi(tok[1]);
				if((alignment == 0) || (alignment == 1)){
					_pco_SetBitAlignment(alignment);
				} else {
					syntax = true;
				}
			}

			if(syntax) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "ERROR: syntax: bitAlignment [<1 (LSB) || 0 (MSB)>]");
				return output;
			}

			_pco_GetBitAlignment(alignment);
			res = alignment == 0 ? "0 (MSB)" : "1 (LSB)";
			ptr += sprintf_s(ptr, ptrMax - ptr, "%s", res);
			return output;


		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "getCheckImgNrResults";     
		keys_desc[ikey++] = "(R) get the last checkImgNr results";
		if(_stricmp(cmd, key) == 0){
			//int alignment;
			bool syntax = false;
			//char *res;
			

			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"pco: %d lima: %d diff: %d",  
				m_pcoData->traceAcq.checkImgNrPco,
				m_pcoData->traceAcq.checkImgNrLima,
				m_pcoData->traceAcq.checkImgNrPco -	m_pcoData->traceAcq.checkImgNrLima);

			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "testFrameFirst0"; 
		keys_desc[ikey++] = "(RW) TEST - force FrameFirst to 0 [<0 | 1>]";
		if(_stricmp(cmd, key) == 0){
			if(tokNr == 0) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "%d", m_pcoData->testForceFrameFirst0);
				return output;
			}

			m_pcoData->testForceFrameFirst0 = !!atoi(tok[1]);

			ptr += sprintf_s(ptr, ptrMax - ptr, "%d", m_pcoData->testForceFrameFirst0);
			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "pcoLogsEnabled";
		keys_desc[ikey++] = "(R) PCO log files enalbled";
		if(_stricmp(cmd, key) == 0){
			ptr += sprintf_s(ptr, ptrMax - ptr, "%d", m_pcoData->pcoLogActive);
			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "pixelRate";
		keys_desc[ikey++] = "(RW) pixelrate (Hz) for reading images from the image sensor";
		if(_stricmp(cmd, key) == 0){
			DWORD pixRate, pixRateNext; int error;

			if(_isCameraType(Dimax) ) tokNr = 0;
			
			if( (tokNr < 0) || (tokNr > 1)) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "ERROR: syntax: pixelRate [<new value (Hz)>]");
				return output;
			}
			
			if(tokNr == 0) {
				_pco_GetPixelRate(pixRate, pixRateNext, error);
				ptr += sprintf_s(ptr, ptrMax - ptr, "%ld", pixRateNext);
				return output;
			}

			pixRate = atoi(tok[1]);
			_presetPixelRate(pixRate, error);
			
			if(error){
				ptr += sprintf_s(ptr, ptrMax - ptr, "ERROR: unsupported cam or invalid value");
				return output;
			}

			ptr += sprintf_s(ptr, ptrMax - ptr, "%ld", m_pcoData->dwPixelRateRequested);
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "pixelRateInfo";
		keys_desc[ikey++] = "(R) pixelrate (Hz) for reading images from the image sensor (actual & valid values)";
		if(_stricmp(cmd, key) == 0){

			_camInfo(ptr, ptrMax, CAMINFO_PIXELRATE);
			
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "pixelRateValidValues";
		keys_desc[ikey++] = "(R) pixelrate (Hz) valid values";
		if(_stricmp(cmd, key) == 0){
			DWORD dwPixRate, dwPixRateNext ; int error, i, nr;

		    _pco_GetPixelRate(dwPixRate, dwPixRateNext, error);

			for(nr = i=0; i<4; i++) {
				dwPixRate = m_pcoData->stcPcoDescription.dwPixelRateDESC[i];
				if(dwPixRate){
					nr++;
					ptr += sprintf_s(ptr, ptrMax - ptr, "%ld  ",dwPixRate);
				}  
			}	

			if(nr == 0)			
				ptr += sprintf_s(ptr, ptrMax - ptr, "%d  ",nr);
			return output;
		}



		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "roi";
		keys_desc[ikey++] = "get actual (fixec) last ROI requested (unfixed) ROIs";
		if(_stricmp(cmd, key) == 0){
			unsigned int x0, x1, y0, y1;
			Roi new_roi;

			Roi limaRoi;
			_get_Roi(limaRoi);

			if((tokNr != 0) ){
					ptr += sprintf_s(ptr, ptrMax - ptr, "syntax ERROR - %s ", cmd);
					return output;
			}
				

			_get_Roi(x0, x1, y0, y1);
			ptr += sprintf_s(ptr, ptrMax - ptr, "* roi PCO X(%d,%d) Y(%d,%d) size(%d,%d)\n",  
					x0, x1, y0, y1, x1-x0+1, y1-y0+1);
			{
			Point top_left = m_RoiLima.getTopLeft();
			Point bot_right = m_RoiLima.getBottomRight();
			Size size = m_RoiLima.getSize();

			ptr += sprintf_s(ptr, ptrMax - ptr, "* roiLima PCO XY0(%d,%d) XY1(%d,%d) size(%d,%d)\n",  
					top_left.x, top_left.y,
					bot_right.x, bot_right.y,
					size.getWidth(), size.getHeight());

			top_left = m_RoiLimaRequested.getTopLeft();
			bot_right = m_RoiLimaRequested.getBottomRight();
			size = m_RoiLimaRequested.getSize();

			ptr += sprintf_s(ptr, ptrMax - ptr, "* roiLima REQUESTED XY0(%d,%d) XY1(%d,%d) size(%d,%d)\n",  
					top_left.x, top_left.y,
					bot_right.x, bot_right.y,
					size.getWidth(), size.getHeight());
			}



			return output;

		}


		//----------------------------------------------------------------------------------------------------------
		// dwGeneralCapsDESC1;      // General capabilities:
        //		Bit 3: Timestamp ASCII only available (Timestamp mode 3 enabled)
		//		Bit 8: Timestamp not available
		// m_pcoData->stcPcoDescription.dwGeneralCapsDESC1 & BIT3 / BIT8

		
		key = keys[ikey] = "timestampMode"; 
		keys_desc[ikey++] = "(RW) pco timestampMode [<new value (0, 1, 2, 3)>]"; 
		if(_stricmp(cmd, key) == 0){
			int error, val;
			WORD wTimeStampMode;
			DWORD capsDesc1; 
			_pco_GetGeneralCapsDESC(capsDesc1, error);

			int valMax;

			if(capsDesc1 & BIT8)
			{
				ptr += sprintf_s(ptr, ptrMax - ptr, "timestampmode not allowed\n");
				return output;
			}
			valMax = (capsDesc1 & BIT3) ? 3 : 2;

			_pco_GetTimestampMode(wTimeStampMode, error);
			if(error) 
			{
				ptr += sprintf_s(ptr, ptrMax - ptr, "SDK ERROR _pco_GetTimestampMode\n");
				return output;
			}
			ptr += sprintf_s(ptr, ptrMax - ptr, "%d   ", wTimeStampMode);

			if((tokNr == 1)){
				val = atoi(tok[1]);
				
				if((val < 0) ||(val>valMax)) {
					ptr += sprintf_s(ptr, ptrMax - ptr, "invalid value [%d] must be (0 - %d)",  val, valMax);
				}else {
					wTimeStampMode = val;
					_pco_SetTimestampMode(wTimeStampMode, error);
					if(error) 
					{
						ptr += sprintf_s(ptr, ptrMax - ptr, "SDK ERROR _pco_SetTimestampMode\n");
						return output;
					}
					_pco_GetTimestampMode(wTimeStampMode, error);
					if(error) 
					{
						ptr += sprintf_s(ptr, ptrMax - ptr, "SDK ERROR _pco_GetTimestampMode\n");
						return output;
					}
					ptr += sprintf_s(ptr, ptrMax - ptr, "%d   ", wTimeStampMode);
				}
			}
			ptr += sprintf_s(ptr, ptrMax - ptr, "\n");
			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "debug";     
		keys_desc[ikey++] = "(RW) pco debug level [<new value in hex format (0x123)>]";     
		if(_stricmp(cmd, key) == 0){
			int nr;

			ptr += sprintf_s(ptr, ptrMax - ptr, "0x%llx",  m_pcoData->debugLevel);

			if((tokNr == 1)){
					nr = sscanf_s(tok[1], "0x%llx",  &m_pcoData->debugLevel);
					ptr += sprintf_s(ptr, ptrMax - ptr, "   %s>  ",  (nr == 1) ? "changed OK": "NOT changed");
					ptr += sprintf_s(ptr, ptrMax - ptr, "0x%llx",  m_pcoData->debugLevel);
			
					DEB_ALWAYS() << output ;
			}
			
#define _PRINT_DBG( x )	ptr += sprintf_s(ptr, ptrMax - ptr, "%15s  0x%08x\n", #x, x ) 	

			if((tokNr == 0)){
				ptr += sprintf_s(ptr, ptrMax - ptr, "\n");
				_PRINT_DBG( DBG_BUFF ) ;
				_PRINT_DBG( DBG_XFER2LIMA ) ;
				_PRINT_DBG( DBG_LIMABUFF ) ;
				_PRINT_DBG( DBG_EXP ) ;
				_PRINT_DBG( DBG_XFERMULT ) ;
				_PRINT_DBG( DBG_XFERMULT1 ) ;
				_PRINT_DBG( DBG_ASSIGN_BUFF ) ;
				_PRINT_DBG( DBG_DUMMY_IMG ) ;
				_PRINT_DBG( DBG_WAITOBJ ) ;
				_PRINT_DBG( DBG_XFER_IMG ) ;
				_PRINT_DBG( DBG_ROI ) ;
			}

			return output;
		}



		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "ADC";     
		keys_desc[ikey++] = "(RW) ADC working ADC [<new value>]";     
		if(_stricmp(cmd, key) == 0){
			int error;
			int adc_new, adc_working, adc_max;

			error = _pco_GetADCOperation(adc_working, adc_max);
			if((tokNr <1)){
				ptr += sprintf_s(ptr, ptrMax - ptr, "%d", adc_working);
				return output;
			}

			adc_new = atoi(tok[1]);
			error = _pco_SetADCOperation(adc_new, adc_working);
			ptr += sprintf_s(ptr, ptrMax - ptr, "%d", adc_working);

			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "ADCmax";     
		keys_desc[ikey++] = "(R) max nr of ADC";     
		if(_stricmp(cmd, key) == 0){
			int error;
			int adc_working, adc_max;

			error = _pco_GetADCOperation(adc_working, adc_max);
			if(error) adc_max = adc_working;
			ptr += sprintf_s(ptr, ptrMax - ptr, "%d", adc_max);
			
			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "camType";     
		keys_desc[ikey++] = "(R) cam type, interface, serial number, hardware & firmware version";     
		if(_stricmp(cmd, key) == 0){
			_camInfo(ptr, ptrMax, CAMINFO_CAMERATYPE);
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "camSN";     
		keys_desc[ikey++] = "(R) cam serial number";     
		if(_stricmp(cmd, key) == 0){
			ptr += sprintf_s(ptr, ptrMax - ptr, "%d", _getCameraSerialNumber());
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "lastError";     
		keys_desc[ikey++] = "(R) last PCO SDK error";     
		if(_stricmp(cmd, key) == 0){
			m_pcoData->pcoErrorMsg[ERR_SIZE] = 0;
			ptr += sprintf_s(ptr, ptrMax - ptr, "[x%08x] [%s]\n", 
				m_pcoData->pcoError, m_pcoData->pcoErrorMsg
				);
			
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "msgLog";     
		keys_desc[ikey++] = "(R) log of last cmds executed ";     
		if(_stricmp(cmd, key) == 0){

			ptr += m_msgLog->dump(ptr, (int)(ptrMax - ptr), 0);
			m_msgLog->dumpPrint(true);			
			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "msgLogFlush";     
		keys_desc[ikey++] = "flush log of last cmds executed ";     
		if(_stricmp(cmd, key) == 0){
			m_msgLog->flush(-1);			
			ptr += sprintf_s(ptr, ptrMax - ptr, "flushed ...");
			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "dumpData";     
		keys_desc[ikey++] = "(R) hex dump of the stcPcoData";     
		if(_stricmp(cmd, key) == 0){

			print_hex_dump_buff(m_pcoData, sizeof(stcPcoData));
			ptr += sprintf_s(ptr, ptrMax - ptr, "dumped\n");
			
			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "acqEnable";     
		keys_desc[ikey++] = "(R) acq enable signal status (BNC acq enbl in)";     
		if(_stricmp(cmd, key) == 0){
			int error;
			WORD wAcquEnableState;

			error = PcoCheckError(__LINE__, __FILE__, PCO_GetAcqEnblSignalStatus(m_handle, &wAcquEnableState));
			
			ptr += sprintf_s(ptr, ptrMax - ptr, "%d", wAcquEnableState);
			
			return output;
		}


		
		
		/***************************************************************
wSignalFilter: Flags showing the filter option:
- 0x01: Filter can be switched off (t > ~65ns)
- 0x02: Filter can be switched to medium (t > ~1us)
- 0x04: Filter can be switched to high (t > ~100ms)
Notes: the command will be rejected, if Recording State is [run]
***************************************************************/

		key = keys[ikey] = "hwioSignalsLin";     //----------------------------------------------------------------
		keys_desc[ikey++] = "(R) for DIMAX/EDGE only / get hw io signals";     
		if(_stricmp(cmd, key) == 0){
			int error, i;

			_pco_GetHWIOSignal(error);
			if(error) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "ERROR [%d]\n", error);
				//return output;
			}
			//ptr += sprintf_s(ptr, ptrMax - ptr, "signals [%d] [%d]\n", m_pcoData->wNrPcoHWIOSignal0, m_pcoData->wNrPcoHWIOSignal);
			
			for(i=0; i< m_pcoData->wNrPcoHWIOSignal; i++) 
			{
			    ptr += sprintf_s(ptr, ptrMax - ptr,"\n\n#======================== descriptor [%d]\n",i);
			    
			    WORD wSelected = 0;
				ptr += sprintf_s(ptr, ptrMax - ptr, 
					"sigNames[%s] [%s] [%s] [%s] idx[%d]/[%d] sigNum[%d] \n"
					"-def:     def[0x%x] type[0x%x] pol[0x%x] filt[0x%x]\n"
#if 0
					"-sig:    enab[0x%x] type[0x%x] pol[0x%x] filt[0x%x] signalSelected[0x%x]\n" 
#endif
					"-sig:    name[%s]\n\n", 
					m_pcoData->stcPcoHWIOSignalDesc[i].strSignalName[0],
					m_pcoData->stcPcoHWIOSignalDesc[i].strSignalName[1],
					m_pcoData->stcPcoHWIOSignalDesc[i].strSignalName[2],
					m_pcoData->stcPcoHWIOSignalDesc[i].strSignalName[3],
					i, m_pcoData->wNrPcoHWIOSignal,
//++					m_pcoData->stcPcoHWIOSignal[i][wSelected].wSignalNum,
					m_pcoData->stcPcoHWIOSignal[i].wSignalNum,

					m_pcoData->stcPcoHWIOSignalDesc[i].wSignalDefinitions,
					m_pcoData->stcPcoHWIOSignalDesc[i].wSignalTypes,
					m_pcoData->stcPcoHWIOSignalDesc[i].wSignalPolarity,
					m_pcoData->stcPcoHWIOSignalDesc[i].wSignalFilter,

#if 0
					m_pcoData->stcPcoHWIOSignal[i][wSelected].wEnabled,
					m_pcoData->stcPcoHWIOSignal[i][wSelected].wType,
					m_pcoData->stcPcoHWIOSignal[i][wSelected].wPolarity,
					m_pcoData->stcPcoHWIOSignal[i][wSelected].wFilterSetting,
					m_pcoData->stcPcoHWIOSignal[i][wSelected].wSelected,
#endif

					"DUMMY - TO DEFINE"
//++					&m_pcoData->sPcoHWIOSignalDesc[i][0]
					//str_printable(&m_pcoData->sPcoHWIOSignalDesc[i][0])
					//&(m_pcoData->sPcoHWIOSignalDesc[iSignal][0])
					);

			}

			for(i=0; i< m_pcoData->wNrPcoHWIOSignal; i++) 
			{
				WORD val;


/***************************************************************
wSignalDefinitions: Flags showing signal options:
- 0x01: Signal can be enabled/disabled
- 0x02: Signal is a status output
- 0x10: Signal function 1 has got parameter value
- 0x20: Signal function 2 has got parameter value
- 0x40: Signal function 3 has got parameter value
- 0x80: Signal function 4 has got parameter value
****************************************************************/
			    ptr += sprintf_s(ptr, ptrMax - ptr,
					"\n=============================\n"
					"OPTIONS of the selected signal / descriptor[%d]\n",
					i);

				val = m_pcoData->stcPcoHWIOSignalDesc[i].wSignalDefinitions;
				ptr += sprintf_s(ptr, ptrMax - ptr, "   def[0x%x]: ", val); 
                if(val & 0x01) ptr += sprintf_s(ptr, ptrMax - ptr, "[Signal can be enabled/disabled]");				
                if(val & 0x02) ptr += sprintf_s(ptr, ptrMax - ptr, "[Signal is a status output]");				
                if(val & 0x10) ptr += sprintf_s(ptr, ptrMax - ptr, "[Signal function 1 has got parameter value]");				
                if(val & 0x20) ptr += sprintf_s(ptr, ptrMax - ptr, "[Signal function 2 has got parameter value]");				
                if(val & 0x40) ptr += sprintf_s(ptr, ptrMax - ptr, "[Signal function 3 has got parameter value]");				
                if(val & 0x80) ptr += sprintf_s(ptr, ptrMax - ptr, "[Signal function 4 has got parameter value]");				
				ptr += sprintf_s(ptr, ptrMax - ptr, "\n"); 
					

/***************************************************************
wSignalTypes: Flags showing which signal type is available:
- 0x01: TTL
- 0x02: High Level TTL
- 0x04: Contact Mode
- 0x08: RS485 differential
***************************************************************/
				val = m_pcoData->stcPcoHWIOSignalDesc[i].wSignalTypes;
				ptr += sprintf_s(ptr, ptrMax - ptr, "   type[0x%x]: ", val); 
                if(val & 0x01) ptr += sprintf_s(ptr, ptrMax - ptr, "[TTL]");				
                if(val & 0x02) ptr += sprintf_s(ptr, ptrMax - ptr, "[High Level TTL]");				
                if(val & 0x04) ptr += sprintf_s(ptr, ptrMax - ptr, "[Contact Mode]");				
                if(val & 0x08) ptr += sprintf_s(ptr, ptrMax - ptr, "[RS485 differential]");				
				ptr += sprintf_s(ptr, ptrMax - ptr, "\n"); 


/***************************************************************
wSignalPolarity: Flags showing which signal polarity can be selected:
- 0x01: Low level active
- 0x02: High Level active
- 0x04: Rising edge active
- 0x08: Falling edge active
***************************************************************/
				val = m_pcoData->stcPcoHWIOSignalDesc[i].wSignalPolarity;
				ptr += sprintf_s(ptr, ptrMax - ptr, "   pol[0x%x]: ", val); 
                if(val & 0x01) ptr += sprintf_s(ptr, ptrMax - ptr, "[Low level active]");				
                if(val & 0x02) ptr += sprintf_s(ptr, ptrMax - ptr, "[High Level active]");				
                if(val & 0x04) ptr += sprintf_s(ptr, ptrMax - ptr, "[Rising edge active]");				
                if(val & 0x08) ptr += sprintf_s(ptr, ptrMax - ptr, "[Falling edge active]");				
				ptr += sprintf_s(ptr, ptrMax - ptr, "\n"); 

/***************************************************************
wSignalFilter: Flags showing the filter option:
- 0x01: Filter can be switched off (t > ~65ns)
- 0x02: Filter can be switched to medium (t > ~1us)
- 0x04: Filter can be switched to high (t > ~100ms)
Notes: the command will be rejected, if Recording State is [run]
***************************************************************/

				val = m_pcoData->stcPcoHWIOSignalDesc[i].wSignalFilter;
				ptr += sprintf_s(ptr, ptrMax - ptr, "   filter[0x%x]: ", val); 
                if(val & 0x01) ptr += sprintf_s(ptr, ptrMax - ptr, "[Filter can be switched off (t > ~65ns)]");				
                if(val & 0x02) ptr += sprintf_s(ptr, ptrMax - ptr, "[Filter can be switched to medium (t > ~1us)]");				
                if(val & 0x04) ptr += sprintf_s(ptr, ptrMax - ptr, "[Filter can be switched to high (t > ~100ms)]");				
				ptr += sprintf_s(ptr, ptrMax - ptr, "\n"); 



                for(WORD wSelected = 0; wSelected < 3 ; wSelected++)
                { 

//                    if(m_pcoData->stcPcoHWIOSignal[i][wSelected].wSelected <4)
                    if(m_pcoData->stcPcoHWIOSignalDesc[i].strSignalName[wSelected][0])
                    {
    			    ptr += sprintf_s(ptr, ptrMax - ptr,
						"\n"
						"[%s] STATUS of the selected signal / descriptor[%d] wSelected[%d]\n",
						m_pcoData->stcPcoHWIOSignalDesc[i].strSignalName[wSelected],
						i, wSelected);

				        //val = m_pcoData->stcPcoHWIOSignal[i][wSelected].wSelected;
				        //val = wSelected;
				        //ptr += sprintf_s(ptr, ptrMax - ptr, "   signalSelected[%d] [%s]\n", 
				        //    wSelected, m_pcoData->stcPcoHWIOSignalDesc[i].strSignalName[wSelected]);				

        /***************************************************************
        Enabled Flags showing enable state of the signal
         0x00: Signal is off
         0x01: Signal is active
        ***************************************************************/

//++				        val = m_pcoData->stcPcoHWIOSignal[i][wSelected].wEnabled;
				        val = m_pcoData->stcPcoHWIOSignal[i].wEnabled;
				        ptr += sprintf_s(ptr, ptrMax - ptr, "   enabled[0x%x]: ", val); 
						ptr += sprintf_s(ptr, ptrMax - ptr, 
							val ? "[Signal is active]" : "[Signal is off]");				
						ptr += sprintf_s(ptr, ptrMax - ptr, "\n"); 


        /***************************************************************
        Type Flags showing which signal type is selected
         0x01: TTL
         0x02: High Level TTL
         0x04: Contact Mode
         0x08: RS485 differential
        ***************************************************************/
//++				        val = m_pcoData->stcPcoHWIOSignal[i][wSelected].wType;
				        val = m_pcoData->stcPcoHWIOSignal[i].wType;
				        ptr += sprintf_s(ptr, ptrMax - ptr, "   type[0x%x]: ", val); 
                        if(val & 0x01) ptr += sprintf_s(ptr, ptrMax - ptr, "[TTL]");				
                        if(val & 0x02) ptr += sprintf_s(ptr, ptrMax - ptr, "[High Level TTL]");				
                        if(val & 0x04) ptr += sprintf_s(ptr, ptrMax - ptr, "[Contact Mode]");				
                        if(val & 0x08) ptr += sprintf_s(ptr, ptrMax - ptr, "[RS485 differential]");				
				        ptr += sprintf_s(ptr, ptrMax - ptr, "\n"); 


        /***************************************************************
        Polarity Flags showing which signal polarity is selected
         0x01: High level active
         0x02: Low level active
         0x04: Rising edge active
         0x08: Falling edge active
        ***************************************************************/

//++				        val = m_pcoData->stcPcoHWIOSignal[i][wSelected].wPolarity;
				        val = m_pcoData->stcPcoHWIOSignal[i].wPolarity;
				        ptr += sprintf_s(ptr, ptrMax - ptr, "   pol[0x%x]: ", val); 
                        if(val & 0x01) ptr += sprintf_s(ptr, ptrMax - ptr, "[Low level active]");				
                        if(val & 0x02) ptr += sprintf_s(ptr, ptrMax - ptr, "[High Level active]");				
                        if(val & 0x04) ptr += sprintf_s(ptr, ptrMax - ptr, "[Rising edge active]");				
                        if(val & 0x08) ptr += sprintf_s(ptr, ptrMax - ptr, "[Falling edge active]");				
				        ptr += sprintf_s(ptr, ptrMax - ptr, "\n"); 


        /***************************************************************
        FilterSetting Flags showing the filter option which is selected
         0x01: Filter can be switched off (t > ~65ns)
         0x02: Filter can be switched to medium (t > ~1 u)
         0x04: Filter can be switched to high (t > ~100ms)
        Selected In case the HWIOSignaldescription shows more than one SignalNames, this parameter can be
        used to select a different signal, e.g. ’Status Busy’ or ’Status Exposure’.
        ***************************************************************/

//++				        val = m_pcoData->stcPcoHWIOSignal[i][wSelected].wFilterSetting;
				        val = m_pcoData->stcPcoHWIOSignal[i].wFilterSetting;
				        ptr += sprintf_s(ptr, ptrMax - ptr, "   filter[0x%x]: ", val); 
                        if(val & 0x01) ptr += sprintf_s(ptr, ptrMax - ptr, "[Filter can be switched off (t > ~65ns)]");				
                        if(val & 0x02) ptr += sprintf_s(ptr, ptrMax - ptr, "[Filter can be switched to medium (t > ~1us)]");				
                        if(val & 0x04) ptr += sprintf_s(ptr, ptrMax - ptr, "[Filter can be switched to high (t > ~100ms)]");				
				        ptr += sprintf_s(ptr, ptrMax - ptr, "\n"); 


                    }
				}
					//print_hex_dump_buff(&m_pcoData->stcPcoHWIOSignalDesc[i].szSignalName[0][0], 24*4);
			}

			
			return output;
		}

		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "hwioSignals";     
		keys_desc[ikey++] = "(R) for DIMAX/EDGE only / get hw io signals";     
		if(_stricmp(cmd, key) == 0){
			int error, i;

			_pco_GetHWIOSignal(error);
			if(error) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "ERROR [%d]", error);
				return output;
			}
			//ptr += sprintf_s(ptr, ptrMax - ptr, "signals [%d] [%d]\n", m_pcoData->wNrPcoHWIOSignal0, m_pcoData->wNrPcoHWIOSignal);
			
			for(i=0; i< m_pcoData->wNrPcoHWIOSignal; i++) {
				ptr += sprintf_s(ptr, ptrMax - ptr, 
					"name[%s] [%s] [%s] [%s] idx[%d] num[%d] \n"
					"-def:     def[0x%x] type[0x%x] pol[0x%x] filt[0x%x]\n"
					"-sig:    enab[0x%x] type[0x%x] pol[0x%x] filt[0x%x] sel[0x%x]\n\n", 
					m_pcoData->stcPcoHWIOSignalDesc[i].strSignalName[0],
					m_pcoData->stcPcoHWIOSignalDesc[i].strSignalName[1],
					m_pcoData->stcPcoHWIOSignalDesc[i].strSignalName[2],
					m_pcoData->stcPcoHWIOSignalDesc[i].strSignalName[3],
					i, 
					m_pcoData->stcPcoHWIOSignal[i].wSignalNum,

					m_pcoData->stcPcoHWIOSignalDesc[i].wSignalDefinitions,
					m_pcoData->stcPcoHWIOSignalDesc[i].wSignalTypes,
					m_pcoData->stcPcoHWIOSignalDesc[i].wSignalPolarity,
					m_pcoData->stcPcoHWIOSignalDesc[i].wSignalFilter,

					m_pcoData->stcPcoHWIOSignal[i].wEnabled,
					m_pcoData->stcPcoHWIOSignal[i].wType,
					m_pcoData->stcPcoHWIOSignal[i].wPolarity,
					m_pcoData->stcPcoHWIOSignal[i].wFilterSetting,
					m_pcoData->stcPcoHWIOSignal[i].wSelected
					);
			}

			
			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "sethwioSignals";     
		keys_desc[ikey++] = "(R) for DIMAX only / get hw io signals";     
		if(_stricmp(cmd, key) == 0){
			int error, idx;
			WORD val;

			if(tokNr != 1){
				ptr += sprintf_s(ptr, ptrMax - ptr, "ERROR tokNr[%d]", tokNr);
				return output;
			}

			_pco_GetHWIOSignal(error);
			if(error) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "ERROR [%d]", error);
				return output;
			}

    		val = atoi(tok[1]);
				

			idx = 0;
			m_pcoData->stcPcoHWIOSignal[idx].wPolarity = val;

	
			_pco_SetHWIOSignal(idx,error);
			
			ptr += sprintf_s(ptr, ptrMax - ptr, "error [%d]", error);

			
			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "dumpData";     
		keys_desc[ikey++] = "(R) hex dump of the stcPcoData";     
		if(_stricmp(cmd, key) == 0){

			print_hex_dump_buff(m_pcoData, sizeof(stcPcoData));
			ptr += sprintf_s(ptr, ptrMax - ptr, "dumped\n");
			
			return output;
		}



		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "winMem";     
		keys_desc[ikey++] = "(R) read win memory";     
		if(_stricmp(cmd, key) == 0){
			const float gB = 1024. * 1024. * 1024.;
			MEMORYSTATUSEX statex;
			statex.dwLength = sizeof(statex);
			
			GlobalMemoryStatusEx(&statex);

			ptr += sprintf_s(ptr, ptrMax - ptr, "               dwLength [%u]\n", statex.dwLength);
			ptr += sprintf_s(ptr, ptrMax - ptr, "           dwMemoryLoad [%u]\n", statex.dwMemoryLoad);
			ptr += sprintf_s(ptr, ptrMax - ptr, "           ullTotalPhys [%g GB][%llu]\n", statex.ullTotalPhys/gB, statex.ullTotalPhys);
			ptr += sprintf_s(ptr, ptrMax - ptr, "           ullAvailPhys [%g GB][%llu]\n", statex.ullAvailPhys/gB, statex.ullAvailPhys);

			ptr += sprintf_s(ptr, ptrMax - ptr, "       ullTotalPageFile [%g GB][%llu]\n", statex.ullTotalPageFile/gB, statex.ullTotalPageFile);
			ptr += sprintf_s(ptr, ptrMax - ptr, "       ullAvailPageFile [%g GB][%llu]\n", statex.ullAvailPageFile/gB, statex.ullAvailPageFile);

			ptr += sprintf_s(ptr, ptrMax - ptr, "        ullTotalVirtual [%g GB][%llu]\n", statex.ullTotalVirtual/gB, statex.ullTotalVirtual);
			ptr += sprintf_s(ptr, ptrMax - ptr, "        ullAvailVirtual [%g GB][%llu]\n", statex.ullAvailVirtual/gB, statex.ullAvailVirtual);
			ptr += sprintf_s(ptr, ptrMax - ptr, "ullAvailExtendedVirtual [%g GB][%llu]\n", statex.ullAvailExtendedVirtual/gB, statex.ullAvailExtendedVirtual);


			return output;
		}


		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "comment";     
		keys_desc[ikey++] = "(W) print timestamp & comment in the screen";     
		if(_stricmp(cmd, key) == 0){
			char *comment = str_trim(cmdBuffAux + strlen(cmd));

			ptr += sprintf_s(ptr, ptrMax - ptr, _sprintComment(comment) );

			DEB_ALWAYS() << output ;
			return output;
		}

#if 0
		//----------------------------------------------------------------------------------------------------------
		key = keys[ikey] = "?";     
		keys_desc[ikey++] = "(R) this help / list of the talk cmds";     
		if(_stricmp(cmd, key) == 0){
			for(int i = 0; i < ikey; i++) {
				ptr += sprintf_s(ptr, ptrMax - ptr, "%18s - %s\n", keys[i], keys_desc[i]);
			}
			ptr += sprintf_s(ptr, ptrMax - ptr, "--- nrCmds[%d][%d]\n", ikey, NRCMDS);
			return output;
		}


		sprintf_s(ptr, ptrMax - ptr, "ERROR unknown cmd [%s]", cmd);
		return output;
}
#endif


		//----------------------------------------------------------------------------------------------------------
		// this must be the last cmd
		//----------------------------------------------------------------------------------------------------------
		if(ikey >= NRCMDS) 
		{
			char *msg =  "FATAL ERROR - too many talk cmds - increase NRCMDS & recompile";
			DEB_ALWAYS() << msg;		
			throw LIMA_HW_EXC(Error, msg);
		}

		key = keys[ikey] = "?";     
		keys_desc[ikey++] = "(R) this help / list of the talk cmds";     
		if(_stricmp(cmd, key) == 0){
			int i, j, ikeyMax;
			char *ptri, *ptrj;
			size_t len = 0;

			ikeyMax = ikey;

			for(i = 0; i < ikeyMax; i++) 
			{
				for(j = i; j < ikeyMax; j++) 
				{
					ptri = keys[i]; ptrj = keys[j];
					if(_stricmp(ptri,ptrj) > 0)
					{
						keys[j] = ptri;
						keys[i] = ptrj;
						ptri = keys_desc[i];
						keys_desc[i] = keys_desc[j];
						keys_desc[j] = ptri;
					}
				}
				len = max(len, (strlen(keys[i])));
			}

			for(i = 0; i < ikeyMax; i++) 
			{
				ptr += sprintf_s(ptr, ptrMax - ptr, "%*s - %s\n", -(int) len, keys[i], keys_desc[i]);
			}
			ptr += sprintf_s(ptr, ptrMax - ptr, "--- nrCmds[%d][%d]\n", ikeyMax, NRCMDS);
			return output;
		}


		sprintf_s(ptr, ptrMax - ptr, "ERROR unknown cmd [%s]", cmd);
		return output;
}





//====================================================================
// utils
//====================================================================

#define LEN_LINE_BUFF	128
#define BYTES_PER_LINE		16
#define OFFSET_COL_HEX1	(4 + 1 + 3)
#define OFFSET_COL_HEX2	(OFFSET_COL_HEX1 + (8 * 3) + 2)
#define OFFSET_COL_ASC1	(OFFSET_COL_HEX2 + (8 * 3) + 3)
#define OFFSET_COL_ASC2	(OFFSET_COL_ASC1 + 8 + 1)

//--------------------------------------------------------------------
//--------------------------------------------------------------------
char *nibble_to_hex(char *s, BYTE nibble)
{
  nibble &= 0x0f;
  *s = (nibble <= 9) ?  '0' + nibble : 'a' + nibble - 10;
  return s+1;
}



//--------------------------------------------------------------------
//--------------------------------------------------------------------
char *byte_to_hex(char *s, BYTE byte)
{
  s = nibble_to_hex(s, byte >> 4);
  return nibble_to_hex(s, byte);;
}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
char *word_to_hex(char *s, WORD word)
{
  s = byte_to_hex(s, word >> 8);
  return byte_to_hex(s, (BYTE) word);
  
}


//--------------------------------------------------------------------
//--------------------------------------------------------------------
char *dword_to_hex(char *s, DWORD dword)
{
  s = word_to_hex(s, dword >> 16);
  return word_to_hex(s, (WORD) dword);
  
}


//--------------------------------------------------------------------
//--------------------------------------------------------------------
char *_hex_dump_bytes(void *obj, size_t lenObj, char *buff, size_t lenBuff) {
	char *ptr = buff;
	char *ptrMax = buff + lenBuff;
	char *ptrObj = (char *) obj;

	memset(buff, ' ', lenBuff);

	for(unsigned int i=0; i< lenObj; i++) {

		if(ptr+3 > ptrMax) break;
		ptr = byte_to_hex(ptr, *ptrObj ++);
		if(ptr+1 == ptrMax) break;
		ptr++;

	}

	*ptr = 0; 
	return buff;

}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
const char *hex_dump_line(void *buff, size_t len, size_t *nr, WORD *offset) {

	static char line_buff[LEN_LINE_BUFF];

	char *s;
	BYTE *ptr_buff;
	BYTE *ptr_buff0;
	int i, nr1, nr2;

	ptr_buff = (BYTE *) buff;
	ptr_buff0 = ptr_buff;

	s = line_buff;


	if(len <= 0) {
		strcpy_s(s,LEN_LINE_BUFF,"<empty>");
		return line_buff;
	}

	memset(line_buff, ' ', LEN_LINE_BUFF);

	*nr = (len < BYTES_PER_LINE) ? len : BYTES_PER_LINE;

	s = word_to_hex(s, *offset);
	*offset += (WORD) *nr;
	*s = ':';
	s = line_buff + OFFSET_COL_HEX1;

	if(*nr <= BYTES_PER_LINE / 2) {
		nr1 = (int) *nr;
		nr2 = -1;
	} else {
		nr1 = BYTES_PER_LINE / 2;
		nr2 = ((int) *nr) - nr1;
	}

	for(i = 0; i < nr1; i++) {
		s = byte_to_hex(s, *ptr_buff++);
		s++;
	}

	if(nr2 >0 ){
		*s = '-';
		s = line_buff + OFFSET_COL_HEX2;
		for(i = 0; i < nr2; i++) {
			s = byte_to_hex(s, *ptr_buff++);
			s++;
		}
	}	

	ptr_buff = ptr_buff0;
	s = line_buff + OFFSET_COL_ASC1;
	for(i = 0; i < nr1; i++) {
		*s++ = (isprint(*ptr_buff)) ? *ptr_buff : '.';
		ptr_buff++;
	}

	if(nr2 >0 ){
		s = line_buff + OFFSET_COL_ASC2;
		for(i = 0; i < nr2; i++) {
			*s++ = (isprint(*ptr_buff)) ? *ptr_buff : '.';
			ptr_buff++;
		}
	}	
	
	*s = 0;

	return line_buff;

}

//--------------------------------------------------------------------
//--------------------------------------------------------------------
void print_hex_dump_buff(void *ptr_buff, size_t len) {
	WORD offset = 0;
	size_t nr = 0;
	BYTE * ptr = (BYTE *) ptr_buff;
	;
	
	printf("dump buff / len: %d\n", len);
	
	while(len > 0) {
		printf("%s\n", hex_dump_line(ptr, len, &nr, &offset));
		len -= nr;
		ptr += nr;
	}

}


//--------------------------------------------------------------------
//--------------------------------------------------------------------


ringLog::ringLog(int size) {
        buffer = new struct data [size];
        m_capacity = m_capacity_max = size;
        m_size = 0;
        m_head = 0;
}
void ringLog::flush(int capacity) {
		if((capacity > 0) &&(capacity <= m_capacity_max))
			m_capacity = capacity;
        m_size = 0;
        m_head = 0;
}

ringLog::~ringLog() {
        delete buffer;
}

int ringLog::add(char *s) {

        struct data *ptr;
        int offset;
        
        if (m_size < m_capacity){
                offset = (m_head + m_size) % m_capacity;
                ptr = buffer + offset;
                m_size++;
        } else {
                ptr = buffer + m_head;
                m_head = (m_head + 1) % m_capacity;
                m_size = m_capacity;
        }
        
        ptr->timestamp = getTimestamp();
        strncpy_s(ptr->str, s,RING_LOG_BUFFER_SIZE);
        return m_size;

}


void ringLog::dumpPrint(bool direction) {

        static char timeline[128];
        struct data *ptr;
        int offset;
        time_t ltime;
        struct tm today;
        char *fmt = "%Y/%m/%d %H:%M:%S";
        int i;
        errno_t err;
        
        for(i=0; i< m_size; i++) {
        
                offset = direction ? i : m_size -1 -i;
                
                ptr = buffer + (m_head + offset) % m_capacity;
                ltime = ptr->timestamp;

				err = localtime_s( &today, &ltime );

                strftime(timeline, 128, fmt, &today);
                
                printf("%s> %s\n", timeline, ptr->str);
        }
        
}

int ringLog::dump(char *s, int lgMax, bool direction) {

        static char timeline[128];
        struct data *ptr;
        int offset;
        time_t ltime;
        struct tm today;
        char *fmt = "%Y/%m/%d %H:%M:%S";
		int linMax = 25 + RING_LOG_BUFFER_SIZE;
        int i;
		char *ptrOut;
        errno_t err;
        int lg = 0;
		ptrOut = s;

        for(i=0; (i< m_size) && ((lgMax - lg) > linMax); i++) {
        
                offset = direction ? i : m_size -1 -i;
                
                ptr = buffer + (m_head + offset) % m_capacity;
                ltime = ptr->timestamp;

				err = localtime_s( &today, &ltime );

                lg += (int) strftime(s + lg, lgMax - lg, fmt, &today);
                lg += sprintf_s(s + lg, lgMax - lg, "> %s\n", ptr->str);
        }
        
		return lg;
}

//=========================================================================================================
//=========================================================================================================

unsigned long long Camera::_getDebug(unsigned long long mask = ULLONG_MAX){

		return m_pcoData->debugLevel & mask;

}
//=========================================================================================================
//=========================================================================================================

char *Camera::_checkLogFiles(bool firstCall) {
	const char *logFiles[] = {
		"C:\\ProgramData\\pco\\SC2_Cam.log", 
		"C:\\ProgramData\\pco\\PCO_CDlg.log", 
		"C:\\ProgramData\\pco\\PCO_Conv.log",
        "C:\\ProgramData\\pco\\me4_memlog_end.log",
		NULL};
	const char **ptr = logFiles;
	char *logOn = "\n\n"		
		"###############################################################################\n"
		"###############################################################################\n"
		"###############################################################################\n"
		"###                                                                         ###\n"
		"###                           !!!  ATTENTION !!!                            ###\n"
		"###                                                                         ###\n"
		"###                     THE PCO LOG FILES ARE ENABLED                       ###\n"
		"###                                                                         ###\n"
		"###                 this option is ONLY for DEBUG & TESTS                   ###\n"
		"###                                                                         ###\n"
		"###                   it downgrades the acquisition time                    ###\n"
		"###                                                                         ###\n"
		"###                                                                         ###\n"
		"###     to DISABLE it:                                                      ###\n"
		"###          * stop the device server                                       ###\n"
		"###          * open the directory C:\\ProgramData\\pco\\                       ###\n"
		"###          * rename all the .log files as .txt                            ###\n"
		"###          * start again the device server                                ###\n"
		"###                                                                         ###\n"
		"###############################################################################\n"
		"###############################################################################\n"
		"###############################################################################\n\n\n";

	char *logOff = "";
	struct stat fileStat;
	int error;
	bool found = false;


	if(firstCall) {
		while(*ptr != NULL) {
			error = stat(*ptr, &fileStat);
			//printf("----------- [%d][%s]\n", error, *ptr);
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
void printError( TCHAR* msg );

char * _getComputerName(char *infoBuff, DWORD  bufCharCount  )
{

  // Get and display the name of the computer. 
  if( !GetComputerName( infoBuff, &bufCharCount ) )
	  sprintf_s(infoBuff, bufCharCount, "ERROR: GetComputerName" ); 

  return infoBuff ;
}

char * _getUserName(char *infoBuff, DWORD  bufCharCount  )
{
 
  // Get and display the user name. 
  if( !GetUserName( infoBuff, &bufCharCount ) )
	  sprintf_s(infoBuff, bufCharCount, "ERROR: GetUserName" ); 
  return infoBuff ;
}

//====================================================================
//====================================================================
#define PCOSDK_FILENAME "sc2_cam.dll"

#include <winver.h>
#pragma comment(lib, "version.lib")

struct TRANSLATION {
  WORD langID;
  WORD charset;
} ;

int _getFileVerStruct(const TCHAR* pzFileName, int* ima, int* imi, int* imb, TCHAR* pcver, int ipclen)
{
  TCHAR* pzStr;
  DWORD dwVerInfoSize = 0;
  DWORD dwVerHnd = 0;
  TCHAR* lpstrVffInfo;
  UINT VersionLen;
  TRANSLATION Translation;

  // extracting the version info structure size
  if ((dwVerInfoSize = GetFileVersionInfoSize(pzFileName, &dwVerHnd)) > 0)
  {
    LPVOID lpvi;
    UINT iLen;
    lpstrVffInfo = (TCHAR*)malloc(dwVerInfoSize*sizeof(TCHAR));
    
    GetFileVersionInfo(pzFileName, dwVerHnd, dwVerInfoSize, lpstrVffInfo);
    // extracting the language/character value from the file.
    if ((VerQueryValue((LPVOID)lpstrVffInfo, TEXT("\\VarFileInfo\\Translation"),
      &lpvi, &iLen)) > 0)
    {
      Translation = *(TRANSLATION*)lpvi;
      
      pzStr = _T("");
      TCHAR strVerCar[_MAX_PATH] = _T("\0");
      _stprintf_s(strVerCar, sizeof(strVerCar)/sizeof(TCHAR), _T("\\StringFileInfo\\%04x%04x\\%s"),
        Translation.langID,
        Translation.charset,
        _T("ProductVersion"));
      // querying the file with the correct language/character value.
      VerQueryValue((LPVOID)lpstrVffInfo,
        strVerCar,
        (LPVOID *)&pzStr,
        &VersionLen);
      
      unsigned int j = 0;

      if(_tcsstr(pzStr, _T(",")))
        _stscanf_s(pzStr, _T("%d,%d,%d,%d"), ima, imi, &j, imb);
      else
        _stscanf_s(pzStr, _T("%d.%d.%d.%d"), ima, imi, &j, imb);
      if(pcver != NULL)
      {
        if(_tcslen(pzStr) < (unsigned int)ipclen)
          _tcscpy_s(pcver, ipclen, pzStr);
      }
    }
    else
    {
      // an error occurred trying to retrieve the version structure
      return -1;
    }
    free(lpstrVffInfo);
  }
  else
  {
    // could not open or find file to get the version information
    return -1;
  }
 
  return 0;
}

#define LEN_DRIVE	7
#define LEN_DIR		MAX_PATH

//====================================================================
//====================================================================
char * _getDllPath(const char* pzFileName, char *path, size_t strLen)
{
	errno_t err;

	char drive[LEN_DRIVE+1];
	char dir[LEN_DIR+1];
	char _pathFn[MAX_PATH+1];
	char _pathFnInstall[MAX_PATH+1];
	char *ptr;
	size_t nr;
	FILE *stream;

	*path = 0;

	GetModuleFileName(GetModuleHandle(pzFileName), _pathFn, MAX_PATH);

	err = _splitpath_s(_pathFn, drive, LEN_DRIVE, dir, LEN_DIR, NULL, 0, NULL, 0);

	size_t l = strlen(dir);
	ptr = dir + l -2;
	while(*ptr != '\\') ptr--;
	*ptr = 0;

	ptr = path;
	err = _makepath_s(_pathFnInstall, MAX_PATH, drive, dir, FILENAME_INSTALL_VERSION, FILEEXT_INSTALL_VERSION);
	printf("----- path[%s] path1[%s] drive[%s] dir[%s]\n", _pathFn, _pathFnInstall, drive, dir);
	nr = sprintf_s(ptr, strLen-1,"%s\n", _pathFnInstall);
    ptr += nr;
	strLen -= nr;

	err  = fopen_s( &stream, _pathFnInstall, "r" );
#if 0
	if(err == 0) {
		if( fgets( path, strLen, stream ) == NULL){
			*ptr = 0;
		}
	}
#endif
	nr = fread(ptr, 1, strLen-1, stream); 
	ptr[nr] = 0;
    fclose( stream );
	return path;
}



char * _getPcoSdkVersion(char *infoBuff, int strLen, char *lib)
{
	int ima, imi, imb;
	//char *lib = PCOSDK_FILENAME;
	int nr;
	char *ptr = infoBuff;

	nr = sprintf_s(ptr, strLen, "file[%s] ver[", lib);

	if(_getFileVerStruct(lib, &ima, &imi, &imb, ptr+nr, strLen-nr-2))
	{
		strncat_s(ptr, strLen, "NOT FOUND", _TRUNCATE);
	}
	else
	{
		strncat_s(ptr, strLen, "]", _TRUNCATE);
	}

	return infoBuff ;
}


//====================================================================
//====================================================================
char * Camera::_camInfo(char *ptr, char *ptrMax, long long int flag)
{
	DEB_MEMBER_FUNCT();

	long long int lgbuff, lgbuffmax;
	lgbuffmax = ptrMax - ptr;

	if(flag & CAMINFO_BLOCK) {
		ptr += sprintf_s(ptr, ptrMax - ptr, "\n* camInfo [begin]\n");
		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... timestamp[%s]\n", getTimestamp(Iso));

	}


	//--------------- CAMERA TYPE
	if(flag & CAMINFO_CAMERATYPE) {

		ptr += sprintf_s(ptr, ptrMax - ptr, "* camera type \n");

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... cam_name[%s]\n", m_pcoData->camera_name);

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... dwSerialNumber[%d]\n", 
			_getCameraSerialNumber());

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... wCamType[0x%x] [%s]\n", 
			_getCameraType(), _getCameraTypeStr()); 

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... wCamSubType[0x%x] [%s]\n", 
			_getCameraSubType(), _getCameraSubTypeStr()); 

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... wInterfaceType[0x%x] [%s]\n", 
			_getInterfaceType(), _getInterfaceTypeStr()); 
	
	}
	
	//--------------- general info
	if(flag & CAMINFO_GENERAL) {

		ptr += sprintf_s(ptr, ptrMax - ptr, "* general info \n");

		
		// OBSOLETE after sdk 120
		//ptr += sprintf_s(ptr, ptrMax - ptr, "* ... GigE IP [%d.%d.%d.%d]\n", 
		//	m_pcoData->ipField[0], m_pcoData->ipField[1], m_pcoData->ipField[2], m_pcoData->ipField[3]);
		
		double pixSizeX, pixSizeY;
		_get_PixelSize(pixSizeX, pixSizeY);
		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... pixelSize (um) [%g,%g] \n",  pixSizeX, pixSizeY);
	}
			
	//--------------- version
	if(flag & CAMINFO_VERSION) {
		ptr += sprintf_s(ptr, ptrMax - ptr, "* lima version \n");
		ptr += sprintf_s(ptr, ptrMax - ptr, "%s", m_pcoData->version);
	}
	//--------------- firmware
	if(flag & CAMINFO_FIRMWARE) {
		ptr += sprintf_s(ptr, ptrMax - ptr, "* firmware \n");
		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... firmware dwHWVersion[%lx]  dwFWVersion[%lx] <- not used\n", 
			m_pcoData->stcPcoCamType.dwHWVersion, 
			m_pcoData->stcPcoCamType.dwFWVersion);
		

		int nrDev, iDev;

		nrDev=m_pcoData->stcPcoCamType.strHardwareVersion.BoardNum;
		ptr += sprintf_s(ptr, ptrMax - ptr, "* Hardware_DESC device[%d]  szName          wBatchNo/wRevision   wVariant\n", nrDev);
		for(iDev = 0; iDev< nrDev; iDev++) {
			PCO_SC2_Hardware_DESC *ptrhw;
			ptrhw = &m_pcoData->stcPcoCamType.strHardwareVersion.Board[iDev];
			ptr += sprintf_s(ptr, ptrMax - ptr, "* %20d      %-18s   %4d.%-4d    %4d\n", 
				iDev, 
				ptrhw->szName,
				ptrhw->wBatchNo,
				ptrhw->wRevision,
				ptrhw->wVariant
				);
		}

		nrDev=m_pcoData->stcPcoCamType.strFirmwareVersion.DeviceNum;
		ptr += sprintf_s(ptr, ptrMax - ptr, 
			"* Firmware_DESC device[%d]  szName          bMajorRev/Minor   wVariant\n", nrDev);

		for(iDev = 0; iDev< nrDev; iDev++) {
			PCO_SC2_Firmware_DESC *ptrfw;
			ptrfw = &m_pcoData->stcPcoCamType.strFirmwareVersion.Device[iDev];
			ptr += sprintf_s(ptr, ptrMax - ptr, "* %20d      %-18s   %4d.%-4d    %4d\n", 
				iDev,
				ptrfw->szName,
				ptrfw->bMajorRev,
				ptrfw->bMinorRev,
				ptrfw->wVariant
				);
		}

		PCO_FW_Vers strFirmwareVersion;
		WORD wblock = 0;
		int iCnt, err;
		err =  PCO_GetFirmwareInfo(m_handle, wblock++, &strFirmwareVersion);
		nrDev = (err == PCO_NOERROR) ? strFirmwareVersion.DeviceNum : 0;

		if(nrDev > 0){
			ptr += sprintf_s(ptr, ptrMax - ptr, 
				"* Firmware_DESC device[%d]  szName          bMajorRev/Minor   wVariant (PCO_GetFirmwareInfo)\n", 
				nrDev);

			for(iDev = 0, iCnt = 0; iDev< nrDev; iDev++, iCnt++) {
				PCO_SC2_Firmware_DESC *ptrfw;
				if(iCnt >= 10) {
					iCnt = 0;
					err =  PCO_GetFirmwareInfo(m_handle, wblock++, &strFirmwareVersion);
					if (err != PCO_NOERROR) break;
				} // iCnt
				
				ptrfw = &strFirmwareVersion.Device[iCnt];
				ptr += sprintf_s(ptr, ptrMax - ptr, "* %20d      %-18s   %4d.%-4d    %4d\n", 
					iDev, ptrfw->szName, ptrfw->bMajorRev, ptrfw->bMinorRev, ptrfw->wVariant);
			} // for
		} // if nrDev
	}

	//--------------- adc, pixelrate, 
	if(flag & CAMINFO_ADC) {
		int err;
		ptr += sprintf_s(ptr, ptrMax - ptr, "*** adc\n");
		int adc_working, adc_max;

		err = _pco_GetADCOperation(adc_working, adc_max);
		ptr += sprintf_s(ptr, ptrMax - ptr, "* ADC working[%d] max[%d]\n", 
				adc_working, adc_max);
	}

	if(flag & CAMINFO_PIXELRATE) {
		DWORD dwPixRate, dwPixRateNext ; int error, i;
		_pco_GetPixelRate(dwPixRate, dwPixRateNext, error);

		ptr += sprintf_s(ptr, ptrMax - ptr, "* pixelRate\n");
		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... pixelRate[%ld](%g MHz)\n", dwPixRate, dwPixRate/1000000.);
		ptr += sprintf_s(ptr, ptrMax - ptr, "* ...    pixelRateRequested[%ld](%g MHz) \n", 	dwPixRateNext, dwPixRateNext/1000000.);
		ptr += sprintf_s(ptr, ptrMax - ptr, "* ...    valid pixelRates "); 
		for(i=0; i<4; i++) {
			dwPixRate = m_pcoData->stcPcoDescription.dwPixelRateDESC[i];
			if(dwPixRate){ptr += sprintf_s(ptr, ptrMax - ptr, " [%ld]",dwPixRate);}  
		}	
		ptr += sprintf_s(ptr, ptrMax - ptr, "\n",dwPixRate);
	}


	//--------------------- size, roi, ...
	if(flag & CAMINFO_ROI){
		ptr += sprintf_s(ptr, ptrMax - ptr, "* size, roi, ... \n");
		unsigned int maxWidth, maxHeight,Xstep, Ystep; 
		getMaxWidthHeight(maxWidth, maxHeight);
		getXYsteps(Xstep, Ystep);

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... maxWidth=[%d] maxHeight=[%d] \n",  maxWidth,  maxHeight);
		ptr += sprintf_s(ptr, ptrMax - ptr, "* ...    Xstep=[%d] Ystep=[%d] (PCO ROI steps)\n",  Xstep,  Ystep);

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... wXResActual=[%d] wYResActual=[%d] \n",  m_pcoData->wXResActual,  m_pcoData->wYResActual);

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... wXResMax=[%d] wYResMax=[%d] \n",  m_pcoData->wXResMax,  m_pcoData->wYResMax);


		unsigned int x0,x1,y0,y1;
		_get_Roi(x0, x1, y0, y1);

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... roi X[%d,%d] Y[%d,%d] size[%d,%d]\n",  
				x0, x1, y0, y1, x1-x0+1, y1-y0+1);

		Roi limaRoi;
		_get_Roi(limaRoi);
		Point top_left = limaRoi.getTopLeft();
		Point bot_right = limaRoi.getBottomRight();
		Size size = limaRoi.getSize();			

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... roiLima XY0[%d,%d] XY1[%d,%d] size[%d,%d]\n",  
						top_left.x, top_left.y,
						bot_right.x, bot_right.y,
						size.getWidth(), size.getHeight());

		unsigned int bytesPerPix; getBytesPerPixel(bytesPerPix);
		WORD bitsPerPix; getBitsPerPixel(bitsPerPix);


		long long imgSizePix = size.getWidth()* size.getHeight();
		long long imgSize = imgSizePix * bytesPerPix;
		double imgSizeMb =  imgSize/(1024.*1024.);
		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... imgSize[%lld px][%lld B][%g MB] \n",  
				imgSizePix, imgSize, imgSizeMb);
		ptr += sprintf_s(ptr, ptrMax - ptr, "* bitsPerPix[%d] bytesPerPix[%d] \n",  bitsPerPix,  bytesPerPix);

		int iFrames;
		m_sync->getNbFrames(iFrames);
		double totalImgSizeMb =  imgSizeMb * iFrames;
		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... m_sync->getNbFrames=[%d frames] totalSize[%g MB]\n", iFrames, totalImgSizeMb);

	}
	//--------------------- exp, coc, ...
	if(flag & CAMINFO_EXP){
		ptr += sprintf_s(ptr, ptrMax - ptr, "* exp, coc, ... \n");
		double _exposure, _delay;
		struct lima::HwSyncCtrlObj::ValidRangesType valid_ranges;
		m_sync->getValidRanges(valid_ranges);
		m_sync->getExpTime(_exposure);
		m_sync->getLatTime(_delay);

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ... exp[%g ms][%g s] delay[%g ms][%g s]\n", 
			_exposure*1.0e3,_exposure, 
			_delay*1.0e3, _delay); 

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ...    exp valid min[%g us] max[%g ms] step[%g us]\n", 
			valid_ranges.min_exp_time * 1.0e6, 
			valid_ranges.max_exp_time * 1.0e3,
			m_pcoData->stcPcoDescription.dwMinExposureStepDESC * 1.0e-3  );

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ...    delay valid min[%g us] max[%g ms] step[%g us]\n", 
			valid_ranges.min_lat_time * 1.0e6, 
			valid_ranges.max_lat_time * 1.0e3,
			m_pcoData->stcPcoDescription.dwMinDelayStepDESC * 1.0e-3 );
			

		ptr += sprintf_s(ptr, ptrMax - ptr, "* ...    cocRunTime[%g] (s) frameRate[%g] (fps)\n",  
			m_pcoData->cocRunTime, m_pcoData->frameRate);
			
	}


    //------ DIMAX
	if( (flag & CAMINFO_DIMAX) && (_isCameraType(Dimax | Pco2k | Pco4k)) ){
		unsigned int bytesPerPix; getBytesPerPixel(bytesPerPix);
		int segmentPco = m_pcoData->wActiveRamSegment;
		int segmentArr = segmentPco -1;

		ptr += sprintf_s(ptr, ptrMax - ptr, "*** DIMAX - 2k - 4k info \n");
		ptr += sprintf_s(ptr, ptrMax - ptr, "* pagesInRam[%ld] pixPerPage[%d] bytesPerPix[%d] ramGB[%.3g]\n",  
			m_pcoData->dwRamSize, m_pcoData->wPixPerPage, bytesPerPix,
			(1.0e-9 * m_pcoData->dwRamSize) * m_pcoData->wPixPerPage *  bytesPerPix);

		ptr += sprintf_s(ptr, ptrMax - ptr, "* PcoActiveSegment=[%d]\n", segmentArr+1);
		ptr += sprintf_s(ptr, ptrMax - ptr, "* m_pcoData->dwMaxFramesInSegment[%d]=[%d frames]\n", segmentArr, m_pcoData->dwMaxFramesInSegment[segmentArr]);
		ptr += sprintf_s(ptr, ptrMax - ptr, "* m_pcoData->dwSegmentSize[%d]=[%d pages]\n", segmentArr, m_pcoData->dwSegmentSize[segmentArr]);
		ptr += sprintf_s(ptr, ptrMax - ptr, "* m_pcoData->dwValidImageCnt[%d]=[%ld]\n", segmentArr, m_pcoData->dwValidImageCnt[segmentArr]);
		ptr += sprintf_s(ptr, ptrMax - ptr, "* m_pcoData->dwMaxImageCnt[%d]=[%ld]\n", segmentArr, m_pcoData->dwMaxImageCnt[segmentArr]);

		_pco_GetStorageMode_GetRecorderSubmode();
				
		ptr += sprintf_s(ptr, ptrMax - ptr, "* mode[%s] storage_mode[%d] recorder_submode[%d]\n", 
			m_pcoData->storage_str, m_pcoData->storage_mode, m_pcoData->recorder_submode);
		ptr += sprintf_s(ptr, ptrMax - ptr, 
			"* Acq: rec[%ld] xfer[%ld] recNow[%ld] recTout[%ld] (ms) [%s]\n",
			m_pcoData->msAcqRec, m_pcoData->msAcqXfer,  
			m_pcoData->msAcqTnow, m_pcoData->msAcqTout, 
			getTimestamp(Iso, m_pcoData->msAcqRecTimestamp));

	}


    //------ unsorted


	if(flag & CAMINFO_CAMERALINK) {
		if (_getInterfaceType()==INTERFACE_CAMERALINK){ 
			ptr += sprintf_s(ptr, ptrMax - ptr, "*** CameraLink transfer parameters\n");
			ptr += sprintf_s(ptr, ptrMax - ptr, "*      baudrate[%u] %g Kbps\n", m_pcoData->clTransferParam.baudrate, m_pcoData->clTransferParam.baudrate/1000.);
			ptr += sprintf_s(ptr, ptrMax - ptr, "*ClockFrequency[%u] %g MHz\n", m_pcoData->clTransferParam.ClockFrequency, m_pcoData->clTransferParam.ClockFrequency/1000000.);
			ptr += sprintf_s(ptr, ptrMax - ptr, "*        CCline[%u]\n", m_pcoData->clTransferParam.CCline);
			ptr += sprintf_s(ptr, ptrMax - ptr, "*    DataFormat[x%x]\n", m_pcoData->clTransferParam.DataFormat);
			ptr += sprintf_s(ptr, ptrMax - ptr, "*      Transmit[%u]\n", m_pcoData->clTransferParam.Transmit);
		} else {
			ptr += sprintf_s(ptr, ptrMax - ptr, "*** CameraLink transfer parameters - NO CAMERALINK interface\n");
		}
	}

	if(flag & CAMINFO_UNSORTED) {
		ptr += sprintf_s(ptr, ptrMax - ptr, "*** unsorted parameters\n");
		ptr += sprintf_s(ptr, ptrMax - ptr, "* bMetaDataAllowed[%d] wMetaDataSize[%d] wMetaDataVersion[%d] \n",  
			m_pcoData->bMetaDataAllowed, m_pcoData->wMetaDataSize,  m_pcoData->wMetaDataVersion);

		ptr += sprintf_s(ptr, ptrMax - ptr, "* wLUT_Identifier[x%04x] wLUT_Parameter [x%04x]\n",
			m_pcoData->wLUT_Identifier, m_pcoData->wLUT_Parameter);

		int alignment;
		char *res;
		_pco_GetBitAlignment(alignment);
		res = alignment == 0 ? "[0][MSB]" : "[1][LSB]";
		ptr += sprintf_s(ptr, ptrMax - ptr, "* data bit alignment%s\n", res);

		WORD wTimeStampMode;
		char *mode;
		int error;
		_pco_GetTimestampMode(wTimeStampMode, error);
		if(error) wTimeStampMode = 100;
		switch(wTimeStampMode) {
			case 0: mode = "no stamp in image"; break;
			case 1: mode = "BCD coded stamp in the first 14 pixel"; break;
			case 2: mode = "BCD coded stamp in the first 14 pixel + ASCII text"; break;
			case 3: mode = "ASCII text only"; break;
			default: mode = "unknown"; break;
		}

		ptr += sprintf_s(ptr, ptrMax - ptr, "* imageTimestampMode[%d][%s]\n", wTimeStampMode, mode);


		_pco_GetTemperatureInfo(error);

		ptr += sprintf_s(ptr, ptrMax - ptr, "* temperature: CCD[%.1f]  CAM[%d]  PS[%d]\n", 
			m_pcoData->temperature.wCcd/10., 
			m_pcoData->temperature.wCam, 
			m_pcoData->temperature.wPower);

		ptr += sprintf_s(ptr, ptrMax - ptr, "*    cooling: min[%d]  max[%d]  Setpoint[%d]\n",  
			m_pcoData->temperature.wMinCoolSet, 
			m_pcoData->temperature.wMaxCoolSet,
			m_pcoData->temperature.wSetpoint);

	}


#if 0
//--------------------------- TODO / NEED to use _snprintf_s and control of error!
	int _nr;

	for(int ii=1; ii <= 1000; ii++) {
		_nr = _snprintf_s(ptr, ptrMax - ptr, _TRUNCATE, "************************************************************** %d\n",ii);
		if(_nr < 0) {return ptr; } else { ptr += _nr; }
	}

	if(flag & CAMINFO_LOG) {
		ptr += sprintf_s(ptr, ptrMax - ptr, "*** log \n");
//------------------------------------ the function  m_log.c_str() FAILS!
		_nr = _snprintf_s(ptr, ptrMax - ptr, _TRUNCATE, "%s\n", m_log.c_str());
		if(_nr < 0) {return ptr; } else { ptr += _nr; }
	}
#endif





	if(flag & CAMINFO_BLOCK) {
		lgbuff = ptrMax - ptr;
		int used = int((100. * (lgbuffmax - lgbuff))/lgbuffmax);
		ptr += sprintf_s(ptr, ptrMax - ptr, "\n*** camInfo (buff free[%lld B] used[%d %%] size[%lld B]) [end]\n", lgbuff, used, lgbuffmax);
	}

	return ptr;
}



//===============================================================
//===============================================================
// include these definitions in the PreprocessorDefinitions
//    liblimapco -> properties -> c/c++ -> preprocessor -> preprocessor definitions
//
//    VS_PLATFORM=$(PlatformName)
//    VS_CONFIGURATION=$(ConfigurationName)
//===============================================================
//===============================================================

#define Win32				"Win32"
#define x64					"x64"
#define Release_Win7_Sync	"Release_Win7_Sync"
#define Release				"Release"

#ifndef VS_PLATFORM
#pragma message ( "--- VS_PLATFORM - UNDEFINED" )
#define VS_PLATFORM "undef"
#endif

#ifndef VS_CONFIGURATION
#pragma message ( "--- VS_CONFIGURATION - UNDEFINED" )
#define VS_CONFIGURATION "undef"
#endif 


#ifndef VS_PLATFORM
#pragma message ( "--- VS_PLATFORM - UNDEFINED" )
#define VS_PLATFORM "undef"
#endif

#ifndef VS_CONFIGURATION
#pragma message ( "--- VS_CONFIGURATION - UNDEFINED" )
#define VS_CONFIGURATION "undef"
#endif

char * _getVSconfiguration(char *infoBuff, DWORD  bufCharCount  )
{
	sprintf_s(infoBuff, bufCharCount, "platform[%s] configuration[%s]",  
		VS_PLATFORM,
		VS_CONFIGURATION); 
	return infoBuff ;
}
//====================================================================
//====================================================================

void Camera::_traceMsg(char *msg)
{
	DEB_MEMBER_FUNCT();
	DEB_ALWAYS() << "\n>>>  " << msg ;		

}

//====================================================================
//====================================================================
#define LEN_COMMENT 511
char * _sprintComment(char *comment, char *comment1, char *comment2)
{
	static char buff[LEN_COMMENT+1];

	sprintf_s(buff, LEN_COMMENT, 
			"\n=================================================\n--- %s %s %s %s\n",
			getTimestamp(Iso), comment, comment1, comment2);

	return buff ;
}
				

//====================================================================
//====================================================================

/*************************************************************************
#define TIMESTAMP_MODE_BINARY           1
#define TIMESTAMP_MODE_BINARYANDASCII   2

SYSTEMTIME st;
int act_timestamp;
int shift;

if(camval.strImage.wBitAlignment==0)
  shift = (16-camval.strSensor.strDescription.wDynResDESC);
else
  shift = 0;

err=PCO_SetTimestampMode(hdriver,TIMESTAMP_MODE_BINARY); //Binary+ASCII

grab image to adr

time_from_timestamp(adr,shift,&st);
act_timestamp=image_nr_from_timestamp(adr,shift);
*************************************************************************/

int _get_imageNr_from_imageTimestamp(void *buf,int shift)
{
	unsigned short *b;
	unsigned short c;
	int y;
	int image_nr=0;

	b=(unsigned short *)(buf);
	y=100*100*100;

	for(;y>0;y/=100)
	{
		c=*b>>shift;
		image_nr+= (((c&0x00F0)>>4)*10 + (c&0x000F))*y;
		b++;
	}

	return image_nr;
}

int _get_time_from_imageTimestamp(void *buf,int shift,SYSTEMTIME *st)
{
	unsigned short *b;
	unsigned short c;
	int x,us;

	memset(st,0,sizeof(SYSTEMTIME));
	b=(unsigned short *)buf;

	//counter
	for(x=0;x<4;x++)
	{
		c=*(b+x)>>shift;
	}

	x=4;
	//year
	c=*(b+x)>>shift;
	st->wYear+=(c>>4)*1000;
	st->wYear+=(c&0x0F)*100;
	x++;

	c=*(b+x)>>shift;
	st->wYear+=(c>>4)*10;
	st->wYear+=(c&0x0F);
	x++;

	//month
	c=*(b+x)>>shift;
	st->wMonth+=(c>>4)*10;
	st->wMonth+=(c&0x0F);
	x++;


	//day
	c=*(b+x)>>shift;
	st->wDay+=(c>>4)*10;
	st->wDay+=(c&0x0F);
	x++;

	//hour
	c=*(b+x)>>shift;
	st->wHour+=(c>>4)*10;
	st->wHour+=(c&0x0F);
	x++;

	//min   
	c=*(b+x)>>shift;
	st->wMinute+=(c>>4)*10;
	st->wMinute+=(c&0x0F);
	x++;

	//sec   
	c=*(b+x)>>shift;
	st->wSecond+=(c>>4)*10;
	st->wSecond+=(c&0x0F);
	x++;

	//us   
	us=0;
	c=*(b+x)>>shift;
	us+=(c>>4)*100000;
	us+=(c&0x0F)*10000;
	x++;

	c=*(b+x)>>shift;
	us+=(c>>4)*1000;
	us+=(c&0x0F)*100;
	x++;

	c=*(b+x)>>shift;
	us+=(c>>4)*10;
	us+=(c&0x0F);
	x++;

	st->wMilliseconds=us/100;

	return 0;
}
