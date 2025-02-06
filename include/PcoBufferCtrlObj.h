// ###########################################################################
//  This file is part of LImA, a Library for Image Acquisition
//
//  Copyright (C) : 2009-2024
//  European Synchrotron Radiation Facility
//  BP 220, Grenoble 38043
//  FRANCE
//
//  This is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This software is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, see <http://www.gnu.org/licenses/>.
// ###########################################################################

#ifndef PCOBUFFERCTRLOBJ_H
#define PCOBUFFERCTRLOBJ_H

#include "Pco.h"

#include "lima/HwBufferMgr.h"

#define PCO_MAX_NR_ALLOCATED_BUFFERS 8

struct stcTraceAcq;

struct stcAllocBuff
{
    bool pcoAllocBufferDone;
    bool createEventsDone;

    // bufnrM buffer number allocated by PCO_AllocateBuffer
    SHORT pcoAllocBufferNr[PCO_MAX_NR_ALLOCATED_BUFFERS];
    // buffer allocated by PCO_AllocateBuffer
    WORD *pcoAllocBufferPtr[PCO_MAX_NR_ALLOCATED_BUFFERS];
    DWORD dwPcoAllocBufferSize[PCO_MAX_NR_ALLOCATED_BUFFERS]; // buffer allocated by
                                                              // PCO_AllocateBuffer

    WORD *limaAllocBufferPtr[PCO_MAX_NR_ALLOCATED_BUFFERS];    // buffer allocated
                                                               // by Lima
    WORD *limaAllocBufferPtr1[PCO_MAX_NR_ALLOCATED_BUFFERS];   // buffer allocated
                                                               // by Lima
    DWORD dwLimaAllocBufferSize[PCO_MAX_NR_ALLOCATED_BUFFERS]; // buffer
                                                               // allocated by
                                                               // Lima
    int limaAllocBufferNr[PCO_MAX_NR_ALLOCATED_BUFFERS];       // buffer nr allocated
                                                               // by Lima
    int limaAllocBufferNr1[PCO_MAX_NR_ALLOCATED_BUFFERS];      // buffer nr allocated
                                                               // by Lima

    DWORD dwStatus[8]; // PCO_AddBufferEx status/error return

#if !defined(__linux__)
    HANDLE bufferAllocEvent[PCO_MAX_NR_ALLOCATED_BUFFERS];
#endif //! defined(__linux__)

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

        class DLL_EXPORT CheckImgNr
        {
          public:
            CheckImgNr(Camera *cam);
            ~CheckImgNr();
            void init(stcTraceAcq *traceAcq);
            void update(int iLimaFrame, void *ptrImage);

            int _get_imageNr_from_imageTimestamp(void *buf, int shift);
#ifndef __linux__
            int _get_time_from_imageTimestamp(void *buf, int shift, SYSTEMTIME *st);
#endif

          private:
            Camera *m_cam;
            stcTraceAcq *m_traceAcq;
            bool checkImgNr;
            int pcoImgNrDiff;
            int pcoImgNrOrder;
            int pcoImgNrLast;
            int alignmentShift;
            bool bNoTimestamp;
        }; // class CheckImgNr

        class DLL_EXPORT BufferCtrlObj : public SoftBufferCtrlObj
        {
            friend class Interface;
            friend class Camera;

            DEB_CLASS_NAMESPC(DebModCamera, "BufferCtrlObj", "Pco");

          private:
            Camera *m_cam;
            struct stcPcoData *m_pcoData;

            SoftBufferCtrlObj::Sync *m_bufferSync;
            Cond cond;
            struct stcAllocBuff m_allocBuff;
            unsigned long m_frames_per_buffer;
            //-------------------------------------------------------------

            int m_frame[2];
            SyncCtrlObj *m_sync;
            int m_requestStop, m_requestStopRetry;
            int m_ImageBufferSize;

            CheckImgNr m_checkImgNr;

          public:
            BufferCtrlObj(Camera *cam);
            void prepareAcq();
            void startAcq();
            // void getStatus(int &err,bool& exposing) {err = m_status,exposing
            // = m_exposing;}
            void getStatus(int &err);

            // void setStatus(int status) {m_status = status;}

            void _setNewFrameReady(int iLimaFrame);
            int _xferImagMultDoubleImage();
            int _xferImagDoubleImage();
            int _xferImag();

            int _xferImag_buff2lima(DWORD const &dwFrameIdx, int &bufIdx);

            int _xferImag_getImage();
            int _xferImag_getImage_edge();
            int _xferImagMult();
            void *_getLimaBuffer(int lima_buffer_nb, Sync::Status &status);
            void _pcoAllocBuffersFree();
            void _pcoAllocBuffersInfo(int &nr, DWORD &size);

            void *_getFrameBufferPtr(int nb_frame, int &nb_allocated_buffers);

          private:
            int _assignImage2Buffer(DWORD &dwFrameFirst, DWORD &dwFrameLast, DWORD dwRequestedFrames, int bufIdx,
                                    bool live_mode, WORD wDoubleImage = 0);

            void _pcoAllocBuffers(bool max = false);
            //-------------------------------------------------------------
        };
    } // namespace Pco
} // namespace lima
#endif