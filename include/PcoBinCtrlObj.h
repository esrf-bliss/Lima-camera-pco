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

#ifndef PCOBINCTRLOBJ_H
#define PCOBINCTRLOBJ_H

//#include "PcoCompatibility.h"
#include "lima/HwBinCtrlObj.h"
#include "PcoCamera.h"
#include "lima/Exceptions.h"

namespace lima
{
    namespace Pco
    {
        /*******************************************************************
         * \class BinCtrlObj
         * \brief Control object providing Pco Bin interface
         *******************************************************************/
        class DLL_EXPORT BinCtrlObj : public HwBinCtrlObj
        {
            DEB_CLASS_NAMESPC(DebModCamera, "BinCtrlObj", "Pco");

          public:
            BinCtrlObj(Camera &cam);
            virtual ~BinCtrlObj()
            {
            }

            virtual void setBin(const Bin &bin);
            virtual void getBin(Bin &bin);
            // allow all binning
            virtual void checkBin(Bin &bin);

          private:
            Camera &m_cam;
        };

    } // namespace Pco
} // namespace lima

#endif // PCOBINCTRLOBJ_H
