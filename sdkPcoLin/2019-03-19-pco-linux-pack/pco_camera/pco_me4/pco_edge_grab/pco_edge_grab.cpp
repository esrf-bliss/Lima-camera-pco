//-----------------------------------------------------------------//
// Name        | pco_edge_grab.cpp           | Type: (*) source    //
//-------------------------------------------|       ( ) header    //
// Project     | pco.camera                  |       ( ) others    //
//-----------------------------------------------------------------//
// Platform    | Linux                                             //
//-----------------------------------------------------------------//
// Environment |                                                   //
//             |                                                   //
//-----------------------------------------------------------------//
// Purpose     | pco.camera - CameraLink Communication             //
//-----------------------------------------------------------------//
// Author      | MBL, PCO AG                                       //
//-----------------------------------------------------------------//
// Revision    | rev. 1.10                                         //
//-----------------------------------------------------------------//
// Notes       | main console program to show how to use           //
//             | class Cpco_cl_me4 and Cpco_cl_com to grab         //
//             | images from camera pco.edge Rolling Shutter       //
//             | with the Me4 grabber and save them to tif or b16  //
//             | files                                             //
//             |                                                   //
//-----------------------------------------------------------------//
// (c) 2010 - 2015 PCO AG                                          //
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
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,    //
// Boston, MA 02111-1307, USA                                      //
//-----------------------------------------------------------------//

//-----------------------------------------------------------------//
// Revision History:                                               //
//-----------------------------------------------------------------//
// Rev.:     | Date:      | Changed:                               //
// --------- | ---------- | ---------------------------------------//
//  1.01     | 08.10.2011 | ported from Windows SDK                //
//           |            |                                        //
//-----------------------------------------------------------------//
//  1.02     | 14.01.2011 | more commands                          //
//           |            |                                        //
//-----------------------------------------------------------------//
//  1.03     | 24.01.2012 | use pcofile library                    //
//           |            | more commands                          //
//           |            |                                        //
//-----------------------------------------------------------------//
//  1.05     | 12.06.2012 | with timebase command                  //
//           |            |                                        //
//-----------------------------------------------------------------//
//  1.10     | 22.10.2015 | use new class functions                //
//           |            | more commands                          //
//           |            |                                        //
//-----------------------------------------------------------------//
//  0.0x     | xx.xx.2012 |                                        //
//           |            |                                        //
//-----------------------------------------------------------------//


#include <iostream>

#include "VersionNo.h"

#include "Cpco_com.h"
#include "Cpco_com_cl_me4.h"
#include "Cpco_grab_cl_me4.h"
#include "file12.h"

DWORD grab_single(CPco_grab_cl_me4 *grabber,char * filename);
DWORD grab_count(CPco_grab_cl_me4 *grabber,int ima_count);
DWORD grab_loop(CPco_grab_cl_me4 *grabber,int ima_count);

int image_nr_from_timestamp(void *buf,int shift);
void get_number(char *number,int len);
void get_text(char *text,int len);
void get_hexnumber(int *num,int len);

//USERSET can be used to switch off auto setting of Dataformat
//#define USERSET 

CPco_Log mylog("pco_edge_grab.log");

const char tb[3][3]={"ns","us","ms"};

