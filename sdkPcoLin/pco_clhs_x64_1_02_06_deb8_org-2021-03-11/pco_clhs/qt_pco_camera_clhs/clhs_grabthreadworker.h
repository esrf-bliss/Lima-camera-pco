//-----------------------------------------------------------------//
// Name        | cl_grabthreadworker.h      | Type: ( ) source    //
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
// Notes       |                                                   //
//             |                                                   //
//             |                                                   //
//-----------------------------------------------------------------//
// (c) 2010 - 2014 PCO AG                                          //
// Donaupark 11 D-93309  Kelheim / Germany                         //
// Phone: +49 (0)9441 / 2005-0   Fax: +49 (0)9441 / 2005-20        //
// Email: info@pco.de                                              //
//-----------------------------------------------------------------//

#ifndef GRABTHREADWORKER_H
#define GRABTHREADWORKER_H

#include <QThread>
#include <qvector.h>
#include "qt_pco_camera.h"
#include "interface_includes.h"


class GrabThreadWorker : public QThread
{
  Q_OBJECT

public:
  GrabThreadWorker(CPco_grabber* grabber, QVector<unsigned short*>& buffer);
  ~GrabThreadWorker();
  void stopGrabbing();

private:
  QVector<unsigned short*> buffer;
  volatile int stop;
  DWORD err;
  CPco_grabber* grabber;

signals:
  void finishedAllSignal(int);
  void finishedPicture(int);
  void recorderError(int);

protected:
  void run();



};



#endif // GRABTHREADWORKER_H
