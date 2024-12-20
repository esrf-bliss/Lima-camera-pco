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

//#include <cstdlib>
//#include <WinDef.h>
//#include <WinNt.h>
#include "lima/Exceptions.h"
#include "PcoCamera.h"
#include "PcoRoiCtrlObj.h"

using namespace lima;
using namespace lima::Pco;

//=========================================================================================================
//=========================================================================================================
RoiCtrlObj::RoiCtrlObj(Camera *cam) : m_cam(cam), m_handle(cam->getHandle())
{
    DEB_CONSTRUCTOR();
}

//=========================================================================================================
//=========================================================================================================
RoiCtrlObj::~RoiCtrlObj()
{
}

//=========================================================================================================
//=========================================================================================================
void RoiCtrlObj::checkRoi(const Roi &set_roi, Roi &hw_roi)
{
    DEB_MEMBER_FUNCT();
    int iFixed;

    iFixed = m_cam->_checkValidRoi(set_roi, hw_roi);

    // Point align; m_cam->_get_XYsteps(align);
    // hw_roi = set_roi;
    // hw_roi.alignCornersTo(align, Ceil);
    // if(m_cam->_getDebug(DBG_ROI)) {DEB_ALWAYS() << DEB_VAR3(align, set_roi,
    // hw_roi);}

    // if(m_cam->_getDebug(DBG_ROI)) {DEB_ALWAYS() << DEB_VAR3(iFixed, set_roi,
    // hw_roi);}

    DEB_RETURN() << "\n... " << DEB_VAR3(set_roi, hw_roi, iFixed);
}

//=========================================================================================================
//=========================================================================================================
void RoiCtrlObj::setRoi(const Roi &set_roi)
{
    DEB_MEMBER_FUNCT();

    Roi hw_roi;
    int error = 0;
    int iRoi_error = 0;

    if (set_roi.isEmpty())
    {
        m_cam->_get_MaxRoi(hw_roi);
    }
    else
    {
        iRoi_error = m_cam->_checkValidRoi(set_roi, hw_roi);
    }

    m_cam->_pco_SetROI(hw_roi, error);

    // if(m_cam->_getDebug(DBG_ROI)) {DEB_ALWAYS() << DEB_VAR3(set_roi, hw_roi,
    // iRoi_error);}
    // DEB_TRACE() << "\n  setRoi "	<< DEB_VAR3(set_roi, hw_roi,
    // iRoi_error);

    if (iRoi_error)
    {
        DEB_ALWAYS() << "m_cam->setRoi FIXED: " << DEB_VAR2(set_roi, hw_roi);

        m_cam->_set_logLastFixedRoi(set_roi, hw_roi);
    }

    DEB_RETURN() << "\n... " << DEB_VAR2(set_roi, hw_roi);
}

//=========================================================================================================
//=========================================================================================================
void RoiCtrlObj::getRoi(Roi &hw_roi)
{
    DEB_MEMBER_FUNCT();
    int err;

    m_cam->_pco_GetROI(hw_roi, err);

    DEB_TRACE() << "\n  setRoi " << DEB_VAR1(hw_roi);
}

//=================================================================================================
// ----- ROI
//=================================================================================================
void Camera::_xlatRoi_lima2pco(Roi roiLima, unsigned int &x0, unsigned int &x1,
                               unsigned int &y0, unsigned int &y1)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    Point top_left = roiLima.getTopLeft();
    Point bot_right = roiLima.getBottomRight();

    x0 = top_left.x + 1;
    y0 = top_left.y + 1;
    x1 = bot_right.x + 1;
    y1 = bot_right.y + 1;
}

//=================================================================================================
//=================================================================================================
void Camera::_xlatRoi_pco2lima(Roi &roiLima, unsigned int x0, unsigned int x1,
                               unsigned int y0, unsigned int y1)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    roiLima.setTopLeft(Point(x0 - 1, y0 - 1));
    roiLima.setSize(Size(x1 - x0 + 1, y1 - y0 + 1));
}

/****************************************************************************************
 Some sensors have a ROI stepping. See the camera description and check the
parameters wRoiHorStepsDESC and/or wRoiVertStepsDESC.

 For dual ADC mode the horizontal ROI must be symmetrical. For a pco.dimax the
horizontal and vertical ROI must be symmetrical. For a pco.edge the vertical ROI
must be symmetrical.
****************************************************************************************/

