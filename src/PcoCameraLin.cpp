/**************************************************************************
###########################################################################
 This file is part of LImA, a Library for Image Acquisition

 Copyright (C) : 2009-2011
 European Synchrotron Radiation Facility
 BP 220, Grenoble 38043
 FRANCE

 This is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as publishey
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

#ifndef __linux__
#    include <process.h>
#    include <sys/stat.h>
#    include <sys/timeb.h>
#else
#    include <sys/stat.h>
#    include <sys/time.h>
#endif

#include <time.h>

#include "lima/Exceptions.h"
#include "lima/HwSyncCtrlObj.h"

#include "Pco.h"
#include "PcoCamera.h"
#include "PcoSyncCtrlObj.h"
#include "PcoBufferCtrlObj.h"
#include "PcoCameraUtils.h"

using namespace lima;
using namespace lima::Pco;

//=================================================================================================
//=================================================================================================
//---------------------------
//- utility thread
//---------------------------

class Camera::_AcqThread : public Thread
{
    DEB_CLASS_NAMESPC(DebModCamera, "Camera", "_AcqThread");

  public:
    _AcqThread(Camera &aCam, threadType aType);
    virtual ~_AcqThread();

  protected:
    virtual void threadFunction();
    virtual void threadFunction_Dimax();
    virtual void threadFunction_Edge();
    virtual void threadFunction_SwitchEdge();
    virtual void threadFunction_Edge_clhs();

  private:
    Camera &m_cam;
    threadType m_ty;
};

//=================================================================================================
//=================================================================================================
Camera::_AcqThread::_AcqThread(Camera &aCam, threadType aType)
    : m_cam(aCam), m_ty(aType)
{
    DEB_CONSTRUCTOR();
    // DEB_ALWAYS() << "[entry]" ;
    pthread_attr_setscope(&m_thread_attr, PTHREAD_SCOPE_PROCESS);
    // DEB_ALWAYS() << "[exit]" ;
}
//=================================================================================================
//=================================================================================================

Camera::_AcqThread::~_AcqThread()
{
    DEB_DESTRUCTOR();
    // DEB_ALWAYS() << "[entry]" ;

    AutoMutex aLock(m_cam.m_cond_thread.mutex());
    m_cam.m_quit = true;
    // m_cam.WaitObject_.Signal();
    m_cam.m_cond_thread.broadcast();
    aLock.unlock();

    join();
    // DEB_ALWAYS() << "[exit]" ;
}

//=================================================================================================
//=================================================================================================
//---------------------------
//- Camera::_AcqThread::threadFunction()
//---------------------------
void Camera::_AcqThread::threadFunction()
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    if (m_ty == threadSw)
    {
        m_cam.m_wait_flag_rolling = true;
        m_cam.m_quit_rolling = false;
        m_cam.m_thread_running_rolling = false;

        DEB_ALWAYS() << "+++ START threadFunction_SwitchEdge";
        threadFunction_SwitchEdge();
    }

    if (m_ty == threadAcq)
    {
        m_cam.m_wait_flag = true;
        m_cam.m_quit = false;
        m_cam.m_thread_running = false;

        if (m_cam.grabber_me4 && m_cam._isCameraType(Dimax))
        {
            DEB_ALWAYS() << "+++ START threadFunction_Dimax";
            threadFunction_Dimax();
            DEB_ALWAYS() << "+++ EXIT threadFunction_Dimax";
        }
        else if (m_cam.grabber_me4 && m_cam._isCameraType(Edge))
        {
            DEB_ALWAYS() << "+++ START threadFunction_Edge";
            threadFunction_Edge();
            DEB_ALWAYS() << "+++ EXIT threadFunction_Edge";
        }
        else if (m_cam.grabber_clhs)
        {
            DEB_ALWAYS() << "+++ START threadFunction_Edge_clhs";
            threadFunction_Edge_clhs();
            DEB_ALWAYS() << "+++ EXIT threadFunction_Edge_clhs";
        }
    }

    {
        DEB_ALWAYS() << "ABORT - camera not supported!!!";
        throw LIMA_HW_EXC(Error, "camera not supported");
    }
}

//=================================================================================================
// FOR DIMAX - BEGIN
//=================================================================================================
void Camera::_AcqThread::threadFunction_Dimax()
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    DEB_ALWAYS() << fnId << "[ENTRY]" ;

#ifdef ME4
    const char *_msgAbort = "notSet";
    TIME_USEC tStart;
    TIME_USEC tXferStart;

    long long usStart, usStartTot;

    // Camera &m_cam = *this;
    m_cam.m_pcoData->traceAcq.fnId = fnId;

    int err, errTot;
    int pcoBuffIdx, pcoFrameNr, pcoFrameNrTimestamp;
    void *limaBuffPtr;
    // void *pcoBuffPtr;
    DWORD width, height;
    bool bRequestStop;
    int _nrStop;

    AutoMutex aLock(m_cam.m_cond_thread.mutex());

    int nb_allocated_buffers;

    int _nb_frames, limaFrameNr;

    limaFrameNr = 0;
    if (m_cam.m_sync)
    {
        m_cam.m_sync->getNbFrames(_nb_frames);
    }
    else
    {
        _nb_frames = -1;
    }

    DEB_ALWAYS() << DEB_VAR3(m_cam.m_wait_flag, m_cam.m_quit, _nb_frames);

    while (!m_cam.m_quit)
    {
        aLock.lock();
        m_cam.m_thread_running = false;
        m_cam.m_cond_thread.broadcast();

        while (m_cam.m_wait_flag && !m_cam.m_quit)
        {
            // DEB_ALWAYS() << "++++++++++++++++++++++++++++++++++Wait " <<
            // DEB_VAR3(m_cam.m_wait_flag, m_cam.m_quit, m_cam.m_thread_running)
            // << "  " << getTimestamp(Iso);
            m_cam.m_cond_thread.wait();
        } // while wait

        DEB_ALWAYS() << "++++++++++++++++++++++++++++++++++Run "
                     << getTimestamp(Iso);
        m_cam.m_thread_running = true;
        m_cam.m_cond_thread.broadcast();
        aLock.unlock();

        if (m_cam.m_quit)
            return;

        HwInterface::StatusType::Basic _statusReturn = HwInterface::StatusType::Basic::Ready;
        bool continueAcq = true;

        m_cam.m_checkImgNr->init(& m_cam.m_pcoData->traceAcq);

        bool bNoTimestamp;
        WORD wTimestampMode;
        m_cam._pco_GetTimestampMode(wTimestampMode, err);
        bNoTimestamp = err || !wTimestampMode;

        m_cam.m_pcoData->traceAcq.usTicks[traceAcq_execTimeTot].desc =
            "total execTime";
        m_cam.m_pcoData->traceAcq.usTicks[traceAcq_Lima].desc = "Lima execTime";
        m_cam.m_pcoData->traceAcq.usTicks[traceAcq_pcoSdk].desc =
            "SDK execTime";

        msElapsedTimeSet(tStart);
        usElapsedTimeSet(usStart);
        usElapsedTimeSet(usStartTot);
        m_cam.m_pcoData->traceAcq.fnId = fnId;
        m_cam.m_pcoData->traceAcq.fnTimestampEntry = getTimestamp();
        m_cam.m_pcoData->traceAcq.fnIdXfer = fnId;

        m_cam.m_pcoData->traceAcq.msStartAcqStart = msElapsedTime(tStart);

        // m_cam.m_pcoData->traceAcq.usTicks[traceAcq_Lima].desc = "xfer to lima
        // / total execTime";

        //m_cam.setStarted(true);

        m_cam.m_sync->getNbFrames(_nb_frames);
        limaFrameNr = 0; // 0 ..... N-1

        m_cam.m_pcoData->traceAcq.nrImgRequested = _nb_frames;

        aLock.lock();
        m_cam.m_status = HwInterface::StatusType::Basic::Exposure;
        m_cam.m_cond_thread.broadcast();
        aLock.unlock();

        err = m_cam.grabber_me4->Get_actual_size(&width, &height, NULL);
        PCO_CHECK_ERROR1(err, "Get_actual_size");
        if (err)
            m_cam.m_pcoData->traceAcq.nrErrors++;

        DEB_ALWAYS() << DEB_VAR3(width, height, _nb_frames);

        pcoBuffIdx = 1;

        bool acquireFirst = m_cam._isCameraType(Edge) &&
                            m_cam._isInterfaceType(INTERFACE_CAMERALINK);

        errTot = 0;
        if (acquireFirst)
        {
            // err = grabber_me4->Start_Acquire(_nb_frames);
            // PCO_CHECK_ERROR1(err, "Start_Acquire");
            // if(err)  {errTot ++; m_cam.m_pcoData->traceAcq.nrErrors++;}

            DEB_ALWAYS()
                << fnId
                << " _pco_SetRecordingState AFTER grabber_me4->Start_Acquire";
            m_cam._pco_SetRecordingState(1, err);
            PCO_CHECK_ERROR1(err, "SetRecordingState(1)");
            if (err)
            {
                errTot++;
                m_cam.m_pcoData->traceAcq.nrErrors++;
            }
        }
        else
        {
            DEB_ALWAYS()
                << fnId
                << " _pco_SetRecordingState BEFORE grabber_me4->Start_Acquire";
            m_cam._pco_SetRecordingState(1, err);
            PCO_CHECK_ERROR1(err, "SetRecordingState(1)");
            if (err)
            {
                errTot++;
                m_cam.m_pcoData->traceAcq.nrErrors++;
            }

            // err = grabber_me4->Start_Acquire(_nb_frames);
            // PCO_CHECK_ERROR1(err, "Start_Acquire");
            // if(err)  {errTot ++; m_cam.m_pcoData->traceAcq.nrErrors++;}
        }

        bool bAbort = false;
        Event::Severity abortSeverity = Event::Error;

        if (errTot)
        {
            m_cam.m_pcoData->traceAcq.nrErrors++;
            //setStatus(HwInterface::StatusType::Basic::Fault,false);
            //_statusReturn = HwInterface::StatusType::Basic::Fault;
            continueAcq = false;
            m_cam.m_wait_flag = true;
            bAbort = true;
        }

        ///=========== recording phase [begin]
        if (m_cam._isCameraType(Dimax) && continueAcq)
        {
            DWORD validCount, maxCount;
            int error;

            DEB_ALWAYS() << "INFO - recording [begin]";
            while (1)
            {
                m_cam._waitForRecording(_nb_frames, validCount, maxCount,
                                        error);

                bRequestStop = !!(m_cam.getRequestStop(_nrStop));

                if (error || ((DWORD)_nb_frames > maxCount))
                {
                    continueAcq = false;
                    m_cam.m_wait_flag = true;
                    bAbort = true;
                    _msgAbort = "ABORT - _waitForRecording ";
                    DEB_ALWAYS()
                        << _msgAbort
                        << DEB_VAR4(_nb_frames, validCount, maxCount, error);
                    break;
                }

                if ((validCount >= (DWORD)_nb_frames) || bRequestStop)
                {
                    m_cam._pco_SetRecordingState(0, err);
                    PCO_CHECK_ERROR1(err, "SetRecordingState(0)");
                    if (err)
                    {
                        continueAcq = false;
                        m_cam.m_wait_flag = true;
                        bAbort = true;
                        _msgAbort = "ABORT - _SetRecordingState(0)";
                        DEB_ALWAYS() << _msgAbort;
                        break;
                    }
                    else
                    {
                        DEB_ALWAYS() << "INFO - recording [end]";
                        break;
                    }
                }
            } // while(1)
        }     // if(m_cam._isCameraType(Dimax) && continueAcq)
              ///=========== recording phase [end]

        msElapsedTimeSet(tXferStart);

        //time_t tsEnd = m_pcoData->traceAcq.endXferTimestamp;

        m_cam.m_pcoData->traceAcq.startXferTimestamp = getTimestamp();

        // WORD wActSeg = m_cam._pco_GetActiveRamSegment();
        WORD wActSeg;
        m_cam._pco_GetActiveRamSegment(wActSeg, err);

        while (!m_cam.m_wait_flag && continueAcq &&
               ((_nb_frames == 0) || limaFrameNr < _nb_frames))
        {
            m_cam.m_pcoData->traceAcq.usTicks[traceAcq_pcoSdk].value +=
                usElapsedTime(usStart);
            usElapsedTimeSet(usStart);

            pcoFrameNr = limaFrameNr + 1;
            limaBuffPtr = m_cam.m_buffer->_getFrameBufferPtr(
                limaFrameNr, nb_allocated_buffers);
            // DEB_ALWAYS() << DEB_VAR4(nb_allocated_buffers, _nb_frames,
            // limaFrameNr, limaBuffPtr);

            m_cam.m_pcoData->traceAcq.usTicks[traceAcq_GetImageEx].value +=
                usElapsedTime(usStart);
            usElapsedTimeSet(usStart);

            m_cam.setStatus(HwInterface::StatusType::Basic::Readout, false);

            m_cam.m_pcoData->traceAcq.usTicks[traceAcq_Lima].value +=
                usElapsedTime(usStart);
            usElapsedTimeSet(usStart);

            usElapsedTimeSet(usStart);

            // err = camera-> pco
            // _ReadImagesFromSegment(wActSeg,pcoFrameNr,pcoFrameNr);
            // err=grabber_me4->Get_Framebuffer_adr(pcoBuffIdx,&pcoBuffPtr);

            {
                // grabber_me4->Extract_Image(limaBuffPtr,pcoBuffPtr,width,height);
                // DWORD Get_Image ( WORD Segment, DWORD ImageNr, void * adr )
                // DEB_ALWAYS() << DEB_VAR3(wActSeg, pcoFrameNr, limaBuffPtr);

                m_cam.grabber_me4->Get_Image(wActSeg, pcoFrameNr, limaBuffPtr);
                PCO_CHECK_ERROR1(err, "Get_Image");
                if (err != PCO_NOERROR)
                {
                    m_cam.m_pcoData->traceAcq.nrErrors++;
                    _msgAbort = "ABORT - Get_Image ";
                    DEB_ALWAYS() << _msgAbort
                                 << DEB_VAR3(wActSeg, pcoFrameNr, limaBuffPtr);
                    continueAcq = false;
                    m_cam.m_wait_flag = true;
                    bAbort = true;
                    break;
                }

                m_cam.m_pcoData->traceAcq.usTicks[traceAcq_pcoSdk].value +=
                    usElapsedTime(usStart);
                usElapsedTimeSet(usStart);

                // check PCO ImgNr with the limaFrame
                m_cam.m_checkImgNr->update(limaFrameNr, limaBuffPtr);
                pcoFrameNrTimestamp = m_cam.m_pcoData->traceAcq.checkImgNrPcoTimestamp;

                HwFrameInfoType frame_info;
                frame_info.acq_frame_nb = limaFrameNr;
                continueAcq =
                    m_cam.m_buffer->m_buffer_cb_mgr.newFrameReady(frame_info);

                m_cam.m_pcoData->traceAcq.usTicks[traceAcq_Lima].value +=
                    usElapsedTime(usStart);
                usElapsedTimeSet(usStart);
            }

#ifdef ME4
            err = m_cam.grabber_me4->Unblock_buffer(pcoBuffIdx);
#endif
            PCO_CHECK_ERROR1(err, "Unblock_buffer");
            if (err != PCO_NOERROR)
            {
                m_cam.m_pcoData->traceAcq.nrErrors++;
                _msgAbort = "ABORT - Unblock_buffer ";
                DEB_ALWAYS() << _msgAbort << DEB_VAR1(err);
                continueAcq = false;
                m_cam.m_wait_flag = true;
                bAbort = true;
                break; // while frames
            }
            m_cam.m_pcoData->traceAcq.usTicks[traceAcq_pcoSdk].value +=
                usElapsedTime(usStart);
            usElapsedTimeSet(usStart);

            if ((limaFrameNr % 100) == 0)
            {
                printf("pcoFrameNr [%d] diff[%d]\r", pcoFrameNr,
                       pcoFrameNrTimestamp - pcoFrameNr);
                // printf("\n");
            }
            ++limaFrameNr;

            int _nrStop;
            if (m_cam.getRequestStop(_nrStop))
            {
                _msgAbort = "STOP REQUEST";
                DEB_ALWAYS() << _msgAbort;
                continueAcq = false;
                m_cam.m_wait_flag = true;
                bAbort = true;
                abortSeverity = Event::Info;
            }

        } // while  nb_frames, continue, wait

        m_cam._stopAcq(false);

        printf("\n");

        //------ xfer statistics	

        m_cam.m_pcoData->traceAcq.endXferTimestamp = getTimestamp();
        m_cam.m_pcoData->traceAcq.msXfer = msElapsedTime(tXferStart);


        m_cam.m_pcoData->traceAcq.usTicks[traceAcq_pcoSdk].value +=
            usElapsedTime(usStart);
        usElapsedTimeSet(usStart);
        m_cam._pco_SetRecordingState(0, err);
        PCO_CHECK_ERROR1(err, "SetRecordingState(0)");
        if (err)
        {
            m_cam.m_pcoData->traceAcq.nrErrors++;
            _msgAbort = "ABORT - SetRecordingState(0)";
            DEB_ALWAYS() << _msgAbort;
            bAbort = true;
        }

        err = m_cam.grabber_me4->Stop_Acquire();
        PCO_CHECK_ERROR1(err, "Stop_Acquire");
        if (err != PCO_NOERROR)
        {
            m_cam.m_pcoData->traceAcq.nrErrors++;
            _msgAbort = "ABORT - Stop_Acquire()";
            DEB_ALWAYS() << _msgAbort;
            bAbort = true;
        }

        err = m_cam.grabber_me4->Free_Framebuffer();
        PCO_CHECK_ERROR1(err, "Free_Framebuffer");
        if (err != PCO_NOERROR)
        {
            m_cam.m_pcoData->traceAcq.nrErrors++;
            _msgAbort = "ABORT - Free_Framebuffer()";
            DEB_ALWAYS() << _msgAbort;
            bAbort = true;
        }
        m_cam.m_pcoData->traceAcq.usTicks[traceAcq_pcoSdk].value +=
            usElapsedTime(usStart);
        usElapsedTimeSet(usStart);

        m_cam.m_pcoData->msAcqAll = 
            m_cam.m_pcoData->traceAcq.msTotal=
            msElapsedTime(tStart);

        m_cam.m_pcoData->traceAcq.fnTimestampExit = getTimestamp();
        m_cam.m_pcoData->traceAcq.msStartAcqEnd = msElapsedTime(tStart);
        m_cam.m_pcoData->traceAcq.usTicks[traceAcq_execTimeTot].value =
            usElapsedTime(usStartTot);
        m_cam.setStatus(_statusReturn, false);

        //aLock.lock();
        m_cam.m_wait_flag = true;
        m_cam.setStarted(false);

        if (bAbort)
        {
            DEB_ALWAYS() << _msgAbort;
            {
                Event *ev = new Event(Hardware, abortSeverity, Event::Camera,
                                      Event::Default, _msgAbort);
                m_cam._getPcoHwEventCtrlObj()->reportEvent(ev);
            }
        }

    } // while quit
#endif

    DEB_ALWAYS() << fnId << "[EXIT]" ;
}

//=================================================================================================
// FOR DIMAX - END
//=================================================================================================

//=================================================================================================
// FOR EDGE - BEGIN
//=================================================================================================
void Camera::_AcqThread::threadFunction_Edge()
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    DEB_ALWAYS() << fnId << "[ENTRY]" ;

#ifdef ME4

    const char *_msgAbort = "notSet";
    TIME_USEC tStart;
    TIME_USEC tXferStart;
    
    long long usStart, usStartTot;

    m_cam.m_pcoData->traceAcq.fnId = fnId;

    int err, errTot;
    int pcoBuffIdx, pcoFrameNr, pcoFrameNrTimestamp;
    void *limaBuffPtr;
    void *pcoBuffPtr;
    DWORD width, height;

    AutoMutex aLock(m_cam.m_cond_thread.mutex());

    int nb_allocated_buffers;

    int _nb_frames, limaFrameNr;

    limaFrameNr = 0;
    if (m_cam.m_sync)
    {
        m_cam.m_sync->getNbFrames(_nb_frames);
    }
    else
    {
        _nb_frames = -1;
    }

    DEB_ALWAYS() << DEB_VAR3(m_cam.m_wait_flag, m_cam.m_quit, _nb_frames);

    while (!m_cam.m_quit)
    {
        while (m_cam.m_wait_flag && !m_cam.m_quit)
        {
            // DEB_ALWAYS() << "++++++++++++++++++++++++++++++++++Wait " <<
            // getTimestamp(Iso);
            m_cam.m_thread_running = false;
            m_cam.m_cond_thread.broadcast();
            m_cam.m_cond_thread.wait();
        } // while wait

        DEB_ALWAYS() << "++++++++++++++++++++++++++++++++++Run "
                     << getTimestamp(Iso);
        m_cam.m_thread_running = true;
        if (m_cam.m_quit)
            return;

        m_cam.m_checkImgNr->init(& m_cam.m_pcoData->traceAcq);

        HwInterface::StatusType::Basic _statusReturn = HwInterface::StatusType::Basic::Ready;
        bool continueAcq = true;

        bool bNoTimestamp;
        WORD wTimestampMode;
        m_cam._pco_GetTimestampMode(wTimestampMode, err);
        bNoTimestamp = err || !wTimestampMode;

        m_cam.m_pcoData->traceAcq.usTicks[traceAcq_execTimeTot].desc =
            "total execTime";
        m_cam.m_pcoData->traceAcq.usTicks[traceAcq_Lima].desc = "Lima execTime";
        m_cam.m_pcoData->traceAcq.usTicks[traceAcq_pcoSdk].desc =
            "SDK execTime";

        msElapsedTimeSet(tStart);
        usElapsedTimeSet(usStart);
        usElapsedTimeSet(usStartTot);
        m_cam.m_pcoData->traceAcq.fnId = fnId;
        m_cam.m_pcoData->traceAcq.fnTimestampEntry = getTimestamp();
        m_cam.m_pcoData->traceAcq.fnIdXfer = fnId;


        m_cam.m_pcoData->traceAcq.msStartAcqStart = msElapsedTime(tStart);
        msElapsedTimeSet(tXferStart);
        m_cam.m_pcoData->traceAcq.endXferTimestamp = 0;


        // m_cam.m_pcoData->traceAcq.usTicks[traceAcq_Lima].desc = "xfer to lima
        // / total execTime";

        m_cam.setStarted(true);

        m_cam.m_sync->getNbFrames(_nb_frames);
        limaFrameNr = 0; // 0 ..... N-1

        m_cam.m_pcoData->traceAcq.nrImgRequested = _nb_frames;

        m_cam.m_status = HwInterface::StatusType::Basic::Exposure;
        m_cam.m_cond_thread.broadcast();
        aLock.unlock();

        err = m_cam.grabber_me4->Get_actual_size(&width, &height, NULL);
        PCO_CHECK_ERROR1(err, "Get_actual_size");
        if (err)
            m_cam.m_pcoData->traceAcq.nrErrors++;

        DEB_ALWAYS() << DEB_VAR3(width, height, _nb_frames);

        pcoBuffIdx = 1;

        // bool acquireFirst = m_cam._isCameraType(Edge) &&
        // m_cam._isInterfaceType(INTERFACE_CAMERALINK);
        bool acquireFirst = m_cam._isCameraType(Edge) &&
                            m_cam._isInterfaceType(ifCameralinkAll);

        errTot = 0;
        if (acquireFirst)
        {
            DEB_ALWAYS() << "Start_Acquire";
#ifdef ME4
            err = m_cam.grabber_me4->Start_Acquire(_nb_frames);
#endif
            PCO_CHECK_ERROR1(err, "Start_Acquire");
            if (err)
            {
                errTot++;
                m_cam.m_pcoData->traceAcq.nrErrors++;
            }

            DEB_ALWAYS() << "_pco_SetRecordingState(1)";
            m_cam._pco_SetRecordingState(1, err);
            PCO_CHECK_ERROR1(err, "SetRecordingState(1)");
            if (err)
            {
                errTot++;
                m_cam.m_pcoData->traceAcq.nrErrors++;
            }
        }
        else
        {
            DEB_ALWAYS() << "_pco_SetRecordingState(1)";
            m_cam._pco_SetRecordingState(1, err);
            PCO_CHECK_ERROR1(err, "SetRecordingState(1)");
            if (err)
            {
                errTot++;
                m_cam.m_pcoData->traceAcq.nrErrors++;
            }

            DEB_ALWAYS() << "Start_Acquire";
#ifdef ME4
            err = m_cam.grabber_me4->Start_Acquire(_nb_frames);
#endif
            PCO_CHECK_ERROR1(err, "Start_Acquire");
            if (err)
            {
                errTot++;
                m_cam.m_pcoData->traceAcq.nrErrors++;
            }
        }

        if (errTot)
        {
            m_cam.m_pcoData->traceAcq.nrErrors++;
            // m_cam.setStatus(HwInterface::StatusType::Basic::Fault,false);
            //_statusReturn = HwInterface::StatusType::Basic::Fault;
            continueAcq = false;
            m_cam.m_wait_flag = true;
        }

        bool bAbort = false;
        Event::Severity abortSeverity = Event::Error;

        while (!m_cam.m_wait_flag && continueAcq &&
               ((_nb_frames == 0) || limaFrameNr < _nb_frames))
        {
            // DEB_ALWAYS() << DEB_VAR4(m_cam.m_wait_flag, continueAcq,
            // _nb_frames, limaFrameNr);

            m_cam.m_pcoData->traceAcq.usTicks[traceAcq_pcoSdk].value +=
                usElapsedTime(usStart);
            usElapsedTimeSet(usStart);

            pcoFrameNr = limaFrameNr + 1;
            limaBuffPtr = m_cam.m_buffer->_getFrameBufferPtr(
                limaFrameNr, nb_allocated_buffers);
            // DEB_ALWAYS() << DEB_VAR4(nb_allocated_buffers, _nb_frames,
            // limaFrameNr, limaBuffPtr);

            m_cam.m_pcoData->traceAcq.usTicks[traceAcq_GetImageEx].value +=
                usElapsedTime(usStart);
            usElapsedTimeSet(usStart);

            m_cam.setStatus(HwInterface::StatusType::Basic::Readout, false);

            m_cam.m_pcoData->traceAcq.usTicks[traceAcq_Lima].value +=
                usElapsedTime(usStart);
            usElapsedTimeSet(usStart);

            usElapsedTimeSet(usStart);
            err = m_cam.grabber_me4->Wait_For_Next_Image(&pcoBuffIdx, 10);
            PCO_CHECK_ERROR1(err, "Wait_For_Next_Image");
            if (err != PCO_NOERROR)
            {
                m_cam.m_pcoData->traceAcq.nrErrors++;
                _msgAbort = "ABORT - Wait_For_Next_Image ";
                DEB_ALWAYS() << _msgAbort << DEB_VAR1(pcoFrameNr);
                continueAcq = false;
                m_cam.m_wait_flag = true;
                bAbort = true;
                break; // while frames
            }

            if (err == PCO_NOERROR)
            {
#ifdef ME4
                err = m_cam.grabber_me4->Check_DMA_Length(pcoBuffIdx);
#endif
                PCO_CHECK_ERROR1(err, "Check_DMA_Length");
                if (err != PCO_NOERROR)
                {
                    m_cam.m_pcoData->traceAcq.nrErrors++;
                    _msgAbort = "ABORT - Check_DMA_Length ";
                    DEB_ALWAYS() << _msgAbort << DEB_VAR1(err);
                    continueAcq = false;
                    m_cam.m_wait_flag = true;
                    bAbort = true;
                    break; // while frames
                }
            }

            if (err != PCO_NOERROR)
            {
                _msgAbort =
                    "ABORT - grab_loop Error break loop at image number ";
                DEB_ALWAYS() << _msgAbort << DEB_VAR1(pcoFrameNr);
                //_statusReturn = HwInterface::StatusType::Basic::Fault;
                // m_cam.setStatus(HwInterface::StatusType::Basic::Fault,false);
                continueAcq = false;
                m_cam.m_wait_flag = true;
                continueAcq = false;
                m_cam.m_wait_flag = true;
                bAbort = true;
                break; // while frames
                // continue;   // while wait
            }

            // DEB_ALWAYS()  << "lima image#  " << DEB_VAR1(limaFrameNr) <<"
            // acquired !";

#ifdef ME4
            err = m_cam.grabber_me4->Get_Framebuffer_adr(pcoBuffIdx, &pcoBuffPtr);
#endif
            PCO_CHECK_ERROR1(err, "Get_Framebuffer_adr");
            if (err != PCO_NOERROR)
            {
                m_cam.m_pcoData->traceAcq.nrErrors++;
                _msgAbort = "ABORT - Get_Framebuffer_adr ";
                DEB_ALWAYS() << _msgAbort << DEB_VAR1(pcoBuffIdx);
                continueAcq = false;
                m_cam.m_wait_flag = true;
                bAbort = true;
                break; // while frames
            }
            if (err == PCO_NOERROR)
            {
#ifdef ME4
                m_cam.grabber_me4->Extract_Image(limaBuffPtr, pcoBuffPtr, width,
                                             height);
#endif

                m_cam.m_pcoData->traceAcq.usTicks[traceAcq_pcoSdk].value +=
                    usElapsedTime(usStart);
                usElapsedTimeSet(usStart);

                // check PCO ImgNr with the limaFrame
                m_cam.m_checkImgNr->update(limaFrameNr, limaBuffPtr);
                pcoFrameNrTimestamp = m_cam.m_pcoData->traceAcq.checkImgNrPcoTimestamp;
                    
                HwFrameInfoType frame_info;
                frame_info.acq_frame_nb = limaFrameNr;
                continueAcq =
                    m_cam.m_buffer->m_buffer_cb_mgr.newFrameReady(frame_info);

                m_cam.m_pcoData->traceAcq.usTicks[traceAcq_Lima].value +=
                    usElapsedTime(usStart);
                usElapsedTimeSet(usStart);
            }

#ifdef ME4
            err = m_cam.grabber_me4->Unblock_buffer(pcoBuffIdx);
#endif
            PCO_CHECK_ERROR1(err, "Unblock_buffer");
            if (err != PCO_NOERROR)
            {
                m_cam.m_pcoData->traceAcq.nrErrors++;
                _msgAbort = "ABORT - Unblock_buffer ";
                DEB_ALWAYS() << _msgAbort << DEB_VAR1(err);
                continueAcq = false;
                m_cam.m_wait_flag = true;
                bAbort = true;
                break; // while frames
            }
            m_cam.m_pcoData->traceAcq.usTicks[traceAcq_pcoSdk].value +=
                usElapsedTime(usStart);
            usElapsedTimeSet(usStart);

            if ((limaFrameNr % 100) == 0)
            {
                printf("pcoFrameNr [%d] diff[%d]\r", pcoFrameNr,
                       pcoFrameNrTimestamp - pcoFrameNr);
                // printf("\n");
            }
            ++limaFrameNr;

            int _nrStop;
            if ((m_cam.getRequestStop(_nrStop)))
            {
                _msgAbort = "STOP REQUEST";
                DEB_ALWAYS() << _msgAbort;
                continueAcq = false;
                m_cam.m_wait_flag = true;
                bAbort = true;
                abortSeverity = Event::Warning;
            }

        } // while  nb_frames, continue, wait

        m_cam._stopAcq(false);

        printf("\n");
        
        m_cam.m_pcoData->traceAcq.endXferTimestamp = getTimestamp();
        m_cam.m_pcoData->traceAcq.msXfer = msElapsedTime(tXferStart);

        m_cam.m_pcoData->traceAcq.usTicks[traceAcq_pcoSdk].value +=
            usElapsedTime(usStart);
        usElapsedTimeSet(usStart);
        m_cam._pco_SetRecordingState(0, err);
        PCO_CHECK_ERROR1(err, "SetRecordingState(0)");
        if (err)
        {
            m_cam.m_pcoData->traceAcq.nrErrors++;
            _msgAbort = "ABORT - SetRecordingState(0)";
            DEB_ALWAYS() << _msgAbort;
            bAbort = true;
        }

        err = m_cam.grabber_me4->Stop_Acquire();
        PCO_CHECK_ERROR1(err, "Stop_Acquire");
        if (err != PCO_NOERROR)
        {
            m_cam.m_pcoData->traceAcq.nrErrors++;
            _msgAbort = "ABORT - Stop_Acquire()";
            DEB_ALWAYS() << _msgAbort;
            bAbort = true;
        }

        err = m_cam.grabber_me4->Free_Framebuffer();
        PCO_CHECK_ERROR1(err, "Free_Framebuffer");
        if (err != PCO_NOERROR)
        {
            m_cam.m_pcoData->traceAcq.nrErrors++;
            _msgAbort = "ABORT - Free_Framebuffer()";
            DEB_ALWAYS() << _msgAbort;
            bAbort = true;
        }
        m_cam.m_pcoData->traceAcq.usTicks[traceAcq_pcoSdk].value +=
            usElapsedTime(usStart);
        usElapsedTimeSet(usStart);

        m_cam.m_pcoData->msAcqAll = 
            m_cam.m_pcoData->traceAcq.msTotal=
            msElapsedTime(tStart);

        m_cam.m_pcoData->traceAcq.fnTimestampExit = getTimestamp();
        m_cam.m_pcoData->traceAcq.msStartAcqEnd = msElapsedTime(tStart);
        m_cam.m_pcoData->traceAcq.usTicks[traceAcq_execTimeTot].value =
            usElapsedTime(usStartTot);
        m_cam.setStatus(_statusReturn, false);

        aLock.lock();
        m_cam.m_wait_flag = true;
        m_cam.setStarted(false);

        if (bAbort)
        {
            DEB_ALWAYS() << _msgAbort;
            {
                Event *ev = new Event(Hardware, abortSeverity, Event::Camera,
                                      Event::Default, _msgAbort);
                m_cam._getPcoHwEventCtrlObj()->reportEvent(ev);
            }
        }

    } // while quit
#endif

        DEB_ALWAYS() << fnId << "[EXIT]" ;
}
//=================================================================================================
// FOR EDGE - END
//=================================================================================================


//=================================================================================================
// FOR EDGE HS - BEGIN
//=================================================================================================
void Camera::_AcqThread::threadFunction_Edge_clhs()
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    DEB_ALWAYS() << fnId << "[ENTRY]" ;

#ifdef CLHS

    char buff[MSG4K + 1];

    const char *_msgAbort = "notSet";
    TIME_USEC tStart;
    TIME_USEC tXferStart;
    
    
    long long usStart, usStartTot;

    m_cam.m_pcoData->traceAcq.fnId = fnId;
    // DEB_ALWAYS() << "[entry]" ;

    int err, errTot;
    int pcoFrameNr, pcoFrameNrTimestamp;
    void *limaBuffPtr;
    void *pcoBuffPtr;
    DWORD width, height;
    int _nrStop;


	/// 2020/05/18
	/// in this moment grabber->Wait_For_Next_Image can't be recalled again
	/// support is working to fix this
	/// now -> we will use timeout ~ 5 - 10s
	/// if timeout ABORT
	


    int timeoutMs, timeoutMsTot;
    int timeoutMsMax = 10 * (60 * 1000); 
    int timeoutMs0 = 5000;  // 5 sec  will not retry Wait_For_Next_Image
    


    


	CPco_grab_clhs *grabber = m_cam.grabber_clhs;
	if(grabber == NULL)
	{
		THROW_FATAL(Hardware, Error) << "threadFunction_Edge_clhs: grabber == NULL";
	}
	

	err = grabber->Set_Grabber_Timeout(timeoutMs0);
    PCO_CHECK_ERROR1(err, "Set_Grabber_Timeout");
    err = grabber->Get_Grabber_Timeout(&timeoutMs);
    PCO_CHECK_ERROR1(err, "Get_Grabber_Timeout");

    AutoMutex aLock(m_cam.m_cond_thread.mutex());

    int nb_allocated_buffers;

    int _nb_frames, limaFrameNr;

    limaFrameNr = 0;
    if (m_cam.m_sync)
    {
        m_cam.m_sync->getNbFrames(_nb_frames);
    }
    else
    {
        _nb_frames = -1;
    }

    DEB_ALWAYS() << DEB_VAR3(m_cam.m_wait_flag, m_cam.m_quit, _nb_frames);

    while (!m_cam.m_quit)
    {
        while (m_cam.m_wait_flag && !m_cam.m_quit)
        {
            // DEB_ALWAYS() << "++++++++++++++++++++++++++++++++++Wait " <<
            // getTimestamp(Iso);
            m_cam.m_thread_running = false;
            m_cam.m_cond_thread.broadcast();
            m_cam.m_cond_thread.wait();
        } // while wait

        DEB_ALWAYS() << "++++++++++++++++++++++++++++++++++Run "
                     << getTimestamp(Iso);
        m_cam.m_thread_running = true;
        if (m_cam.m_quit)
            return;

        m_cam.m_checkImgNr->init(& m_cam.m_pcoData->traceAcq);

        HwInterface::StatusType::Basic _statusReturn = HwInterface::StatusType::Basic::Ready;
        bool continueAcq = true;

        bool bNoTimestamp;
        WORD wTimestampMode;
        m_cam._pco_GetTimestampMode(wTimestampMode, err);
        bNoTimestamp = err || !wTimestampMode;

        m_cam.m_pcoData->traceAcq.usTicks[traceAcq_execTimeTot].desc =
            "total execTime";
        m_cam.m_pcoData->traceAcq.usTicks[traceAcq_Lima].desc = "Lima execTime";
        m_cam.m_pcoData->traceAcq.usTicks[traceAcq_pcoSdk].desc =
            "SDK execTime";

        msElapsedTimeSet(tStart);
        usElapsedTimeSet(usStart);
        usElapsedTimeSet(usStartTot);
        m_cam.m_pcoData->traceAcq.fnId = fnId;
        m_cam.m_pcoData->traceAcq.fnTimestampEntry = getTimestamp();
        m_cam.m_pcoData->traceAcq.fnIdXfer = fnId;


        m_cam.m_pcoData->traceAcq.msStartAcqStart = msElapsedTime(tStart);
        msElapsedTimeSet(tXferStart);
        m_cam.m_pcoData->traceAcq.endXferTimestamp = 0;


        // m_cam.m_pcoData->traceAcq.usTicks[traceAcq_Lima].desc = "xfer to lima
        // / total execTime";

        m_cam.setStarted(true);

        m_cam.m_sync->getNbFrames(_nb_frames);
        limaFrameNr = 0; // 0 ..... N-1

        m_cam.m_pcoData->traceAcq.nrImgRequested = _nb_frames;

        m_cam.m_status = HwInterface::StatusType::Basic::Exposure;
        m_cam.m_cond_thread.broadcast();
        aLock.unlock();

        err = grabber->Get_actual_size(&width, &height, NULL);
        PCO_CHECK_ERROR1(err, "Get_actual_size");
        if (err)
            m_cam.m_pcoData->traceAcq.nrErrors++;

        DEB_ALWAYS() << DEB_VAR3(width, height, _nb_frames);

        // bool acquireFirst = m_cam._isCameraType(Edge) &&
        // m_cam._isInterfaceType(INTERFACE_CAMERALINK);
        bool acquireFirst = m_cam._isCameraType(Edge) &&
                            m_cam._isInterfaceType(ifCameralinkAll);

        errTot = 0;
        if (acquireFirst)
        {
            DEB_ALWAYS() << "Start_Acquire";
            err = grabber->Start_Acquire();
            PCO_CHECK_ERROR1(err, "Start_Acquire");
            if (err)
            {
                errTot++;
                m_cam.m_pcoData->traceAcq.nrErrors++;
            }

            DEB_ALWAYS() << "_pco_SetRecordingState(1)";
            m_cam._pco_SetRecordingState(1, err);
            PCO_CHECK_ERROR1(err, "SetRecordingState(1)");
            if (err)
            {
                errTot++;
                m_cam.m_pcoData->traceAcq.nrErrors++;
            }
        }
        else
        {
            DEB_ALWAYS() << "_pco_SetRecordingState(1)";
            m_cam._pco_SetRecordingState(1, err);
            PCO_CHECK_ERROR1(err, "SetRecordingState(1)");
            if (err)
            {
                errTot++;
                m_cam.m_pcoData->traceAcq.nrErrors++;
            }

            DEB_ALWAYS() << "Start_Acquire";
            err = grabber->Start_Acquire();
            PCO_CHECK_ERROR1(err, "Start_Acquire");
            if (err)
            {
                errTot++;
                m_cam.m_pcoData->traceAcq.nrErrors++;
            }
        }

        if (errTot)
        {
            m_cam.m_pcoData->traceAcq.nrErrors++;
            // m_cam.setStatus(HwInterface::StatusType::Basic::Fault,false);
            //_statusReturn = HwInterface::StatusType::Basic::Fault;
            continueAcq = false;
            m_cam.m_wait_flag = true;
        }

        bool bAbort = false;
        Event::Severity abortSeverity = Event::Error;

		double exp_time_ms = m_cam.m_pcoData->traceAcq.sExposure * 1000.0;
		double lat_time_ms = m_cam.m_pcoData->traceAcq.sDelay * 1000.0;
		double coc_time_ms = m_cam.m_pcoData->traceAcq.msImgCoc;

		__sprintfSExt(buff, sizeof(buff), "*** %s [%s] acq loop [ENTRY]\n"
			"  nrFrames[%d] timeout: now[%d ms] default[%d ms] max[%g min]\n"
			"  expTime[%g ms] latTime[%g ms] cocTime[%g ms]\n",
		fnId, getTimestamp(Iso),
		_nb_frames, timeoutMs, timeoutMs0, timeoutMsMax/60000.0,
		exp_time_ms, lat_time_ms, coc_time_ms);
		DEB_ALWAYS() << buff;
		m_cam.m_log.append(buff);

        while (!m_cam.m_wait_flag && continueAcq &&
               ((_nb_frames == 0) || limaFrameNr < _nb_frames))
        {
            // DEB_ALWAYS() << DEB_VAR4(m_cam.m_wait_flag, continueAcq,
            // _nb_frames, limaFrameNr);

            m_cam.m_pcoData->traceAcq.usTicks[traceAcq_pcoSdk].value +=
                usElapsedTime(usStart);
            usElapsedTimeSet(usStart);

            pcoFrameNr = limaFrameNr + 1;
            limaBuffPtr = m_cam.m_buffer->_getFrameBufferPtr(
                limaFrameNr, nb_allocated_buffers);
            // DEB_ALWAYS() << DEB_VAR4(nb_allocated_buffers, _nb_frames,
            // limaFrameNr, limaBuffPtr);

            m_cam.m_pcoData->traceAcq.usTicks[traceAcq_GetImageEx].value +=
                usElapsedTime(usStart);
            usElapsedTimeSet(usStart);

            m_cam.setStatus(HwInterface::StatusType::Basic::Readout, false);

            m_cam.m_pcoData->traceAcq.usTicks[traceAcq_Lima].value +=
                usElapsedTime(usStart);
            usElapsedTimeSet(usStart);

            usElapsedTimeSet(usStart);



    
			/// \brief Waits until the next image is completely transferred to the given address.	
			/// \param adr Pointer to address where the image gets stored	
			/// \param timeout Sets a timeout in milliseconds. After this the function returns with an error.
			/// \return Error code or 0 in case of success

			//DWORD Wait_For_Next_Image(void* adr,int timeout);
			//DWORD Wait_For_Next_Image(int *nr_of_pic,int timeout);
			// WORD *picbuf[BUFNUM];
			
            //err=grabber->Wait_For_Next_Image(picbuf[buf_nr],timeout);
			//err = grabber->Wait_For_Next_Image(&pcoBuffIdx, 10);
            
            
            timeoutMsTot = 0;
            
            while(true)
            {
                //DEB_ALWAYS() << "---------------- Wait_For_Next_Image [BEFORE]";
				err = grabber->Wait_For_Next_Image(limaBuffPtr, timeoutMs);
                PCO_CHECK_ERROR1(err, "Wait_For_Next_Image");
				//DEB_ALWAYS() << "---------------- Wait_For_Next_Image [AFTER]";
				
				if ( err == PCO_NOERROR)
				{
                    DEB_ALWAYS() << "---------------- Wait_For_Next_Image [NO ERROR]";
					break;
				}
						
				if ((m_cam.getRequestStop(_nrStop)))
				{
                    DEB_ALWAYS() << "---------------- Wait_For_Next_Image [STOP]";
					break;
				}

				if ( err & (PCO_ERROR_TIMEOUT | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK))
				{
                    DEB_ALWAYS() << "---------------- Wait_For_Next_Image [TIMEOUT]";
                    break;
                    // we will NOT retry -> ABORT
					timeoutMsTot += timeoutMs;
					if(timeoutMsTot > timeoutMsMax)
					{
						break;
					}
				}
			} // while(true) - wait loop


			if (err != PCO_NOERROR)
			{
				m_cam.m_pcoData->traceAcq.nrErrors++;
				_msgAbort = "ABORT - Wait_For_Next_Image ";
				DEB_ALWAYS() << _msgAbort << DEB_VAR1(pcoFrameNr);
				continueAcq = false;
				m_cam.m_wait_flag = true;
				bAbort = true;
				break; // while frames
			}





            // DEB_ALWAYS()  << "lima image#  " << DEB_VAR1(limaFrameNr) <<"
            // acquired !";

            {

                m_cam.m_pcoData->traceAcq.usTicks[traceAcq_pcoSdk].value +=
                    usElapsedTime(usStart);
                usElapsedTimeSet(usStart);

                // check PCO ImgNr with the limaFrame
                m_cam.m_checkImgNr->update(limaFrameNr, limaBuffPtr);
                pcoFrameNrTimestamp = m_cam.m_pcoData->traceAcq.checkImgNrPcoTimestamp;
                    
                HwFrameInfoType frame_info;
                frame_info.acq_frame_nb = limaFrameNr;
                continueAcq =
                    m_cam.m_buffer->m_buffer_cb_mgr.newFrameReady(frame_info);

                m_cam.m_pcoData->traceAcq.usTicks[traceAcq_Lima].value +=
                    usElapsedTime(usStart);
                usElapsedTimeSet(usStart);
            }

            m_cam.m_pcoData->traceAcq.usTicks[traceAcq_pcoSdk].value +=
                usElapsedTime(usStart);
            usElapsedTimeSet(usStart);

            if ((limaFrameNr % 100) == 0)
            {
                printf("pcoFrameNr [%d] diff[%d]\r", pcoFrameNr,
                       pcoFrameNrTimestamp - pcoFrameNr);
                // printf("\n");
            }
            ++limaFrameNr;

            if ((m_cam.getRequestStop(_nrStop)))
            {
                _msgAbort = "STOP REQUEST";
                DEB_ALWAYS() << _msgAbort;
                continueAcq = false;
                m_cam.m_wait_flag = true;
                bAbort = true;
                abortSeverity = Event::Warning;
            }

        } // while  nb_frames, continue, wait

        m_cam._stopAcq(false);

        printf("\n");
        
        m_cam.m_pcoData->traceAcq.endXferTimestamp = getTimestamp();
        m_cam.m_pcoData->traceAcq.msXfer = msElapsedTime(tXferStart);

        m_cam.m_pcoData->traceAcq.usTicks[traceAcq_pcoSdk].value +=
            usElapsedTime(usStart);
        usElapsedTimeSet(usStart);
        m_cam._pco_SetRecordingState(0, err);
        PCO_CHECK_ERROR1(err, "SetRecordingState(0)");
        if (err)
        {
            m_cam.m_pcoData->traceAcq.nrErrors++;
            _msgAbort = "ABORT - SetRecordingState(0)";
            DEB_ALWAYS() << _msgAbort;
            bAbort = true;
        }

        err = grabber->Stop_Acquire();
        PCO_CHECK_ERROR1(err, "Stop_Acquire");
        if (err != PCO_NOERROR)
        {
            m_cam.m_pcoData->traceAcq.nrErrors++;
            _msgAbort = "ABORT - Stop_Acquire()";
            DEB_ALWAYS() << _msgAbort;
            bAbort = true;
        }

        err = grabber->Free_Framebuffer();
        PCO_CHECK_ERROR1(err, "Free_Framebuffer");
        if (err != PCO_NOERROR)
        {
            m_cam.m_pcoData->traceAcq.nrErrors++;
            _msgAbort = "ABORT - Free_Framebuffer()";
            DEB_ALWAYS() << _msgAbort;
            bAbort = true;
        }
        m_cam.m_pcoData->traceAcq.usTicks[traceAcq_pcoSdk].value +=
            usElapsedTime(usStart);
        usElapsedTimeSet(usStart);

        m_cam.m_pcoData->msAcqAll = 
            m_cam.m_pcoData->traceAcq.msTotal=
            msElapsedTime(tStart);

        m_cam.m_pcoData->traceAcq.fnTimestampExit = getTimestamp();
        m_cam.m_pcoData->traceAcq.msStartAcqEnd = msElapsedTime(tStart);
        m_cam.m_pcoData->traceAcq.usTicks[traceAcq_execTimeTot].value =
            usElapsedTime(usStartTot);
        m_cam.setStatus(_statusReturn, false);

        aLock.lock();
        m_cam.m_wait_flag = true;
        m_cam.setStarted(false);

        if (bAbort)
        {
            DEB_ALWAYS() << _msgAbort;
            {
                Event *ev = new Event(Hardware, abortSeverity, Event::Camera,
                                      Event::Default, _msgAbort);
                m_cam._getPcoHwEventCtrlObj()->reportEvent(ev);
            }
        }

    } // while quit
#endif
    DEB_ALWAYS() << fnId << "[EXIT]" ;
}
//=================================================================================================
// FOR EDGE HS - END
//=================================================================================================








//=========================================================================================================
//=========================================================================================================

Camera::Camera(const std::string &camPar)
{
    DEF_FNID;
    DEB_CONSTRUCTOR();

    char buff[MSG4K + 1];

    
    m_log.clear();
    
    __sprintfSExt(buff, sizeof(buff), "*** %s (linux) [%s] [ENTRY]\n",fnId, getTimestamp(Iso));
 	DEB_ALWAYS() << buff;
	m_log.append(buff);

    m_pco_buffer_nrevents = 0;
    
    m_cam_connected = false;
    m_acq_frame_nb = 1;
    m_sync = NULL;
    m_buffer = NULL;
    m_handle = 0;

    camera = NULL;
    grabber_me4 = NULL;

    m_quit = false;
    m_wait_flag = true;

    // delay_time = exp_time = 0;
    // int error=0;

    m_config = true;
    DebParams::checkInit();



    setStatus(HwInterface::StatusType::Basic::Config, true);

    m_msgLog = new ringLog(300);
    m_tmpLog = new ringLog(300);
    if (m_msgLog == NULL)
        throw LIMA_HW_EXC(Error, "m_msgLog > creation error");
    if (m_tmpLog == NULL)
        throw LIMA_HW_EXC(Error, "m_tmpLog > creation error");

    m_pcoData = new stcPcoData();
    if (m_pcoData == NULL)
        throw LIMA_HW_EXC(Error, "m_pcoData > creation error");

    m_pcoData->traceAcqClean();

    m_checkImgNr = new CheckImgNr(this);

    // properties: params
    init_properties(camPar.c_str());

    char *value;
    const char *key;
    UNUSED bool ret;

    mybla = new char[LEN_BLA + 1];
    myblamax = mybla + LEN_BLA;

    mytalk = new char[LEN_TALK + 1];
    mytalkmax = mytalk + LEN_TALK;

    /***
    key = "test";
    key = "withConfig";
    key = "testMode";
    key = "debugPco";
    ***/
    key = "extValue";
    ret = getProperty(key, value);

    // ======================== params / logFile & logBits
    // logBits=FFFF
    // logPath=/tmp

    // #define ERROR_M     0x0001
    // #define INIT_M      0x0002
    // #define BUFFER_M    0x0004
    // #define PROCESS_M   0x0008

    // #define COC_M       0x0010
    // #define INFO_M      0x0020
    // #define COMMAND_M   0x0040

    // #define PCI_M       0x0080

    // #define TIME_M      0x1000
    // #define TIME_MD     0x2000

    // #define NONE_M      0x01000000
    // ======d

    DEB_ALWAYS() << "setting the log";

    char fnLog[PATH_MAX];
    unsigned long debugSdk = 0;
    unsigned long debugSdk_get = 0;
    // unsigned long long debugSdk = 0x0000F0FF;
    key = "logBits";
    ret = getProperty(key, value);
    debugSdk = ret ? strtol(value, NULL, 16) : 0;

    DEB_ALWAYS() << "par: " << DEB_VAR4(key, ret, value, debugSdk) << " "
                 << DEB_HEX(debugSdk);

    key = "logPath";
    ret = getProperty(key, value);

    DEB_ALWAYS() << "par: " << DEB_VAR3(key, ret, value);

    //debugSdk = 0x0000FFFF; value = "/tmp"; ret = 1;


    if (ret && debugSdk)
    {
        snprintf(fnLog, PATH_MAX, "%s/pco_%s.log", value, getTimestamp(FnFull));
        mylog = new CPco_Log(fnLog);
        mylog->set_logbits(debugSdk);
        debugSdk_get = mylog->get_logbits();

        m_pcoData->properties.logBits = debugSdk;
        snprintf(m_pcoData->properties.logPath, sizeof(m_pcoData->properties.logPath),
                 fnLog);


    }
    else
    {
        mylog = new CPco_Log(NULL);
        mylog->set_logbits(0);
        fnLog[0]= 0;
        debugSdk= debugSdk_get=0;

        m_pcoData->properties.logBits = 0;
        m_pcoData->properties.logPath[0] = 0;

        DEB_ALWAYS() << "setLog: NO LOGS " << DEB_VAR2(ret, debugSdk) << " "
                     << DEB_HEX(debugSdk);
    }

	__sprintfSExt(buff, sizeof(buff), "PCO SDK log %s: logbits: set[0x%08x] get[0x%08x] logpath[%s]\n",
		debugSdk ? "ENABLED" : "DISABLED",
		debugSdk, debugSdk_get, fnLog);
	DEB_ALWAYS() << buff;
	m_log.append(buff);

    // ========================

    m_pcoData->testCmdMode = 0;
    // getProperty("testMode", m_pcoData->testCmdMode);
    getProperty("testMode", value);
    m_pcoData->testCmdMode = m_pcoData->properties.testMode =
        strtoull(value, NULL, 0);

    DEB_ALWAYS() << ALWAYS_NL << DEB_VAR1(m_pcoData->version) << ALWAYS_NL
                 << _checkLogFiles();

    // m_bin.changed = Invalid;

    _init();

    DEB_ALWAYS() << "... new _AcqThread (Acq) & start";
    m_acq_threadAcq = new _AcqThread(*this, threadAcq);
    m_acq_threadAcq->start();

    DEB_ALWAYS() << "... new _AcqThread (Switch) & start";
    m_acq_threadSw = new _AcqThread(*this, threadSw);
    m_acq_threadSw->start();

    m_pcoData->timestamps.constructor = getTimestamp();

    m_config = false;
    setStatus(HwInterface::StatusType::Basic::Ready, true);

    __sprintfSExt(buff, sizeof(buff), "*** %s (linux) [%s] [EXIT]\n",fnId, getTimestamp(Iso));
 	DEB_ALWAYS() << buff;
	m_log.append(buff);
    DEB_ALWAYS() << "m_log:\n" << m_log;

}

