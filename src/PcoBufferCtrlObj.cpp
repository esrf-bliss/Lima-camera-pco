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

#include "lima/Exceptions.h"
#include "PcoBufferCtrlObj.h"
#include "PcoSyncCtrlObj.h"
#include "PcoCamera.h"
#include <sys/timeb.h>
#include <time.h>

#undef DEBUG_XFER_IMAG
#define COMPILE_WAIT_CONDITION
#undef COMPILEIT
#define USING_PCO_ALLOCATED_BUFFERS
#define EVENT_WAIT_TMOUT_MS 10000

#define THROW_LIMA_HW_EXC(e, x)  { \
	printf("========*** LIMA_HW_EXC %s\n", x ); \
			throw LIMA_HW_EXC(e, x); \
} 


using namespace lima;
using namespace lima::Pco;

//=========================================================================================================
char* _timestamp_pcobufferctrlobj() {return ID_TIMESTAMP ;}
//=========================================================================================================

//=========================================================================================================
//=========================================================================================================
BufferCtrlObj::BufferCtrlObj(Camera *cam) :
  m_handle(cam->getHandle()),
  m_cam(cam),
  m_pcoData(m_cam->_getPcoData())
  //m_status(0)
{
  DEB_CONSTRUCTOR();


	//SoftBufferCtrlObj::Sync &m_bufferSync = *getBufferSync(cond);
	m_bufferSync = getBufferSync(cond);

  //----------------------------------------------- initialization buffers & creating events
  for(int i=0; i < PCO_BUFFER_NREVENTS; i++) {
		m_allocBuff.pcoAllocBufferNr[i] = -1;
		m_allocBuff.pcoAllocBufferPtr[i]	= NULL;
		m_allocBuff.dwPcoAllocBufferSize[i]	= 0;

		m_allocBuff.limaAllocBufferPtr[i]	= NULL;
		m_allocBuff.dwLimaAllocBufferSize[i]	= 0;
  }
	m_allocBuff.pcoAllocBufferDone = false;
	m_allocBuff.createEventsDone = false;

}
//=========================================================================================================
//=========================================================================================================
void BufferCtrlObj::prepareAcq()
{
	DEB_MEMBER_FUNCT();

	//_pcoAllocBuffers();

	FrameDim dim;

	getFrameDim(dim);

	m_ImageBufferSize = dim.getMemSize();

	if(m_cam->_getDebug(DBG_BUFF)) {DEB_ALWAYS() << DEB_VAR2(dim, m_ImageBufferSize);}
	DEB_TRACE() << DEB_VAR1(m_ImageBufferSize);

}

//=========================================================================================================
//=========================================================================================================
void BufferCtrlObj::startAcq()
{
	DEB_MEMBER_FUNCT();
	std::string name;

	StdBufferCbMgr& buffer_mgr = m_buffer_cb_mgr;
	buffer_mgr.setStartTimestamp(Timestamp::now());
}


//===================================================================================================================
//===================================================================================================================
    /********************************************************************************************************
      4.7.22 PCO_AddBufferExtern (Only for experienced users!)

      Adds a buffer to the driver queue. This function returns immediately. If the desired image is
      transferred to the buffer the buffer event will be fired. The user can start a thread, which can wait
      for the event of the buffer (WaitFor(Single/Multiple)Objects). This function can be used to view
      images while the recording is enabled (the user must set dw1stImage=dwLastImage=0).
      To read out previously recorded images with recording disabled, the user can call
      PCO_GetImageEx. Nevertheless you can use this function to read out single images while the
      camera is not in recording state, by setting dw1stImage=dwLastImage=x, where x is a valid image
      number (1…max available).

      a.) Prototype:
      SC2_SDK_FUNC int WINAPI PCO_AddBufferExtern(HANDLE ph, HANDLE hEvent, DWORD dw1stImage,
      DWORD dwLastImage, DWORD dwSynch, void* pBuf, DWORD dwLen, DWORD* dwStatus)

b.) Input parameter:
      · HANDLE ph: Handle to a previously opened camera device.
      · HANDLE hEvent: Handle to an externally allocated event.
      · DWORD dw1stImage: Set dw1stImage=dwLastImage=0 during record for actual image
      · DWORD dwLastImage: Set dw1stImage=dwLastImage=x after record for desired image
      · DWORD dwSynch: Synchronization paremeter, usually 0.
      · void *pBuf: Pointer to the buffer to receive the transferred image.
      · DWORD dwLen: Length of the buffer.
      · DWORD *dwStatus: Driver status.

      The input data should be filled with the following parameter:
      · hEvent = externally created event used to signal an occurred transfer.

      · dw1stImage = set to 0 for live view mode(“live view” transfers the most recent image to the
      PC for viewing / monitoring)
      - 0 = live view mode. x = set to the same value as dwLastImage. Has to be a valid
      image number (see PCO_GetNumberOfImagesInSegment, 1…max available).

      · dwLastImage = set to 0 in preview mode.
      - 0 = live view mode. x = set to the same value as dw1stImage. Has to be a valid image
      number (see PCO_GetNumberOfImagesInSegment, 1…max available).

      · dwSynch: set to 0.
      · pBuf: Address of the first buffer element to which the image should be transferred.
      · dwLen: Length of the buffer in bytes.
      · dwStatus: Address of a DWORD to receive the buffer status.

      c.) Return value:
      · int: Error message, 0 in case of success else less than 0: see Error / Warning Codes
    ********************************************************************************************************/
//===================================================================================================================
//===================================================================================================================
#define BUFFER_DUMMY_IMG_LEN	(2 * 2016 * 2016)

