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

//=========================================================================================================
//=========================================================================================================

void _pco_acq_thread_dimax(void *argin);
void _pco_acq_thread_dimax_trig_single(void *argin);

void _pco_acq_thread_dimax_live(void *argin);
void _pco_acq_thread_ringBuffer(void *argin);

void _pco_acq_thread_edge(void *argin);
void _pco_shutter_thread_edge(void *argin);

//=========================================================================================================
//=========================================================================================================

Camera::Camera(const std::string &camPar)
    : m_cam_connected(false), m_acq_frame_nb(1), m_sync(NULL), m_buffer(NULL),
      m_handle(NULL), m_Roi_lastFixed_time(0),
      m_pco_buffer_nrevents(PCO_BUFFER_NREVENTS), m_iPcoAcqMode(0),
      m_bRecorderForcedFifo(false)
{
    DEF_FNID;
    DEB_CONSTRUCTOR();

    int error = 0;
    m_config = true;
    DebParams::checkInit();

    m_msgLog = new ringLog(300);
    m_tmpLog = new ringLog(300);
    if (m_msgLog == NULL)
        throw LIMA_HW_EXC(Error, "m_msgLog > creation error");
    if (m_tmpLog == NULL)
        throw LIMA_HW_EXC(Error, "m_tmpLog > creation error");

    m_pcoData = new stcPcoData();
    if (m_pcoData == NULL)
        throw LIMA_HW_EXC(Error, "m_pcoData > creation error");

    m_checkImgNr = new CheckImgNr(this);

    // properties: params
    paramsInit(camPar.c_str());

    char *value;
    const char *key;
    bool ret;
    int iValue;

    /***
    key = "test";
    key = "withConfig";
    key = "testMode";
    key = "debugPco";
    ***/
    key = "testMode";
    ret = paramsGet(key, value);
    if (ret)
    {
        m_pcoData->testCmdMode = _atoi64(value);
    }

    key = "acqTimeoutRetry";
    ret = paramsGet(key, value);
    iValue = ret ? atoi(value) : 3;
    m_pcoData->acqTimeoutRetry = (iValue < 0) ? 0 : iValue;

    // patch in xMinSize meanwhile firmware for CLHS 1.19 is fixed
    key = "xMinSize";
    m_pcoData->params_xMinSize = !!paramsGet(key, value);

    // ignore the nbMaxImages calculated for dimax HS
    key = "ignoreMaxImages";
    m_pcoData->params_ignoreMaxImages = !!paramsGet(key, value);

    DEB_ALWAYS() << ALWAYS_NL << DEB_VAR1(m_pcoData->version) << ALWAYS_NL
                 << _checkLogFiles(true);

    // m_bin.changed = Invalid;

    _init();
    m_config = false;
    _setActionTimestamp(tsConstructor);
}

//=========================================================================================================
//=========================================================================================================
Camera::~Camera()
{
    DEB_DESTRUCTOR();
    DEB_TRACE() << "DESTRUCTOR ....................";

    m_cam_connected = false;

    reset(RESET_CLOSE_INTERFACE);
}