//=========================================================================================================
//=========================================================================================================
Camera::~Camera()
{
    DEB_DESTRUCTOR();
    DEB_TRACE() << "DESTRUCTOR ....................";

    delete m_acq_threadAcq;
    m_acq_threadAcq = NULL;

    delete m_acq_threadSw;
    m_acq_threadSw = NULL;

    m_cam_connected = false;

    reset(RESET_CLOSE_INTERFACE);
}

//==========================================================================================================
// 2017/05/16
//==========================================================================================================

//==========================================================================================================
//==========================================================================================================
void Camera::_waitForRecording(int nrFrames, DWORD &_dwValidImageCnt,
                               DWORD &_dwMaxImageCnt, int &error)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    static char msgErr[LEN_ERROR_MSG + 1];

    int _nrStop;

    char _msg[LEN_MSG + 1];
    __sprintfSExt(_msg, LEN_MSG, "%s> [ENTRY]", fnId);
    _traceMsg(_msg);

    m_pcoData->traceAcq.fnId = fnId;

    const char *msg;

    TIME_USEC tStart, tStart0;

    msElapsedTimeSet(tStart);
    tStart0 = tStart;
    
    m_pcoData->traceAcq.startRecordTimestamp = getTimestamp();


    long timeout, timeout0, msNowRecordLoop, msXfer, msTotal;
    long msRecord=0;
    int nb_acq_frames;
    int requestStop = stopNone;

    WORD wSegment;
    _pco_GetActiveRamSegment(wSegment, error);
    double msPerFrame = (pcoGetCocRunTime() * 1000.);
    m_pcoData->traceAcq.msImgCoc = msPerFrame;

    DWORD dwMsSleepOneFrame = (DWORD)(msPerFrame / 5.0); // 4/5 rounding
    if (dwMsSleepOneFrame == 0)
        dwMsSleepOneFrame = 1; // min sleep

    bool nb_frames_fixed = false;
    int nb_frames;
    m_sync->getNbFrames(nb_frames);
    m_pcoData->traceAcq.nrImgRequested0 = nb_frames;

    m_sync->setAcqFrames(0);

    timeout = timeout0 = (long)(msPerFrame * (nb_frames * 1.3)); // 30% guard
    if (timeout < TOUT_MIN_DIMAX)
        timeout = TOUT_MIN_DIMAX;

    m_pcoData->traceAcq.msTout = m_pcoData->msAcqTout = timeout;
    _dwValidImageCnt = 0;

    m_sync->getExpTime(m_pcoData->traceAcq.sExposure);
    m_sync->getLatTime(m_pcoData->traceAcq.sDelay);

    setExposing(pcoAcqRecordStart);

    DEB_ALWAYS() << "waiting for the recording ...";
    while (true)
    {
        _pco_GetNumberOfImagesInSegment(wSegment, _dwValidImageCnt,
                                        _dwMaxImageCnt, error);
        PCO_CHECK_ERROR(error, "_pco_GetNumberOfImagesInSegment");
        if (error)
        {
            throw LIMA_HW_EXC(Error, "PCO_GetNumberOfImagesInSegment");
        }

        // DEB_ALWAYS() << DEB_VAR3(wSegment, _dwValidImageCnt, _dwMaxImageCnt);

        m_pcoData->dwValidImageCnt[wSegment - 1] =
            m_pcoData->traceAcq.nrImgRecorded = _dwValidImageCnt;
        m_pcoData->dwMaxImageCnt[wSegment - 1] =
            m_pcoData->traceAcq.maxImgCount = _dwMaxImageCnt;

        m_pcoData->msAcqTnow = msNowRecordLoop = msElapsedTime(tStart);
        m_pcoData->traceAcq.msRecordLoop = msNowRecordLoop;

        if (((DWORD)nb_frames > _dwMaxImageCnt))
        {
            nb_frames_fixed = true;
            DEB_ALWAYS() << "ERROR nb_frames > memSize: "
                         << DEB_VAR2(nb_frames, _dwMaxImageCnt);
            setExposing(pcoAcqError);
            error = -1;
            break;
        }

        if ((_dwValidImageCnt >= (DWORD)nb_frames))
        {
            error = 0;
            break;
        }

        if ((timeout < msNowRecordLoop) && !m_pcoData->bExtTrigEnabled)
        {
            setExposing(pcoAcqStop);
            DEB_ALWAYS() << "ERROR TIMEOUT!!!: "
                         << DEB_VAR5(timeout, timeout0, msNowRecordLoop,
                                     _dwValidImageCnt, nb_frames);
            error = -1;
            break;
        }

        if ((requestStop = getRequestStop(_nrStop)))
        {
            DEB_ALWAYS() << "STOP REQ (recording): "
                         << DEB_VAR2(_dwValidImageCnt, nb_frames);
            break;
        }
        usleep(dwMsSleepOneFrame * 1000); // sleep one frame
    }                                     // while(true)

    m_pcoData->msAcqTnow = msNowRecordLoop = msElapsedTime(tStart);
    m_pcoData->traceAcq.msRecordLoop = msNowRecordLoop;

    _pco_SetRecordingState(0, error);
    PCO_CHECK_ERROR(error, "_pco_SetRecordingState");
    if (error)
    {
        throw LIMA_HW_EXC(Error, "_pco_SetRecordingState");
    }

    //if ((requestStop != stopRequest) && (!nb_frames_fixed))
    if ((!requestStop) && (!nb_frames_fixed))
    {
        if (getExposing() == pcoAcqRecordStart)
            setExposing(pcoAcqRecordEnd);

        _pco_GetNumberOfImagesInSegment(wSegment, _dwValidImageCnt,
                                        _dwMaxImageCnt, error);
        PCO_CHECK_ERROR(error, "_pco_GetNumberOfImagesInSegment");
        if (error)
        {
            throw LIMA_HW_EXC(Error, "PCO_GetNumberOfImagesInSegment");
        }

        m_pcoData->dwValidImageCnt[wSegment - 1] =
            m_pcoData->traceAcq.nrImgRecorded = _dwValidImageCnt;

        nb_acq_frames = (_dwValidImageCnt < (DWORD)nb_frames) ? _dwValidImageCnt
                                                              : nb_frames;

        // dimax recording time
        m_pcoData->msAcqRec = msRecord = msElapsedTime(tStart);
        m_pcoData->traceAcq.msRecord = msRecord; // loop & stop record
        
        m_pcoData->traceAcq.endRecordTimestamp = getTimestamp();

        m_pcoData->traceAcq.nrImgAcquired = nb_acq_frames;
        m_pcoData->traceAcq.nrImgRequested = nb_frames;

        msElapsedTimeSet(tStart); // reset for xfer   TOCHECK

        if (nb_acq_frames < nb_frames)
            m_sync->setNbFrames(nb_acq_frames);

        // --- in case of stop request during the record phase, the transfer
        // --- is made to avoid lose the image recorded
        {
            pcoAcqStatus status;

            if (nb_frames_fixed)
                status = pcoAcqError;

            setExposing(status);
        }

    } // if nb_frames_fixed && no stopRequested

    // setExposing(status);
    m_pcoData->dwMaxImageCnt[wSegment - 1] = m_pcoData->traceAcq.maxImgCount =
        _dwMaxImageCnt;

    // traceAcq info - dimax xfer time ---> not transfer is recording TOCHECK
    m_pcoData->msAcqXfer = msXfer = msElapsedTime(tStart);
    //m_pcoData->traceAcq.msXfer = msXfer;

    m_pcoData->msAcqAll = msTotal = msElapsedTime(tStart0);
    m_pcoData->traceAcq.msTotal = msTotal;

    m_pcoData->traceAcq.endRecordTimestamp = getTimestamp();

    __sprintfSExt(_msg, LEN_MSG,
                  "%s [%d]> [EXIT] imgRecorded[%d] coc[%g] recLoopTime[%ld] "
                  "tout[(%ld) 0(%ld)] rec[%ld] xfer[%ld] all[%ld](ms)\n",
                  fnId, __LINE__, _dwValidImageCnt, msPerFrame, msNowRecordLoop,
                  timeout, timeout0, msRecord, msXfer, msTotal);
    _traceMsg(_msg);

}

