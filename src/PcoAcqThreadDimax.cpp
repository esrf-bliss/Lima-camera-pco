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

const int TOUT_MIN_DIMAX = 500;

namespace lima
{
    namespace Pco
    {

        void _pco_acq_thread_dimax(void *argin)
        {
            DEF_FNID;

            static char msgErr[LEN_ERROR_MSG + 1];

            int error;
            int _nrStop;
            DWORD _dwPcoValidImageCnt, _dwPcoMaxImageCnt;

            Camera *m_cam = (Camera *)argin;
            SyncCtrlObj *m_sync = m_cam->_getSyncCtrlObj();
            // BufferCtrlObj* m_buffer = m_sync->_getBufferCtrlObj();
            BufferCtrlObj *m_buffer = m_cam->_getBufferCtrlObj();

            char _msg[LEN_MSG + 1];
            __sprintfSExt(_msg, LEN_MSG, "%s> [ENTRY]", fnId);
            m_cam->_traceMsg(_msg);

            m_cam->_sprintComment(true, fnId, "[ENTRY]");

            struct stcPcoData *m_pcoData = m_cam->_getPcoData();
            m_pcoData->traceAcq.fnId = fnId;

            const char *msg;
            TIME_USEC tStart, tStart0;
            msElapsedTimeSet(tStart);
            tStart0 = tStart;

            long timeout, timeout0, msNowRecordLoop, msRecord, msXfer, msTotal;
            DWORD dwPcoAcquiredFrames;
            int iLimaAcquiredFrames;

            int requestStop = stopNone;

            HANDLE m_handle = m_cam->getHandle();

            WORD wSegment;
            m_cam->_pco_GetActiveRamSegment(wSegment, error);
            double msPerFrame = (m_cam->pcoGetCocRunTime() * 1000.);
            m_pcoData->traceAcq.msImgCoc = msPerFrame;

            // DWORD dwMsSleepOneFrame = (DWORD) (msPerFrame + 0.5);	// 4/5 rounding
            DWORD dwMsSleepOneFrame = (DWORD)(msPerFrame / 5.0); // 4/5 rounding
            if (dwMsSleepOneFrame == 0)
                dwMsSleepOneFrame = 1; // min sleep

            WORD wDoubleImage;
            int err;
            m_cam->_pco_GetDoubleImageMode(wDoubleImage, err);

            bool nb_frames_fixed = false;

            int iLimaRequestedFrames;
            m_sync->getNbFrames(iLimaRequestedFrames);
            m_pcoData->traceAcq.nrImgRequested0 = iLimaRequestedFrames;
            WORD dwPcoRequestedFrames = wDoubleImage ? iLimaRequestedFrames / 2 : iLimaRequestedFrames;

            m_sync->setAcqFrames(0);

            timeout = timeout0 = (long)(msPerFrame * (iLimaRequestedFrames * 1.3)); // 30% guard
            if (timeout < TOUT_MIN_DIMAX)
                timeout = TOUT_MIN_DIMAX;

            m_pcoData->traceAcq.msTout = m_pcoData->msAcqTout = timeout;
            _dwPcoValidImageCnt = 0;

            m_sync->getExpTime(m_pcoData->traceAcq.sExposure);
            m_sync->getLatTime(m_pcoData->traceAcq.sDelay);

            m_sync->setExposing(pcoAcqRecordStart);

            while (true)
            {
                m_cam->_pco_GetNumberOfImagesInSegment(wSegment, _dwPcoValidImageCnt, _dwPcoMaxImageCnt, error);
                if (error)
                {
                    printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__, "_pco_GetNumberOfImagesInSegment");
                    throw LIMA_HW_EXC(Error, "PCO_GetNumberOfImagesInSegment");
                }

                m_pcoData->dwValidImageCnt[wSegment - 1] = m_pcoData->traceAcq.nrImgRecorded = _dwPcoValidImageCnt;
                m_pcoData->dwMaxImageCnt[wSegment - 1] = m_pcoData->traceAcq.maxImgCount = _dwPcoMaxImageCnt;

                m_pcoData->msAcqTnow = msNowRecordLoop = msElapsedTime(tStart);
                m_pcoData->traceAcq.msRecordLoop = msNowRecordLoop;

                if ((dwPcoRequestedFrames > _dwPcoMaxImageCnt))
                {
                    nb_frames_fixed = true;

                    __sprintfSExt(msgErr, LEN_ERROR_MSG,
                                  "=== %s [%d]> ERROR INVALID NR FRAMES fixed "
                                  "nb_frames[%d] _dwPcoMaxImageCnt[%d]",
                                  fnId, __LINE__, dwPcoRequestedFrames, _dwPcoMaxImageCnt);
                    printf("%s\n", msgErr);

                    m_sync->setExposing(pcoAcqError);
                    break;
                }

                if ((_dwPcoValidImageCnt >= dwPcoRequestedFrames))
                    break;

                if ((timeout < msNowRecordLoop) && !m_pcoData->bExtTrigEnabled)
                {
                    // m_sync->setExposing(pcoAcqRecordTimeout);
                    // m_sync->stopAcq();
                    m_sync->setExposing(pcoAcqStop);
                    printf("=== %s [%d]> TIMEOUT!!! tout[(%ld) 0(%ld)] recLoopTime[%ld "
                           "ms] lastImgRecorded[%ld] dwPcoRequestedFrames[%d]\n",
                           fnId, __LINE__, timeout, timeout0, msNowRecordLoop, _dwPcoValidImageCnt,
                           dwPcoRequestedFrames);
                    break;
                }

                if ((requestStop = m_sync->_getRequestStop(_nrStop)) == stopRequest)
                {
                    m_sync->_setRequestStop(stopNone);

                    char msg[LEN_TRACEACQ_MSG + 1];
                    // m_buffer->_setRequestStop(stopProcessing);
                    // m_sync->setExposing(pcoAcqStop);

                    snprintf(msg, LEN_TRACEACQ_MSG, "=== %s> STOP REQ (recording). lastImgRec[%d]\n", fnId,
                             _dwPcoValidImageCnt);
                    printf(msg);
                    m_pcoData->traceMsg(msg);
                    break;
                }
                Sleep(dwMsSleepOneFrame); // sleep 1 frame
            } // while(true)