int BufferCtrlObj::_assignImage2Buffer(DWORD &dwFrameFirst, DWORD &dwFrameLast, 
									   DWORD dwRequestedFrames, int bufIdx, bool live_mode) {
	
	DEF_FNID;
	static char _buffer[2][BUFFER_DUMMY_IMG_LEN]; 
	DEB_MEMBER_FUNCT();
    int error = 0;
    char *sErr;
	void *myBuffer;
	DWORD myBufferLen;
	int lima_buffer_nb;
	double timeout = 30;
	char *msg;

	if(m_cam->_getDebug(DBG_ASSIGN_BUFF)) 
		{DEB_ALWAYS() << "entry -> " << DEB_VAR4(dwFrameFirst, dwFrameLast, dwRequestedFrames, bufIdx);}
	
	StdBufferCbMgr& buffer_mgr = m_buffer_cb_mgr;

    buffer_mgr.setStartTimestamp(Timestamp::now());
	lima_buffer_nb = dwFrameFirst-1;		


#ifdef COMPILE_WAIT_CONDITION
	Sync::Status status;
	AutoMutex lock(cond.mutex());

	Timestamp t0 = Timestamp::now();

	for (bool doit = true; doit; ) {
		double wait_timeout = timeout - double(Timestamp::now() - t0);
		if (wait_timeout <= 0) {
				msg = "=== Sync wait INTERRUPTED + TIMEOUT === ";
				DEB_ALWAYS() << msg << DEB_VAR2(lima_buffer_nb , timeout);
				return -1;
		}

		status = m_bufferSync->wait(lima_buffer_nb, wait_timeout);
		if(m_cam->_getDebug(DBG_LIMABUFF)) {DEB_ALWAYS() << DEB_VAR3(lima_buffer_nb, timeout, status);}

		switch(status){
			case Sync::AVAILABLE:
				myBuffer = buffer_mgr.getFrameBufferPtr(lima_buffer_nb);
				buffer_mgr.getNbBuffers(m_pcoData->iAllocatedBufferNumberLima);
				doit = false;
				break;
			case Sync::TIMEOUT:
				msg = "=== Sync wait TIMEOUT === ";
				DEB_ALWAYS() << msg << DEB_VAR2(lima_buffer_nb , timeout);
				return -1;
			case Sync::INTERRUPTED:
				msg = "=== Sync wait INTERRUPTED === ";
				DEB_ALWAYS() << msg << DEB_VAR2(lima_buffer_nb , timeout);
				break;
			default:
				msg = "=== Sync wait UNKNOWN STATUS === ";
				DEB_ALWAYS() << msg << DEB_VAR2(lima_buffer_nb , timeout);
				return -1;
		}
	}

#else
#pragma message ("============================================== BYPASSED ----- COMPILE_WAIT_CONDITION -----")

	myBuffer = buffer_mgr.getFrameBufferPtr(lima_buffer_nb);
#endif

 	myBufferLen = m_ImageBufferSize;
    
    DWORD dwMaxWidth, dwMaxHeight;
    WORD wArmWidth, wArmHeight, wBitPerPixel;

    unsigned int bytesPerPixel;

    m_cam->getArmWidthHeight(wArmWidth, wArmHeight);
    m_cam->getMaxWidthHeight(dwMaxWidth, dwMaxHeight);
    m_cam->getBytesPerPixel(bytesPerPixel);
    m_cam->getBitsPerPixel(wBitPerPixel);

    DWORD dwLen = wArmWidth * wArmHeight * bytesPerPixel;
    //DWORD dwAllocatedBufferSize = dwMaxWidth * dwMaxHeight * (DWORD) bytesPerPixel;

 	myBufferLen = m_ImageBufferSize = dwLen;

#ifdef DEBUG_XFER_IMAG
	DEB_ALWAYS() << "_assignImage2Buffer: " << DEB_VAR3(myBufferLen, m_ImageBufferSize, dwLen);
#endif

	FrameDim dim;
	getFrameDim(dim);
	int dimSize = dim.getMemSize();

	if(m_cam->_getDebug(DBG_ASSIGN_BUFF)) 
		{DEB_ALWAYS() << DEB_VAR5(dwLen, dimSize, myBuffer, lima_buffer_nb, m_pcoData->iAllocatedBufferNumberLima);}
	
	if(myBuffer == NULL) {
		msg = "=== ERROR myBuffer = NULL ===";
		DEB_ALWAYS() << msg;
		return -1;
	}

	if(myBufferLen < dwLen) {
		msg = "=== ERROR invalid myBufferLen === ";
		DEB_ALWAYS() << msg << DEB_VAR2(myBufferLen, dwLen);
		return -1;
	}
	if(m_ImageBufferSize < (int) dwLen) {
		msg = "=== ERROR invalid m_ImageBufferSize === ";
		DEB_ALWAYS() << msg << DEB_VAR2(m_ImageBufferSize, dwLen);
		return -1;
	}

	m_allocBuff.limaAllocBufferPtr[bufIdx] = (WORD *) myBuffer; 

#ifdef DEBUG_XFER_IMAG
	DEB_ALWAYS() << "_assignImage2Buffer: " << DEB_VAR1(myBufferLen);
#endif
	m_allocBuff.dwLimaAllocBufferSize[bufIdx] = myBufferLen; 



// ---------- NEW function with our assigned buffers
//SC2_SDK_FUNC int WINAPI PCO_AddBufferExtern(HANDLE ph, HANDLE hEvent, DWORD dw1stImage,
//        DWORD dwLastImage, DWORD dwSynch, void* pBuf, DWORD dwLen, DWORD* dwStatus)

      DWORD dwSynch = 0;  // must be 0
      DWORD dwStatus = 0;
      HANDLE hEvent = m_allocBuff.bufferAllocEvent[bufIdx];   // assigned in the constructor of  BufferCtrlObj


#ifndef USING_PCO_ALLOCATED_BUFFERS 
	  WORD wActSeg = m_cam->pcoGetActiveRamSegment();
    	sErr = m_cam->_PcoCheckError(__LINE__, __FILE__, PCO_GetActiveRamSegment(m_handle, &wActSeg), error);
        //_PCO_TRACE("PCO_GetActiveRamSegment", sErr) ;

		// the data transfer is made directly to the buffer allocated by LIMA		
	sErr =  m_cam->_PcoCheckError(__LINE__, __FILE__, PCO_AddBufferExtern(m_handle, hEvent,wActSeg,dwFrameFirst, dwFrameLast, dwSynch, myBuffer, \
	            dwLen, &dwStatus), error);
	if(error) {
    	DEB_TRACE() << sErr;
    	DEB_TRACE() << DEB_VAR3(wActSeg,dwFrameFirst, dwFrameLast);
    	DEB_TRACE() << DEB_VAR3(dwSynch, myBuffer,dwLen);
		THROW_HW_ERROR(NotSupported) << sErr;
	}
#else
	// the data transfer is made to the buffer allocated by PCO, after that we must to copy this buffer
	// to the LIMA allocated one

	  int iPcoBufIdx = m_allocBuff.pcoAllocBufferNr[bufIdx];	
	  DWORD dwFrame = 
		  (m_cam->_isCameraType(Edge)
		  || m_cam->_isCameraType(Pco2k) 
		  || live_mode) ? 0 :  dwFrameFirst;

	if(m_cam->_getDebug(DBG_ASSIGN_BUFF)) 
	{
		DEB_ALWAYS() << "PCO_AddBufferEx -> " << DEB_VAR6(dwFrame, iPcoBufIdx, bufIdx,wArmWidth, wArmHeight, wBitPerPixel);
	}


	  sErr =  m_cam->_PcoCheckError(__LINE__, __FILE__, PCO_AddBufferEx(m_handle, \
				dwFrame, dwFrame, iPcoBufIdx, \
				wArmWidth, wArmHeight, wBitPerPixel), error) ;
	if(m_cam->_getDebug(DBG_ASSIGN_BUFF)) 
	{
		DEB_ALWAYS() << "PCO_AddBufferEx -> " << DEB_VAR6(error, dwFrame, iPcoBufIdx, bufIdx,wArmWidth, wArmHeight);
	}

	if(error) {
		printf("==== %s error [%s]\n", fnId, sErr);
		printf("==== dwFrame[%d] dwFrameFirst[%d] dwFrameLast[%d]\n", dwFrame, dwFrameFirst, dwFrameLast);
		printf("==== wArmWidth[%d] wArmHeight[%d] wBitPerPixel[%d]\n", wArmWidth, wArmHeight, wBitPerPixel);

    	DEB_TRACE() << sErr;
    	DEB_TRACE() << DEB_VAR2(dwFrameFirst, dwFrameLast);
    	DEB_TRACE() << DEB_VAR3(wArmWidth, wArmHeight, wBitPerPixel);
		THROW_HW_ERROR(NotSupported) << sErr;
	}

#endif

	m_allocBuff.bufferAssignedFrameFirst[bufIdx] = dwFrameFirst;
	m_allocBuff.bufferAssignedFrameLast[bufIdx] = dwFrameLast;
	m_allocBuff.bufferReady[bufIdx] = 0;

     //----- prepartion of dwFrameFirst2assign & dwFrameLast2assign for the NEXT call to addBuffer
	dwFrameFirst = dwFrameLast + 1;
	dwFrameLast = dwFrameFirst + m_cam->pcoGetFramesPerBuffer() - 1;
	if(dwFrameLast > dwRequestedFrames) dwFrameLast = dwRequestedFrames;

	if(m_cam->_getDebug(DBG_ASSIGN_BUFF)) {
		DEB_ALWAYS() << "exit -> " << DEB_VAR4(dwFrameFirst, dwFrameLast, dwRequestedFrames, bufIdx);
	}

	return 0;

}
//===================================================================================================================
//===================================================================================================================
int BufferCtrlObj::_xferImag()
{
	DEB_MEMBER_FUNCT();
	DEF_FNID;

	DWORD dwFrameIdx;
	DWORD dwFrameFirst2assign, dwFrameLast2assign;
	DWORD dwEvent;
	long long nr =0;
	long long bytesWritten = 0;
	int bufIdx;
	int error;
	int lima_buffer_nb;
	bool live_mode;
	int _nrStop;
	char msg[RING_LOG_BUFFER_SIZE+1];
	m_cam->m_tmpLog->flush(-1);
	int maxWaitTimeout = 3;

	
// --------------- get the requested nr of images 
	int requested_nb_frames;
	DWORD dwFramesPerBuffer, dwRequestedFrames;
	DWORD dwRequestedFramesMax =DWORD_MAX;

// --------------- live video -> nr frames = 0 / idx lima buffers 32b (0...ffff)
	m_sync->getNbFrames(requested_nb_frames);
	
	if(requested_nb_frames > 0){
		dwRequestedFrames = (DWORD) requested_nb_frames;
		live_mode =false;
	} else {
		dwRequestedFrames = dwRequestedFramesMax;
		live_mode = true;
	}
		
	dwRequestedFrames = (requested_nb_frames > 0) ? (DWORD) requested_nb_frames : dwRequestedFramesMax;
	dwFramesPerBuffer = m_cam->pcoGetFramesPerBuffer(); // for dimax = 1

	DEB_ALWAYS() << "\n_xferImag():\n" 
		;

//----------------- traceAcq init

	m_pcoData->traceAcq.fnIdXfer = fnId;
	m_pcoData->traceAcq.msImgCoc = (m_cam->pcoGetCocRunTime() * 1000.);

	struct __timeb64 tStart;
	msElapsedTimeSet(tStart);
	m_pcoData->traceAcq.nrImgRequested = dwRequestedFrames;


// --------------- prepare the first buffer 
// ------- in PCO DIMAX only 1 image can be retreived
//         (dwFramesPerBuffer = 1) ====> (dwFrameLast2assign = dwFrameFirst2assign)
	dwFrameFirst2assign = 1;
	dwFrameLast2assign = dwFrameFirst2assign + dwFramesPerBuffer - 1;
	if(dwFrameLast2assign > dwRequestedFrames) dwFrameLast2assign = dwRequestedFrames;

	for(int i = 0; i < PCO_BUFFER_NREVENTS; i++) {
						// --------------- if needed prepare the next buffer 
		if(dwFrameFirst2assign > dwRequestedFrames) break;
			bufIdx = i;
			
			if(m_cam->_getDebug(DBG_WAITOBJ)){
				sprintf_s(msg, RING_LOG_BUFFER_SIZE, "... ASSIGN BUFFER[%d] frame[%d]", bufIdx, dwFrameFirst2assign);
				m_cam->m_tmpLog->add(msg);
			}

			if(error = _assignImage2Buffer(dwFrameFirst2assign, dwFrameLast2assign, dwRequestedFrames, bufIdx,live_mode)) {
				DEB_TRACE() << "ERROR _assignImage2Buffer";
					return pcoAcqPcoError;
			}
	}

	WORD wArmWidth, wArmHeight;
	unsigned int bytesPerPixel;
	m_cam->getArmWidthHeight(wArmWidth, wArmHeight);
	m_cam->getBytesPerPixel(bytesPerPixel);
	DWORD dwLen = wArmWidth * wArmHeight * bytesPerPixel;

 	// Edge cam must be started just after assign buff to avoid lost of img
	if(m_cam->_isCameraType(Edge)) {
			m_cam->_pcoSet_RecordingState(1, error);
	}


	// --------------- loop - process the N frames


	dwFrameIdx = 1;
	while(dwFrameIdx <= dwRequestedFrames) {

_RETRY:

	if((m_sync->_getRequestStop(_nrStop) == stopRequest) && (_nrStop > MAX_NR_STOP)) {goto _EXIT_STOP;}

	// --------------- look if one of buffer is READY and has the NEXT frame => proccess it
    // m_allocatedBufferAssignedFrameFirst[bufIdx] -> first frame in the buffer (we are using only 1 frame per buffer)
    // m_allocatedBufferReady[bufIdx] -> is already filled by sdk (ready)

    for(bufIdx = 0; bufIdx < PCO_BUFFER_NREVENTS; bufIdx++) {
      if((m_allocBuff.bufferAssignedFrameFirst[bufIdx] == dwFrameIdx) && m_allocBuff.bufferReady[bufIdx]) {
			// lima frame nr is from 0 .... N-1        
			lima_buffer_nb = dwFrameIdx -1; // this frame was already readout to the buffer
          
#ifdef DEBUG_XFER_IMAG
			if(m_cam->_getDebug(DBG_WAITOBJ)){
				sprintf_s(msg, RING_LOG_BUFFER_SIZE, "... BUFFER[%d] lima[%d] frame[%d]", bufIdx, lima_buffer_nb, dwFrameIdx);
				m_cam->m_tmpLog->add(msg);
			}
#endif
			m_pcoData->traceAcq.nrImgAcquired = dwFrameIdx;

#ifdef DEBUG_XFER_IMAG
			if(m_cam->_getDebug(DBG_BUFF)) {	
				DEB_ALWAYS() << "========================================FOUND " << DEB_VAR3(lima_buffer_nb, dwFrameIdx, bufIdx); 
			}
#endif

#ifdef USING_PCO_ALLOCATED_BUFFERS
		// we are using the PCO allocated buffer, so this buffer must be copied to the lima buffer
		void * ptrDest = (void *)m_allocBuff.limaAllocBufferPtr[bufIdx];
		void *ptrSrc = (void *) m_allocBuff.pcoAllocBufferPtr[bufIdx];
		size_t sizeLima = m_allocBuff.dwLimaAllocBufferSize[bufIdx];
		size_t size = m_allocBuff.dwPcoAllocBufferSize[bufIdx];

#ifdef DEBUG_XFER_IMAG
		if(m_cam->_getDebug(DBG_XFER2LIMA)) {
			DEB_ALWAYS() << "xferImag: " << DEB_VAR6(size, sizeLima, dwLen, wArmWidth, wArmHeight, bytesPerPixel);
		}
#endif

		size = sizeLima;
		SHORT sBufNr = 	m_allocBuff.pcoAllocBufferNr[bufIdx];

#ifdef DEBUG_XFER_IMAG
		if(m_cam->_getDebug(DBG_BUFF)) {
			DEB_ALWAYS() << "========================================FOUND " << DEB_VAR5(ptrDest, ptrSrc, size, sizeLima, sBufNr);
		}
#endif

		DWORD dwStatusDll, dwStatusDrv;
		if((m_sync->_getRequestStop(_nrStop) == stopRequest) && (_nrStop > MAX_NR_STOP)) {goto _EXIT_STOP;}

		int errPco = PCO_GetBufferStatus(m_handle, sBufNr, &dwStatusDll, &dwStatusDrv);		
		if((dwStatusDll != 0x80000000) || dwStatusDrv || errPco) {
			printf("=== %s> got frame[%d / %d] bufIdx[%d] size[%ld] dest[%08lx] src[%08lx] \n"
				"dwStatusDll[%08lx] dwStatusDrv[%08lx] errPco[%08lx] err[%s]\n", fnId, 
				dwFrameIdx, dwRequestedFrames, bufIdx,
				size, ((DWORD) ptrDest), ((DWORD) ptrSrc),
				dwStatusDll, dwStatusDrv, errPco,
				m_cam->_PcoCheckError(__LINE__, __FILE__, dwStatusDrv, error));
		}
		
#ifdef DEBUG_XFER_IMAG
		if(m_cam->_getDebug(DBG_BUFF)) {
			DEB_ALWAYS() << "===== " << DEB_VAR5(ptrDest, ptrSrc, size, sizeLima, sBufNr);
		}
#endif

		if(m_cam->_getDebug(DBG_DUMMY_IMG)){
			int val = dwFrameIdx & 0xf;
			memset(ptrDest, val, size);
			DEB_ALWAYS() << "===== dummy image!!! " << DEB_VAR1(val);
		} else {

#ifdef DEBUG_XFER_IMAG
			if(m_cam->_getDebug(DBG_XFER_IMG)) {
				DEB_ALWAYS() << "xferImag: " << DEB_VAR6(size, sizeLima, dwLen, wArmWidth, wArmHeight, bytesPerPixel);
				DEB_ALWAYS() << "xferImag - memcpy " << DEB_VAR3(ptrDest, ptrSrc, size);
			}
#endif
			memcpy(ptrDest, ptrSrc, size);
		}		
		
		
#ifdef DEBUG_XFER_IMAG
		if(m_cam->_getDebug(DBG_XFER2LIMA)) {
			DEB_ALWAYS() << "===== after xfer pco buffer to lima " << DEB_VAR6(ptrDest, ptrSrc, size, sizeLima, sBufNr, dwFrameIdx);
		}
#endif

#endif


		HwFrameInfoType frame_info;
		frame_info.acq_frame_nb = lima_buffer_nb;
		m_buffer_cb_mgr.newFrameReady(frame_info);

        //----- the image dwFrameIdx is already in the buffer -> callback!
		if((m_sync->_getRequestStop(_nrStop) == stopRequest) && (_nrStop > MAX_NR_STOP)) {goto _EXIT_STOP;}
		if(dwFrameFirst2assign <= dwRequestedFrames) {

#ifdef DEBUG_XFER_IMAG
			if(m_cam->_getDebug(DBG_WAITOBJ)){
				sprintf_s(msg, RING_LOG_BUFFER_SIZE, "... ASSIGN BUFFER[%d] frame[%d]", bufIdx, dwFrameFirst2assign);
				m_cam->m_tmpLog->add(msg);
			}
#endif
			if(error = _assignImage2Buffer(dwFrameFirst2assign, dwFrameLast2assign, dwRequestedFrames, bufIdx, live_mode)) {
				return pcoAcqPcoError;
			}
        }

#ifdef DEBUG_XFER_IMAG
		if(m_cam->_getDebug(DBG_BUFF)) {
			DEB_ALWAYS() << "===== " << DEB_VAR5(ptrDest, ptrSrc, size, sizeLima, sBufNr);
		}
#endif
		goto _WHILE_CONTINUE;
      }
    } // for(bufIdx = 0; bufIdx < PCO_BUFFER_NREVENTS; bufIdx++)

_RETRY_WAIT:
// --------------- check if there is some buffer ready
		dwEvent = WaitForMultipleObjects( 
			PCO_BUFFER_NREVENTS,           // number of objects in array
			m_allocBuff.bufferAllocEvent,     // array of objects
			FALSE,       // wait for any object
			EVENT_WAIT_TMOUT_MS);       // ms wait

    // The return value indicates which event is signaled

#if PCO_BUFFER_NREVENTS != 4
  #pragma message ("============================================== ABORT - wrong nr of WAIT_OBJECT ")
    DUMMY_FOR_ABORT = 5;
#endif

    switch (dwEvent) { 
        case WAIT_OBJECT_0 + 0: 
			m_allocBuff.bufferReady[0] = 1; 
			if(m_cam->_getDebug(DBG_WAITOBJ)){m_cam->m_tmpLog->add("... WAITOBJ 0");}
			goto _RETRY;
        case WAIT_OBJECT_0 + 1: 
			m_allocBuff.bufferReady[1] = 1; 
			if(m_cam->_getDebug(DBG_WAITOBJ)){m_cam->m_tmpLog->add("... WAITOBJ 1");}
			goto _RETRY;
        case WAIT_OBJECT_0 + 2: 
			m_allocBuff.bufferReady[2] = 1;
			if(m_cam->_getDebug(DBG_WAITOBJ)){m_cam->m_tmpLog->add("... WAITOBJ 2");}
			goto _RETRY;
        case WAIT_OBJECT_0 + 3: 
			m_allocBuff.bufferReady[3] = 1; 
			if(m_cam->_getDebug(DBG_WAITOBJ)){m_cam->m_tmpLog->add("... WAITOBJ 3");}
			goto _RETRY;

        case WAIT_TIMEOUT: 
			maxWaitTimeout--;
			if(m_cam->_getDebug(DBG_WAITOBJ)){m_cam->m_tmpLog->dumpPrint(true);}
			printf("=== %s> WAITOBJ ERROR - TIMEOUT [%d]\n", fnId, maxWaitTimeout);
			if(maxWaitTimeout > 0 ) goto _RETRY_WAIT;
			return pcoAcqWaitTimeout;

        default: 
			printf("=== %s> WAITOBJ default ????\n", fnId);
			return pcoAcqWaitError;
    }

_WHILE_CONTINUE:
	m_sync->setAcqFrames(dwFrameIdx);
    dwFrameIdx++;
  } // while(frameIdx ...

  // if(m_cam->_isCameraType(Edge)) {m_sync->setAcqFrames(dwFrameIdx-1);}

  if(m_cam->_getDebug(DBG_WAITOBJ)){m_cam->m_tmpLog->dumpPrint(true);}  // true first .... last
	
	m_pcoData->traceAcq.msXfer = msElapsedTime(tStart);
	m_pcoData->traceAcq.endXferTimestamp = getTimestamp();

	return pcoAcqTransferEnd;

_EXIT_STOP:
	DEB_ALWAYS() << "\nSTOP REQUESTED " << DEB_VAR3(_nrStop, dwFrameIdx, dwRequestedFrames);

	m_pcoData->traceAcq.msXfer = msElapsedTime(tStart);
	m_pcoData->traceAcq.endXferTimestamp = getTimestamp();

	return pcoAcqTransferStop;

}



