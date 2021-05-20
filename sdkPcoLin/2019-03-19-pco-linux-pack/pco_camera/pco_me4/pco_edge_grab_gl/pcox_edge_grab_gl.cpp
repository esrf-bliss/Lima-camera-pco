//-----------------------------------------------------------------//
// Name        | pcox_edge_grab_gl.cpp       | Type: (*) source    //
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
//             | images from camera pco.edge Global Shutter        //
//             | with the Me4 grabber and display them             //
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
//  1.03     | 24.01.2012 | new for pco.edge Global Shutter        //
//           |            | use pcodisp library                    //
//           |            | use pcolog library                     //
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

#include "pco_includes.h"
#include "Cpco_com_cl_me4.h"
#include "Cpco_grab_cl_me4_GL.h"
#include "file12.h"

#include "Ccambuf.h"
#include "Cpcodisp.h"

int image_nr_from_timestamp(void *buf,int shift);
DWORD grab_single(CPco_grab_cl_me4 *grabber,void *picbuf);
DWORD grab_count(CPco_grab_cl_me4 *grabber,int ima_count,CPCODisp *Cdispwin,CCambuf *Cbuf);
DWORD grab_loop(CPco_grab_cl_me4 *grabber,int ima_count,CPCODisp *Cdispwin,CCambuf *Cbuf);

void get_number(char *number,int len);
void get_text(char *text,int len);
void get_hexnumber(int *num,int len);

CPco_Log mylog("pcox_edge_grab_gl.log");

const char tb[3][3]={"ns","us","ms"};