int main(int argc, char* argv[])
{
  DWORD err;
  CPco_com *camera;
  CPco_grab_cl_me4* grabber;


  int board=0;
  char infostr[100];
  char number[20];

  int x;
  char c;
  int ima_count=100;
  int loop_count=1;
  int PicTimeOut=10000; //10 seconds
  WORD act_recstate;
  DWORD exp_time,delay_time,pixelrate;
  WORD exp_timebase,del_timebase;
  DWORD width,height,secs,nsecs;
  SC2_Camera_Description_Response description;
  PCO_SC2_CL_TRANSFER_PARAM clpar;

  double freq;
  SHORT ccdtemp,camtemp,pstemp;
  WORD camtype;
  DWORD serialnumber;
  WORD actlut,lutparam;

  int bufnum=20;

  mylog.set_logbits(0x0000F0FF);
  printf("Logging set to 0x%x\n",mylog.get_logbits());

  camera= new CPco_com_cl_me4();
  camera->SetLog(&mylog);

  printf("Try to open Camera\n");
  err=camera->Open_Cam(0);
  if(err!=PCO_NOERROR)
  {
   printf("error 0x%x in Open_Cam, close application",err);
   delete camera;

   getchar();
   return -1;
  }

  err=camera->PCO_GetCameraType(&camtype,&serialnumber);
  if(err!=PCO_NOERROR)
  {
   printf("error 0x%x in PCO_GetCameraType",err);
   camera->Close_Cam();
   delete camera;

   getchar();
   return -1;
  }

  if(camtype==CAMERATYPE_PCO_EDGE)
  {
   printf("Grabber is CPco_grab_cl_me4_edge\n");
   grabber=new CPco_grab_cl_me4_edge((CPco_com_cl_me4*)camera);
  }
  else if(camtype==CAMERATYPE_PCO_EDGE_42)
  {
   printf("Grabber is CPco_grab_cl_me4_edge42\n");
   grabber=new CPco_grab_cl_me4_edge42((CPco_com_cl_me4*)camera);
  }
  else
  {
   printf("Wrong camera for this application");
   camera->Close_Cam();
   delete camera;

   getchar();
   return -1;
  }

  grabber->SetLog(&mylog);

  printf("Try to open Grabber\n");
  err=grabber->Open_Grabber(board);
  if(err!=PCO_NOERROR)
  {
   printf("error 0x%x in Open_Grabber, close application",err);
   delete grabber;


   camera->Close_Cam();
   delete camera;

   getchar();
   return -1;
  }

  err=camera->PCO_GetCameraDescriptor(&description);
  if(err!=PCO_NOERROR)
   printf("PCO_GetCameraDescriptor() Error 0x%x\n",err);


  err=camera->PCO_GetInfo(1,infostr,sizeof(infostr));
  if(err!=PCO_NOERROR)
   printf("PCO_GetInfo() Error 0x%x\n",err);
  else
   printf("Camera Name is: %s serialnumber %d\n",infostr,serialnumber);

  err=camera->PCO_SetCameraToCurrentTime();
  if(err!=PCO_NOERROR)
   printf("PCO_SetCameraToCurrentTime() Error 0x%x\n",err);


  err=camera->PCO_GetTransferParameter(&clpar,sizeof(clpar));
  if(err!=PCO_NOERROR)
   printf("PCO_GetTransferParameter() Error 0x%x\n",err);
  else
  {
   printf("Baudrate      : %d\n",clpar.baudrate);
   printf("Clockfrequency: %d\n",clpar.ClockFrequency);
   printf("Dataformat    : 0x%x\n",clpar.DataFormat);
   printf("Transmit       : 0x%x\n",clpar.Transmit);
  }

  err=camera->PCO_GetTemperature(&ccdtemp,&camtemp,&pstemp);
  if(err!=PCO_NOERROR)
   printf("PCO_GetTemperature() Error 0x%x\n",err);
  else
  {
   printf("current temperatures\n");
   printf("Sensor:      %d\n",ccdtemp);
   printf("Camera:      %d\n",camtemp);
   printf("PowerSupply: %d\n",pstemp);
  }


  err=camera->PCO_GetPixelRate(&pixelrate);
  if(err!=PCO_NOERROR)
   printf("PCO_GetPixelrate() Error 0x%x\n",err);
  else
   printf("actual PixelRate: %d\n",pixelrate);
  printf("possible PixelRates:\n");
  for(x=0;x<4;x++)
  {
   if(description.dwPixelRateDESC[x]!=0)
   {
    printf("%d: %d\n",x,description.dwPixelRateDESC[x]);
   }
  }

  err=camera->PCO_GetLut(&actlut,&lutparam);
  if(err!=PCO_NOERROR)
   printf("PCO_GetLut() Error 0x%x\n",err);

//set RecordingState to STOP
  err=camera->PCO_SetRecordingState(0);
  if(err!=PCO_NOERROR)
   printf("PCO_SetRecordingState() Error 0x%x\n",err);

  err=camera->PCO_SetTimestampMode(2);
  if(err!=PCO_NOERROR)
   printf("PCO_SetTimestampMode() Error 0x%x\n",err);

  exp_time=5000;
  delay_time=1000;
  exp_timebase=1;
  del_timebase=1;
  err=camera->PCO_SetTimebase(exp_timebase,del_timebase);
  if(err!=PCO_NOERROR)
   printf("PCO_SetTimebase() Error 0x%x\n",err);

  err=camera->PCO_SetDelayExposure(delay_time,exp_time);
  if(err!=PCO_NOERROR)
   printf("PCO_SetDelayExposure() Error 0x%x\n",err);


//prepare Camera for recording
  err=camera->PCO_ArmCamera();
  if(err!=PCO_NOERROR)
   printf("PCO_ArmCamera() Error 0x%x\n",err);

  err=camera->PCO_GetActualSize(&width,&height);
  if(err!=PCO_NOERROR)
   printf("PCO_GetActualSize() Error 0x%x\n",err);

  printf("Actual Resolution %d x %d\n",width,height);

#ifdef USERSET
//transfer dataformat must be changed depending on pixelrate and horizontal resolution
  WORD lut;
  err=camera->PCO_GetPixelRate(&pixelrate);
  if(err!=PCO_NOERROR)
   printf("PCO_GetPixelrate() Error 0x%x\n",err);

  if((width>1920)&&(pixelrate>=286000000)&&(camtype==CAMERATYPE_PCO_EDGE))
  {
   clpar.DataFormat=SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER|PCO_CL_DATAFORMAT_5x12;
   printf("width>1920 %d && pixelrate >=286000000 %d Dataformat 0x%x\n",width,pixelrate,clpar.DataFormat);
   lut=0x1612;
  }
  else
  {
   clpar.DataFormat=SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER|PCO_CL_DATAFORMAT_5x16;
   printf("width<=1920 %d || pixelrate<286000000 %d Dataformat 0x%x\n",width,pixelrate,clpar.DataFormat);
   lut=0;
  }

  actlut=lut; 
  err=camera->PCO_SetLut(actlut,0);
  if(err!=PCO_NOERROR)
   printf("PCO_SetLut() Error 0x%x\n",err);

  err=camera->PCO_SetTransferParameter(&clpar,sizeof(clpar));
  if(err!=PCO_NOERROR)
   printf("PCO_TransferParameter() Error 0x%x\n",err);

  err=camera->PCO_ArmCamera();
  if(err!=PCO_NOERROR)
   printf("PCO_ArmCamera() Error 0x%x\n",err);

  err=grabber->Set_DataFormat(clpar.DataFormat);
  if(err!=PCO_NOERROR)
   printf("Set_DataFormat() Error 0x%x\n",err);

  err=grabber->Set_Grabber_Size(width,height);
  if(err!=PCO_NOERROR)
   printf("Set_Grabber_Size() Error 0x%x\n",err);

  err=grabber->PostArm(1);
  if(err!=PCO_NOERROR)
   printf("PostArm() Error 0x%x\n",err);
#else
  err=grabber->PostArm();
  if(err!=PCO_NOERROR)
   printf("PostArm() Error 0x%x\n",err);
#endif

  err=grabber->Allocate_Framebuffer(bufnum);
  if(err!=PCO_NOERROR)
   printf("Allocate_Framebuffer() Error 0x%x\n",err);

  err=grabber->Set_Grabber_Timeout(PicTimeOut);
  if(err!=PCO_NOERROR)
   printf("\nSet_Grabber_Timeout error 0x%x\n",err);

  err=camera->PCO_SetRecordingState(1);
  if(err!=PCO_NOERROR)
   printf("PCO_SetRecordingState() Error 0x%x\n",err);


  c=' ';
  while(c!='x')
  {
   int ch;
   c=' ';

   printf("\n");
   camera->PCO_GetRecordingState(&act_recstate);
   camera->PCO_GetDelayExposure(&delay_time,&exp_time);
   camera->PCO_GetCOCRuntime(&secs,&nsecs);
   camera->PCO_GetTransferParameter(&clpar,sizeof(clpar));
   freq=nsecs;
   freq/=1000000000;
   freq+=secs;
   freq=1/freq;
   printf("actual recording state %s actual freq: %.3lfHz %.2lfMB/sec\n",act_recstate ? "RUN" : "STOP",freq,(freq*width*height*2)/(1024*1024));
   printf("\n");

   printf("x to close camera and program   actual values\n");
   printf("l to set loop_count              loop_count      %d\n",loop_count);
   printf("c to set imagecount              imagecount      %d\n",ima_count);
   printf("b to set and allocate buffers    nr_of_buffer    %d\n",bufnum);
   printf("t to set picture timeout         timeout         %dms\n",PicTimeOut);
   printf("e to set exposure time           exposuretime    %d%s\n",exp_time,tb[exp_timebase]);
   printf("d to set delay time              delaytime       %d%s\n",delay_time,tb[del_timebase]);
   printf("p to set camera pixelrate        pixelrate       %dHz\n",pixelrate);
   printf("r to set transfer parameter      Transmit        0x%x\n",clpar.Transmit);
   printf("                                 Dataformat      0x%x\n",clpar.DataFormat);
   printf("                                 Clockfrequency  %d\n",clpar.ClockFrequency);
   printf("                                 Baudrate        %d\n",clpar.baudrate);
//   printf("f to set Dataformat 0x%x and LUT 0x%x\n",clpar.DataFormat,actlut);
   printf("\n");
   printf("0 to set recording state to OFF\n");
   printf("1 to set recording state to ON\n");
   printf("2 Single Acquire_Image (recording camera)\n");
   printf("3 Start Grab in grabber allocated buffers\n");
   printf("4 Start Grab loop (%d images)\n",ima_count);
   fflush(stdin);

   for( x = 0; (x < 2) &&  ((ch = getchar()) != EOF)
                        && (ch != '\n'); x++ )
    c=(char)ch;

   if(c=='c')
   {
    printf("enter ima_count ...<CR>: ");
    get_number(number,10);
    if(strlen(number))
     ima_count=atoi(number);
   }
   else if(c=='l')
   {
    printf("enter loop_count ...<CR>: ");
    get_number(number,10);
    if(strlen(number))
     loop_count=atoi(number);
   }
   else if(c=='b')
   {
    printf("enter number of buffers to allocate ...<CR>: ");
    get_number(number,5);
    if(strlen(number))
    {
     int i=atoi(number);
     grabber->Free_Framebuffer();
     if((err=grabber->Allocate_Framebuffer(i))!=PCO_NOERROR)
     {
      printf("Allocation failed use old number %d\n",bufnum);
      err=grabber->Allocate_Framebuffer(bufnum);
      printf("Any key <CR> to proceed");
      getchar();
     }
     else
      bufnum=i;
    }
   }
   else if(c=='t')
   {
    printf("enter picture timeout in ms ...<CR>: ");
    get_number(number,10);
    if(strlen(number))
    {
     PicTimeOut=atoi(number);
     grabber->Set_Grabber_Timeout(PicTimeOut);
    }
   }
   else if(c=='e')
   {
    int val=-1;
    int base=exp_timebase;
    printf("enter new exposure timebase 0=ns 1=us 2=ms ...<CR>: ");
    get_number(number,2);
    if(strlen(number))
    {
     val=atoi(number);
     if((val<0)||(val>2))
      val=exp_timebase;
     base=val;
     err=camera->PCO_SetTimebase(del_timebase,val);
     if(err!=PCO_NOERROR)
      printf("PCO_SetTimebase() Error 0x%x\n",err);
    }
    printf("enter new exposure time in %s ...<CR>: ",tb[base]);
    get_number(number,10);
    if((strlen(number))||(val>=0))
    {
     exp_time=atoi(number);
     if(act_recstate==1)
      camera->PCO_SetRecordingState(0);
     err=camera->PCO_SetDelayExposure(delay_time,exp_time);
     err=camera->PCO_ArmCamera();
     if(err!=PCO_NOERROR)
      printf("PCO_ArmCamera() Error 0x%x\n",err);

     err=camera->PCO_GetDelayExposure(&delay_time,&exp_time);
     err=camera->PCO_GetTimebase(&del_timebase,&exp_timebase);
     if(act_recstate==1)
      camera->PCO_SetRecordingState(1);
    }
   }
   else if(c=='d')
   {
    int val=-1;
    int base=del_timebase;
    printf("enter new delay timebase 0=ns 1=us 2=ms ...<CR>: ");
    get_number(number,2);
    if(strlen(number))
    {
     val=atoi(number);
     if((val<0)||(val>2))
      val=del_timebase;
     base=val;
     err=camera->PCO_SetTimebase(val,exp_timebase);
     if(err!=PCO_NOERROR)
      printf("PCO_SetTimebase() Error 0x%x\n",err);
    }
    printf("enter new delay time in %s ...<CR>: ",tb[base]);
    get_number(number,10);
    if((strlen(number))||(val>=0))
    {
     delay_time=atoi(number);
     if(act_recstate==1)
      camera->PCO_SetRecordingState(0);

     err=camera->PCO_SetDelayExposure(delay_time,exp_time);
     err=camera->PCO_ArmCamera();
     if(err!=PCO_NOERROR)
      printf("PCO_ArmCamera() Error 0x%x\n",err);

     err=camera->PCO_GetDelayExposure(&delay_time,&exp_time);
     err=camera->PCO_GetTimebase(&del_timebase,&exp_timebase);
     if(act_recstate==1)
      camera->PCO_SetRecordingState(1);
    }
   }
   else if(c=='p')
   {
    int index=-1;
    printf("possible pixelrates: \n");
    for(x=0;x<4;x++)
    {
     if(description.dwPixelRateDESC[x]!=0)
     {
      printf("Index %d: %d\n",x,description.dwPixelRateDESC[x]);  
      index++;
     }
    }
    printf("enter index to table  ...<CR>: ");
    get_number(number,2);
    if(strlen(number))
    {
     x=atoi(number);
     if((x<0)||(x>index))
     {
      printf("Wrong index\n");
      continue;
     }

     if(act_recstate==1)
      camera->PCO_SetRecordingState(0);

     printf("set pixelrate: %d\n",description.dwPixelRateDESC[x]);

     err=camera->PCO_SetPixelRate(description.dwPixelRateDESC[x]);
     if(err!=PCO_NOERROR)
      printf("PCO_SetPixelRate() Error 0x%x\n",err);

     err=camera->PCO_ArmCamera();
     if(err!=PCO_NOERROR)
      printf("PCO_ArmCamera() Error 0x%x\n",err);

#ifdef USERSET
//this is a must do
     DWORD data_format;
     WORD lut;

     data_format=clpar.DataFormat;

     if((width>1920)&&(pixelrate>=286000000))
     {
      clpar.DataFormat=(clpar.DataFormat&~PCO_CL_DATAFORMAT_MASK)|PCO_CL_DATAFORMAT_5x12;
      lut=0x1612;
     }
     else
     {
      clpar.DataFormat=(clpar.DataFormat&~PCO_CL_DATAFORMAT_MASK)|PCO_CL_DATAFORMAT_5x16;
      lut=0;
     }

     if(data_format!=clpar.DataFormat)
     {
      actlut=lut; 
      err=camera->PCO_SetLut(actlut,0);
      if(err!=PCO_NOERROR)
       printf("PCO_SetLut() Error 0x%x\n",err);

      err=camera->PCO_SetTransferParameter(&clpar,sizeof(clpar));
      if(err!=PCO_NOERROR)
       printf("PCO_TransferParameter() Error 0x%x\n",err);

      err=camera->PCO_ArmCamera();
      if(err!=PCO_NOERROR)
       printf("PCO_ArmCamera() Error 0x%x\n",err);

      err=grabber->Set_DataFormat(clpar.DataFormat);
      if(err!=PCO_NOERROR)
       printf("Set_DataFormat() Error 0x%x\n",err);

      err=grabber->Set_Grabber_Size(width,height);
      if(err!=PCO_NOERROR)
       printf("Set_Grabber_Size() Error 0x%x\n",err);
     }

     err=grabber->PostArm(1);
     if(err!=PCO_NOERROR)
      printf("grabber.PostArm() Error 0x%x\n",err);
#else
     err=grabber->PostArm();
     if(err!=PCO_NOERROR)
      printf("grabber.PostArm() Error 0x%x\n",err);
#endif

     camera->PCO_GetPixelRate(&pixelrate);

     if(act_recstate==1)
      camera->PCO_SetRecordingState(1);
    }
   }
   else if(c=='r')
   {
    PCO_SC2_CL_TRANSFER_PARAM new_clpar;
    int val=-1;

#ifdef USERSET
    DWORD data_format;
    data_format=clpar.DataFormat;
    WORD lut=0;
#endif

    new_clpar.Transmit        = clpar.Transmit;
    new_clpar.DataFormat      = clpar.DataFormat;
    new_clpar.ClockFrequency  = clpar.ClockFrequency;
    new_clpar.baudrate        = clpar.baudrate;

    printf("enter new DataFormat     ...<CR>: 0x");
    get_hexnumber(&val,8);
    if(val>=0)
     new_clpar.DataFormat = val;

    printf("enter new baudrate       ...<CR>: ");
    get_number(number,10);
    if(strlen(number))
     new_clpar.baudrate=atoi(number);

    if(  (new_clpar.Transmit!=clpar.Transmit)||(new_clpar.DataFormat!=clpar.DataFormat)
       ||(new_clpar.ClockFrequency!=clpar.ClockFrequency)||(new_clpar.baudrate!=clpar.baudrate))
    {

     printf("Baudrate      : %d\n",new_clpar.baudrate);
     printf("Dataformat    : 0x%x\n",new_clpar.DataFormat);

     if(act_recstate==1)
      camera->PCO_SetRecordingState(0);

     if((width>1920)&&(pixelrate>=286000000))
     {
      switch(new_clpar.DataFormat&PCO_CL_DATAFORMAT_MASK)
      {
       case PCO_CL_DATAFORMAT_5x12:
       case PCO_CL_DATAFORMAT_5x12L:
       case PCO_CL_DATAFORMAT_5x12R:
        err=PCO_NOERROR;
#ifdef USERSET
        lut=0x1612;
#endif
        break;

       default:
        err=PCO_ERROR_WRONGVALUE;
        break;
      }
     }

     if(err!=PCO_NOERROR)
     {
      printf("Wrong DataFormat for this setting\n");
      continue;
     }

     err=camera->PCO_SetTransferParameter(&new_clpar,sizeof(new_clpar));
     if(err!=PCO_NOERROR)
      printf("PCO_SetTransferParameter Error 0x%x\n",err);

     err=camera->PCO_ArmCamera();
     if(err!=PCO_NOERROR)
      printf("PCO_ArmCamera() Error 0x%x\n",err);


#ifdef USERSET
     if(data_format!=clpar.DataFormat)
     {
      actlut=lut
      err=camera->PCO_SetLut(actlut,0);
      if(err!=PCO_NOERROR)
       printf("PCO_SetLut() Error 0x%x\n",err);

      err=grabber->Set_DataFormat(clpar.DataFormat);
      if(err!=PCO_NOERROR)
       printf("Set_DataFormat() Error 0x%x\n",err);

      err=grabber->Set_Grabber_Size(width,height);
      if(err!=PCO_NOERROR)
       printf("Set_Grabber_Size() Error 0x%x\n",err);
     }

     err=grabber->PostArm(1);
     if(err!=PCO_NOERROR)
      printf("grabber.PostArm() Error 0x%x\n",err);
#else
     err=grabber->PostArm();
     if(err!=PCO_NOERROR)
      printf("grabber.PostArm() Error 0x%x\n",err);
#endif

     camera->PCO_GetTransferParameter(&clpar,sizeof(clpar));

     if(act_recstate==1)
      camera->PCO_SetRecordingState(1);
    }
   }
/*
   else if(c=='f')
   {
    int dataformat,lut;

    dataformat=lut=-1;
    printf("enter new Dataformat in HEX ...<CR>: ");
    get_hexnumber(&dataformat,4);
    if((dataformat&SCCMOS_FORMAT_MASK)==0)
     dataformat|=SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER;
    if(   ((dataformat& PCO_CL_DATAFORMAT_MASK)!=PCO_CL_DATAFORMAT_5x16)
        &&((dataformat& PCO_CL_DATAFORMAT_MASK)!=PCO_CL_DATAFORMAT_5x12)
        &&((dataformat& PCO_CL_DATAFORMAT_MASK)!=PCO_CL_DATAFORMAT_5x12L))
    {
     printf("DataFormat 0x%x not supported" ,dataformat);
     continue;
    }

    printf("installed LUT's:\n");
    for(int x=0;x<camera->num_lut;x++)
     printf("%d: '%20s' Id 0x%x %02d->%02d Format 0x%x\n",x+1
       ,camera->cam_lut[x].Description,camera->cam_lut[x].wIdentifier
       ,camera->cam_lut[x].bInputWidth,camera->cam_lut[x].bOutputWidth
       ,camera->cam_lut[x].wFormat);
    printf("enter new LUT ID in HEX ...<CR>: ");
    get_hexnumber(&lut,4);

    if((dataformat>0)&&(lut>=0))
    {
     if(act_recstate==1)
      camera->PCO_SetRecordingState(0);

     if((width>1920)&&(pixelrate>=286000000))
      clpar.DataFormat=(dataformat&~PCO_CL_DATAFORMAT_MASK)|PCO_CL_DATAFORMAT_5x12;
     else
      clpar.DataFormat=dataformat;

     err=camera->PCO_SetTransferParameter(&clpar,sizeof(clpar));
     if(err!=PCO_NOERROR)
      printf("PCO_TransferParameter() Error 0x%x\n",err);

     actlut=lut;
     err=camera->PCO_SetLut(actlut,0);
     if(err!=PCO_NOERROR)
      printf("PCO_TransferParameter() Error 0x%x\n",err);


     err=camera->PCO_ArmCamera();
     if(err!=PCO_NOERROR)
      printf("PCO_ArmCamera() Error 0x%x\n",err);

     err=grabber.Set_DataFormat(clpar.DataFormat);
     if(err!=PCO_NOERROR)
      printf("Set_DataFormat() Error 0x%x\n",err);

     err=grabber.Set_Grabber_Size(width,height);
     if(err!=PCO_NOERROR)
      printf("Set_Grabber_Size() Error 0x%x\n",err);

     if(act_recstate==1)
      camera->PCO_SetRecordingState(1);
    }
   }
*/
   else if(c=='0')
   {
    err=camera->PCO_SetRecordingState(0);
    if(err==PCO_NOERROR)
     printf("\nrecoding state is set to STOP\n");
   }
   else if(c=='1')
   {
    err=camera->PCO_SetRecordingState(1);
    if(err==PCO_NOERROR)
     printf("\nrecoding state is set to RUN\n");
   }
   else if(c=='2')
   {
    if(act_recstate==0)
     printf("\nStart Camera before grabbing\n");
    else
    {
     char filename[300];
     printf("enter filename ...<CR>: \n");
     printf("if filename has suffix .b16 save image in b16 format\n");
     printf("if filename has suffix .tif save image in tif format\n");
     printf("if nothing is entered, no file will be saved\n");
     get_text(filename,300);
     if(strlen(filename))
      grab_single(grabber,filename);
     else
      grab_single(grabber,NULL);
    }
    printf("\n");
   }
   else if(c=='3')
   {
    if(act_recstate==0)
     printf("\nStart Camera before grabbing\n");
    else
     grab_count(grabber,bufnum);
    printf("\n");
   }
   else if(c=='4')
   {
    if(act_recstate==0)
     printf("\nStart Camera before grabbing\n");
    else
     grab_loop(grabber,ima_count);
    printf("\n");
   }
  }

  err=grabber->Free_Framebuffer();

  grabber->Close_Grabber();
  delete grabber;

  camera->Close_Cam();
  delete camera;

  return 0;
}