//===================================================================================================================
//===================================================================================================================
void * BufferCtrlObj::_getLimaBuffer(int lima_buffer_nb, Sync::Status &status)
{
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	
	void *myBuffer;
	double timeout = 30;
	char *msg;

	StdBufferCbMgr& buffer_mgr = m_buffer_cb_mgr;

    buffer_mgr.setStartTimestamp(Timestamp::now());


#ifdef COMPILE_WAIT_CONDITION
	AutoMutex lock(cond.mutex());

	Timestamp t0 = Timestamp::now();

	while(true) {
		double wait_timeout = timeout - double(Timestamp::now() - t0);
		if (wait_timeout <= 0) {
				msg = "=== Sync wait INTERRUPTED + TIMEOUT === ";
				DEB_ALWAYS() << msg << DEB_VAR2(lima_buffer_nb , timeout);
				return NULL;
		}

		status = m_bufferSync->wait(lima_buffer_nb, wait_timeout);
		if(m_cam->_getDebug(DBG_LIMABUFF)) {DEB_ALWAYS() << DEB_VAR3(lima_buffer_nb, timeout, status);}

		switch(status){
			case Sync::AVAILABLE:
				myBuffer = buffer_mgr.getFrameBufferPtr(lima_buffer_nb);
				buffer_mgr.getNbBuffers(m_pcoData->iAllocatedBufferNumberLima);
				return myBuffer;

			case Sync::TIMEOUT:
				msg = "=== Sync wait TIMEOUT === ";
				DEB_ALWAYS() << msg << DEB_VAR2(lima_buffer_nb , timeout);
				return NULL;

			case Sync::INTERRUPTED:
				msg = "=== Sync wait INTERRUPTED === ";
				DEB_ALWAYS() << msg << DEB_VAR2(lima_buffer_nb , timeout);
				break;

			default:
				msg = "=== Sync wait UNKNOWN STATUS === ";
				DEB_ALWAYS() << msg << DEB_VAR2(lima_buffer_nb , timeout);
				return NULL;
		}
	}

#else
#pragma message ("============================================== BYPASSED ----- COMPILE_WAIT_CONDITION -----")

	myBuffer = buffer_mgr.getFrameBufferPtr(lima_buffer_nb);
	status = 0;
	return myBuffer;
#endif

}