int main(int argc, char* argv[])
{
  DWORD err;
  CPco_com *camera;
  CPco_grab_cl_me4 *grabber;

  int board=0;
  char number[20];
  char infostr[100];
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
  int bufnum=20;
  double freq;
  SHORT ccdtemp,camtemp,pstemp;
  WORD camtype;
  DWORD serialnumber;

  int bwmin,bwmax;
  CCambuf Cbuf;
  CPCODisp *Cdispwin=NULL;


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

  if(camtype==CAMERATYPE_PCO_EDGE_GL)
  {
   printf("Grabber is CPco_grab_cl_me4_edge\n");
   grabber=new CPco_grab_cl_me4_edge_GL((CPco_com_cl_me4*)camera);
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
   printf("Camera Name is: %s\n",infostr);

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
   printf("PCO_GetTransferParameter() Error 0x%x\n",err);
  else
  {
   printf("current temparatures\n");
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

//set RecordingState to STOP
  err=camera->PCO_SetRecordingState(0);
  if(err!=PCO_NOERROR)
   printf("PCO_SetRecordingState() Error 0x%x\n",err);

  err=camera->PCO_SetTimestampMode(2);
  if(err!=PCO_NOERROR)
   printf("PCO_SetTimestampMode() Error 0x%x\n",err);

//set camera timebase to us
  exp_timebase=del_timebase=1;
  err=camera->PCO_SetTimebase(exp_timebase,del_timebase);
  if(err!=PCO_NOERROR)
   printf("PCO_SetTimebase() Error 0x%x\n",err);

  exp_time=5000;
  delay_time=1000;
  err=camera->PCO_SetDelayExposure(delay_time,exp_time);
  if(err!=PCO_NOERROR)
   printf("PCO_SetDelayExposure() Error 0x%x\n",err);

//transfer dataformat must not be changed for the pco.edge Global Shutter camera
  if(clpar.DataFormat!=(SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER|PCO_CL_DATAFORMAT_5x12))
  {
   clpar.DataFormat=SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER|PCO_CL_DATAFORMAT_5x12;
   err=camera->PCO_SetTransferParameter(&clpar,sizeof(clpar));
   if(err!=PCO_NOERROR)
    printf("PCO_TransferParameter() Error 0x%x\n",err);
  }

//prepare Camera for recording
  err=camera->PCO_ArmCamera();
  if(err!=PCO_NOERROR)
   printf("PCO_ArmCamera() Error 0x%x\n",err);

  err=camera->PCO_GetActualSize(&width,&height);
  if(err!=PCO_NOERROR)
   printf("PCO_GetActualSize() Error 0x%x\n",err);

  printf("Actual Resolution %d x %d\n",width,height);

#ifdef USERSET
  WORD val;

  if(description.wColorPatternTyp)
   grabber->Set_Grabber_Param(ID_COLORSENSOR,1);
  else
   grabber->Set_Grabber_Param(ID_COLORSENSOR,0);

  err=camera->PCO_GetNoiseFilterMode(&val);
  if(err!=PCO_NOERROR)
   printf("PCO_GetNoiseFilterMode() Error 0x%x\n",err);
  else
   grabber->Set_Grabber_Param(ID_NOISEFILTER,val);

  err=camera->PCO_GetTimestampMode(&val);
  if(err!=PCO_NOERROR)
   printf("PCO_SetTimestampMode() Error 0x%x\n",err);
  else
   grabber->Set_Grabber_Param(ID_TIMESTAMP,val);

  err=camera->PCO_GetDoubleImageMode(&val); 
  if(err!=PCO_NOERROR)
   printf("PCO_SetTimestampMode() Error 0x%x\n",err);
  else
   grabber->Set_Grabber_Param(ID_DOUBLEMODE,val);

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

  err=camera->PCO_SetRecordingState(1);
  if(err!=PCO_NOERROR)
   printf("PCO_SetRecordingState() Error 0x%x\n",err);

  Cdispwin= new CPCODisp;

  Cbuf.Allocate(width,height,16,0,IN_BW);
  sprintf(infostr,"edge GS size %dx%d",width,height);

  if(Cdispwin->initialize((char*)infostr)!=PCO_NOERROR)
  {
   delete Cdispwin;
   Cdispwin=NULL;
  }

  if(Cdispwin)
  {
   Cdispwin->Set_Actual_pic(&Cbuf);
   bwmin=300;
   bwmax=10000;
   Cdispwin->SetConvert(bwmin,bwmax);
   Cdispwin->convert();
  }


  c=' ';
  while(c!='x')
  {
   int ch;
   c=' ';

   printf("\n");
   camera->PCO_GetRecordingState(&act_recstate);
   camera->PCO_GetDelayExposure(&delay_time,&exp_time);
   camera->PCO_GetCOCRuntime(&secs,&nsecs);
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
   printf("v to set convert values          min             %d\n",bwmin);
   printf("                                 max             %d\n",bwmax);

//   printf("f to set Dataformat 0x%x and LUT 0x%x\n",clpar.DataFormat,actlut);
   printf("0 to set recording state to OFF\n");
   printf("1 to set recording state to ON\n");
   printf("2 Single Grab\n");
   printf("3 Start Grab in allocated buffers (No display while grabbing)\n");
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
    get_number(number,4);
    if(strlen(number))
    {
     int i;
     i=atoi(number);
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
    printf("enter picture timeout ...<CR>: ");
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
   else if(c=='v')
   {
    printf("enter convert values\n min: ...<CR>: ");
    get_number(number,10);
    if(strlen(number))
     bwmin=atoi(number);
    printf(" max: ...<CR>: ");
    get_number(number,10);
    if(strlen(number))
     bwmax=atoi(number);
    Cdispwin->SetConvert(bwmin,bwmax);
    Cdispwin->convert();
   }
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
     if(grab_single(grabber,Cbuf.Get_actadr())==PCO_NOERROR)
      Cdispwin->convert();
    }
    printf("\n");
   }
   else if(c=='3')
   {
    if(act_recstate==0)
     printf("\nStart Camera before grabbing\n");
    else
     grab_count(grabber,bufnum,Cdispwin,&Cbuf);
    printf("\n");
   }
   else if(c=='4')
   {
    if(act_recstate==0)
     printf("\nStart Camera before grabbing\n");
    else
     grab_loop(grabber,ima_count,Cdispwin,&Cbuf);
    printf("\n");
   }
  }

  Cbuf.FreeBuffer();

  delete Cdispwin;
  Cdispwin=NULL;

  err=grabber->Free_Framebuffer();

  grabber->Close_Grabber();
  delete grabber;

  camera->Close_Cam();
  delete camera;

  return 0;
}

DWORD grab_single(CPco_grab_cl_me4 *grabber,void *picbuf)
{
 int err;
 unsigned int w,h,l;
 int ima_num; 
 
 grabber->Get_actual_size(&w,&h,NULL);
  
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
 }

 return err;
}