DWORD grab_single(CPco_grab_cl_me4 *grabber,char* filename)
{
 int err;
 unsigned int w,h,l;
 int ima_num; 
 WORD *picbuf;

 grabber->Get_actual_size(&w,&h,NULL);
 picbuf=(WORD*)malloc(w*h*sizeof(WORD));
 if(picbuf==NULL)
 {
  printf("\ngrab_single cannot allocate buffer\n");
  return PCO_ERROR_NOMEMORY | PCO_ERROR_APPLICATION;
 }
  
 err=grabber->Acquire_Image(picbuf);
 if(err!=PCO_NOERROR)
  printf("\ngrab_single Acquire_Image error 0x%x\n",err);

 if(err==PCO_NOERROR)
 {
  int min,max,v;
  WORD* adr; 

  ima_num=image_nr_from_timestamp(picbuf,0);
  printf("\ngrab_single done successful, timestamp image_nr: %d\n",ima_num);
  max=0;
  min=0xFFFF;

  adr=(WORD*)picbuf;
  l=w*20; //skip first lines with timestamp
  for(;l<w*h;l++)
  {
   v=*(adr+l);
   if(v<min)
    min=v;
   if(v>max)
    max=v;
  }
  printf("grab_single pixels min_value: %d max_value %d\n",min,max);

  if(filename!=NULL)
  {
   char *txt;
   do
   {
    txt=strchr(filename,'.');
    printf("filename %s txt %p\n",filename,txt);
   }
   while((txt)&&(strlen(txt)>4));

   if(txt==NULL)
   {
    txt=filename;
    strcat(txt,".b16");
   }

   if(strstr(txt,"b16"))
   {
    store_b16(filename,w,h,0,picbuf);
    printf("b16 image saved to %s\n",filename);
   }
   else if(strstr(txt,"tif"))
   {
    store_tif(filename,w,h,0,picbuf);
    printf("tif image saved to %s\n",filename);
   }
  }
 }
 free(picbuf);

 return err;
}