//===================================================================================================================
//===================================================================================================================


/********************************************************************************

------------- in dimax is NOT possible read MULTIPLES images (PCO_GetImageEx)

On 2013/09/17 11:44, PCO Support Team wrote:

> So there are a few camera, which support reading multiple images (i.e.
> the 1200hs), but the pco.dimax does not. Also not all interfaces
> support the mode, Martin from the software and driver team has
> realized it for a few CameraLink boards. Our experience is, that it is
> pretty difficult and time consuming to provide the feature for all
> cameras and all interfaces in a reliable way.
>
> Concisely, it's not possible for the pco.dimax, even if the command
> "PCO_GetImageEx" mentions the multiple reading mode.
********************************************************************************/


int BufferCtrlObj::_xferImagMult()
{
	DEB_MEMBER_FUNCT();
	DEF_FNID;
	

	int mode = m_cam->_pco_GetStorageMode_GetRecorderSubmode();
	bool continuous = (mode != RecSeq) ;

	m_pcoData->traceAcq.fnIdXfer = fnId;


	DWORD dwFrameIdx;
	long long nr =0;
	long long bytesWritten = 0;
	int bufIdx;
	int error;
	bool live_mode;
	DWORD dwFrameIdxFirst, dwFrameIdxLast;
	int maxWaitTimeout = 10;
	WORD _wBitPerPixel;
	char *sErr;
	void *ptrLimaBuffer;
	Sync::Status status;
	int requested_nb_frames;
	DWORD dwFramesPerBuffer, dwRequestedFrames;
	DWORD dwRequestedFramesMax =DWORD_MAX;
	unsigned int _uiBytesPerPixel;
	WORD _wArmWidth, _wArmHeight;
	int _iPcoAllocatedBuffNr;
	DWORD _dwPcoAllocatedBuffSize;
	SHORT sBufNr;
	WORD wSegment;
	int iPcoFrame, iLimaFrame;
	DWORD dwFrameSize ;
	int _retStatus, _stopReq, _nrStop;
    int _newFrameReady = -1;

	struct __timeb64 tStart;
	msElapsedTimeSet(tStart);

	LARGE_INTEGER usStart;
	usElapsedTimeSet(usStart);


	//_pcoAllocBuffers(true); // allocate 2 pco buff at max size

	wSegment = m_cam->pcoGetActiveRamSegment();
	
//------------------- nr of frames per buffer
	m_cam->getBitsPerPixel(_wBitPerPixel);
	m_cam->getBytesPerPixel(_uiBytesPerPixel);
	m_cam->getArmWidthHeight(_wArmWidth, _wArmHeight);  // actual
	_pcoAllocBuffersInfo(_iPcoAllocatedBuffNr, _dwPcoAllocatedBuffSize);

	WORD _wRoiWidth = m_pcoData->wRoiX1Now - m_pcoData->wRoiX0Now +1;
	WORD _wRoiHeight = m_pcoData->wRoiX1Now - m_pcoData->wRoiX0Now +1;

	//dwFrameSize = (DWORD) _wRoiWidth * (DWORD) _wRoiHeight * (DWORD) _uiBytesPerPixel;

	dwFrameSize = (DWORD) _wArmWidth * (DWORD) _wArmHeight * (DWORD) _uiBytesPerPixel;
	//dwFramesPerBuffer = _dwPcoAllocatedBuffSize / dwFrameSize ;
	dwFramesPerBuffer = 1;
	
    Roi roiNow;
    m_cam->_get_Roi(roiNow);


// --------------- live video -> nr frames = 0 / idx lima buffers 32b (0...ffff)
	m_sync->getNbFrames(requested_nb_frames);
	
	if(requested_nb_frames > 0){
		dwRequestedFrames = (DWORD) requested_nb_frames;
		live_mode =false;
	} else {
		dwRequestedFrames = dwRequestedFramesMax;
		live_mode = true;
	}
		
	dwRequestedFrames = (requested_nb_frames > 0) ? (DWORD) requested_nb_frames : dwRequestedFramesMax;


	DEB_ALWAYS() << "\n" 
		<< "    " << fnId << " [ ENTRY]:\n" 
		<< "    " << DEB_VAR2(_iPcoAllocatedBuffNr, _dwPcoAllocatedBuffSize) << "\n"  
		<< "    " << DEB_VAR2(_wArmWidth, _wArmHeight) << "\n" 
		<< "    " << DEB_VAR1(roiNow) << "\n" 
		<< "    " << DEB_VAR4(_wRoiWidth, _wRoiHeight, _uiBytesPerPixel, _wBitPerPixel) << "\n" 
		<< "    " << DEB_VAR2( dwFramesPerBuffer, dwFrameSize) << "\n"
		<< "    " << DEB_VAR3( requested_nb_frames, dwRequestedFrames, live_mode)
		;


    // lima frame nr is from 0 .... N-1        
    // pco frame nr is from 1 .... N        

	// --------------- loop - process the N frames
	dwFrameIdx = 1;
	bufIdx = 0;

	DWORD _dwValidImageCnt, _dwMaxImageCnt;

	sErr = m_cam->_PcoCheckError(__LINE__, __FILE__, 
			PCO_GetNumberOfImagesInSegment(m_handle, wSegment, &_dwValidImageCnt, &_dwMaxImageCnt), error, "PCO_GetNumberOfImagesInSegment");
	if(error) {
		printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__, sErr);
		throw LIMA_HW_EXC(Error, "PCO_GetNumberOfImagesInSegment");
	}

	if(m_cam->_getDebug(DBG_XFERMULT)){
		DEB_ALWAYS() << "\n     " << DEB_VAR3(dwRequestedFrames,_dwValidImageCnt, _dwMaxImageCnt);
	}	

	m_pcoData->traceAcq.nrImgRequested = dwRequestedFrames;

	m_pcoData->traceAcq.usTicks[6].desc = "PCO_GetImageEx total execTime";
	m_pcoData->traceAcq.usTicks[7].desc = "xfer to lima / total execTime";

	while(dwFrameIdx <= dwRequestedFrames) {
		bufIdx++; if(bufIdx >= _iPcoAllocatedBuffNr) bufIdx = 0;
		sBufNr = m_allocBuff.pcoAllocBufferNr[bufIdx];


		if(continuous) {
			dwFrameIdxLast = dwFrameIdxFirst = 0;
		} else {
			dwFrameIdxLast = dwFrameIdxFirst = dwFrameIdx;
		}
		
		if(m_cam->_getDebug(DBG_XFERMULT1)){
			DEB_ALWAYS() 
				<< "\n     PCO_GetImageEx() ===>"
				<< "\n     " << DEB_VAR5( dwFrameIdx, dwFrameIdxFirst, dwFrameIdxLast, dwFramesPerBuffer,  dwRequestedFrames)
				<< "\n     " << DEB_VAR5(wSegment, sBufNr, _wArmWidth, _wArmHeight, _wBitPerPixel);
		}

		usElapsedTimeSet(usStart);

		sErr =  m_cam->_PcoCheckError(__LINE__, __FILE__, PCO_GetImageEx(m_handle, \
			wSegment, dwFrameIdxFirst, dwFrameIdxLast, \
			sBufNr, _wArmWidth, _wArmHeight, _wBitPerPixel), error, "PCO_GetImageEx");
		
		m_pcoData->traceAcq.usTicks[6].value += usElapsedTime(usStart);
		usElapsedTimeSet(usStart);

		if(error) {
			DEB_ALWAYS() 
				<<  "\n     PCO_GetImageEx() ===> "  
				<<  "\n     " << DEB_VAR4( sErr, wSegment, dwFrameIdxFirst, dwFrameIdxLast) 
				<<  "\n     " << DEB_VAR4(sBufNr, _wArmWidth, _wArmHeight, _wBitPerPixel);
		}

		void *ptrSrc =  m_allocBuff.pcoAllocBufferPtr[bufIdx];
		
		iPcoFrame = dwFrameIdx;
		iLimaFrame = iPcoFrame -1;

		ptrLimaBuffer = _getLimaBuffer(iLimaFrame, status);

		if(ptrLimaBuffer == NULL) {
			DEB_ALWAYS() 
				<<  "\n     " << DEB_VAR3(ptrLimaBuffer, iLimaFrame, status);
			THROW_HW_ERROR(NotSupported) << "Lima ptr = NULL";
		}

		if(m_cam->_getDebug(DBG_XFERMULT1)){
			DEB_ALWAYS() 
				<<  "\n     " << DEB_VAR3( ptrLimaBuffer, ptrSrc, dwFrameSize);
		}

		memcpy(ptrLimaBuffer, ptrSrc, dwFrameSize);
		ptrSrc = ((char *)ptrSrc) + dwFrameSize;
		
		HwFrameInfoType frame_info;
		frame_info.acq_frame_nb = _newFrameReady = iLimaFrame;
		m_buffer_cb_mgr.newFrameReady(frame_info);

		m_pcoData->traceAcq.nrImgAcquired = dwFrameIdx;

		if((_stopReq = m_sync->_getRequestStop(_nrStop)) == stopRequest) {
			if(_nrStop > MAX_NR_STOP) {
				char msg[LEN_TRACEACQ_MSG+1];
				snprintf(msg,"%s> STOP REQ (saving), framesReq[%d] frameReady[%d]\n", fnId, requested_nb_frames, _newFrameReady);
				m_pcoData->traceMsg(msg);
				break;
			}
		}

		m_sync->setAcqFrames(dwFrameIdx);
		dwFrameIdx++;

		m_pcoData->traceAcq.usTicks[7].value += usElapsedTime(usStart);

	} // while(frameIdx ...

  // if(m_cam->_isCameraType(Edge)) {m_sync->setAcqFrames(dwFrameIdx-1);}

	switch(_stopReq) {
		//case stopProcessing: 
		case stopRequest: 
			_retStatus = pcoAcqTransferStop;
			break;
		default:
			_retStatus = pcoAcqTransferEnd;
	}
			
	DEB_ALWAYS() 
		<< "\n    " << fnId << " [ EXIT]:\n" 
		<< "\n     " << DEB_VAR3(_retStatus, _stopReq, _newFrameReady);  


	return _retStatus;

}




