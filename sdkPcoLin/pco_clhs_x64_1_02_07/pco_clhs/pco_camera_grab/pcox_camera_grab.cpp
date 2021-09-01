//-----------------------------------------------------------------//
// Name        | pcox_camera_grab.cpp        | Type: (*) source    //
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
// Revision    | rev. 1.011                                        //
//-----------------------------------------------------------------//
// Notes       | main console program to show how to use           //
//             | class Cpco_camera and Cpco_cl_com to grab         //
//             | images from clhs pco.camera                        //
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
//  1.07     | 25.02.2014 | new for pco usb cameras                //
//           |            | use pcodisp library                    //
//           |            | use pcolog library                     //
//           |            |                                        //
//-----------------------------------------------------------------//
//  1.11     | 16.12.2015 | use new class functions                //
//           |            | more commands                          //
//           |            |                                        //
//-----------------------------------------------------------------//
//  0.0x     | xx.xx.2012 |                                        //
//           |            |                                        //
//-----------------------------------------------------------------//


#include <iostream>

#include "VersionNo.h"

#include "Cpco_com.h"
#include "Cpco_com_clhs.h"
#include "Cpco_grab_clhs.h"
#include "file12.h"
#include "Ccambuf.h"
#include "Cpcodisp.h"

#define BUFNUM 4


int image_nr_from_timestamp(void* buf,int shift);
DWORD grab_single(CPco_grab_clhs* grabber,void* picbuf);
DWORD get_image(CPco_grab_clhs* grabber,void* picbuf,WORD Segment,DWORD ImageNr);
DWORD grab_count_single(CPco_grab_clhs* grabber,int ima_count,CPCODisp* Cdispwin,CCambuf* Cbuf);
DWORD grab_count_wait(CPco_grab_clhs* grabber,int ima_count,CPCODisp* Cdispwin,CCambuf* Cbuf);

void get_number(char* number,int len);
void get_text(char* text,int len);
void get_hexnumber(int* num,int len);

int shift;

CPco_Log mylog("pcox_camera_grab.log");
const char tb[3][3]={"ns","us","ms"};
const char tmode[4][20]={"Auto","SW-Trig","Ext-Exp. Start","Ext-Exp. Ctrl"};

