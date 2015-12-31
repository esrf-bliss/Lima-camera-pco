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
#ifndef PCOINTERFACE_H
#define PCOINTERFACE_H



#include "lima/LimaCompatibility.h"

#include "lima/Debug.h"
#include "lima/HwInterface.h"
#include "PcoCamera.h"



namespace lima
{
  namespace Pco
  {
    class Camera;
    class DetInfoCtrlObj;
    class BufferCtrlObj;
    class SyncCtrlObj;
    class RoiCtrlObj;
    class PcoHwEventCtrlObj;

    class  DLL_EXPORT Interface : public HwInterface
    {
      DEB_CLASS_NAMESPC(DebModCamera, "Interface", "Pco");

    public:
      Interface(Camera*);
      virtual ~Interface();

      virtual void getCapList(CapList &) const;

      virtual void reset(ResetLevel reset_level);
      virtual void prepareAcq();
      virtual void startAcq();
      virtual void stopAcq();
      virtual void getStatus(StatusType& status);

      virtual int getNbAcquiredFrames();
      virtual int getNbHwAcquiredFrames();

      double getCocRunTime(){ return m_cam->pcoGetCocRunTime() ;};
      double getFrameRate(){ return m_cam->pcoGetFrameRate() ;};

      //! get the camera object to access it directly from client
      Camera* getCamera() { return m_cam;}


    private:
      Camera* 		m_cam;
      DetInfoCtrlObj* 	m_det_info;
      BufferCtrlObj* 	m_buffer;
      SyncCtrlObj* 	m_sync;
      RoiCtrlObj*       m_RoiCtrlObj;
      PcoHwEventCtrlObj*       m_HwEventCtrlObj;
      static RoiCtrlObj*       m_RoiCtrlObjXXX;
    };

  } // namespace Pco

} // namespace lima

#endif // PCOINTERFACE_H