/*
DWORD grab_single(CPco_grab_cl_me4 *grabber,void *picbuf)
{
 int err;
 int picnum;
 unsigned int w,h,l;
 int min,max,v;
 WORD *adr;
 WORD *linebuf;


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
  grabber->Get_actual_size(&w,&h,NULL);
  linebuf=(WORD*)malloc(w*sizeof(WORD));
  memset(linebuf,0,w*sizeof(WORD));
  grabber->Get_Image_Line(linebuf,adr,1,w,h);

  picnum=image_nr_from_timestamp(linebuf,0);
  printf("grab_single done successful, timestamp image_nr: %d\n",picnum);

  grabber->Extract_Image(picbuf,adr,w,h);
  free(linebuf);

  max=0;
  min=0xFFFF;
  adr=(WORD*)picbuf;
  l=w*20; //skip first line with timestamp
  for(;l<w*h;l++)
  {
   v=*(adr+l);
   if(v<min)
    min=v;
   if(v>max)
    max=v;
  }
  printf("grab_single pixels min_value: %d max_value %d\n",min,max);
 }

 return err;
}
*/
DWORD grab_count(CPco_grab_cl_me4 *grabber,int ima_count,CPCODisp *Cdispwin,CCambuf *Cbuf)
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
   printf("\ngrab_loop Check_DMA_Length error 0x%x at image number %d\n",err,x);
 }

 err=grabber->Stop_Acquire();
 if(err!=PCO_NOERROR)
  printf("\ngrab_count Stop_Acquire error 0x%x\n",err);


 if(picnum>0)
 {
  char number[20];

  err=grabber->Get_Framebuffer_adr(picnum,(void**)&adr);
  if(err!=PCO_NOERROR)
   printf("\ngrab_count Get_Framebuffer_adr error 0x%x\n",err);

  grabber->Get_Image_Line(linebuf,adr,1,width,height);
  lastnum=image_nr_from_timestamp(linebuf,0);

  printf("\ngrab_count done successful");

  freq=(ima_count-1)*1000;
  freq/=time;
  printf("\n%05d images grabbed time %dms freq: %.2fHz %.2fMB/sec",ima_count,(int)time,freq,(freq*width*height*2/(1024*1024)));

  freq=(lastnum-firstnum)*1000;
  freq/=time;
  printf("\nImagenumbers: first %d last %d count %d freq: %.2fHz",firstnum,lastnum,lastnum-firstnum+1,freq);


  printf("\nShow images 1 to %d ('0' to exit)\n",ima_count);
  while(TRUE)
  {
   printf("Enter imagenumber ...<CR>: ");
   get_number(number,6);
   picnum=atoi(number);
   if(picnum<=0)
    break;
   err=grabber->Get_Framebuffer_adr(picnum,(void**)&adr);
   if(err==PCO_NOERROR)
   {
    void *bufadr=Cbuf->Get_actadr();
    grabber->Extract_Image(bufadr,adr,width,height);
    Cdispwin->convert();
   }
  }
 }

 free(linebuf);
 return err;
}



DWORD grab_loop(CPco_grab_cl_me4 *grabber,int ima_count,CPCODisp *Cdispwin,CCambuf *Cbuf)
{
 int picnum,err,imanum;
 unsigned int width,height;
 void *adr;
 WORD *linebuf;
 void *bufadr;
 int firstnum,lastnum,currentnum;
 int numoff;
 double time,freq;

 time=freq=1;


 grabber->Get_actual_size(&width,&height,NULL);
 linebuf=(WORD*)malloc(width*sizeof(WORD)*2);
 if(linebuf==NULL)
 {
  printf("\ngrab_count cannot allocate buffer\n");
  return PCO_ERROR_NOMEMORY | PCO_ERROR_APPLICATION;
 }

 err=PCO_NOERROR;

 numoff=firstnum=lastnum=currentnum=0;
 picnum=1;

 bufadr=Cbuf->Get_actadr();

 mylog.start_time_mess();
 grabber->Start_Acquire(ima_count);
 if(err!=PCO_NOERROR)
 {
  printf("\ngrab_loop Start_Acquire error 0x%x\n",err);
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
   grabber->Get_Image_Line(linebuf,adr,1,width,height);
   currentnum=image_nr_from_timestamp(linebuf,0);

   grabber->Extract_Image(bufadr,adr,width,height);
   currentnum=image_nr_from_timestamp(bufadr,0);
  }

  if(imanum==1)
   firstnum=currentnum;

  err=grabber->Unblock_buffer(picnum); 
  if(err!=PCO_NOERROR)
   printf("\ngrab_loop Unblock_buffer error 0x%x\n",err);

  Cdispwin->convert();

  printf("imanum %06d picnum %03d currentnum %06d %06d\r",imanum,picnum,currentnum,currentnum-firstnum+1);
  if(ima_count<=20)
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
   currentnum=image_nr_from_timestamp(bufadr,0);
  }
  grabber->Unblock_buffer(picnum); 
  printf("imanum %06d picnum %03d currentnum %06d %06d\r",imanum,picnum,currentnum,currentnum-firstnum+1);
  lastnum=currentnum;
 }

 if(grabber->Stop_Acquire()!=PCO_NOERROR)
  printf("\ngrab_loop Stop_Acquire error");

 printf("\n");

 if(err==PCO_NOERROR)
 {
  printf("Grab %d images done (picnum %d)\n",ima_count,picnum);

  freq=(imanum-1)*1000;
  freq/=time;
  printf("%05d images grabbed time %dms freq: %.2fHz %.2fMB/sec \n",imanum,(int)time,freq,(freq*width*height*2)/(1024*1024));

  freq=(lastnum-firstnum)*1000;
  freq/=time;
  printf("Imagenumbers: first %d last %d count %d freq: %.2fHz\n",firstnum,lastnum,lastnum-firstnum+1,freq);

  Cdispwin->convert();
 }

 free(linebuf);
 return PCO_NOERROR;
}