            m_pcoData->msAcqTnow = msNowRecordLoop = msElapsedTime(tStart);
            m_pcoData->traceAcq.msRecordLoop = msNowRecordLoop;

            msg = m_cam->_pco_SetRecordingState(0, error);
            if (error)
            {
                printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__, msg);
                throw LIMA_HW_EXC(Error, "_pco_SetRecordingState");
            }

            if ((requestStop != stopRequest) && (!nb_frames_fixed))
            {
                if (m_sync->getExposing() == pcoAcqRecordStart)
                    m_sync->setExposing(pcoAcqRecordEnd);

                m_cam->_pco_GetNumberOfImagesInSegment(wSegment, _dwPcoValidImageCnt, _dwPcoMaxImageCnt, error);
                if (error)
                {
                    printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__, msg);
                    throw LIMA_HW_EXC(Error, "PCO_GetNumberOfImagesInSegment");
                }

                m_pcoData->dwValidImageCnt[wSegment - 1] = m_pcoData->traceAcq.nrImgRecorded = _dwPcoValidImageCnt;

                dwPcoAcquiredFrames =
                    (_dwPcoValidImageCnt < dwPcoRequestedFrames) ? _dwPcoValidImageCnt : dwPcoRequestedFrames;
                iLimaAcquiredFrames = wDoubleImage ? dwPcoAcquiredFrames * 2 : dwPcoAcquiredFrames;
                // m_sync->setAcqFrames(nb_acq_frames);

                // dimax recording time
                m_pcoData->msAcqRec = msRecord = msElapsedTime(tStart);
                m_pcoData->traceAcq.msRecord = msRecord; // loop & stop record

