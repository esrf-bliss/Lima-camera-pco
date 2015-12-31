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

//#include <cstdlib>
//#include <WinDef.h>
//#include <WinNt.h>
#include "lima/Exceptions.h"
#include "PcoCamera.h"
#include "PcoDetInfoCtrlObj.h"


using namespace lima;
using namespace lima::Pco;

//=========================================================================================================
char* _timestamp_pcodetinfoctrlobj() {return ID_TIMESTAMP ;}
//=========================================================================================================

//=========================================================================================================
//=========================================================================================================
DetInfoCtrlObj::DetInfoCtrlObj(Camera *cam):
  m_cam(cam),
  m_handle(cam->getHandle())
{
}

//=========================================================================================================
//=========================================================================================================
DetInfoCtrlObj::~DetInfoCtrlObj()
{
}

//=========================================================================================================
//=========================================================================================================
void DetInfoCtrlObj::getMaxImageSize(Size& max_image_size)
{
	  m_cam->_get_MaxImageSize(max_image_size); 
}


//=========================================================================================================
//=========================================================================================================
void DetInfoCtrlObj::getDetectorImageSize(Size& det_image_size)
{
	m_cam->_get_RoiSize(det_image_size);
}


//=========================================================================================================
//=========================================================================================================
void DetInfoCtrlObj::getDefImageType(ImageType& def_image_type)
{
	m_cam->_get_ImageType( def_image_type);
}


//=========================================================================================================
//=========================================================================================================
void DetInfoCtrlObj::getCurrImageType(ImageType& curr_image_type)
{
    // ---- DONE
  getDefImageType(curr_image_type);
}


//=========================================================================================================
//=========================================================================================================
void DetInfoCtrlObj::setCurrImageType(ImageType curr_image_type)
{
	m_cam->_set_ImageType(curr_image_type);
}


//=========================================================================================================
//=========================================================================================================

void DetInfoCtrlObj::getPixelSize(double& x_size,double &y_size)
{  
	m_cam->_get_PixelSize(x_size, y_size);
}



//=========================================================================================================
//=========================================================================================================
void DetInfoCtrlObj::getDetectorType(std::string& det_type)
{
	m_cam->_get_DetectorType(det_type);
}


//=========================================================================================================
//=========================================================================================================
void DetInfoCtrlObj::getDetectorModel(std::string& det_model)
{
    // ---- DONE
  m_cam->getCameraName(det_model);
}



//=========================================================================================================
//=========================================================================================================
void DetInfoCtrlObj::registerMaxImageSizeCallback(HwMaxImageSizeCallback& cb)
{
	// TOCHECK
	// will not be used - from HwMaxImageSizeCallbackGen::registerMaxImageSizeCallback
  m_cam->registerMaxImageSizeCallback(cb);
}


//=========================================================================================================
//=========================================================================================================
void DetInfoCtrlObj::unregisterMaxImageSizeCallback(HwMaxImageSizeCallback& cb)
{
	// TOCHECK
  m_cam->unregisterMaxImageSizeCallback(cb);
}