/*
DWORD grab_count(CPco_grab_cl_me4 *grabber,int ima_count,CPCODisp *Cdispwin,CCambuf *Cbuf)
{
 int err;
 int firstnum,lastnum;
 unsigned int width,height;
 WORD *adr;
 WORD *linebuf;
 double time,freq;

 grabber->Get_actual_size(&width,&height,NULL);
 linebuf=(WORD*)malloc(width*sizeof(WORD));

 firstnum=1;
 lastnum=ima_count;

 err=grabber->Start_Acquire(ima_count);
 if(err!=PCO_NOERROR)
  printf("\ngrab_count Start_Acquire error 0x%x\n",err);

 if(err==PCO_NOERROR)
 {
  err=grabber->Wait_For_Image(&firstnum,10);
  mylog.start_time_mess();
  if(err!=PCO_NOERROR)
   printf("\ngrab_count Error waiting for first image number %d\n",firstnum);
 }

 if(err==PCO_NOERROR)
 {
  err=grabber->Get_Framebuffer_adr(firstnum,(void**)&adr);
  if(err!=PCO_NOERROR)
   printf("\ngrab_count Get_Framebuffer_adr error 0x%x\n",err);
 }

 if(err==PCO_NOERROR)
 {
  grabber->Get_Image_Line(linebuf,adr,1,width,height);
  firstnum=image_nr_from_timestamp(linebuf,0);
  printf("wait until %d images are grabbed waittime %dms",ima_count,ima_count*1000);
  fflush(stdout);
  err=grabber->Wait_For_Image(&lastnum,1*ima_count);
  if(err!=PCO_NOERROR)
   printf("\ngrab_count Error waiting for last image picnum %d\n",lastnum);
 }

 time=mylog.stop_time_mess();

 for(int x=1;x<=ima_count;x++)
 {
  err=grabber->Check_DMA_Length(x);
  if(err!=PCO_NOERROR)
   printf("\ngrab_loop Check_DMA_Length error 0x%x at image number %d\n",err,x);
 }

 err=grabber->Stop_Acquire();
 if(err!=PCO_NOERROR)
  printf("\ngrab_count Stop_Acquire error 0x%x\n",err);

 err=grabber->Get_Framebuffer_adr(lastnum,(void**)&adr);
 if(err!=PCO_NOERROR)
  printf("\ngrab_count Get_Framebuffer_adr error 0x%x\n",err);

 if(err==PCO_NOERROR)
 {
  char number[20];
  grabber->Get_Image_Line(linebuf,adr,1,width,height);
  lastnum=image_nr_from_timestamp(linebuf,0);

  printf("\ngrab_count done successful");

  freq=(ima_count-1)*1000;
  freq/=time;
  printf("\n%05d images grabbed time %dms freq: %.2fHz %.2fMB/sec",ima_count,(int)time,freq,(freq*width*height*2/(1024*1024)));

  freq=(lastnum-firstnum)*1000;
  freq/=time;
  printf("\nImagenumbers: first %d last %d count %d freq: %.2fHz",firstnum,lastnum,lastnum-firstnum+1,freq);

  printf("\nShow images 1 to %d ('0' to exit)\n",ima_count);
  while(TRUE)
  {
   printf("Enter imagenumber ...<CR>: ");
   get_number(number,6);
   lastnum=atoi(number);
   if(lastnum<=0)
    break;
   err=grabber->Get_Framebuffer_adr(lastnum,(void**)&adr);
   if(err==PCO_NOERROR)
   {
    grabber->Extract_Image(Cbuf->Get_actadr(),adr,width,height);
    Cdispwin->convert();
   }
  }
 }

 free(linebuf);
 return err;
}

DWORD grab_loop(CPco_grab_cl_me4 *grabber,int ima_count,CPCODisp *Cdispwin,CCambuf *Cbuf)
{
 int picnum,err;
 unsigned int width,height;
 void *adr;
 WORD *linebuf;
 int firstnum,lastnum,currentnum;
 int numoff;
 double time,freq;

 time=freq=1;

 grabber->Get_actual_size(&width,&height,NULL);
 linebuf=(WORD*)malloc(width*sizeof(WORD));

 err=PCO_NOERROR;

 numoff=0;
 picnum=firstnum=1;
 mylog.start_time_mess();
 grabber->Start_Acquire(ima_count);
 if(err!=PCO_NOERROR)
 {
  printf("\ngrab_loop Start_Acquire error 0x%x",err);
  free(linebuf);
  return err;
 }

 do
 {
  err=grabber->Wait_For_Image(&picnum,10);
  if(err!=PCO_NOERROR)
   printf("\ngrab_loop Error while waiting for image number %d",picnum);
  if(picnum==1)
   mylog.start_time_mess();

  if(err==PCO_NOERROR)
  {
   err=grabber->Check_DMA_Length(picnum);
   if(err!=PCO_NOERROR)
    printf("\ngrab_loop Check_DMA_Length %d error 0x%x",picnum,err);
  }

  if(err!=PCO_NOERROR)
  {
   printf("\ngrab_loop Error break loop at image number %d",picnum);
   break;
  }


  grabber->Get_Framebuffer_adr(picnum,&adr);
  grabber->Get_Image_Line(linebuf,adr,1,width,height);
  currentnum=image_nr_from_timestamp(linebuf,0);
  if(picnum==1)
   firstnum=currentnum;
  grabber->Extract_Image(Cbuf->Get_actadr(),adr,width,height);
  Cdispwin->convert();

  printf("picnum %d currentnum %d %d \r",picnum,currentnum,currentnum-firstnum+1);

  if((currentnum-firstnum-numoff+1)!=picnum)
  {
   numoff++;
   printf("\ngrab_loop Error number: camera %d grabber %d offset now %d\n",currentnum-firstnum+1,picnum,numoff);
  }
  picnum++;
  fflush(stdout);
 }while(picnum<ima_count);

 if(err==PCO_NOERROR)
 {
  picnum=ima_count;
  err=grabber->Wait_For_Image(&picnum,10);
  if(err!=PCO_NOERROR)
   printf("\ngrab_loop Error while waiting for image number %d",picnum);
  time=mylog.stop_time_mess();
  if(err==PCO_NOERROR)
  {
   err=grabber->Check_DMA_Length(picnum);
   if(err!=PCO_NOERROR)
    printf("\ngrab_loop Check_DMA_Length error 0x%x",err);
  }
 }

 if(grabber->Stop_Acquire()!=PCO_NOERROR)
  printf("\ngrab_loop Stop_Acquire error");

 printf("\n");

 if(err==PCO_NOERROR)
 {
  printf("Grab %d Images done (picnum %d)\n",ima_count,picnum);
  grabber->Get_Framebuffer_adr(picnum,&adr);
  grabber->Get_Image_Line(linebuf,adr,1,width,height);
  lastnum=image_nr_from_timestamp(linebuf,0);
  freq=(picnum-1)*1000;
  freq/=time;
  printf("%05d images grabbed time %dms freq: %.2fHz %.2fMB/sec \n",picnum,(int)time,freq,(freq*width*height*2)/(1024*1024));

  freq=(lastnum-firstnum)*1000;
  freq/=time;
  printf("Imagenumbers: first %d last %d count %d freq: %.2fHz\n",firstnum,lastnum,lastnum-firstnum+1,freq);

  grabber->Extract_Image(Cbuf->Get_actadr(),adr,width,height);
  Cdispwin->convert();
 }

 fflush(stdout);

 free(linebuf);
 return PCO_NOERROR;
}
*/

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
    if(isprint(ret_val))
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