//==========================================================================================================
//==========================================================================================================
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
    WORD dwPcoRequestedFrames =
        wDoubleImage ? iLimaRequestedFrames / 2 : iLimaRequestedFrames;

    m_sync->setAcqFrames(0);

    timeout = timeout0 =
        (long)(msPerFrame * (iLimaRequestedFrames * 1.3)); // 30% guard
    if (timeout < TOUT_MIN_DIMAX)
        timeout = TOUT_MIN_DIMAX;

    m_pcoData->traceAcq.msTout = m_pcoData->msAcqTout = timeout;
    _dwPcoValidImageCnt = 0;

    m_sync->getExpTime(m_pcoData->traceAcq.sExposure);
    m_sync->getLatTime(m_pcoData->traceAcq.sDelay);

    setExposing(pcoAcqRecordStart);

    while (true)
    {
        m_cam->_pco_GetNumberOfImagesInSegment(wSegment, _dwPcoValidImageCnt,
                                               _dwPcoMaxImageCnt, error);
        if (error)
        {
            printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__,
                   "_pco_GetNumberOfImagesInSegment");
            throw LIMA_HW_EXC(Error, "PCO_GetNumberOfImagesInSegment");
        }

        m_pcoData->dwValidImageCnt[wSegment - 1] =
            m_pcoData->traceAcq.nrImgRecorded = _dwPcoValidImageCnt;
        m_pcoData->dwMaxImageCnt[wSegment - 1] =
            m_pcoData->traceAcq.maxImgCount = _dwPcoMaxImageCnt;

        m_pcoData->msAcqTnow = msNowRecordLoop = msElapsedTime(tStart);
        m_pcoData->traceAcq.msRecordLoop = msNowRecordLoop;

        if ((dwPcoRequestedFrames > _dwPcoMaxImageCnt))
        {
            nb_frames_fixed = true;

            __sprintfSExt(msgErr, LEN_ERROR_MSG,
                          "=== %s [%d]> ERROR INVALID NR FRAMES fixed "
                          "nb_frames[%d] _dwPcoMaxImageCnt[%d]",
                          fnId, __LINE__, dwPcoRequestedFrames,
                          _dwPcoMaxImageCnt);
            printf("%s\n", msgErr);

            setExposing(pcoAcqError);
            break;
        }

        if ((_dwPcoValidImageCnt >= dwPcoRequestedFrames))
            break;

        if ((timeout < msNowRecordLoop) && !m_pcoData->bExtTrigEnabled)
        {
            // setExposing(pcoAcqRecordTimeout);
            // stopAcq();
            setExposing(pcoAcqStop);
            printf("=== %s [%d]> TIMEOUT!!! tout[(%ld) 0(%ld)] recLoopTime[%ld "
                   "ms] lastImgRecorded[%ld] dwPcoRequestedFrames[%d]\n",
                   fnId, __LINE__, timeout, timeout0, msNowRecordLoop,
                   _dwPcoValidImageCnt, dwPcoRequestedFrames);
            break;
        }

        //if ((requestStop = m_sync->getRequestStop(_nrStop)) == stopRequest)
        if ((requestStop = m_cam->getRequestStop(_nrStop)))
        {
            m_cam->setRequestStop(stopNone);

            char msg[LEN_TRACEACQ_MSG + 1];

            snprintf(msg, LEN_TRACEACQ_MSG,
                     "=== %s> STOP REQ (recording). lastImgRec[%d]\n", fnId,
                     _dwPcoValidImageCnt);
            printf(msg);
            m_pcoData->traceMsg(msg);
            break;
        }
        Sleep(dwMsSleepOneFrame); // sleep 1 frame
    }                             // while(true)

    m_pcoData->msAcqTnow = msNowRecordLoop = msElapsedTime(tStart);
    m_pcoData->traceAcq.msRecordLoop = msNowRecordLoop;

    msg = m_cam->_pco_SetRecordingState(0, error);
    if (error)
    {
        printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__, msg);
        throw LIMA_HW_EXC(Error, "_pco_SetRecordingState");
    }

    //if ((requestStop != stopRequest) && (!nb_frames_fixed))
    if ((!requestStop) && (!nb_frames_fixed))
    {
        if (getExposing() == pcoAcqRecordStart)
            setExposing(pcoAcqRecordEnd);

        m_cam->_pco_GetNumberOfImagesInSegment(wSegment, _dwPcoValidImageCnt,
                                               _dwPcoMaxImageCnt, error);
        if (error)
        {
            printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__, msg);
            throw LIMA_HW_EXC(Error, "PCO_GetNumberOfImagesInSegment");
        }

        m_pcoData->dwValidImageCnt[wSegment - 1] =
            m_pcoData->traceAcq.nrImgRecorded = _dwPcoValidImageCnt;

        dwPcoAcquiredFrames = (_dwPcoValidImageCnt < dwPcoRequestedFrames)
                                  ? _dwPcoValidImageCnt
                                  : dwPcoRequestedFrames;
        iLimaAcquiredFrames =
            wDoubleImage ? dwPcoAcquiredFrames * 2 : dwPcoAcquiredFrames;
        // m_sync->setAcqFrames(nb_acq_frames);

        // dimax recording time
        m_pcoData->msAcqRec = msRecord = msElapsedTime(tStart);
        m_pcoData->traceAcq.msRecord = msRecord; // loop & stop record

        m_pcoData->traceAcq.endRecordTimestamp = m_pcoData->msAcqRecTimestamp =
            getTimestamp();

        m_pcoData->traceAcq.nrImgAcquired = iLimaAcquiredFrames;
        m_pcoData->traceAcq.nrImgRequested = iLimaRequestedFrames;

        msElapsedTimeSet(tStart); // reset for xfer

        if (iLimaAcquiredFrames < iLimaRequestedFrames)
            m_sync->setNbFrames(iLimaAcquiredFrames);


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
                        status =
                            (pcoAcqStatus)m_buffer
                                ->_xferImagMultDoubleImage(); //  <-------------
                                                              //  default
                                                              //  pco2k/4k NO
                                                              //  waitobj
                    }
                    else
                    {
                        status =
                            (pcoAcqStatus)
                                m_buffer->_xferImagMult(); //  <-------------
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
                        status =
                            (pcoAcqStatus)m_buffer
                                ->_xferImagDoubleImage(); //  <-------------
                                                          //  default dimax YES
                                                          //  waitobj
                    }
                    else
                    {
                        status = (pcoAcqStatus)
                                     m_buffer->_xferImag(); //  <-------------
                                                            //  default dimax
                                                            //  YES waitobj
                    }
                }
            }

            if (nb_frames_fixed)
                status = pcoAcqError;
            setExposing(status);
        }

    } // if nb_frames_fixed && no stopRequested

    // setExposing(status);
    m_pcoData->dwMaxImageCnt[wSegment - 1] = m_pcoData->traceAcq.maxImgCount =
        _dwPcoMaxImageCnt;

    // traceAcq info - dimax xfer time
    m_pcoData->msAcqXfer = msXfer = msElapsedTime(tStart);
    m_pcoData->traceAcq.msXfer = msXfer;

    m_pcoData->msAcqAll = msTotal = msElapsedTime(tStart0);
    m_pcoData->traceAcq.msTotal = msTotal;

    m_pcoData->traceAcq.endXferTimestamp = m_pcoData->msAcqXferTimestamp =
        getTimestamp();

    __sprintfSExt(_msg, LEN_MSG,
                  "%s [%d]> [EXIT] imgRecorded[%d] coc[%g] recLoopTime[%ld] "
                  "tout[(%ld) 0(%ld)] rec[%ld] xfer[%ld] all[%ld](ms)\n",
                  fnId, __LINE__, _dwPcoValidImageCnt, msPerFrame,
                  msNowRecordLoop, timeout, timeout0, msRecord, msXfer,
                  msTotal);
    m_cam->_traceMsg(_msg);

    // included in 34a8fb6723594919f08cf66759fe5dbd6dc4287e only for dimax (to
    // check for others)
    m_cam->setStarted(false);


    _endthread();
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

    setExposing(pcoAcqRecordStart);

    m_cam->_pco_GetNumberOfImagesInSegment(wSegment, _dwValidImageCnt,
                                           _dwMaxImageCnt, error);
    if (error)
    {
        printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__,
               "_pco_GetNumberOfImagesInSegment");
        throw LIMA_HW_EXC(Error, "PCO_GetNumberOfImagesInSegment");
    }
    m_pcoData->dwValidImageCnt[wSegment - 1] =
        m_pcoData->traceAcq.nrImgRecorded = _dwValidImageCnt;
    m_pcoData->dwMaxImageCnt[wSegment - 1] = m_pcoData->traceAcq.maxImgCount =
        _dwMaxImageCnt;

    bool doWhile = true;

    if (((DWORD)nb_frames > _dwMaxImageCnt))
    {
        nb_frames_fixed = true;

        __sprintfSExt(msgErr, LEN_ERROR_MSG,
                      "=== %s [%d]> ERROR INVALID NR FRAMES fixed "
                      "nb_frames[%d] _dwMaxImageCnt[%d]",
                      fnId, __LINE__, nb_frames, _dwMaxImageCnt);
        printf("%s\n", msgErr);

        setExposing(pcoAcqError);
        doWhile = false;
    }

    while (doWhile)
    {
        WORD wRecState_actual;

        m_pcoData->msAcqTnow = msNowRecordLoop = msElapsedTime(tStart);
        m_pcoData->traceAcq.msRecordLoop = msNowRecordLoop;

        wRecState_actual = m_cam->_pco_GetRecordingState(error);
        if (error)
        {
            printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__,
                   "_pco_GetRecordingState");
            throw LIMA_HW_EXC(Error, "PCO_GetRecordingState");
        }

        if (wRecState_actual == 0)
            break;

        //if ((requestStop = m_sync->getRequestStop(_nrStop)) == stopRequest)
        if ((requestStop = m_cam->getRequestStop(_nrStop)))
        {
            m_cam->setRequestStop(stopNone);

            char msg[LEN_TRACEACQ_MSG + 1];

            snprintf(msg, LEN_TRACEACQ_MSG,
                     "=== %s> STOP REQ (recording). lastImgRec[%d]\n", fnId,
                     _dwValidImageCnt);
            printf(msg);
            m_pcoData->traceMsg(msg);
            break;
        }
        Sleep(dwMsSleepOneFrame); // sleep 1 frame
    }                             // while(true)

    m_pcoData->msAcqTnow = msNowRecordLoop = msElapsedTime(tStart);
    m_pcoData->traceAcq.msRecordLoop = msNowRecordLoop;

    msg = m_cam->_pco_SetRecordingState(0, error);
    if (error)
    {
        printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__, msg);
        throw LIMA_HW_EXC(Error, "_pco_SetRecordingState");
    }

    //if ((requestStop != stopRequest) && (!nb_frames_fixed))
    if ((!requestStop) && (!nb_frames_fixed))
    {
        if (getExposing() == pcoAcqRecordStart)
            setExposing(pcoAcqRecordEnd);

        m_cam->_pco_GetNumberOfImagesInSegment(wSegment, _dwValidImageCnt,
                                               _dwMaxImageCnt, error);
        if (error)
        {
            printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__, msg);
            throw LIMA_HW_EXC(Error, "PCO_GetNumberOfImagesInSegment");
        }

        m_pcoData->dwValidImageCnt[wSegment - 1] =
            m_pcoData->traceAcq.nrImgRecorded = _dwValidImageCnt;

        nb_acq_frames = (_dwValidImageCnt < (DWORD)nb_frames) ? _dwValidImageCnt
                                                              : nb_frames;
        // m_sync->setAcqFrames(nb_acq_frames);

        // dimax recording time
        m_pcoData->msAcqRec = msRecord = msElapsedTime(tStart);
        m_pcoData->traceAcq.msRecord = msRecord; // loop & stop record

        m_pcoData->traceAcq.endRecordTimestamp = m_pcoData->msAcqRecTimestamp =
            getTimestamp();

        m_pcoData->traceAcq.nrImgAcquired = nb_acq_frames;
        m_pcoData->traceAcq.nrImgRequested = nb_frames;

        msElapsedTimeSet(tStart); // reset for xfer

        if (nb_acq_frames < nb_frames)
            m_sync->setNbFrames(nb_acq_frames);


        // --- in case of stop request during the record phase, the transfer
        // --- is made to avoid lose the image recorded
        {
            pcoAcqStatus status;

            status = (pcoAcqStatus)m_buffer->_xferImag_getImage();

            if (nb_frames_fixed)
                status = pcoAcqError;
            setExposing(status);
        }

    } // if nb_frames_fixed & no stopped

    // setExposing(status);
    m_pcoData->dwMaxImageCnt[wSegment - 1] = m_pcoData->traceAcq.maxImgCount =
        _dwMaxImageCnt;

    // traceAcq info - dimax xfer time
    m_pcoData->msAcqXfer = msXfer = msElapsedTime(tStart);
    m_pcoData->traceAcq.msXfer = msXfer;

    m_pcoData->msAcqAll = msTotal = msElapsedTime(tStart0);
    m_pcoData->traceAcq.msTotal = msTotal;

    m_pcoData->traceAcq.endXferTimestamp = m_pcoData->msAcqXferTimestamp =
        getTimestamp();

    printf("=== %s [%d]> EXIT nb_frames_requested[%d] _dwValidImageCnt[%d] "
           "_dwMaxImageCnt[%d] coc[%g] recLoopTime[%ld] "
           "tout[(%ld) 0(%ld)] rec[%ld] xfer[%ld] all[%ld](ms)\n",
           fnId, __LINE__, nb_frames, _dwValidImageCnt, _dwMaxImageCnt,
           msPerFrame, msNowRecordLoop, timeout, timeout0, msRecord, msXfer,
           msTotal);

    // included in 34a8fb6723594919f08cf66759fe5dbd6dc4287e only for dimax (to
    // check for others)
    m_cam->setStarted(false);


    _endthread();
}