//===================================================================================================================
//===================================================================================================================
#define BUFFER_DUMMY_IMG_LEN	(2 * 2016 * 2016)
// called by startAcq
void BufferCtrlObj::_pcoAllocBuffers(bool max) {
	

	DEB_MEMBER_FUNCT();
	DEF_FNID;

	int bufIdx;

	if(!m_allocBuff.createEventsDone){
		for(bufIdx=0; bufIdx < PCO_BUFFER_NREVENTS; bufIdx++) {
		 // Create two event objects
		   m_allocBuff.bufferAllocEvent[bufIdx] = CreateEvent( 
				NULL,   // default security attributes
				FALSE,  // auto-reset event object
				FALSE,  // initial state is nonsignaled
				NULL);  // unnamed object

			if (m_allocBuff.bufferAllocEvent[bufIdx] == NULL) 
			{ 
				THROW_HW_ERROR(NotSupported) << "CreateEvent error";
			} 
		} 
		m_allocBuff.createEventsDone = true;

	}

#ifdef USING_PCO_ALLOCATED_BUFFERS 
	
	_pcoAllocBuffersFree();
	
	// we are using pco allocated buffer, we must to allocate them
    int error = 0;
    char *sErr;
	DWORD _dwMaxWidth, _dwMaxHeight;
	WORD _wArmWidth, _wArmHeight;
    WORD _wBitPerPixel;
    unsigned int _bytesPerPixel;

	if(!m_allocBuff.pcoAllocBufferDone){
		m_cam->getBytesPerPixel(_bytesPerPixel);
		m_cam->getBitsPerPixel(_wBitPerPixel);

		m_cam->getMaxWidthHeight(_dwMaxWidth, _dwMaxHeight); // max
		m_cam->getArmWidthHeight(_wArmWidth, _wArmHeight);  // actual

		DWORD _dwAllocatedBufferSizeMax = _dwMaxWidth * _dwMaxHeight * (DWORD) _bytesPerPixel ;
		DWORD _dwArmSize = (DWORD) _wArmWidth * (DWORD) _wArmHeight * (DWORD) _bytesPerPixel;
		//_dwAllocatedBufferSizeMax -= _dwAllocatedBufferSizeMax % _dwArmSize;

		DWORD _dwAllocatedBufferSize = max ? _dwAllocatedBufferSizeMax : _dwArmSize ; 

		if(m_cam->_getDebug(DBG_BUFF)) {DEB_ALWAYS() << DEB_VAR4( _dwAllocatedBufferSize, _wArmWidth, _wArmHeight, _bytesPerPixel);}

		m_pcoData->iAllocatedBufferNumber =  PCO_BUFFER_NREVENTS;
		m_pcoData->dwAllocatedBufferSize = _dwAllocatedBufferSize;

		//-------------- allocate 2 buffers (0,1) and received the handle, mem ptr, events
			for(bufIdx = 0; bufIdx < PCO_BUFFER_NREVENTS ; bufIdx ++) {
				m_allocBuff.pcoAllocBufferNr[bufIdx] = -1;
				sErr = m_cam->_PcoCheckError(__LINE__, __FILE__, PCO_AllocateBuffer(m_handle, \
					&m_allocBuff.pcoAllocBufferNr[bufIdx], \
					_dwAllocatedBufferSize, \
					&m_allocBuff.pcoAllocBufferPtr[bufIdx], \
					&m_allocBuff.bufferAllocEvent[bufIdx]\
					), error, "PCO_AllocateBuffer");

					DEB_ALWAYS() << fnId << " " << DEB_VAR2(bufIdx, _dwAllocatedBufferSize);

				if(error) {
					int nrEvents = PCO_BUFFER_NREVENTS;
    				DEB_ALWAYS() << sErr << "\n" 
    						<< DEB_VAR3(nrEvents, bufIdx,_dwAllocatedBufferSize);
					THROW_HW_ERROR(NotSupported) << sErr;
				}
				m_allocBuff.dwPcoAllocBufferSize[bufIdx] = _dwAllocatedBufferSize;

			}
		m_pcoData->bAllocatedBufferDone = m_allocBuff.pcoAllocBufferDone = true;
	}
#endif

	if(m_cam->_getDebug(DBG_BUFF)) {
		void *ptrEvent, *ptrBuff;
		int iPcoBufIdx;

		for(bufIdx = 0; bufIdx < PCO_BUFFER_NREVENTS ; bufIdx ++) {
			ptrEvent = m_allocBuff.bufferAllocEvent[bufIdx];
			ptrBuff = m_allocBuff.pcoAllocBufferPtr[bufIdx];
			iPcoBufIdx = m_allocBuff.pcoAllocBufferNr[bufIdx];
			DEB_ALWAYS() << DEB_VAR4(bufIdx, ptrEvent, ptrBuff, iPcoBufIdx);

		}
	}
}