int Camera::_checkValidRoi(const Roi &roi_new, Roi &roi_fixed)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    int err;

    int iInvalid;
    unsigned int x0, x1, y0, y1;
    unsigned int x0org, x1org, y0org, y1org;

    unsigned int xMax, yMax, xSteps, ySteps, xMinSize, yMinSize;
    getXYdescription(xSteps, ySteps, xMax, yMax, xMinSize, yMinSize);

    // if steps is zero, only the max size is allowed
    bool xStepsNull = (xSteps == 0);
    bool yStepsNull = (ySteps == 0);

    Bin binNow;
    int binX, binY;
    _pco_GetBinning(binNow, err);

    binX = binNow.getX();
    binY = binNow.getY();

    xMax /= binX;
    yMax /= binY;

    xSteps /= binX;
    ySteps /= binY;
    if (xSteps < 1)
        xSteps = 1;
    if (ySteps < 1)
        ySteps = 1;

    xMinSize /= binX;
    yMinSize /= binY;
    if (xMinSize < 1)
        xMinSize = 1;
    if (yMinSize < 1)
        yMinSize = 1;

    bool bSymX = false, bSymY = false;
    if (_isCameraType(Dimax))
    {
        bSymX = bSymY = true;
    }
    if (_isCameraType(Edge))
    {
        bSymY = true;
    }

    int adc_working, adc_max;
    _pco_GetADCOperation(adc_working, adc_max);
    if (adc_working != 1)
    {
        bSymX = true;
    }

    // lima roi [0,2047]
    //  pco roi [1,2048]

    x0org = x0 = roi_new.getTopLeft().x + 1;
    x1org = x1 = roi_new.getBottomRight().x + 1;
    y0org = y0 = roi_new.getTopLeft().y + 1;
    y1org = y1 = roi_new.getBottomRight().y + 1;

    if (xStepsNull)
        xMinSize = xMax;
    if (yStepsNull)
        yMinSize = yMax;

    iInvalid = _fixValidRoi(x0, x1, xMax, xSteps, xMinSize, bSymX) |
               _fixValidRoi(y0, y1, yMax, ySteps, yMinSize, bSymY);

    roi_fixed.setTopLeft(Point(x0 - 1, y0 - 1));
    roi_fixed.setSize(Size(x1 - x0 + 1, y1 - y0 + 1));

    if (_getDebug(DBG_ROI) || iInvalid)
    {
        unsigned int X0, Y0, Xsize, Ysize;
        X0 = x0 - 1;
        Y0 = y0 - 1;
        Xsize = x1 - x0 + 1;
        Ysize = y1 - y0 + 1;
        DEB_ALWAYS() << "\nREQUESTED roiX (pco from 1): "
                     << DEB_VAR5(x0org, x1org, xSteps, xMax, xMinSize)
                     << "\nREQUESTED roiY (pco from 1): "
                     << DEB_VAR5(y0org, y1org, ySteps, yMax, yMinSize)
                     << "\n    FIXED roi  (pco from 1): "
                     << DEB_VAR4(x0, x1, y0, y1)
                     << "\n    FIXED roi (lima from 0): "
                     << DEB_VAR4(X0, Y0, Xsize, Ysize)
                     << "\n                     STATUS: "
                     << DEB_VAR3(iInvalid, bSymX, bSymY);
    }

    return iInvalid;
}

int Camera::_fixValidRoi(unsigned int &x0, unsigned int &x1, unsigned int xMax,
                         unsigned int xSteps, unsigned int xMinSize, bool bSymX)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    int iInvalid;
    unsigned int diff0, diff1, tmp;

    unsigned int size, diff;

    if ((xMax < 1) || (xSteps < 1) || (xMinSize < 1))
    {
        DEB_ALWAYS() << "\nERROR - invalid values - check PcoDescription "
                        "!!!!\n / getXYsteps "
                     << DEB_VAR3(xMax, xSteps, xMinSize);
        throw LIMA_HW_EXC(InvalidValue, "check PcoDescription");
    }

    // lima roi [0,2047]
    //  pco roi [1,2048]

    iInvalid = 0;

    if (x0 < 1)
    {
        x0 = 1;
        iInvalid |= Xrange;
    }
    if (x1 > xMax)
    {
        x1 = xMax;
        iInvalid |= Xrange;
    }
    if (x0 > x1)
    {
        tmp = x0;
        x0 = x1;
        x1 = tmp;
        iInvalid |= Xrange;
    }

    if ((diff = (x0 - 1) % xSteps) != 0)
    {
        x0 -= diff;
        iInvalid |= Xsteps;
    }
    if ((diff = x1 % xSteps) != 0)
    {
        x1 += xSteps - diff;
        iInvalid |= Xsteps;
    }

    if ((size = x1 - x0 + 1) < xMinSize)
    {
        diff = xMinSize - size;
        iInvalid |= Xrange;
        if (x0 >= 1 + diff)
        {
            x0 -= diff;
        }
        else
        {
            x1 += diff;
        }
    }

    if (bSymX)
    {
        if ((diff0 = x0 - 1) != (diff1 = xMax - x1))
        {
            if (diff0 > diff1)
                x0 -= diff0 - diff1;
            else
                x1 += diff1 - diff0;

            iInvalid |= Xsym;
        }
    }

    return iInvalid;
}

//=================================================================================================
//=================================================================================================
void Camera::_set_logLastFixedRoi(const Roi &requested_roi,
                                  const Roi &fixed_roi)
{
    m_Roi_lastFixed_hw = fixed_roi;
    m_Roi_lastFixed_requested = requested_roi;
    m_Roi_lastFixed_time = time(NULL);
}

//=================================================================================================
//=================================================================================================
void Camera::_get_logLastFixedRoi(Roi &requested_roi, Roi &fixed_roi,
                                  time_t &dt)
{
    fixed_roi = m_Roi_lastFixed_hw;
    requested_roi = m_Roi_lastFixed_requested;
    dt = m_Roi_lastFixed_time;
}
//=================================================================================================
//=================================================================================================

void Camera::_get_MaxRoi(Roi &roi)
{
    DEB_MEMBER_FUNCT();
    DEF_FNID;

    unsigned int xMax, yMax, x0, x1, y0, y1;
    int err;
    Bin bin;

    getMaxWidthHeight(xMax, yMax);

    _pco_GetBinning(bin, err);

    x0 = 1;
    x1 = xMax / bin.getX();
    y0 = 1;
    y1 = yMax / bin.getY();

    _xlatRoi_pco2lima(roi, x0, x1, y0, y1);
}

//=========================================================================================================
//=========================================================================================================
void Camera::_get_RoiSize(Size &roi_size)
{
    Roi limaRoi;
    int error;

    _pco_GetROI(limaRoi, error);

    roi_size = limaRoi.getSize();
}