/*
DWORD grab_single(CPco_grab_cl_me4 *grabber)
{
 int err;
 int picnum;
 unsigned int w,h;
 WORD *adr;
 WORD *linebuf;


 picnum=1;
 err=grabber->Start_Acquire(1);
 if(err!=PCO_NOERROR)
  printf("\ngrab_single Start_Acquire error 0x%x\n",err);

 if(err==PCO_NOERROR)
 {
  err=grabber->Wait_For_Next_Image(&picnum,10);
  if(err!=PCO_NOERROR)
   printf("\ngrab_single Wait_For_Image error 0x%x\n",err);
 }

 if(err==PCO_NOERROR)
 {
  err=grabber->Check_DMA_Length(picnum);
  if(err!=PCO_NOERROR)
   printf("\ngrab_single Check_DMA_Length error 0x%x\n",err);
 }

 if(err==PCO_NOERROR)
 {
  err=grabber->Get_Framebuffer_adr(picnum,(void**)&adr);
  if(err!=PCO_NOERROR)
   printf("\ngrab_single Get_Framebuffer_adr error 0x%x\n",err);
 }

 err=grabber->Stop_Acquire();
 if(err!=PCO_NOERROR)
  printf("\ngrab_single Stop_Acquire error 0x%x\n",err);

 if(err==PCO_NOERROR)
 {
  grabber->Get_actual_size(&w,&h,NULL);
  linebuf=(WORD*)malloc(w*sizeof(WORD));
  grabber->Get_Image_Line(linebuf,adr,1,w,h);
  picnum=image_nr_from_timestamp(linebuf,0);
  printf("\ngrab_single done successful, timestamp image_nr: %d\n",picnum);
  free(linebuf);
 }

 return err;
}

DWORD grab_single(CPco_grab_cl_me4 *grabber,char* filename)
{
 int err;
 int picnum;
 unsigned int w,h,l;
 WORD *adr;
 WORD *picbuf;

 picnum=1;
 err=grabber->Start_Acquire(1);
 if(err!=PCO_NOERROR)
  printf("\ngrab_single Start_Acquire error 0x%x\n",err);

 if(err==PCO_NOERROR)
 {
  err=grabber->Wait_For_Image(&picnum,10);
  if(err!=PCO_NOERROR)
   printf("\ngrab_single Wait_For_Image error 0x%x\n",err);
 }

 if(err==PCO_NOERROR)
 {
  err=grabber->Check_DMA_Length(picnum);
  if(err!=PCO_NOERROR)
   printf("\ngrab_single Check_DMA_Length error 0x%x\n",err);
 }

 if(err==PCO_NOERROR)
 {
  err=grabber->Get_Framebuffer_adr(picnum,(void**)&adr);
  if(err!=PCO_NOERROR)
   printf("\ngrab_single Get_Framebuffer_adr error 0x%x\n",err);
 }

 err=grabber->Stop_Acquire();
 if(err!=PCO_NOERROR)
  printf("\ngrab_single Stop_Acquire error 0x%x\n",err);

 if(err==PCO_NOERROR)
 {
  char *txt;
  int min,max,v;

  grabber->Get_actual_size(&w,&h,NULL);
  picbuf=(WORD*)malloc(w*h*sizeof(WORD));
  grabber->Extract_Image(picbuf,adr,w,h);

  picnum=image_nr_from_timestamp(picbuf,0);
  printf("grab_single done successful, timestamp image_nr: %d\n",picnum);
  max=0;
  min=0xFFFF;
  adr=(WORD*)picbuf;
  l=w*20; //skip first lines with timestamp
  for(;l<w*h;l++)
  {
   v=*(adr+l);
   if(v<min)
    min=v;
   if(v>max)
    max=v;
  }
  printf("grab_single pixels min_value: %d max_value %d\n",min,max);


  do
  {
   txt=strchr(filename,'.');
  }
  while(strlen(txt)>4);

  if((txt)&&(strstr(txt,"b16")))
  {
   store_b16(filename,w,h,0,picbuf);
   printf("b16 image saved to %s\n",filename);
  }
  else if((txt)&&(strstr(txt,"tif")))
  {
   store_tif(filename,w,h,0,picbuf);
   printf("tif image saved to %s\n",filename);
  }

  free(picbuf);
 }

 return err;
}
*/


