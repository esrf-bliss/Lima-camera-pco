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
#ifndef PCOBUFFERCTRLOBJ_H
#define PCOBUFFERCTRLOBJ_H

#include "Pco.h"
#include "PcoCamera.h"

#include "lima/HwBufferMgr.h"

#define PCO_MAX_NR_ALLOCATED_BUFFERS 8

struct stcAllocBuff {
		bool pcoAllocBufferDone;
		bool createEventsDone;
		
		SHORT	pcoAllocBufferNr[PCO_MAX_NR_ALLOCATED_BUFFERS];				// bufnrM buffer number allocated by PCO_AllocateBuffer
        WORD	*pcoAllocBufferPtr[PCO_MAX_NR_ALLOCATED_BUFFERS];			// buffer allocated by PCO_AllocateBuffer
        DWORD	dwPcoAllocBufferSize[PCO_MAX_NR_ALLOCATED_BUFFERS];			// buffer allocated by PCO_AllocateBuffer

		WORD	*limaAllocBufferPtr[PCO_MAX_NR_ALLOCATED_BUFFERS];			// buffer allocated by Lima
        DWORD	dwLimaAllocBufferSize[PCO_MAX_NR_ALLOCATED_BUFFERS];			// buffer allocated by Lima
		DWORD	dwStatus[8];			// PCO_AddBufferEx status/error return

		//HANDLE bufferEvent[PCO_MAX_NR_ALLOCATED_BUFFERS];
        HANDLE bufferAllocEvent[PCO_MAX_NR_ALLOCATED_BUFFERS];

        DWORD bufferAssignedFrameFirst[PCO_MAX_NR_ALLOCATED_BUFFERS];
        DWORD bufferAssignedFrameLast[PCO_MAX_NR_ALLOCATED_BUFFERS];
        int bufferReady[PCO_MAX_NR_ALLOCATED_BUFFERS];


};

namespace lima
{
  namespace Pco
  {
    class Camera;
    class SyncCtrlObj;
    class Interface;

	//int Camera::pcoGetError();

    class DLL_EXPORT BufferCtrlObj : public SoftBufferCtrlObj
    {
      friend class Interface;
	  friend class Camera;

      DEB_CLASS_NAMESPC(DebModCamera,"BufferCtrlObj","Pco");
	
    
	public:
      BufferCtrlObj(Camera *cam);
      void prepareAcq();
      void startAcq();
      //void getStatus(int &err,bool& exposing) {err = m_status,exposing = m_exposing;}
      void getStatus(int &err) {err = m_cam->pcoGetError();}
      //void setStatus(int status) {m_status = status;}

        //-------------------------------------------------------------  moved from taco
        
	  
        int _xferImag();
        int _xferImagMult();
		void * BufferCtrlObj::_getLimaBuffer(int lima_buffer_nb, Sync::Status &status);
		void _pcoAllocBuffersFree();
		void _pcoAllocBuffersInfo(int &nr, DWORD &size);

	
	private:
		SoftBufferCtrlObj::Sync *m_bufferSync;
		Cond cond;
		Camera* m_cam;
		int _assignImage2Buffer(DWORD &dwFrameFirst, DWORD &dwFrameLast, DWORD dwRequestedFrames, int bufIdx, bool live_mode);
		
		void _pcoAllocBuffers(bool max = false);
		struct stcAllocBuff m_allocBuff;
		unsigned long	m_frames_per_buffer;
		//-------------------------------------------------------------

		HANDLE&      	m_handle;
		int        	m_frame[2];
		SyncCtrlObj* 	m_sync;
		int m_requestStop, m_requestStopRetry;
		int m_ImageBufferSize;
		struct stcPcoData *m_pcoData;

	};
  }
}
#endif