//=========================================================================================================
// 2018/03/18
//=========================================================================================================


//=================================================================================================
//=================================================================================================
void Camera::_stopAcq(bool waitForThread)
{
    DEB_MEMBER_FUNCT();

    DEB_ALWAYS() << "[entry]" << DEB_VAR2(waitForThread, m_status);

    m_wait_flag = true;
    return;

    AutoMutex aLock(m_cond_thread.mutex());
    if (m_status != HwInterface::StatusType::Basic::Ready)
    {
        // waitForThread == true / while thread is running
        while (waitForThread && m_thread_running)
        {
            m_wait_flag = true;
            // WaitObject_.Signal();
            m_cond_thread.wait();
        }
        aLock.unlock();

        //  waitForThread == true / thread is not running - Let the acq thread
        //  stop the acquisition
        if (waitForThread)
        {
            DEB_ALWAYS() << "[return]" << DEB_VAR2(waitForThread, m_status);
            return;
        }

        // waitForThread == false / Stop acquisition here
        DEB_TRACE() << "Stop acquisition";
        // Camera_->AcquisitionStop.Execute();

        DEB_ALWAYS() << "[set Ready]" << DEB_VAR1(waitForThread);
        setStatus(HwInterface::StatusType::Basic::Ready, false);
    }
    DEB_ALWAYS() << "[return]" << DEB_VAR2(waitForThread, m_status);
}