DWORD grab_count(CPco_grab_cl_me4 *grabber,int ima_count)
{
 int err;
 int firstnum,lastnum,picnum;
 unsigned int width,height;
 WORD *adr;
 WORD *linebuf;
 double time,freq;

 grabber->Get_actual_size(&width,&height,NULL);
 linebuf=(WORD*)malloc(width*sizeof(WORD));
 if(linebuf==NULL)
 {
  printf("\ngrab_count cannot allocate buffer\n");
  return PCO_ERROR_NOMEMORY | PCO_ERROR_APPLICATION;
 }

 picnum=1;
 lastnum=firstnum=0;

 err=grabber->Start_Acquire_NonBlock(ima_count);
 if(err!=PCO_NOERROR)
  printf("\ngrab_count Start_Acquire error 0x%x\n",err);

 if(err==PCO_NOERROR)
 {
  err=grabber->Wait_For_Next_Image(&picnum,10);
  mylog.start_time_mess();
  if(err!=PCO_NOERROR)
   printf("\ngrab_count Error waiting for first image picnum %d\n",picnum);
 }

 if(err==PCO_NOERROR)
 {
  err=grabber->Get_Framebuffer_adr(picnum,(void**)&adr);
  if(err!=PCO_NOERROR)
   printf("\ngrab_single Get_Framebuffer_adr error 0x%x\n",err);
 }

 if(err==PCO_NOERROR)
 {
  grabber->Get_Image_Line(linebuf,adr,1,width,height);
  firstnum=image_nr_from_timestamp(linebuf,0);
 }

 if(err==PCO_NOERROR)
 {
  printf("wait until %d images are grabbed waittime %ds",ima_count,ima_count*1);
  fflush(stdout);
  err=grabber->Wait_For_Images(ima_count,1*ima_count);
  if(err!=PCO_NOERROR)
  {
   printf("\ngrab_count Error waiting for last image\n");
   picnum=0;
  }
  else
   picnum=ima_count;
 }


 time=mylog.stop_time_mess();

 for(int x=1;x<=ima_count;x++)
 {
  err=grabber->Check_DMA_Length(x);
  if(err!=PCO_NOERROR)
   printf("\ngrab_count Check_DMA_Length error 0x%x at image number %d\n",err,x);
 }


 err=grabber->Stop_Acquire();
 if(err!=PCO_NOERROR)
  printf("\ngrab_count Stop_Acquire error 0x%x\n",err);

 if(picnum>0)
 {
  err=grabber->Get_Framebuffer_adr(picnum,(void**)&adr);
  if(err!=PCO_NOERROR)
   printf("\ngrab_count Get_Framebuffer_adr error 0x%x\n",err);

  grabber->Get_Image_Line(linebuf,adr,1,width,height);
  lastnum=image_nr_from_timestamp(linebuf,0);
  printf("\ngrab_count done successful");

  if(err==PCO_NOERROR)
  {
   freq=(ima_count-1)*1000;
   freq/=time;
   printf("\n%05d images grabbed time %dms freq: %.2fHz %.2fMB/sec",ima_count,(int)time,freq,(freq*width*height*2/(1024*1024)));

   freq=(lastnum-firstnum)*1000;
   freq/=time;
   printf("\nImagenumbers: first %d last %d count %d freq: %.2fHz",firstnum,lastnum,lastnum-firstnum+1,freq);
  }
 }

 free(linebuf);
 return err;
}