//=====================================================================
//=====================================================================

void _pco_acq_thread_edge(void *argin)
{
    DEF_FNID;

    Camera *m_cam = (Camera *)argin;
    SyncCtrlObj *m_sync = m_cam->_getSyncCtrlObj();
    // BufferCtrlObj* m_buffer = m_sync->_getBufferCtrlObj();
    BufferCtrlObj *m_buffer = m_cam->_getBufferCtrlObj();

    char _msg[LEN_MSG + 1];
    __sprintfSExt(_msg, LEN_MSG, "%s> [ENTRY]", fnId);
    m_cam->_traceMsg(_msg);

    m_cam->_sprintComment(true, fnId, "[ENTRY]");

    struct stcPcoData *m_pcoData = m_cam->_getPcoData();

    TIME_USEC tStart;
    msElapsedTimeSet(tStart);
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

    setExposing(status);
    // stopAcq();

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
    __sprintfSExt(_msg, LEN_MSG, "%s> [EXIT] xfer[%ld] (ms) status[%s]\n", fnId,
                  msXfer, sPcoAcqStatus[status]);
    m_cam->_traceMsg(_msg);

    m_cam->setStarted(false); // updated

    char *errMsg = NULL;
    switch (status)
    {
        case pcoAcqRecordTimeout:
            errMsg = "pcoAcqRecordTimeout";
            break;
        case pcoAcqWaitTimeout:
            errMsg = "pcoAcqWaitTimeout";
            break;
        case pcoAcqWaitError:
            errMsg = "pcoAcqWaitError";
            break;
        case pcoAcqError:
            errMsg = "pcoAcqError";
            break;
        case pcoAcqPcoError:
            errMsg = "pcoAcqPcoError";
            break;
        default:
            errMsg = NULL;
    }

    if (errMsg != NULL)
    {
        Event *ev = new Event(Hardware, Event::Error, Event::Camera,
                              Event::CamFault, errMsg);
        //m_cam->_getPcoHwEventCtrlObj()->reportEvent(ev);
    }

    printf("=== %s [%d]> TRACE %s\n", fnId, __LINE__, "exit thread");

    _endthread();
}