//=================================================================================================
//=================================================================================================
void Camera::__startAcq()
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;
    DEB_ALWAYS() << "[ENTRY]";

    m_acq_frame_nb = -1;
    m_pcoData->pcoError = 0;
    m_pcoData->pcoErrorMsg[0] = 0;

    TIME_USEC tStart;
    msElapsedTimeSet(tStart);

    m_sync->getExpTime(m_pcoData->traceAcq.sExposure);
    m_sync->getLatTime(m_pcoData->traceAcq.sDelay);
    m_pcoData->traceAcq.msImgCoc = pcoGetCocRunTime() * 1000.;

    m_pcoData->timestamps.startAcq = getTimestamp();

    //=====================================================================
    // WORD state;
    // HANDLE hEvent= NULL;


    // int error;
    // const char *msg;

    int iRequestedFrames;

    // live video requested frames = 0
    m_sync->getNbFrames(iRequestedFrames);

    //------------------------------------------------- start acquisition

    DEB_ALWAYS() << "[... starting]";
    m_pcoData->traceAcq.msStartAcqStart = msElapsedTime(tStart);

    setStarted(true);
    setExposing(pcoAcqRecordStart);

    setStatus(HwInterface::StatusType::Basic::Exposure, false);

    // Start acqusition thread
    AutoMutex aLock(m_cond_thread.mutex());
    m_wait_flag = false;
    m_cond_thread.broadcast();
    DEB_ALWAYS() << "[... starting after mutex] [EXIT]";

    return;
}

