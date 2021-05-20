//-----------------------------------------------------------------//
// Name        | interface_includes.h        | Type: ( ) source    //
//-------------------------------------------|       (*) header    //
// Project     | pco.camera                  |       ( ) others    //
//-----------------------------------------------------------------//
// Platform    | Linux                                             //
//-----------------------------------------------------------------//
// Environment |                                                   //
//             |                                                   //
//-----------------------------------------------------------------//
// Purpose     | pco.camera                                        //
//-----------------------------------------------------------------//
// Author      | EIJ, PCO AG                                       //
//-----------------------------------------------------------------//
// Revision    |                                                   //
//-----------------------------------------------------------------//
// Notes       | All interface specific includes and defines       //
//             |                                                   //
//             |                                                   //
//-----------------------------------------------------------------//
// (c) 2010 - 2014 PCO AG                                          //
// Donaupark 11 D-93309  Kelheim / Germany                         //
// Phone: +49 (0)9441 / 2005-0   Fax: +49 (0)9441 / 2005-20        //
// Email: info@pco.de                                              //
//-----------------------------------------------------------------//

#ifndef INTERFACE_INCLUDES_H
#define INTERFACE_INCLUDES_H

/* camera specific variables */
#define CPco_camera CPco_com_cl_me4
#define CPco_grabber CPco_grab_cl_me4

/* CameraLink ME4 include */
#include "pco_includes.h"
#include "Cpco_com_cl_me4.h"
#include "Cpco_grab_cl_me4.h"
#include "Cpco_grab_cl_me4_GL.h"
#include "cl_grabthreadworker.h"
#include "cl_livethreadworker.h"


/* ME4 Logfile name*/
#define LOGNAME "pco_camera_me4.log"

/* ME4 Main Window Title extension*/
#define MAINWINDOWTITLE "CameraLink ME4"

#endif