DWORD grab_loop(CPco_grab_cl_me4 *grabber,int ima_count)
{
 int picnum,err,imanum;
 unsigned int width,height;
 void *adr;
 WORD *linebuf;
 WORD *bufadr;
 int firstnum,lastnum,currentnum;
 int numoff;
 double time,freq;

 time=freq=1;

 grabber->Get_actual_size(&width,&height,NULL);
 linebuf=(WORD*)malloc(width*sizeof(WORD));
 if(linebuf==NULL)
 {
  printf("\ngrab_loop cannot allocate buffer\n");
  return PCO_ERROR_NOMEMORY | PCO_ERROR_APPLICATION;
 }
 bufadr=(WORD*)malloc(width*height*sizeof(WORD));
 if(bufadr==NULL)
 {
  printf("\ngrab_loop cannot allocate buffer\n");
  free(linebuf); 
  return PCO_ERROR_NOMEMORY | PCO_ERROR_APPLICATION;
 }

 err=PCO_NOERROR;

 numoff=firstnum=lastnum=currentnum=0;
 picnum=1;
 mylog.start_time_mess();
 grabber->Start_Acquire(ima_count);
 if(err!=PCO_NOERROR)
 {
  printf("\ngrab_loop Start_Acquire error 0x%x",err);
  free(bufadr);
  free(linebuf);
  return err;
 }

 imanum=0;
 do
 {
  err=grabber->Wait_For_Next_Image(&picnum,10);
  if(err!=PCO_NOERROR)
   printf("\ngrab_loop Error while waiting for image number %d",imanum+1);

  if(err==PCO_NOERROR)
  {
   err=grabber->Check_DMA_Length(picnum);
   if(err!=PCO_NOERROR)
    printf("\ngrab_loop Check_DMA_Length error 0x%x",err);
  }
  if(err!=PCO_NOERROR)
  {
   printf("\ngrab_loop Error break loop at image number %d",imanum+1);
   break;
  }
  imanum++;

  if(imanum==1)
   mylog.start_time_mess();

  err=grabber->Get_Framebuffer_adr(picnum,&adr);
  if(err!=PCO_NOERROR)
   printf("\ngrab_loop Get_Framebuffer_adr(%d,) error",picnum);

  if(err==PCO_NOERROR)
  {
   grabber->Extract_Image(bufadr,adr,width,height);
   currentnum=image_nr_from_timestamp(bufadr,0);
//  grabber->Get_Image_Line(linebuf,adr,1,width,height);
//  currentnum=image_nr_from_timestamp(linebuf,0);
  }
  if(imanum==1)
   firstnum=currentnum;

  err=grabber->Unblock_buffer(picnum); 
  if(err!=PCO_NOERROR)
   printf("\ngrab_loop Unblock_buffer error 0x%x\n",err);

  printf("imanum %06d picnum %03d currentnum %06d %06d\r",imanum,picnum,currentnum,currentnum-firstnum+1);
  if(ima_count<=40)
   printf("\n");
  fflush(stdout);

//for testing only lost images can also be seen at summary output
  if((currentnum-firstnum-numoff+1)!=imanum)
  {
   numoff++;
//   printf("\ngrab_loop Error number: camera %d grabber %d offset now %d\n",currentnum-firstnum+1,imanum,numoff);
  }

 }while(imanum<ima_count-1);


 if(err==PCO_NOERROR)
 {
  picnum=ima_count;
  err=grabber->Wait_For_Next_Image(&picnum,10);
  if(err!=PCO_NOERROR)
   printf("\ngrab_loop Error while waiting for image number %d",picnum);
  else
   imanum++; 

  time=mylog.stop_time_mess();

  if(err==PCO_NOERROR)
  {
   err=grabber->Check_DMA_Length(picnum);
   if(err!=PCO_NOERROR)
    printf("\ngrab_loop Check_DMA_Length error 0x%x",err);
  }

  err=grabber->Get_Framebuffer_adr(picnum,&adr);
  if(err!=PCO_NOERROR)
   printf("\ngrab_loop Get_Framebuffer_adr(%d,) error",picnum);

  if(err==PCO_NOERROR)
  {
   grabber->Extract_Image(bufadr,adr,width,height);
   grabber->Get_Image_Line(linebuf,adr,1,width,height);
   currentnum=image_nr_from_timestamp(linebuf,0);
   grabber->Unblock_buffer(picnum);
  }
  printf("imanum %06d picnum %03d currentnum %06d %06d\r",imanum,picnum,currentnum,currentnum-firstnum+1);
  lastnum=currentnum; 
 }

 if(grabber->Stop_Acquire()!=PCO_NOERROR)
  printf("\ngrab_loop Stop_Acquire error \n");

 printf("\n");

 if(err==PCO_NOERROR)
 {
  printf("Grab %d images done (picnum %d)\n",ima_count,picnum);
  freq=(imanum-1)*1000;
  freq/=time;
  printf("%06d images grabbed time %dms freq: %.2fHz %.2fMB/sec ",imanum,(int)time,freq,(freq*width*height*2)/(1024*1024));
  switch((grabber->Get_DataFormat()&0xFF))
  {
   case PCO_CL_DATAFORMAT_5x16:
    printf("\n");
    break;

   case PCO_CL_DATAFORMAT_5x12:
   case PCO_CL_DATAFORMAT_5x12L:
   case PCO_CL_DATAFORMAT_5x12R:
    printf(" real %.2fMB/sec\n",((freq*width*height*2*12)/16)/(1024*1024));
    break;
  }
  freq=(lastnum-firstnum)*1000;
  freq/=time;
  printf("Imagenumbers: first %d last %d count %d freq: %.2fHz\n",firstnum,lastnum,lastnum-firstnum+1,freq);
 }

 free(bufadr);
 free(linebuf);
 return PCO_NOERROR;
}