//=================================================================================================
//=================================================================================================
void Camera::dummySip()
{
    unsigned int ui, width, height;
    unsigned long ul;
    double doub;

    int i;
    std::string s;
    if (0)
    {
        Camera("");
        //~Camera();

        startAcq();
        reset(i);

        // talk("");

        getNbAcquiredFrames();

        getMaxWidthHeight(width, height);

        getBytesPerPixel(ui); // bytesPerPixel

        getAcqTimeoutRetry(i); // acqTimeoutRetry
        setAcqTimeoutRetry(i);

        getAdc(i); // adc
        setAdc(i);

        getAdcMax(i); // adcMax

        getClTransferParam(s); // clXferPar

        getLastError(s); // last error

        getTraceAcq(s); // traceAcq

        getLastImgRecorded(ul); // lastImgRecorded

        getLastImgAcquired(ul); // lastImgAcquired

        getMaxNbImages(ul); // maxNbImages

        getCocRunTime(doub); // CocRunTime

        getFrameRate(doub); // frameRate

        getCamInfo(s); // camInfo

        getCamType(s); // camType

        getVersion(s); // version

        getPcoLogsEnabled(i); // pcoLogsEnabled

        getPixelRate(i); // pixelRate
        setPixelRate(i);

        getPixelRateValidValues(s); // pixelRateValidValues

        getPixelRateInfo(s); // pixelRateInfo

        getRollingShutter(i); // rollingShutter
        setRollingShutter(i);

        getRollingShutterInfo(s); // rollingShutterInfo

        getLastFixedRoi(s); // lastFixedRoi

        getCDIMode(i); // cdiMode
        setCDIMode(i);
    }
}