//=====================================================================
//=====================================================================

void _pco_acq_thread_dimax_live(void *argin)
{
    DEF_FNID;

    Camera *m_cam = (Camera *)argin;
    SyncCtrlObj *m_sync = m_cam->_getSyncCtrlObj();
    // BufferCtrlObj* m_buffer = m_sync->_getBufferCtrlObj();
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

    //status = (pcoAcqStatus)m_buffer->_xferImag();
    
    if ( !(m_pcoData->testCmdMode & TESTCMDMODE_DIMAX_XFERMULTI))
    {
        // _xferImagMult [PCO_GetImageEx]
        status = (pcoAcqStatus)m_buffer->_xferImagMult();
    }
    else
    {
        // TODO wDoubleImage
        
        // _xferImag [WaitForMultipleObjects]
        status = (pcoAcqStatus) m_buffer->_xferImag();
    }
    
    setExposing(status);
    // stopAcq();
    const char *msg = m_cam->_pco_SetRecordingState(0, error);
    if (error)
    {
        printf("=== %s [%d]> ERROR %s\n", fnId, __LINE__, msg);
        // throw LIMA_HW_EXC(Error, "_pco_SetRecordingState");
    }

    // dimax xfer time
    m_pcoData->msAcqXfer = msXfer = msElapsedTime(tStart);
    m_pcoData->msAcqXferTimestamp = getTimestamp();
    __sprintfSExt(_msg, LEN_MSG, "%s> [EXIT] xfer[%ld] (ms) status[%s]\n", fnId,
                  msXfer, sPcoAcqStatus[status]);
    m_cam->_traceMsg(_msg);

    m_cam->setStarted(false); // to test

    _endthread();
}