//===================================================================================================================
//===================================================================================================================
void BufferCtrlObj::_pcoAllocBuffersInfo(int &nr, DWORD &size) {

	DEB_MEMBER_FUNCT();

	nr = m_pcoData->iAllocatedBufferNumber; 
	size = m_pcoData->dwAllocatedBufferSize ;

}

//===================================================================================================================
//===================================================================================================================



void BufferCtrlObj::_pcoAllocBuffersFree() {

	DEF_FNID;
	DEB_MEMBER_FUNCT();

	DEB_ALWAYS() << fnId << " [ENTRY]";

#ifdef USING_PCO_ALLOCATED_BUFFERS 
	// free the pco allocated buffers
    int error = 0;

	
		//SC2_SDK_FUNC int WINAPI PCO_FreeBuffer(HANDLE ph, SHORT sBufNr)

			//-------------- allocate 2 buffers (0,1) and received the handle, mem ptr, events
			for(int bufIdx = 0; bufIdx <PCO_MAX_NR_ALLOCATED_BUFFERS ; bufIdx ++) {
				PCO_FreeBuffer(m_handle, bufIdx);

				m_allocBuff.pcoAllocBufferNr[bufIdx]= -1;
				m_allocBuff.dwPcoAllocBufferSize[bufIdx] = 0;
				m_allocBuff.pcoAllocBufferPtr[bufIdx] = NULL;

			}
		m_allocBuff.pcoAllocBufferDone = false;
#endif



#if 0
#ifdef USING_PCO_ALLOCATED_BUFFERS 
	// free the pco allocated buffers
    int error = 0;
    char *sErr;

	if(m_allocBuff.pcoAllocBufferDone){
	
		//SC2_SDK_FUNC int WINAPI PCO_FreeBuffer(HANDLE ph, SHORT sBufNr)

			//-------------- allocate 2 buffers (0,1) and received the handle, mem ptr, events
			for(int bufIdx = 0; bufIdx <PCO_BUFFER_NREVENTS ; bufIdx ++) {
				sErr = m_cam->_PcoCheckError(__LINE__, __FILE__, PCO_FreeBuffer(m_handle, m_allocBuff.pcoAllocBufferNr[bufIdx]), error);

				if(error) {
    				DEB_TRACE() << sErr;
					THROW_HW_ERROR(NotSupported) << sErr;
				}
				m_allocBuff.pcoAllocBufferNr[bufIdx]= -1;
				m_allocBuff.dwPcoAllocBufferSize[bufIdx] = 0;
				m_allocBuff.pcoAllocBufferPtr[bufIdx] = NULL;

			}
		m_allocBuff.pcoAllocBufferDone = false;
	}
#endif
#endif

}