//==========================================================================================================
//==========================================================================================================

///
/// \brief Request the current camera setup
/// \anchor PCO_GetCameraSetup
///
/// This function is used to query the current operation mode of the camera.
/// Some cameras can work at different operation modes with different descriptor
/// settings.\n pco.edge:\n To get the current shutter mode input index setup_id
/// must be set to 0.\n current shutter mode is returned in setup_flag[0]
///  - 0x00000001 = Rolling Shutter
///  - 0x00000002 = Global Shutter
///  - 0x00000004 = Global Reset
///
/// \param setup_id Identification code for selected setup type.
/// \param setup_flag Pointer to a DWORD array to get the current setup flags.
/// If set to NULL in input only the array length is returned.
/// - On input this variable can be set to NULL, then only array length is
/// filled with correct value.
/// - On output the array is filled with the available information for the
/// selected setup_id \param length Pointer to a WORD variable
/// - On input to indicate the length of the Setup_flag array in DWORDs.
/// - On output the length of the setup_flag array in DWORDS
/// \return Error code or PCO_NOERROR on success
///
// DWORD PCO_GetCameraSetup(WORD setup_id, DWORD *setup_flag, WORD *length);

///
/// \brief Sets the camera setup structure (see camera specific structures)
/// \anchor PCO_SetCameraSetup
///
/// pco.edge:\n
/// To get the current shutter mode input index setup_id must be set to 0.\n
/// current shutter mode is returned in setup_flag[0]
///  - 0x00000001 = Rolling Shutter
///  - 0x00000002 = Global Shutter
///  - 0x00000004 = Global Reset
/// When camera is set to a new shuttermode uit must be reinitialized by calling
/// one of the reboot functions. After rebooting, camera description must be
/// read again see \ref  PCO_GetCameraDescription.
///
/// \param setup_id Identification code for selected setup type.
/// \param setup_flag Flags to be set for the selected setup type.
/// \param length Number of valid DWORDs in setup_flag array.
/// \return Error code or PCO_NOERROR on success
///
// DWORD PCO_SetCameraSetup(WORD setup_id, DWORD *setup_flag,WORD length);