//=====================================================================
//=====================================================================
void _pco_acq_thread_ringBuffer(void *argin)
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

    TIME_USEC tStart;
    msElapsedTimeSet(tStart);

    long long usStart;
    usElapsedTimeSet(usStart);

    int error;
    long msXfer;
    int requestStop = stopNone;
    pcoAcqStatus status;

    HANDLE m_handle = m_cam->getHandle();

    m_sync->setAcqFrames(0);

    // traceAcq
    m_pcoData->traceAcq.fnId = fnId;
    double msPerFrame = (m_cam->pcoGetCocRunTime() * 1000.);
    m_pcoData->traceAcq.msImgCoc = msPerFrame;
    m_sync->getExpTime(m_pcoData->traceAcq.sExposure);
    m_sync->getLatTime(m_pcoData->traceAcq.sDelay);

    m_pcoData->msAcqRec = 0;
    m_pcoData->msAcqRecTimestamp = getTimestamp();

    m_pcoData->traceAcq.usTicks[0].value = usElapsedTime(usStart);
    m_pcoData->traceAcq.usTicks[0].desc = "before xferImag execTime";

    usElapsedTimeSet(usStart);

    if (m_pcoData->testCmdMode & TESTCMDMODE_PCO2K_XFER_WAITOBJ)
    {
        status =
            (pcoAcqStatus)m_buffer->_xferImag(); //  <------------- uses WAITOBJ
    }
    else
    {
        status = (pcoAcqStatus)m_buffer->_xferImagMult(); //  <-------------
                                                          //  USES
                                                          //  PCO_GetImageEx (NO
                                                          //  waitobj)   0x20
    }

    m_pcoData->traceAcq.usTicks[1].value = usElapsedTime(usStart);
    m_pcoData->traceAcq.usTicks[1].desc = "xferImag execTime";
    usElapsedTimeSet(usStart);

    setExposing(status);

    m_pcoData->traceAcq.usTicks[2].value = usElapsedTime(usStart);
    m_pcoData->traceAcq.usTicks[2].desc = "setExposing(status) execTime";
    usElapsedTimeSet(usStart);

    // m_sync->stopAcq();

    m_pcoData->traceAcq.usTicks[3].value = usElapsedTime(usStart);
    m_pcoData->traceAcq.usTicks[3].desc = "sync->stopAcq execTime";
    usElapsedTimeSet(usStart);

    const char *msg = m_cam->_pco_SetRecordingState(0, error);
    m_pcoData->traceAcq.usTicks[4].value = usElapsedTime(usStart);
    m_pcoData->traceAcq.usTicks[4].desc = "_pco_SetRecordingState execTime";
    usElapsedTimeSet(usStart);

    if (error)
    {
        __sprintfSExt(_msg, LEN_MSG, "%s> [%d]> ERROR %s", fnId, __LINE__, msg);
        m_cam->_traceMsg(_msg);
        // throw LIMA_HW_EXC(Error, "_pco_SetRecordingState");
    }

    // xfer time
    m_pcoData->msAcqXfer = m_pcoData->traceAcq.msXfer =
        m_pcoData->traceAcq.msTotal = msXfer = msElapsedTime(tStart);

    m_pcoData->traceAcq.endXferTimestamp = m_pcoData->msAcqXferTimestamp =
        getTimestamp();

    __sprintfSExt(_msg, LEN_MSG, "%s> EXIT xfer[%ld] (ms) status[%s]", fnId,
                  msXfer, sPcoAcqStatus[status]);
    m_cam->_traceMsg(_msg);

    m_pcoData->traceAcq.usTicks[5].desc = "up to _endtrhead execTime";
    m_pcoData->traceAcq.usTicks[5].value = usElapsedTime(usStart);

    m_cam->setStarted(false); // to test

    _endthread();
}