                m_pcoData->traceAcq.endRecordTimestamp = m_pcoData->msAcqRecTimestamp = getTimestamp();

                m_pcoData->traceAcq.nrImgAcquired = iLimaAcquiredFrames;
                m_pcoData->traceAcq.nrImgRequested = iLimaRequestedFrames;

                msElapsedTimeSet(tStart); // reset for xfer

                if (iLimaAcquiredFrames < iLimaRequestedFrames)
                    m_sync->setNbFrames(iLimaAcquiredFrames);

                //		if(m_buffer->_getRequestStop()) {
                //			m_sync->setExposing(pcoAcqStop);
                //		} else

                // --- in case of stop request during the record phase, the transfer
                // --- is made to avoid lose the image recorded
                {
                    pcoAcqStatus status;

                    if (m_cam->_isCameraType(Pco2k | Pco4k))
                    {
                        if (m_pcoData->testCmdMode & TESTCMDMODE_DIMAX_XFERMULTI)
                        {
                            status = (pcoAcqStatus)m_buffer->_xferImag();
                        }
                        else
                        {
                            if (wDoubleImage)
                            {
                                status = (pcoAcqStatus)m_buffer->_xferImagMultDoubleImage(); //  <-------------
                                                                                             //  default
                                                                                             //  pco2k/4k NO
                                                                                             //  waitobj
                            }
                            else
                            {
                                status = (pcoAcqStatus)m_buffer->_xferImagMult(); //  <-------------
                                                                                  //  default pco2k/4k
                                                                                  //  NO waitobj
                            }
                        }
                    }
                    else
                    {
                        if (m_pcoData->testCmdMode & TESTCMDMODE_DIMAX_XFERMULTI)
                        {
                            status = (pcoAcqStatus)m_buffer->_xferImagMult();
                        }
                        else
                        {
                            if (wDoubleImage)
                            {
                                status = (pcoAcqStatus)m_buffer->_xferImagDoubleImage(); //  <-------------
                                                                                         //  default dimax YES
                                                                                         //  waitobj
                            }
                            else
                            {
                                status = (pcoAcqStatus)m_buffer->_xferImag(); //  <-------------
                                                                              //  default dimax
                                                                              //  YES waitobj
                            }
                        }
                    }

                    if (nb_frames_fixed)
                        status = pcoAcqError;
                    m_sync->setExposing(status);
                }

            } // if nb_frames_fixed && no stopRequested

            // m_sync->setExposing(status);
            m_pcoData->dwMaxImageCnt[wSegment - 1] = m_pcoData->traceAcq.maxImgCount = _dwPcoMaxImageCnt;

            // traceAcq info - dimax xfer time
            m_pcoData->msAcqXfer = msXfer = msElapsedTime(tStart);
            m_pcoData->traceAcq.msXfer = msXfer;

            m_pcoData->msAcqAll = msTotal = msElapsedTime(tStart0);
            m_pcoData->traceAcq.msTotal = msTotal;

            m_pcoData->traceAcq.endXferTimestamp = m_pcoData->msAcqXferTimestamp = getTimestamp();

            __sprintfSExt(_msg, LEN_MSG,
                          "%s [%d]> [EXIT] imgRecorded[%d] coc[%g] recLoopTime[%ld] "
                          "tout[(%ld) 0(%ld)] rec[%ld] xfer[%ld] all[%ld](ms)\n",
                          fnId, __LINE__, _dwPcoValidImageCnt, msPerFrame, msNowRecordLoop, timeout, timeout0, msRecord,
                          msXfer, msTotal);
            m_cam->_traceMsg(_msg);

            // included in 34a8fb6723594919f08cf66759fe5dbd6dc4287e only for dimax (to
            // check for others)
            m_sync->setStarted(false);