int main(int argc, char* argv[])
{
  DWORD err;
  CPco_com* camera;
  CPco_grab_clhs* grabber;

  int help=0;
  int board=0;
  char infostr[100];
  char number[20];

  int x;
  char c;
  int ima_count=100;
  int loop_count=1;
  int PicTimeOut=10000; //10 seconds

  WORD act_recstate,act_align;
  DWORD exp_time,delay_time,pixelrate;
  WORD exp_timebase,del_timebase;
  DWORD width,height,secs,nsecs;
  WORD triggermode;
  WORD binhorz,binvert;   
  WORD wRoiX0, wRoiY0, wRoiX1, wRoiY1;
  SC2_Camera_Description_Response description;
  double freq;
  SHORT ccdtemp,camtemp,pstemp;
  WORD camtype;
  DWORD serialnumber;
  int loglevel=0x0000F0FF;

  int bwmin,bwmax;
  CCambuf Cbuf[BUFNUM];
  CPCODisp* Cdispwin=NULL;


  for(int x=argc;x>1;)
  {
   char* a;

   x--;

   if(strstr(argv[x],"-h"))
    help=1;
   if(strstr(argv[x],"-?"))
    help=1;
   if(strstr(argv[x],"?"))
    help=1;

   if((a=strstr(argv[x],"-b")))
   {
    board=atoi(a+2);
   }
   if((a=strstr(argv[x],"-l")))
   {
    loglevel=strtol(a+2,NULL,0);
    printf("loglevel input 0x%x\n",loglevel);
   }
  }

  if(help)
  {
   printf("usage: %s options\n"
          "options: \n"
          "-b[0...7] actual camera number\n"
          "-l[0x0...] actual loglevel default 0x%x\n"
          "-h,-?,? this message\n",argv[0],loglevel);
   exit(0);
  }


  mylog.set_logbits(loglevel);
  printf("Logging set to 0x%08x\n",mylog.get_logbits());
  camera= new CPco_com_clhs();
  if(camera==NULL)
  {
   printf("ERROR: Cannot create camera object\n");
   printf("Any key CR to close  application\n");
   getchar();
   return -1;
  }

  if(loglevel>0)
   camera->SetLog(&mylog);

  printf("Try to open Camera\n");
  err=camera->Open_Cam(board);
  if(err!=PCO_NOERROR)
  {
   printf("ERROR: 0x%x in Open_Cam\n",err);
   printf("Any key CR to close  application\n");
   delete camera;

   getchar();
   return -1;
  }

  err=camera->PCO_GetCameraType(&camtype,&serialnumber);
  if(err!=PCO_NOERROR)
  {
   printf("ERROR: 0x%x in PCO_GetCameraType\n",err);
   printf("Any key CR to close  application\n");
   camera->Close_Cam();
   delete camera;

   getchar();
   return -1;
  }

//the clhs_grabber class supports all PCO clhs cameras

  printf("Grabber is CPco_grab_clhs\n");
  grabber=new CPco_grab_clhs((CPco_com_clhs*)camera);

  if(loglevel>0)
   grabber->SetLog(&mylog);

  printf("Try to open Grabber\n");
  err=grabber->Open_Grabber(board);
  if(err!=PCO_NOERROR)
  {
   printf("ERROR: 0x%x in Open_Grabber",err);
   printf("Any key CR to close  application\n");
   delete grabber;

   camera->Close_Cam();
   delete camera;

   getchar();
   return -1;
  }

  err=grabber->Set_Grabber_Timeout(PicTimeOut);
  if(err!=PCO_NOERROR)
   printf("error 0x%x in Set_Grabber_Timeout",err);

  err=camera->PCO_GetCameraDescriptor(&description);
  if(err!=PCO_NOERROR)
   printf("PCO_GetCameraDescriptor() Error 0x%x\n",err);

  err=camera->PCO_GetInfo(1,infostr,sizeof(infostr));
  if(err!=PCO_NOERROR)
   printf("PCO_GetInfo() Error 0x%x\n",err);
  else
  {
   printf("Camera Name is: %s\n",infostr);
   printf("Camera Typ is : 0x%04x\n",camtype);
   printf("Camera Serial : %d\n",serialnumber);
  }

  err=camera->PCO_SetCameraToCurrentTime();
  if(err!=PCO_NOERROR)
   printf("PCO_SetCameraToCurrentTime() Error 0x%x\n",err);


  err=camera->PCO_GetTemperature(&ccdtemp,&camtemp,&pstemp);
  if(err!=PCO_NOERROR)
   printf("PCO_GetTemperature() Error 0x%x\n",err);
  else
  {
   printf("current temperatures\n");
   printf("Camera:      %d°C\n",camtemp);
   if(ccdtemp != (SHORT)(-32768))
    printf("Sensor:      %d°C\n",ccdtemp);
   if(pstemp != (SHORT)(-32768))
    printf("PowerSupply: %d°C\n",pstemp);
  }


//set RecordingState to STOP
  err=camera->PCO_SetRecordingState(0);
  if(err!=PCO_NOERROR)
   printf("PCO_SetRecordingState() Error 0x%x\n",err);


//start from a known state
  err=camera->PCO_ResetSettingsToDefault();
  if(err!=PCO_NOERROR)
   printf("PCO_ResetSettingsToDefault() Error 0x%x\n",err);


  err=camera->PCO_SetTimestampMode(2);
  if(err!=PCO_NOERROR)
   printf("PCO_SetTimestampMode() Error 0x%x\n",err);

//set camera timebase to us
  exp_time    =5000;
  delay_time  =0;
  exp_timebase=1;
  del_timebase=1;

  err=camera->PCO_SetTimebase(del_timebase,exp_timebase);
  if(err!=PCO_NOERROR)
   printf("PCO_SetTimebase() Error 0x%x\n",err);

  err=camera->PCO_SetDelayExposure(delay_time,exp_time);
  if(err!=PCO_NOERROR)
   printf("PCO_SetDelayExposure() Error 0x%x\n",err);

  if(description.wNumADCsDESC>1)
  {
   err=camera->PCO_SetADCOperation(2);
   if(err!=PCO_NOERROR)
    printf("PCO_SetADCOperation() Error 0x%x\n",err);
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

  err=camera->PCO_SetBitAlignment(BIT_ALIGNMENT_LSB);
  if(err!=PCO_NOERROR)
   printf("PCO_SetBitAlignment() Error 0x%x\n",err);


//prepare Camera for recording
  err=camera->PCO_ArmCamera();
  if(err!=PCO_NOERROR)
   printf("PCO_ArmCamera() Error 0x%x\n",err);

  err=camera->PCO_GetBitAlignment(&act_align);
  if(err!=PCO_NOERROR)
   printf("PCO_GetBitAlignment() Error 0x%x\n",err);

  shift=0;
  if(act_align!=BIT_ALIGNMENT_LSB)
  {
   shift=16-description.wDynResDESC;
   printf("BitAlignment MSB shift %d\n",shift);
  }

  err=camera->PCO_GetTriggerMode(&triggermode);
  if(err!=PCO_NOERROR)
   printf("PCO_GetGetTriggermode() Error 0x%x\n",err);
  else
   printf("actual Triggermode: %d %s\n",triggermode,tmode[triggermode]);

  err=camera->PCO_GetBinning(&binhorz,&binvert);
  if(err!=PCO_NOERROR)
   printf("PCO_GetBinning() Error 0x%x\n",err);
  else
   printf("actual Binning: %dx%d\n",binhorz,binvert);
  
  err=camera->PCO_GetROI(&wRoiX0, &wRoiY0, &wRoiX1, &wRoiY1);
  if(err!=PCO_NOERROR)
   printf("PCO_GetROI() Error 0x%x\n",err);
  else
   printf("actual ROI: %d-%d, %d-%d\n",wRoiX0,wRoiX1,wRoiY0,wRoiY1);

  
  err=camera->PCO_GetActualSize(&width,&height);
  if(err!=PCO_NOERROR)
   printf("PCO_GetActualSize() Error 0x%x\n",err);

  printf("Actual Resolution %d x %d\n",width,height);

  err=grabber->PostArm();
  if(err!=PCO_NOERROR)
   printf("grabber->PostArm() Error 0x%x\n",err);

/*is done from PostArm()

  err=grabber->SetBitAlignment(act_align);
  err=grabber->Set_Grabber_Size(width,height);
  if(err!=PCO_NOERROR)
   printf("Set_Grabber_Size() Error 0x%x\n",err);

*/

  err=camera->PCO_SetRecordingState(1);
  if(err!=PCO_NOERROR)
   printf("PCO_SetRecordingState() Error 0x%x\n",err);

  for(int i=0;i<BUFNUM;i++)
  {
   Cbuf[i].Allocate(width,height,description.wDynResDESC,0,IN_BW);
   printf("Cbuf[%d] allocated width %d,height %d\n",i,Cbuf[i].Get_actwidth(),Cbuf[i].Get_actheight());
  }

  Cdispwin= new CPCODisp;
  printf("CPCODisp created\n");

  sprintf(infostr,"pco.camera size %dx%d",width,height);
  if(Cdispwin->initialize((char*)infostr)!=PCO_NOERROR)
  {
   delete Cdispwin;
   Cdispwin=NULL;
  }

  if(Cdispwin)
  {
   printf("CPCODisp Set_Actual_pic(&Cbuf[0]\n");
   Cdispwin->Set_Actual_pic(&Cbuf[0]);
   bwmin=200;
   bwmax=10000;
   Cdispwin->SetConvert(bwmin,bwmax);
   Cdispwin->convert();
  }

  fflush(stdout); 
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
   printf(" actual recording state %s actual freq: %.3lfHz time/pic: %.2lfms  datarate:%.2lfMB/sec \n",act_recstate ? "RUN" : "STOP",freq,1000/freq,(freq*width*height*2/(1024*1024)));
   printf("\n");
   printf("x to close camera and program   actual values\n");
   printf("l to set loop_count              loop_count      %d\n",loop_count);
   printf("c to set imagecount              imagecount      %d\n",ima_count);
   printf("t to set picture timeout         timeout         %dms\n",PicTimeOut);
   printf("e to set exposure time           exposuretime    %d%s\n",exp_time,tb[exp_timebase]);
   printf("d to set delay time              delaytime       %d%s\n",delay_time,tb[del_timebase]);
   printf("p to set camera pixelrate        pixelrate       %dHz\n",pixelrate);
   printf("a to set triggermode             triggermode     %s\n",tmode[triggermode]);
   printf("b to set binning                 binning         %dx%d\n",binhorz,binvert);
   printf("v to set convert values          min:            %d\n",bwmin);
   printf("                                 max             %d\n",bwmax);
   printf("\n");
   printf("0 to set recording state to OFF\n");
   printf("1 to set recording state to ON\n");
   printf("2 Single Grab\n");
   printf("3 Single Get_Image (recording camera or camera recorder buffer) \n");
   printf("4 Start Grab loop acquire (%d images)\n",ima_count);
   printf("5 Start Grab loop wait    (%d images)\n",ima_count);

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
   else if(c=='t')
   {
    printf("enter picture timeout ...<CR>: ");
    get_number(number,10);
    if(strlen(number))
    {
     PicTimeOut=atoi(number);
     err=grabber->Set_Grabber_Timeout(PicTimeOut);
     if(err!=PCO_NOERROR)
      printf("error 0x%x in Set_Grabber_Timeout",err);
    }
   }
   else if(c=='e')
   {
    int val=-1;
    int base=exp_timebase;
    printf("enter new exposure timebase 0=ns 1=us 2=ms [%1d]...<CR>: ",exp_timebase);
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
    printf("enter new exposure time in %s [%d]...<CR>: ",tb[base],exp_time);
    get_number(number,10);
    if((strlen(number))||(val>=0))
    {
     exp_time=atoi(number);
     if(act_recstate==1)
      camera->PCO_SetRecordingState(0);
     err=camera->PCO_SetDelayExposure(delay_time,exp_time);
     if(err!=PCO_NOERROR)
      printf("PCO_SetDelayExposure Error 0x%x\n",err);
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
    printf("enter new delay timebase 0=ns 1=us 2=ms [%1d]...<CR>: ",del_timebase);
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
    printf("enter new delay time in %s [%d]...<CR>: ",tb[base],delay_time);
    get_number(number,10);
    if((strlen(number))||(val>=0))
    {
     delay_time=atoi(number);
     if(act_recstate==1)
      camera->PCO_SetRecordingState(0);

     err=camera->PCO_SetDelayExposure(delay_time,exp_time);
     if(err!=PCO_NOERROR)
      printf("PCO_SetDelayExposure Error 0x%x\n",err);
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

     err=grabber->PostArm();
     if(err!=PCO_NOERROR)
      printf("grabber->PostArm() Error 0x%x\n",err);

     camera->PCO_GetPixelRate(&pixelrate);

     if(act_recstate==1)
      camera->PCO_SetRecordingState(1);
    }
   }
   else if(c=='a')
   {
    printf("enter new triggermode [0,2,3] ...<CR>: ");
    get_number(number,2);
    if(strlen(number))
    {
     WORD p;

     p=(WORD)atoi(number);
     if((p==1)||(p<0)||(p>3))
     {
      printf("Triggermode %d not supported\n",p);
      continue;
     }

     if(act_recstate==1)
      camera->PCO_SetRecordingState(0);

     err=camera->PCO_SetTriggerMode(p);
     if(err!=PCO_NOERROR)
      printf("PCO_SetTriggerMode() Error 0x%x\n",err);

     err=camera->PCO_ArmCamera();
     if(err!=PCO_NOERROR)
      printf("PCO_ArmCamera() Error 0x%x\n",err);

     camera->PCO_GetTriggerMode(&triggermode);

     if(act_recstate==1)
      camera->PCO_SetRecordingState(1);
    }
   }
   else if(c=='b')
   {
    WORD new_binhorz,new_binvert;
    WORD new_wRoiX0, new_wRoiY0, new_wRoiX1, new_wRoiY1;
    
    new_binhorz=binhorz;
    new_binvert=binvert;
    
    printf("enter new horizontal binning ...<CR>: ");
    get_number(number,2);
    if(strlen(number))
     new_binhorz=(WORD)atoi(number);
    printf("enter new vertical binning ...<CR>: ");
    get_number(number,2);
    if(strlen(number))
     new_binvert=(WORD)atoi(number);

    if(description.wBinHorzSteppingDESC==1) //linear
    {
     int i;   
     for(i = 1; i < description.wMaxBinHorzDESC; i++)
     {
      if(i==new_binhorz)
       break;   
     }
     if(i!=new_binhorz)
     {
      new_binhorz=binhorz;   
      printf("wrong input use horizontal binning %d\n",new_binhorz); 
     }
    }
    else //binary 0 or 2
    {
     int i;   
     for(i = 1; i < description.wMaxBinHorzDESC; i <<= 1 )
     {
      if(i==new_binhorz)
       break;   
     }
     if(i!=new_binhorz)
     {
      new_binhorz=binhorz;   
      printf("wrong input use horizontal binning %d\n",new_binhorz); 
     }
    }
    
    if(description.wBinVertSteppingDESC==1) //linear
    {
     int i;   
     for(i = 1; i < description.wMaxBinVertDESC; i++)
     {
      if(i==new_binvert)
       break;   
     }
     if(i!=new_binvert)
     {
      new_binvert=binvert;   
      printf("wrong input use vertical binning %d\n",new_binvert); 
     }
    }
    else //binary 0 or 2
    {
     int i;   
     for(i = 1; i < description.wMaxBinVertDESC; i <<= 1 )
     {
      if(i==new_binvert)
       break;   
     }
     if(i!=new_binvert)
     {
      new_binvert=binvert;
      printf("wrong input use vertical binning %d\n",new_binvert); 
     }
    }
    
    if((new_binhorz!=binhorz)||(new_binvert!=binvert))
    {
     if(act_recstate==1)
      camera->PCO_SetRecordingState(0);
     
     
     camera->PCO_GetROI(&wRoiX0,&wRoiY0,&wRoiX1,&wRoiY1);
     new_wRoiX0=wRoiX0;
     new_wRoiY0=wRoiY0;
     new_wRoiX1=wRoiX1;
     new_wRoiY1=wRoiY1;
     
     camera->PCO_SetBinning(new_binhorz,new_binvert);
     printf("Set binning %dx%d returned 0x%x\n",new_binhorz,new_binvert,err);

     if((description.wMaxHorzResStdDESC/new_binhorz)!=(wRoiX1-wRoiX0+1))
     {
      new_wRoiX0 = 1;
      new_wRoiX1 = description.wMaxHorzResStdDESC/new_binhorz;
     }
     if((description.wMaxVertResStdDESC/new_binvert)!=(wRoiY1-wRoiY0+1))
     {
      new_wRoiY0 = 1;
      new_wRoiY1 = description.wMaxVertResStdDESC/new_binvert;
     }
     camera->PCO_SetROI(new_wRoiX0, new_wRoiY0, new_wRoiX1, new_wRoiY1);
     printf("Set ROI %d-%d %d-%d returned 0x%x\n",new_wRoiX0, new_wRoiX1, new_wRoiY0, new_wRoiY1,err);
     
     err=camera->PCO_ArmCamera();
     if(err!=PCO_NOERROR)
     {
      printf("PCO_ArmCamera() Error 0x%x\n",err);
      printf("Set old values\n");
      camera->PCO_SetBinning(binhorz,binvert);
      camera->PCO_SetROI(wRoiX0,wRoiY0,wRoiX1,wRoiY1);
      err=camera->PCO_ArmCamera();
     }

     camera->PCO_GetBinning(&binhorz,&binvert);
     camera->PCO_GetROI(&wRoiX0,&wRoiY0,&wRoiX1,&wRoiY1);

     err=camera->PCO_GetActualSize(&width,&height);
     if(err!=PCO_NOERROR)
      printf("PCO_GetActualSize() Error 0x%x\n",err);

     printf("Actual Resolution %d x %d\n",width,height);

     err=grabber->PostArm();
     if(err!=PCO_NOERROR)
      printf("grabber->PostArm() Error 0x%x\n",err);
     
     if(act_recstate==1)
      camera->PCO_SetRecordingState(1);
     if(Cdispwin)
      Cdispwin->Set_Actual_pic(NULL);

     
     for(int i=0;i<BUFNUM;i++)
     {
      Cbuf[i].FreeBuffer();
      Cbuf[i].Allocate(width,height,description.wDynResDESC,0,IN_BW);
      Cbuf[i].Clearbuffer();
      printf("Cbuf[%d] allocated width %d,height %d\n",i,Cbuf[i].Get_actwidth(),Cbuf[i].Get_actheight());
     }

     if(Cdispwin)
     {
      printf("CPCODisp Set_Actual_pic(&Cbuf[0]\n");
      Cdispwin->Set_Actual_pic(&Cbuf[0]);
      Cdispwin->SetConvert(bwmin,bwmax);
      Cdispwin->convert();
     }
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
     Cdispwin->Set_Actual_pic(&Cbuf[0]);
     grab_single(grabber,Cbuf[0].Get_actadr());
     Cdispwin->convert();
    }
    printf("\n");
   }
   else if(c=='3')
   {
    int Segment,image_number;

    Segment=image_number=0;

    if(act_recstate==0)
    {
     DWORD dwValid,dwMax;

     Segment=image_number=1;
     if(description.dwGeneralCaps1&GENERALCAPS1_NO_RECORDER)
     {
      printf("camera does not support image readout from Segments\n");
      continue;
     }
     printf("enter Segment   [%01d]   ...<CR>: ",Segment);
     get_number(number,2);
     if(strlen(number))
      Segment=atoi(number);

     err=camera->PCO_GetNumberOfImagesInSegment(Segment,&dwValid,&dwMax);
     if(err!=PCO_NOERROR)
     {
      printf("no information available for Segment %d\n",Segment);
      continue;
     }


     printf("enter image number (valid %d max %d) ...<CR>: ",dwValid,dwMax);
     get_number(number,10);
     if(strlen(number))
      image_number=atoi(number);

     if(dwValid==0)
     {
      printf("no images available in Segment %d\n",Segment);
      continue;
     }


     if(image_number>(int)dwValid)
     {
      printf("try again with valid image number\n");
      continue;
     }
    }
    get_image(grabber,Cbuf[0].Get_actadr(),Segment,image_number);
    Cdispwin->convert();
    printf("\n");
   }
   else if(c=='4')
   {
    if(act_recstate==0)
     printf("\nStart Camera before grabbing\n");
    else
     grab_count_single(grabber,ima_count,Cdispwin,&Cbuf[0]);
    printf("\n");
   }
   else if(c=='5')
   {
    if(act_recstate==0)
     printf("\nStart Camera before grabbing\n");
    else
     grab_count_wait(grabber,ima_count,Cdispwin,Cbuf);
    printf("\n");
   }
  }

  for(int i=0;i<BUFNUM;i++)
   Cbuf[i].FreeBuffer();

  delete Cdispwin;
  Cdispwin=NULL;

  grabber->Close_Grabber();
  delete grabber;

  camera->Close_Cam();
  delete camera;

  printf("Any key CR to close application\n");
  getchar();

  return 0;
}


DWORD grab_single(CPco_grab_clhs* grabber,void* picbuf)
{
 int err;
 int picnum;
 unsigned int w,h,l;
 int min,max,v;
 WORD* adr;


 err=grabber->Acquire_Image(picbuf);
 if(err!=PCO_NOERROR)
  printf("\ngrab_single Acquire_Image error 0x%x\n",err);
 else
 {
  grabber->Get_actual_size(&w,&h,NULL);

  picnum=image_nr_from_timestamp(picbuf,shift);
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
 }

 return err;
}

DWORD get_image(CPco_grab_clhs* grabber,void* picbuf,WORD Segment,DWORD ImageNr)
{
 int err=-1;
 unsigned int w,h,l;
 int ima_num; 
 
 grabber->Get_actual_size(&w,&h,NULL);

 err=grabber->Get_Image(Segment,ImageNr,picbuf);
 if(err!=PCO_NOERROR)
  printf("\nget_image Acquire_Image error 0x%x\n",err);

 if(err==PCO_NOERROR)
 {
  int min,max,v;
  WORD* adr;

  ima_num=image_nr_from_timestamp(picbuf,0);
  printf("\nget_image done successful, timestamp image_nr: %d\n",ima_num);
 
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
  printf("get_image pixels min_value: %d max_value %d\n",min,max);
 }

 return err;
}


DWORD grab_count_single(CPco_grab_clhs* grabber,int ima_count,CPCODisp* Cdispwin,CCambuf* Cbuf)
{
 int err,imanum;
 unsigned int width,height;
 int firstnum,lastnum,currentnum;
 int numoff;
 double time,freq;

 time=freq=1;

 grabber->Get_actual_size(&width,&height,NULL);

 err=PCO_NOERROR;

 numoff=0;
 firstnum=1;

 imanum=0;
 do
 {
  err=grabber->Acquire_Image(Cbuf->Get_actadr());
  if(err!=PCO_NOERROR)
  {
   printf("\ngrab_loop Acquire_Image error 0x%x\n",err);
   break;
  }
  
  imanum++;
  if(imanum==1)
   mylog.start_time_mess();

  currentnum=image_nr_from_timestamp(Cbuf->Get_actadr(),0);
  if(imanum==1)
   firstnum=currentnum;
  Cdispwin->convert();

  printf("imanum %06d currentnum %06d camera_count %06d\r",imanum,currentnum,currentnum-firstnum+1);
  if(ima_count<=20)
   printf("\n");
  else
   printf("\r");
  fflush(stdout);

//for testing only lost images can also be seen at summary output
  if((currentnum-firstnum-numoff+1)!=imanum)
  {
   numoff++;
//   printf("\ngrab_loop Error number: camera %d grabber %d offset now %d\n",currentnum-firstnum+1,imanum,numoff);
  }


 }while(imanum<ima_count);

 time=mylog.stop_time_mess();

 printf("\n");

 if(err==PCO_NOERROR)
 {
  printf("Grab %d Images done (imanum %d) missed %d\n",ima_count,imanum,numoff);
  lastnum=image_nr_from_timestamp(Cbuf->Get_actadr(),0);
  freq=(imanum-1)*1000;
  freq/=time;
  printf("%05d images grabbed time %dms freq: %.2fHz %.2fMB/sec \n",imanum,(int)time,freq,(freq*width*height*2)/(1024*1024));

  freq=(lastnum-firstnum)*1000;
  freq/=time;
  printf("Imagenumbers: first %d last %d count %d freq: %.2fHz\n",firstnum,lastnum,lastnum-firstnum+1,freq);

  Cdispwin->convert();
 }

 fflush(stdout);

 return PCO_NOERROR;
}


DWORD grab_count_wait(CPco_grab_clhs* grabber,int count,CPCODisp* Cdispwin,CCambuf* Cbuf)
{
 int err,i,timeout;
 int picnum,buf_nr,first_picnum,lost;
 unsigned int w,h,bp;
 double tim,freq;
 int convnum;

 picnum=1;
 err=grabber->Get_actual_size(&w,&h,&bp);
 if(err!=PCO_NOERROR)
 {
  printf("\ngrab_count_wait Get_actual_size error 0x%x\n",err);
  return err;
 }

 printf("\n");
 lost=first_picnum=buf_nr=convnum=0;

 grabber->Get_Grabber_Timeout(&timeout);

 err=grabber->Start_Acquire(); 
 if(err!=PCO_NOERROR)
 {
  printf("\ngrab_count_wait Start_Acquire error 0x%x\n",err);
  return err;
 }

 for(i=0;i<count;i++)
 {
  buf_nr=i%BUFNUM;

  if(Cbuf[buf_nr].status&STATUS_INCONVERT)
  {
   convnum=buf_nr;
   buf_nr++;
   if(buf_nr>=BUFNUM)
    buf_nr=0;
   printf("\nBuffer %d in convert wait for next %d\n",convnum,buf_nr);
  }
  err=grabber->Wait_For_Next_Image(Cbuf[buf_nr].Get_actadr(),timeout);
  if(err!=PCO_NOERROR)
  {
   printf("\nWait_For_Next_Image error 0x%x\n",err);
   break;
  }
  else
  {
   picnum=image_nr_from_timestamp(Cbuf[buf_nr].Get_actadr(),0);
   printf("%05d. Image to %d  ts_nr: %05d",i+1,buf_nr,picnum);
   if(i==0)
   {
    first_picnum=picnum;
    mylog.start_time_mess();
   }
   else if((first_picnum+i)!=picnum)
   {
    printf(" %05d != %05d\n",first_picnum+i,picnum);
    first_picnum=picnum-i;
    lost++;
   }

   if((Cbuf[convnum].status&STATUS_INCONVERT)==0)
   {
    convnum=buf_nr;
    printf(" call convert %d         ",convnum);
    Cdispwin->Set_Actual_pic(&Cbuf[buf_nr]);
    Cdispwin->convert();
   }
   else
    printf(" wait convert %d 0x%x",convnum,Cbuf[convnum].status);

   if((count<=20)||(i<3))
    printf("\n");
   else
    printf("\r");
   fflush(stdout);
  }
 }
 i--;
 tim=mylog.stop_time_mess();
 printf("\n");
 printf("wait convert %d",convnum);
 while(Cbuf[convnum].status&STATUS_INCONVERT)
 {
  usleep(2000);
 }
 printf(" done \n");

 freq=i*1000;
 freq/=tim;
 printf(" %05d images grabbed in %dms lost images %d\n",i+1,(int)tim,lost);
 printf(" freq: %.2fHz time/pic: %.3fms  %.2fMB/sec\n",freq,tim/i,(freq*w*h*2)/(1024*1024));

 err=grabber->Stop_Acquire(); 
 if(err!=PCO_NOERROR)
 {
  printf("\ngrab_count_wait Stop_Acquire error 0x%x\n",err);
 }

 memcpy(Cbuf[0].Get_actadr(),Cbuf[buf_nr].Get_actadr(),Cbuf[buf_nr].Get_Size());
// printf("copy buffer %d size %d and convert ",buf_nr,Cbuf[buf_nr].Get_Size());
// printf("ima_nr %d \n",image_nr_from_timestamp(Cbuf[0].Get_actadr(),0));
 Cdispwin->Set_Actual_pic(&Cbuf[0]);
 Cdispwin->convert();
 return err;
}




int image_nr_from_timestamp(void* buf,int shift)
{
  unsigned short* b;
  int y;
  int image_nr=0;
  b=(unsigned short* )(buf);

  y=100*100*100;
  for(;y>0;y/=100)
  {
   *b>>=shift;
   image_nr+= (((*b&0x00F0)>>4)*10 + (*b&0x000F))*y;
   b++;
  }
  return image_nr;
}


void get_number(char* number,int len)
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

void get_text(char* text,int len)
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


void get_hexnumber(int* num,int len)
{
  int ret_val;
  int c=0;
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
    c++;
   }
  }
  if(c>0)
   *num=cmd;
  else
   *num=-1;
}
