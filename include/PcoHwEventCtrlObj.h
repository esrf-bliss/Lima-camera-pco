//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2024
// European Synchrotron Radiation Facility
// BP 220, Grenoble 38043
// FRANCE
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//###########################################################################

#ifndef PCOHWEVENTCTRLOBJ_H
#define PCOHWEVENTCTRLOBJ_H

#include "Pco.h"
#include "lima/HwEventCtrlObj.h"
#include "lima/Debug.h"

namespace lima
{
    namespace Pco
    {
        class Camera;
        class DLL_EXPORT PcoHwEventCtrlObj : public HwEventCtrlObj
        {
            DEB_CLASS_NAMESPC(DebModCamera, "PcoHwEventCtrlObj", "Pco");

          public:
            PcoHwEventCtrlObj();
            // PcoHwEventCtrlObj(Camera*);
            // virtual ~PcoHwEventCtrlObj();

            // virtual void   registerEventCallback(EventCallback& cb);
            // virtual void unregisterEventCallback(EventCallback& cb);
            // virtual bool hasRegisteredCallback();

            // virtual void reportEvent(Event *event);

          private:
            // Camera* 			m_cam;
            // HANDLE& 		m_handle;
        };

    } // namespace Pco
} // namespace lima

#endif // PCOHWEVENTCTRLOBJ_H