int image_nr_from_timestamp(void *buf,int shift)
{
  unsigned short *b;
  int y;
  int image_nr=0;
  b=(unsigned short *)(buf);
  y=100*100*100;
  for(;y>0;y/=100)
  {
   *b>>=shift;
   image_nr+= (((*b&0x00F0)>>4)*10 + (*b&0x000F))*y;
   b++;
  }
  return image_nr;
}


void get_number(char *number,int len)
{
   int ret_val;
   int x=0;

   while(((ret_val=getchar())!=10)&&(x<len-1))
   {
    if(isdigit(ret_val))
     number[x++]=ret_val;
   }
   number[x]=0;
}

void get_text(char *text,int len)
{
   int ret_val;
   int x=0;

   while(((ret_val=getchar())!=10)&&(x<len-1))
   {
    if(isprint(ret_val))
     text[x++]=ret_val;
   }
   text[x]=0;
}


void get_hexnumber(int *num,int len)
{
  int ret_val;
  int cmd=0;
  while(((ret_val=getchar())!=10)&&(len > 0))
  {
   if(isxdigit(ret_val))
   {
    if(ret_val<0x3A)
     cmd=(ret_val-0x30)+cmd*0x10;
    else if(ret_val<0x47)
     cmd=(ret_val-0x41+0x0A)+cmd*0x10;
    else
     cmd=(ret_val-0x61+0x0A)+cmd*0x10;
    len--;
   }
  }
  *num=cmd;
}