#if 0
    if (requestStop == stopRequest)
    {
        Event* ev = new Event(Hardware, Event::Error, Event::Camera, Event::CamFault, errMsg);
        m_cam->_getPcoHwEventCtrlObj()->reportEvent(ev);
    }

#endif
        }

        //==========================================================================================================
        //==========================================================================================================

        void _pco_acq_thread_dimax_trig_single(void *argin)
        {
            DEF_FNID;
            printf("=== %s [%d]> %s ENTRY\n", fnId, __LINE__, getTimestamp(Iso));

            static char msgErr[LEN_ERROR_MSG + 1];

            int error;
            int _nrStop;
            DWORD _dwValidImageCnt, _dwMaxImageCnt;

            Camera *m_cam = (Camera *)argin;
            SyncCtrlObj *m_sync = m_cam->_getSyncCtrlObj();
            // BufferCtrlObj* m_buffer = m_sync->_getBufferCtrlObj();
            BufferCtrlObj *m_buffer = m_cam->_getBufferCtrlObj();

            struct stcPcoData *m_pcoData = m_cam->_getPcoData();
            m_pcoData->traceAcq.fnId = fnId;

            m_cam->_sprintComment(true, fnId, "[ENTRY]");

            const char *msg;
            TIME_USEC tStart, tStart0;
            msElapsedTimeSet(tStart);
            tStart0 = tStart;

            long timeout, timeout0, msNowRecordLoop, msRecord, msXfer, msTotal;
            int nb_acq_frames;
            int requestStop = stopNone;

            HANDLE m_handle = m_cam->getHandle();

            WORD wSegment;
            m_cam->_pco_GetActiveRamSegment(wSegment, error);
            double msPerFrame = (m_cam->pcoGetCocRunTime() * 1000.);
            m_pcoData->traceAcq.msImgCoc = msPerFrame;

            // DWORD dwMsSleepOneFrame = (DWORD) (msPerFrame + 0.5);	// 4/5 rounding
            DWORD dwMsSleepOneFrame = (DWORD)(msPerFrame / 5.0); // 4/5 rounding
            if (dwMsSleepOneFrame == 0)
                dwMsSleepOneFrame = 1; // min sleep

            bool nb_frames_fixed = false;
            int nb_frames;
            m_sync->getNbFrames(nb_frames);
            // m_pcoData->traceAcq.nrImgRequested = nb_frames;
            m_pcoData->traceAcq.nrImgRequested0 = nb_frames;

            m_sync->setAcqFrames(0);

            timeout = timeout0 = (long)(msPerFrame * (nb_frames * 1.3)); // 30% guard
            if (timeout < TOUT_MIN_DIMAX)
                timeout = TOUT_MIN_DIMAX;

            m_pcoData->traceAcq.msTout = m_pcoData->msAcqTout = timeout;
            _dwValidImageCnt = 0;

            m_sync->getExpTime(m_pcoData->traceAcq.sExposure);
            m_sync->getLatTime(m_pcoData->traceAcq.sDelay);

            m_sync->setExposing(pcoAcqRecordStart);

            m_cam->_pco_GetNumberOfImagesInSegment(wSegment, _dwValidImageCnt, _dwMaxImageCnt, error);
            if (error)
            {
                printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__, "_pco_GetNumberOfImagesInSegment");
                throw LIMA_HW_EXC(Error, "PCO_GetNumberOfImagesInSegment");
            }
            m_pcoData->dwValidImageCnt[wSegment - 1] = m_pcoData->traceAcq.nrImgRecorded = _dwValidImageCnt;
            m_pcoData->dwMaxImageCnt[wSegment - 1] = m_pcoData->traceAcq.maxImgCount = _dwMaxImageCnt;

            bool doWhile = true;

            if (((DWORD)nb_frames > _dwMaxImageCnt))
            {
                nb_frames_fixed = true;

                __sprintfSExt(msgErr, LEN_ERROR_MSG,
                              "=== %s [%d]> ERROR INVALID NR FRAMES fixed "
                              "nb_frames[%d] _dwMaxImageCnt[%d]",
                              fnId, __LINE__, nb_frames, _dwMaxImageCnt);
                printf("%s\n", msgErr);

                m_sync->setExposing(pcoAcqError);
                doWhile = false;
            }

            while (doWhile)
            {
                WORD wRecState_actual;

                m_pcoData->msAcqTnow = msNowRecordLoop = msElapsedTime(tStart);
                m_pcoData->traceAcq.msRecordLoop = msNowRecordLoop;

                error = m_cam->_pco_GetRecordingState(wRecState_actual);
                if (error)
                {
                    printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__, "_pco_GetRecordingState");
                    throw LIMA_HW_EXC(Error, "PCO_GetRecordingState");
                }

                if (wRecState_actual == 0)
                    break;

                if ((requestStop = m_sync->_getRequestStop(_nrStop)) == stopRequest)
                {
                    m_sync->_setRequestStop(stopNone);

                    char msg[LEN_TRACEACQ_MSG + 1];
                    // m_buffer->_setRequestStop(stopProcessing);
                    // m_sync->setExposing(pcoAcqStop);

                    snprintf(msg, LEN_TRACEACQ_MSG, "=== %s> STOP REQ (recording). lastImgRec[%d]\n", fnId,
                             _dwValidImageCnt);
                    printf(msg);
                    m_pcoData->traceMsg(msg);
                    break;
                }
                Sleep(dwMsSleepOneFrame); // sleep 1 frame
            } // while(true)

            m_pcoData->msAcqTnow = msNowRecordLoop = msElapsedTime(tStart);
            m_pcoData->traceAcq.msRecordLoop = msNowRecordLoop;

            msg = m_cam->_pco_SetRecordingState(0, error);
            if (error)
            {
                printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__, msg);
                throw LIMA_HW_EXC(Error, "_pco_SetRecordingState");
            }

            if ((requestStop != stopRequest) && (!nb_frames_fixed))
            {
                if (m_sync->getExposing() == pcoAcqRecordStart)
                    m_sync->setExposing(pcoAcqRecordEnd);

                m_cam->_pco_GetNumberOfImagesInSegment(wSegment, _dwValidImageCnt, _dwMaxImageCnt, error);
                if (error)
                {
                    printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__, msg);
                    throw LIMA_HW_EXC(Error, "PCO_GetNumberOfImagesInSegment");
                }

                m_pcoData->dwValidImageCnt[wSegment - 1] = m_pcoData->traceAcq.nrImgRecorded = _dwValidImageCnt;

                nb_acq_frames = (_dwValidImageCnt < (DWORD)nb_frames) ? _dwValidImageCnt : nb_frames;
                // m_sync->setAcqFrames(nb_acq_frames);

                // dimax recording time
                m_pcoData->msAcqRec = msRecord = msElapsedTime(tStart);
                m_pcoData->traceAcq.msRecord = msRecord; // loop & stop record

                m_pcoData->traceAcq.endRecordTimestamp = m_pcoData->msAcqRecTimestamp = getTimestamp();

                m_pcoData->traceAcq.nrImgAcquired = nb_acq_frames;
                m_pcoData->traceAcq.nrImgRequested = nb_frames;

                msElapsedTimeSet(tStart); // reset for xfer

                if (nb_acq_frames < nb_frames)
                    m_sync->setNbFrames(nb_acq_frames);

                //		if(m_buffer->_getRequestStop()) {
                //			m_sync->setExposing(pcoAcqStop);
                //		} else

                // --- in case of stop request during the record phase, the transfer
                // --- is made to avoid lose the image recorded
                {
                    pcoAcqStatus status;

                    status = (pcoAcqStatus)m_buffer->_xferImag_getImage();

                    if (nb_frames_fixed)
                        status = pcoAcqError;
                    m_sync->setExposing(status);
                }

            } // if nb_frames_fixed & no stopped

            // m_sync->setExposing(status);
            m_pcoData->dwMaxImageCnt[wSegment - 1] = m_pcoData->traceAcq.maxImgCount = _dwMaxImageCnt;

            // traceAcq info - dimax xfer time
            m_pcoData->msAcqXfer = msXfer = msElapsedTime(tStart);
            m_pcoData->traceAcq.msXfer = msXfer;

            m_pcoData->msAcqAll = msTotal = msElapsedTime(tStart0);
            m_pcoData->traceAcq.msTotal = msTotal;

            m_pcoData->traceAcq.endXferTimestamp = m_pcoData->msAcqXferTimestamp = getTimestamp();

            printf("=== %s [%d]> EXIT nb_frames_requested[%d] _dwValidImageCnt[%d] "
                   "_dwMaxImageCnt[%d] coc[%g] recLoopTime[%ld] "
                   "tout[(%ld) 0(%ld)] rec[%ld] xfer[%ld] all[%ld](ms)\n",
                   fnId, __LINE__, nb_frames, _dwValidImageCnt, _dwMaxImageCnt, msPerFrame, msNowRecordLoop, timeout,
                   timeout0, msRecord, msXfer, msTotal);

            // included in 34a8fb6723594919f08cf66759fe5dbd6dc4287e only for dimax (to
            // check for others)
            m_sync->setStarted(false);

