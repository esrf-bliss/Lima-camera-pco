//-----------------------------------------------------------------//
// Name        | cl_grabthreadworker.cpp     | Type: (*) source    //
//-------------------------------------------|       ( ) header    //
// Project     | pco.camera                  |       ( ) others    //
//-----------------------------------------------------------------//
// Platform    | Linux, Windows                                    //
//-----------------------------------------------------------------//
// Environment |                                                   //
//             |                                                   //
//-----------------------------------------------------------------//
// Purpose     | Qt project part                                   //
//-----------------------------------------------------------------//
// Author      | EIJ, PCO AG                                       //
//-----------------------------------------------------------------//
// Revision    | rev. 1.00                                         //
//-----------------------------------------------------------------//
// Notes       |                                                   //
//             |                                                   //
//             |                                                   //
//             |                                                   //
//             |                                                   //
//-----------------------------------------------------------------//
// (c) 2015 PCO AG                                                 //
// Donaupark 11 D-93309  Kelheim / Germany                         //
// Phone: +49 (0)9441 / 2005-0   Fax: +49 (0)9441 / 2005-20        //
// Email: info@pco.de                                              //
//-----------------------------------------------------------------//

//-----------------------------------------------------------------//
// This program is free software; you can redistribute it and/or   //
// modify it under the terms of the GNU General Public License as  //
// published by the Free Software Foundation; either version 2 of  //
// the License, or (at your option) any later version.             //
//                                                                 //
// This program is distributed in the hope that it will be useful, //
// but WITHOUT ANY WARRANTY; without even the implied warranty of  //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the    //
// GNU General Public License for more details.                    //
//                                                                 //
// You should have received a copy of the GNU General Public       //
// License along with this program; if not, write to the           //
// Free Software Foundation, Inc., 59 Temple Place- Suite 330,     //
// Boston, MA 02111-1307, USA                                      //
//-----------------------------------------------------------------//

#include "clhs_grabthreadworker.h"
#include "qt_pco_camera.h"


GrabThreadWorker::GrabThreadWorker(CPco_grabber* grabber, QVector<unsigned short*>& buffer)
{
  this->buffer = buffer;
  this->grabber = grabber;
  stop = 0;
}

GrabThreadWorker:: ~GrabThreadWorker()
{

}

void GrabThreadWorker::run()
{
//  unsigned int gwidth, gheight;
//  void* adr;
//  int picnum;
  int i = 0;
  int count = buffer.size();

//  grabber->Free_Framebuffer();
//  grabber->Get_actual_size(&gwidth, &gheight, NULL);
//  grabber->Allocate_Framebuffer(20);
//  picnum = 1;

  grabber->Start_Acquire();

  while (!stop && (i<count))
  {
    err = grabber->Wait_For_Next_Image(buffer[i],10000);
//    err = grabber->Get_Framebuffer_adr(picnum, (void**)&adr);
//    grabber->Extract_Image(buffer[i], adr, gwidth, gheight);
//    grabber->Unblock_buffer(picnum);
    i++;
    if(err != PCO_NOERROR)
    {
        emit recorderError(i);
        break;
    }
    emit finishedPicture(i);
  }
  grabber->Stop_Acquire();
  emit finishedAllSignal(i);
}

void GrabThreadWorker::stopGrabbing()
{
  stop = 1;
}