//=====================================================================
//=====================================================================

//=====================================================================
//=====================================================================
//=================================================================================================
//=================================================================================================
void Camera::_set_shutter_rolling_edge(DWORD dwRolling, int &error)
{
    DEB_MEMBER_FUNCT();
    error = 0;

    if (!_isValid_rollingShutter(dwRolling))
    {
        DEB_ALWAYS() << "ERROR requested Rolling Shutter not allowed "
                     << DEB_VAR1(dwRolling);
        error = -1;
        return;
    }

    m_pcoData->dwRollingShutter = dwRolling;

    DEB_TRACE() << "requested Rolling Shutter OK " << DEB_VAR1(dwRolling);

    _beginthread(_pco_shutter_thread_edge, 0, (void *)this);

    return;
}
//=====================================================================
//=====================================================================
void _pco_shutter_thread_edge(void *argin)
{
    DEF_FNID;
    int error;

    Camera *m_cam = (Camera *)argin;
    // SyncCtrlObj* m_sync = m_cam->_getSyncCtrlObj();

    char _msg[LEN_MSG + 1];
    __sprintfSExt(_msg, LEN_MSG, "%s> [ENTRY]", fnId);
    m_cam->_traceMsg(_msg);

    m_cam->_sprintComment(true, fnId, "[ENTRY]");

    m_cam->_pco_set_shutter_rolling_edge(error);

    __sprintfSExt(_msg, LEN_MSG, "%s> [EXIT]", fnId);
    m_cam->_traceMsg(_msg);

    // m_cam->setStarted(false); // to test

    _endthread();
} //=================================================================================================
//=================================================================================================