#if 0
    if (requestStop == stopRequest)
    {
        Event* ev = new Event(Hardware, Event::Error, Event::Camera, Event::CamFault, errMsg);
        m_cam->_getPcoHwEventCtrlObj()->reportEvent(ev);
    }

#endif
        }

        //=====================================================================
        //=====================================================================

        void _pco_acq_thread_dimax_live(void *argin)
        {
            DEF_FNID;

            Camera *m_cam = (Camera *)argin;
            SyncCtrlObj *m_sync = m_cam->_getSyncCtrlObj();
            BufferCtrlObj *m_buffer = m_cam->_getBufferCtrlObj();

            char _msg[LEN_MSG + 1];
            __sprintfSExt(_msg, LEN_MSG, "%s> [ENTRY]", fnId);
            m_cam->_traceMsg(_msg);

            struct stcPcoData *m_pcoData = m_cam->_getPcoData();

            TIME_USEC tStart;
            msElapsedTimeSet(tStart);
            int error;
            long msXfer;
            int requestStop = stopNone;

            pcoAcqStatus status;

            m_cam->_sprintComment(true, fnId, "[ENTRY]");

            HANDLE m_handle = m_cam->getHandle();

            m_sync->setAcqFrames(0);

            // dimax recording time -> live NO record
            m_pcoData->msAcqRec = 0;
            m_pcoData->msAcqRecTimestamp = getTimestamp();

            // status = (pcoAcqStatus)m_buffer->_xferImag();

            if (!(m_pcoData->testCmdMode & TESTCMDMODE_DIMAX_XFERMULTI))
            {
                // _xferImagMult [PCO_GetImageEx]
                status = (pcoAcqStatus)m_buffer->_xferImagMult();
            }
            else
            {
                // TODO wDoubleImage

                // _xferImag [WaitForMultipleObjects]
                status = (pcoAcqStatus)m_buffer->_xferImag();
            }

            m_sync->setExposing(status);
            // m_sync->stopAcq();
            const char *msg = m_cam->_pco_SetRecordingState(0, error);
            if (error)
            {
                printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__, msg);
                // throw LIMA_HW_EXC(Error, "_pco_SetRecordingState");
            }

            // dimax xfer time
            m_pcoData->msAcqXfer = msXfer = msElapsedTime(tStart);
            m_pcoData->msAcqXferTimestamp = getTimestamp();
            __sprintfSExt(_msg, LEN_MSG, "%s> [EXIT] xfer[%ld] (ms) status[%s]\n", fnId, msXfer, sPcoAcqStatus[status]);
            m_cam->_traceMsg(_msg);

            m_sync->setStarted(false); // to test
        }

    } // namespace Pco
} // namespace lima