///
/// \brief Reboot the camera
///
/// Camera does a reinitialisation. The function will return immediately and the
/// reboot process in the camera is started. When reboot is finished
/// (approximately after 6 to 10 seconds, up to 40 seconds for cameras with GigE
/// interface) one can try to reconnect again with a simple command like \ref
/// PCO_GetCameraType. The reboot command is used during firmware update and is
/// necessary when camera setup is changed.
/// \return Error code or PCO_NOERROR on success
///
// DWORD PCO_RebootCamera();

// an interface specific function must be used, because depending on used
// interface some constraints must be fullfilled for reconnecting
//
// \brief Reboot the camera and wait until camera is reconnected.
// Camera does a reinitialisation. This function tries to reconnect to the
// camera after waiting the reboot wait time. With parameter connect_time the
// maximum wait time can be set.
//
// \param connect_time Time in ms while trying to reconnect the camera
// \return Error code or PCO_NOERROR on success
//
// DWORD PCO_RebootCamera_Wait(DWORD connect_time);

void Camera::_set_shutter_rolling_edge(DWORD dwRolling, int &error)
{
    DEB_MEMBER_FUNCT();
    // DEB_ALWAYS() << NOT_IMPLEMENTED ;

    // Start switch thread
    AutoMutex aLock(m_cond_thread.mutex());
    m_dwRollingShutterNew = dwRolling;
    m_wait_flag_rolling = false;
    m_quit_rolling = false;
    m_cond_thread.broadcast();
    // DEB_ALWAYS() << "[... starting after mutex]";
}

//=================================================================================================
//=================================================================================================