bool Camera::_isRunAfterAssign()
{
    return (_isCameraType(Edge) && (_isInterfaceType(ifCameralinkAll)));
}

//=================================================================================================
//=================================================================================================
void Camera::__startAcq()
{
    DEB_MEMBER_FUNCT();

    m_acq_frame_nb = -1;
    m_pcoData->pcoError = 0;
    m_pcoData->pcoErrorMsg[0] = 0;

    TIME_USEC tStart;
    msElapsedTimeSet(tStart);

    //=====================================================================
    DEF_FNID;
    HANDLE hEvent = NULL;

    DEB_ALWAYS() << _sprintComment(false, fnId, "[ENTRY]") << _checkLogFiles();

    int error;

    //------------------------------------------------- start acquisition

    m_pcoData->traceAcq.msStartAcqStart = msElapsedTime(tStart);

    m_cam->setStarted(true);
    // setExposing(pcoAcqRecordStart);
    m_cam->setExposing(pcoAcqStart);

    int iRequestedFrames;
    m_sync->getNbFrames(iRequestedFrames);

    int forced = 0;
    getRecorderForcedFifo(forced);

    int iPending;
    PCO_GetPendingBuffer(m_handle, &iPending);
    if (iPending < m_pco_buffer_nrevents)
    {
        PCO_CancelImages(m_handle);
        DEB_ALWAYS() << "PCO_CancelImages " << DEB_VAR1(iPending);
    }

    unsigned long ulFramesMaxInSegment =
        _pco_GetNumberOfImagesInSegment_MaxCalc(m_pcoData->wActiveRamSegment);
    unsigned long ulRequestedFrames = (unsigned long)iRequestedFrames;

    if (ulRequestedFrames > 0)
    {
        WORD wDoubleImage;
        int err;

        // Double Image -> requested images will be the total nr of images
        // (basic + primary)
        //      must be even and twice of the nr of images for pco
        _pco_GetDoubleImageMode(wDoubleImage, err);

        bool bOutOfRange = false;

        if ((wDoubleImage) && ((ulRequestedFrames % 2) != 0))
        {
            DEB_ALWAYS() << "\nERROR odd nr of frames in DoubleImage";
            bOutOfRange = true;
        }

        if ((ulFramesMaxInSegment > 0) &&
            (ulRequestedFrames > ulFramesMaxInSegment) && (!forced))
        {
            DEB_ALWAYS() << "\nERROR many frames in record mode";
            bOutOfRange = true;
        }

        if (bOutOfRange)
        {
            char msg[MSG1K];
            __sprintfSExt(
                msg, sizeof(msg) - 1,
                "ERROR frames OUT OF RANGE ulRequestedFrames[%ld], "
                "ulFramesMaxInSegment[%ld], wDoubleImage[%d], forced[%d]",
                ulRequestedFrames, ulFramesMaxInSegment, wDoubleImage, forced);

            DEB_ALWAYS() << msg;
            m_cam->setStarted(false);
            m_cam->setExposing(pcoAcqError);

            throw LIMA_EXC(CameraPlugin, InvalidValue, msg);
            return;
        }
    }

    if (!_isRunAfterAssign())
    {
        DEB_TRACE() << "========================= recordingState 1 - BEFORE "
                       "ASSIGN (startAcq)";
        _pco_SetRecordingState(1, error);
    }

    if (_isCameraType(Edge))
    {
        _beginthread(_pco_acq_thread_edge, 0, (void *)this);


        m_pcoData->traceAcq.msStartAcqEnd = msElapsedTime(tStart);
        return;
    }

#if 0
    if (_isCameraType(Pco2k | Pco4k)) {
        _beginthread(_pco_acq_thread_ringBuffer, 0, (void*)this);
        m_pcoData->traceAcq.msStartAcqEnd = msElapsedTime(tStart);
        return;
    }
#endif

    if (_isCameraType(Dimax | Pco2k | Pco4k))
    {
        int iRequestedFrames;
        m_sync->getNbFrames(iRequestedFrames);

        TrigMode trig_mode;
        m_sync->getTrigMode(trig_mode);
        _pco_SetRecordingState(1, error);

        int forcedFifo = 0;
        getRecorderForcedFifo(forcedFifo);

        if ((iRequestedFrames > 0) && (forcedFifo == 0))
        {
            if ((trig_mode == ExtTrigSingle))
            {
                _beginthread(_pco_acq_thread_dimax_trig_single, 0,
                             (void *)this);
            }
            else
            {
                _beginthread(_pco_acq_thread_dimax, 0,
                             (void *)this); // normal mode
            }
        }
        else
        {
            _beginthread(_pco_acq_thread_dimax_live, 0, (void *)this);
        }
        m_pcoData->traceAcq.msStartAcqEnd = msElapsedTime(tStart);
        return;
    }

    throw LIMA_HW_EXC(Error, "unkown camera type");
    return;
}
