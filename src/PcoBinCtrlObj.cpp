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

#include "PcoBinCtrlObj.h"

using namespace lima;
using namespace lima::Pco;
using namespace std;

//-----------------------------------------------------
// @brief
//-----------------------------------------------------
BinCtrlObj::BinCtrlObj(Camera &cam) : m_cam(cam)
{
    DEB_CONSTRUCTOR();
}

void BinCtrlObj::setBin(const Bin &aBin)
{
    DEB_MEMBER_FUNCT();
    m_cam.setBin(aBin);

    DEB_RETURN() << "\n... " << DEB_VAR1(aBin);
}

//-----------------------------------------------------
// @brief
//-----------------------------------------------------
void BinCtrlObj::getBin(Bin &aBin)
{
    DEB_MEMBER_FUNCT();
    m_cam.getBin(aBin);
}

//-----------------------------------------------------
// @brief
//-----------------------------------------------------
void BinCtrlObj::checkBin(Bin &aBin)
{
    DEB_MEMBER_FUNCT();
    m_cam.checkBin(aBin);
}

//=================================================================================================
// ----- BIN
//=================================================================================================
void Camera::setBin(const Bin &requestedBin)
{
    DEB_MEMBER_FUNCT();
    int err;

    Bin actualBin;

    _pco_SetBinning(requestedBin, actualBin, err);
    if (err)
    {
        DEB_ALWAYS() << "ERROR - setBin " << DEB_VAR2(requestedBin, actualBin);
    }
}
//=================================================================================================
//=================================================================================================
void Camera::getBin(Bin &aBin)
{
    DEB_MEMBER_FUNCT();
    int err;

    _pco_GetBinning(aBin, err);
    if (err)
    {
        DEB_ALWAYS() << "ERROR - getBin";
    }
}
//=================================================================================================
//=================================================================================================
void Camera::checkBin(Bin &aBin)
{
    DEB_MEMBER_FUNCT();

    int set_binX, hw_binX, binMaxX, binModeX;
    int set_binY, hw_binY, binMaxY, binModeY;

    set_binX = aBin.getX();
    binMaxX = m_pcoData->stcPcoDescription.wMaxBinHorzDESC;
    binModeX = m_pcoData->stcPcoDescription.wBinHorzSteppingDESC;

    set_binY = aBin.getY();
    binMaxY = m_pcoData->stcPcoDescription.wMaxBinVertDESC;
    binModeY = m_pcoData->stcPcoDescription.wBinVertSteppingDESC;

    hw_binX = _binning_fit(set_binX, binMaxX, binModeX);
    hw_binY = _binning_fit(set_binY, binMaxY, binModeY);

    DEB_RETURN() << "\n... " << DEB_VAR4(set_binX, hw_binX, binMaxX, binModeX)
                 << "\n... " << DEB_VAR4(set_binY, hw_binY, binMaxY, binModeY);

    aBin = Bin(hw_binX, hw_binY);
}