//=================================================================================================
// FOR EDGE - SWITCH
//=================================================================================================
void Camera::_AcqThread::threadFunction_SwitchEdge()
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    const char *_msgAbort = "notSet";
    const char *msg;
    TIME_USEC tStart;
    long long usStart, usStartTot;

    bool bAbort = false;
    bool continueAcq = true;

    // Camera &m_cam = *this;
    m_cam.m_pcoData->traceAcq.fnId = fnId;
    // DEB_ALWAYS() << "[entry]" ;

    int errTot, error;

    DWORD dwErr = PCO_NOERROR;
    int err = 0;
    int count;
    WORD setup_id = 0;
    DWORD setup_flag[4];
    WORD length = sizeof(setup_flag);
    WORD length0 = sizeof(setup_flag);
    char infostr[100];
    DWORD baudrate, def_baudrate;
    DWORD comtime = 2000;
    int cam_num = 0;
    int val;

    // dwRolling is set by Camera::_set_shutter_rolling_edge(DWORD dwRolling,
    // int &error)

    AutoMutex aLock(m_cam.m_cond_thread.mutex());

    DEB_ALWAYS() << DEB_VAR2(m_cam.m_wait_flag_rolling, m_cam.m_quit_rolling);

    while (!m_cam.m_quit_rolling)
    {
        m_cam.m_thread_running_rolling = false;
        m_cam.m_cond_thread.broadcast();
        // aLock.unlock();

        while (m_cam.m_wait_flag_rolling && !m_cam.m_quit_rolling)
        {
            m_cam.m_cond_thread.wait();
        } // while wait

        DEB_ALWAYS() << "++++++++++++++++++++++++++++++++++ Running Thread "
                     << getTimestamp(Iso);
        m_cam.m_thread_running_rolling = true;
        m_cam.m_cond_thread.broadcast();
        m_cam.m_config = true;
        m_cam.m_status = HwInterface::StatusType::Basic::Config;
        // aLock.unlock();

        if (m_cam.m_quit_rolling)
            return;

            // m_cam.setStatus(HwInterface::StatusType::Basic::Config,true);

#if 1

        // int switch_camera(CPco_com_cl_me4* cam,int mode,int cam_num)

        msg = "PCO_GetTransferParameter ";
        m_cam.mylog->writelog(INFO_M, "++++++ INFO: %s", msg);
        dwErr =
            m_cam.camera->PCO_GetTransferParameter(&baudrate, sizeof(baudrate));
        PCO_CHECK_ERROR_CAM(dwErr, msg);
        if (dwErr != PCO_NOERROR)
            goto continueWhile;
        DEB_ALWAYS() << msg << DEB_VAR1(baudrate);

        msg = "PCO_SetTransferParameter ";
        m_cam.mylog->writelog(INFO_M, "++++++ INFO: %s", msg);
        def_baudrate = 9600;
        dwErr = m_cam.camera->PCO_SetTransferParameter(&def_baudrate,
                                                       sizeof(def_baudrate));
        PCO_CHECK_ERROR_CAM(dwErr, msg);
        if (dwErr != PCO_NOERROR)
            goto continueWhile;
        DEB_ALWAYS() << msg << DEB_VAR1(def_baudrate);

        msg = "PCO_SetRecordingState(0) ";
        m_cam.mylog->writelog(INFO_M, "++++++ INFO: %s", msg);
        DEB_ALWAYS() << msg;
        dwErr = m_cam.camera->PCO_SetRecordingState(0);
        PCO_CHECK_ERROR_CAM(dwErr, msg);
        if (dwErr != PCO_NOERROR)
            goto continueWhile;

        msg = "Set_Timeouts ";
        m_cam.mylog->writelog(INFO_M, "++++++ INFO: %s", msg);
        DEB_ALWAYS() << msg << DEB_VAR1(comtime);
        m_cam.camera->Set_Timeouts(&comtime, sizeof(comtime)); // void

        msg = "PCO_GetCameraSetup DUMMY CALL ";
        m_cam.mylog->writelog(INFO_M, "++++++ INFO: %s", msg);
        setup_id = 0;
        length0 = 1;
        dwErr = m_cam.camera->PCO_GetCameraSetup(setup_id, NULL, &length0);
        PCO_CHECK_ERROR_CAM(dwErr, msg);
        if (dwErr != PCO_NOERROR)
            goto continueWhile;
        DEB_ALWAYS() << msg << DEB_VAR1(length0);

        msg = "PCO_GetCameraSetup ";
        m_cam.mylog->writelog(INFO_M, "++++++ INFO: %s", msg);
        setup_id = 0;
        dwErr = m_cam.camera->PCO_GetCameraSetup(setup_id, setup_flag, &length);
        PCO_CHECK_ERROR_CAM(dwErr, msg);
        if (dwErr != PCO_NOERROR)
            goto continueWhile;
        DEB_ALWAYS() << msg << DEB_VAR1(setup_flag[0]);

        switch (m_cam.m_dwRollingShutterNew)
        {
            case 2:
                DEB_ALWAYS() << "set to GLOBAL_SHUTTER";
                setup_flag[0] = PCO_EDGE_SETUP_GLOBAL_SHUTTER;
                break;
            case 4:
                DEB_ALWAYS() << "set to GLOBAL_RESET";
                setup_flag[0] = PCO_EDGE_SETUP_GLOBAL_RESET;
                break;
            case 1:
                DEB_ALWAYS() << "set to ROLLING_SHUTTER";
                setup_flag[0] = PCO_EDGE_SETUP_ROLLING_SHUTTER;
                break;
            default:
                DEB_ALWAYS() << "set to ROLLING_SHUTTER (forced invalid value) "
                             << DEB_VAR1(m_cam.m_dwRollingShutterNew);
                setup_flag[0] = PCO_EDGE_SETUP_ROLLING_SHUTTER;
                break;
        }

        setup_flag[1] = 0;
        setup_flag[2] = 0;
        setup_flag[3] = 0;

        msg = "PCO_SetCameraSetup ";
        m_cam.mylog->writelog(INFO_M, "++++++ INFO: %s", msg);
        val = setup_flag[0];
        DEB_ALWAYS() << msg << DEB_VAR1(val);
        dwErr = m_cam.camera->PCO_SetCameraSetup(setup_id, setup_flag, length);
        PCO_CHECK_ERROR_CAM(dwErr, msg);
        if (dwErr != PCO_NOERROR)
            goto continueWhile;

        msg = "PCO_RebootCamera ";
        m_cam.mylog->writelog(INFO_M, "++++++ INFO: %s", msg);
        DEB_ALWAYS() << msg;
        dwErr = m_cam.camera->PCO_RebootCamera();
        PCO_CHECK_ERROR_CAM(dwErr, msg);
        if (dwErr != PCO_NOERROR)
            goto continueWhile;

        msg = "Close_Cam ";
        m_cam.mylog->writelog(INFO_M, "++++++ INFO: %s", msg);
        DEB_ALWAYS() << "Close camera and wait until it is reconnected";
        dwErr = m_cam.camera->Close_Cam();
        PCO_CHECK_ERROR_CAM(dwErr, msg);
        if (dwErr != PCO_NOERROR)
            goto continueWhile;

        msg = "Camera rebooting and wait 8s ";
        m_cam.mylog->writelog(INFO_M, "++++++ INFO: %s", msg);
        DEB_ALWAYS() << msg;

        fflush(stdout);
        for (count = (8 * WAITMS_1S) / WAITMS_100MS; count > 0; count--)
        {
            m_cam.Sleep_ms(WAITMS_100MS);
            if ((count % 4) == 0)
            {
                printf(".");
                fflush(stdout);
            }
        }

        printf("\n");

        msg = "Camera reconnect / Open_Cam /  wait up to 6s ";
        m_cam.mylog->writelog(INFO_M, "++++++ INFO: %s", msg);
        DEB_ALWAYS() << msg;

        count = (6 * WAITMS_1S) / WAITMS_100MS;
        do
        {
            dwErr = m_cam.camera->Open_Cam(cam_num);
            m_cam.Sleep_ms(WAITMS_100MS);
            if ((count % 2) == 0)
            {
                printf(".");
                fflush(stdout);
            }
        }

        while ((dwErr != PCO_NOERROR) && (count-- > 0));

        printf("\n");
        msg = "Open_Cam ";
        m_cam.mylog->writelog(INFO_M, "++++++ INFO: %s", msg);
        PCO_CHECK_ERROR_CAM(dwErr, msg);
        if (dwErr != PCO_NOERROR)
            goto continueWhile;

        msg = "Camera opened OK ";
        m_cam.mylog->writelog(INFO_M, "++++++ INFO: %s", msg);
        DEB_ALWAYS() << msg;

        msg = "PCO_SetTransferParameter (original value) ";
        m_cam.mylog->writelog(INFO_M, "++++++ INFO: %s", msg);
        DEB_ALWAYS() << msg << DEB_VAR1(baudrate);
        dwErr =
            m_cam.camera->PCO_SetTransferParameter(&baudrate, sizeof(baudrate));
        PCO_CHECK_ERROR_CAM(dwErr, msg);
        if (dwErr != PCO_NOERROR)
            goto continueWhile;

        msg = "PCO_GetInfo ";
        m_cam.mylog->writelog(INFO_M, "++++++ INFO: %s", msg);
        dwErr = m_cam.camera->PCO_GetInfo(1, infostr, sizeof(infostr));
        PCO_CHECK_ERROR_CAM(dwErr, msg);
        if (dwErr != PCO_NOERROR)
            goto continueWhile;
        dwErr = m_cam.camera->PCO_GetInfo(1, infostr, sizeof(infostr));
        DEB_ALWAYS() << msg << "[" << infostr << "]";

        msg = "PCO_GetCameraSetup ";
        m_cam.mylog->writelog(INFO_M, "++++++ INFO: %s", msg);
        setup_id = 0;
        dwErr = m_cam.camera->PCO_GetCameraSetup(setup_id, setup_flag, &length);
        PCO_CHECK_ERROR_CAM(dwErr, msg);
        if (dwErr != PCO_NOERROR)
            goto continueWhile;
        DEB_ALWAYS() << msg << DEB_VAR1(setup_flag[0]);

        msg = "Camera reconnected OK ";
        m_cam.mylog->writelog(INFO_M, "++++++ INFO: %s", msg);
        DEB_ALWAYS() << msg;

        goto continueWhile;

        ;

#endif

        //----------------------------------------------

    continueWhile:
        m_cam.m_wait_flag_rolling = true;
        m_cam.m_config = false;
        // m_cam.setStatus(HwInterface::StatusType::Basic::Ready,true);
        m_cam.m_status = HwInterface::StatusType::Basic::Ready;
        m_cam.m_cond_thread.broadcast();

        DEB_ALWAYS() << "++++++++++++++++++++++++++++++++++ Finished Thread "
                     << getTimestamp(Iso);
        continue;

    } // while quit
}
