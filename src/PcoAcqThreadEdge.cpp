// ###########################################################################
//  This file is part of LImA, a Library for Image Acquisition
//
//  Copyright (C) : 2009-2011
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

#include "PcoAcqThread.h"

namespace lima
{
    namespace Pco
    {

        void _pco_acq_thread_edge(void *argin)
        {
            DEF_FNID;

            Camera *m_cam = (Camera *)argin;
            SyncCtrlObj *m_sync = m_cam->_getSyncCtrlObj();
            BufferCtrlObj *m_buffer = m_cam->_getBufferCtrlObj();

            char _msg[LEN_MSG + 1];
            __sprintfSExt(_msg, LEN_MSG, "%s> [ENTRY]", fnId);
            m_cam->_traceMsg(_msg);

            m_cam->_sprintComment(true, fnId, "[ENTRY]");

            struct stcPcoData *m_pcoData = m_cam->_getPcoData();

            const auto tStart = std::chrono::steady_clock::now();

            int error;
            long msXfer;
            int requestStop = stopNone;
            pcoAcqStatus status;

            HANDLE m_handle = m_cam->getHandle();

            m_sync->setAcqFrames(0);

            if (m_pcoData->testCmdMode & TESTCMDMODE_EDGE_XFER)
            {
                status = (pcoAcqStatus)m_buffer->_xferImag_getImage_edge();
            }
            else
            {
                status = (pcoAcqStatus)m_buffer->_xferImag(); // original
            }

            m_sync->setExposing(status);
            // m_sync->stopAcq();

            printf("=== %s [%d]> _pco_SetRecordingState(0)\n", fnId, __LINE__);

            const char *msg = m_cam->_pco_SetRecordingState(0, error);
            if (error)
            {
                printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__, msg);
                // throw LIMA_HW_EXC(Error, "_pco_SetRecordingState");
            }

            m_pcoData->traceAcq.fnId = fnId;

            m_sync->getExpTime(m_pcoData->traceAcq.sExposure);
            m_sync->getLatTime(m_pcoData->traceAcq.sDelay);

            m_pcoData->msAcqXfer = msXfer = msElapsedTime(tStart);
            // __sprintfSExt(_msg, LEN_MSG, "%s> [EXIT] xfer[%ld] (ms) status[%s]\n", fnId, msXfer, sPcoAcqStatus[status]);
            // m_cam->_traceMsg(_msg);

            m_sync->setStarted(false); // updated

            // const char *errMsg = NULL;
            // switch (status)
            // {
            //     case pcoAcqRecordTimeout:
            //         errMsg = "pcoAcqRecordTimeout";
            //         break;
            //     case pcoAcqWaitTimeout:
            //         errMsg = "pcoAcqWaitTimeout";
            //         break;
            //     case pcoAcqWaitError:
            //         errMsg = "pcoAcqWaitError";
            //         break;
            //     case pcoAcqError:
            //         errMsg = "pcoAcqError";
            //         break;
            //     case pcoAcqPcoError:
            //         errMsg = "pcoAcqPcoError";
            //         break;
            //     default:
            //         errMsg = NULL;
            // }

            // If status is error
            if (status == pcoAcqRecordTimeout || status == pcoAcqWaitTimeout || status == pcoAcqWaitError ||status == pcoAcqError || status == pcoAcqPcoError)
            {
                Event *ev = new Event(Hardware, Event::Error, Event::Camera, Event::CamFault, sPcoAcqStatus[status]);
                // m_cam->_getPcoHwEventCtrlObj()->reportEvent(ev);
            }

            printf("=== %s [%d]> TRACE %s\n", fnId, __LINE__, "exit thread");
        }

    } // namespace Pco
} // namespace lima