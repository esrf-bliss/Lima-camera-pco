//-----------------------------------------------------------------//
// Name        | pco_camera_grab.cpp         | Type: (*) source    //
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
//             | class Cpco_camera and Cpco_com to grab            //
//             | images from a pco usb camera                      //
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
//  1.07     | 24.01.2014 | usb camera                             //
//           |            |                                        //
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
#include "Cpco_grab_clhs.h"
#include "file12.h"

#define BUFNUM 4
#define MAXCAM 4


int image_nr_from_timestamp(void* buf,int shift);
DWORD grab_single(CPco_grab_clhs* grabber,char * filename);
DWORD get_image(CPco_grab_clhs* grabber,char* filename,WORD Segment,DWORD ImageNr);
DWORD grab_count_single(CPco_grab_clhs* grabber,int count);
DWORD grab_count_wait(CPco_grab_clhs* grabber,int count);

void get_number(char* number,int len);
void get_text(char* text,int len);
void get_hexnumber(int* num,int len);


CPco_Log mylog("pco_camera_grab_m.log");

const char tb[3][3]={"ns","us","ms"};
const char tmode[4][20]={"Auto","SW-Trig","Ext-Exp. Start","Ext-Exp. Ctrl"};

int main(int argc, char* argv[])
{
  DWORD err;
  CPco_com* camera[MAXCAM];
  CPco_grab_clhs* grabber[MAXCAM];

  int help=0;
  int camnum[MAXCAM];
  char infostr[100];

/*
  int loop_count=1;
  int ima_count=100;
  
  char number[20];
  char c;

  double freq;
  DWORD secs,nsecs;

  
  WORD act_recstate[MAXCAM];
*/
  int x;

  int PicTimeOut=10000; //10 seconds
  
  DWORD exp_time[MAXCAM],delay_time[MAXCAM];
  WORD exp_timebase[MAXCAM],del_timebase[MAXCAM];
  DWORD width[MAXCAM],height[MAXCAM];
  SC2_Camera_Description_Response description[MAXCAM];
  WORD camtype[MAXCAM];
  DWORD serialnumber[MAXCAM];
  
  WORD triggermode;
  WORD binhorz,binvert;   
  WORD wRoiX0, wRoiY0, wRoiX1, wRoiY1;
  WORD act_align;
  DWORD pixelrate;
  
  SHORT ccdtemp,camtemp,pstemp;
  int shift;
  int loglevel=0x000FF0FF;

  for(int i=0;i<MAXCAM;i++)
  {
   camnum[i]=-1;
   camera[i]=NULL; 
   grabber[i]=NULL;
  }
  
  for(int x=argc;x>1;)
  {
   char *a;

   x--;

   if(strstr(argv[x],"-h"))
    help=1;
   if(strstr(argv[x],"-?"))
    help=1;
   if(strstr(argv[x],"?"))
    help=1;

   if((a=strstr(argv[x],"-b")))
   {
    int board=atoi(a+2);
    for(int i=0;i<board;i++)
     camnum[i]=i;
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
          "-b[0...7] camera numbers\n"
          "-l[0x0...] actual loglevel default 0x%x\n"
          "-h,-?,? this message\n",argv[0],loglevel);
   exit(0);
  }


  mylog.set_logbits(loglevel);
  printf("Logging set to 0x%08x\n",mylog.get_logbits());

  err=0; 
  for(int i=0;i<MAXCAM;i++)
  {
   if(camnum[i]<0)
    break;
   camera[i]= new CPco_com_clhs();
   if(camera[i]==NULL)
   {
    printf("ERROR: Cannot create camera object\n");
    err=-1; 
    break;
   }

   if(loglevel>0)
    camera[i]->SetLog(&mylog);

 
   printf("Try to open Camera %d\n",camnum[i]);
   err=camera[i]->Open_Cam(camnum[i]);
   if(err!=PCO_NOERROR)
   { 
    printf("ERROR: 0x%x in Open_Cam\n",err);
    err=-1; 
    break;
   }

   err=camera[i]->PCO_GetCameraType(&camtype[i],&serialnumber[i]);
   if(err!=PCO_NOERROR)
   {
    printf("ERROR: 0x%x in PCO_GetCameraType\n",err);
    err=-1;
    break;
   }

   printf("Grabber is CPco_grab_clhs\n");
   grabber[i]=new CPco_grab_clhs((CPco_com_clhs*)camera[i]);
   if(grabber[i]==NULL)
   {
    printf("ERROR: Cannot create grabber object\n");
    err=-1; 
    break;
   }

   if(loglevel>0)
    grabber[i]->SetLog(&mylog);

   printf("Try to open Grabber %d\n",camnum[i]);
   err=grabber[i]->Open_Grabber(camnum[i]);
   if(err!=PCO_NOERROR)
   {
    printf("ERROR: 0x%x in Open_Grabber\n",err);
    err=-1;
    break;
   }
   printf("camera %1d and grabber %1d created and opened\n\n",i,i); 
  }
  
  if(err!=0)
  {
   for(int i=0;i<MAXCAM;i++)
   {
    if(camnum[i]>=0)
    {
     if(grabber[i]!=NULL)
     {
      delete grabber[i];
     }
     if(camera[i]!=NULL) 
     {
      delete camera[i];
     }
    } 
   }      
   printf("Any key CR to close  application\n");
   getchar();
  }
  
  
  for(int i=0;i<MAXCAM;i++)
  {
   if(camnum[i]<0)
    break;
   
   printf("Get parameters camera %1d\n",i); 
   
   err=grabber[i]->Set_Grabber_Timeout(PicTimeOut);
   if(err!=PCO_NOERROR)
    printf("error 0x%x in Set_Grabber_Timeout",err);

   err=camera[i]->PCO_GetCameraDescriptor(&description[i]);
   if(err!=PCO_NOERROR)
    printf("PCO_GetCameraDescriptor() Error 0x%x\n",err);

   err=camera[i]->PCO_GetInfo(1,infostr,sizeof(infostr));
   if(err!=PCO_NOERROR)
    printf("PCO_GetInfo() Error 0x%x\n",err);
   else
   {
    printf("cam%1d Camera Name is: %s\n",i,infostr);
    printf("cam%1d Camera Typ is : 0x%04x\n",i,camtype[i]);
    printf("cam%1d Camera Serial : %d\n",i,serialnumber[i]);
   } 

   err=camera[i]->PCO_SetCameraToCurrentTime();
   if(err!=PCO_NOERROR)
    printf("PCO_SetCameraToCurrentTime() Error 0x%x\n",err);


   err=camera[i]->PCO_GetTemperature(&ccdtemp,&camtemp,&pstemp);
   if(err!=PCO_NOERROR)
    printf("PCO_GetTemperature() Error 0x%x\n",err);
   else
   {
    printf("current temperatures\n");
    printf("cam%1d Camera:      %d°C\n",i,camtemp);
    if(ccdtemp != (SHORT)(-32768))
     printf("Sensor:      %d°C\n",ccdtemp);
    if(pstemp != (SHORT)(-32768))
     printf("PowerSupply: %d°C\n",pstemp);
   }


//set RecordingState to STOP
   err=camera[i]->PCO_SetRecordingState(0);
   if(err!=PCO_NOERROR)
    printf("PCO_SetRecordingState() Error 0x%x\n",err);


//start from a known state
   err=camera[i]->PCO_ResetSettingsToDefault();
   if(err!=PCO_NOERROR)
    printf("PCO_ResetSettingsToDefault() Error 0x%x\n",err);


   err=camera[i]->PCO_SetTimestampMode(2);
   if(err!=PCO_NOERROR)
    printf("PCO_SetTimestampMode() Error 0x%x\n",err);

//set camera timebase to us
   exp_time[i]    =10000;
   delay_time[i]  =0;
   exp_timebase[i]=1;
   del_timebase[i]=1;

   err=camera[i]->PCO_SetTimebase(del_timebase[i],exp_timebase[i]);
   if(err!=PCO_NOERROR)
    printf("PCO_SetTimebase() Error 0x%x\n",err);

   err=camera[i]->PCO_SetDelayExposure(delay_time[i],exp_time[i]);
   if(err!=PCO_NOERROR)
    printf("PCO_SetDelayExposure() Error 0x%x\n",err);

   if(description[i].wNumADCsDESC>1)
   {
    err=camera[i]->PCO_SetADCOperation(2);
    if(err!=PCO_NOERROR)
     printf("PCO_SetADCOperation() Error 0x%x\n",err);
   }

   err=camera[i]->PCO_GetPixelRate(&pixelrate);
   if(err!=PCO_NOERROR)
    printf("PCO_GetPixelrate() Error 0x%x\n",err);
   else
    printf("cam%1d actual PixelRate: %d\n",i,pixelrate);
   printf("possible PixelRates:\n");
   for(x=0;x<4;x++)
   { 
    if(description[i].dwPixelRateDESC[x]!=0)
    {
     printf("%d: %d\n",x,description[i].dwPixelRateDESC[x]);
    }
   }

   err=camera[i]->PCO_SetBitAlignment(BIT_ALIGNMENT_LSB);
   if(err!=PCO_NOERROR)
    printf("PCO_SetBitAlignment() Error 0x%x\n",err);


//prepare Camera for recording
   err=camera[i]->PCO_ArmCamera();
   if(err!=PCO_NOERROR)
    printf("PCO_ArmCamera() Error 0x%x\n",err);

/*
  printf("second PCO_ArmCamera\n");
  err=camera->PCO_ArmCamera();
  if(err!=PCO_NOERROR)
   printf("PCO_ArmCamera() Error 0x%x\n",err);
*/

   err=camera[i]->PCO_GetBitAlignment(&act_align);
   if(err!=PCO_NOERROR)
    printf("PCO_GetBitAlignment() Error 0x%x\n",err);

   shift=0;
   if(act_align!=BIT_ALIGNMENT_LSB)
   {
    shift=16-description[i].wDynResDESC;
    printf("BitAlignment MSB shift %d\n",shift);
   }

   err=camera[i]->PCO_GetTriggerMode(&triggermode);
   if(err!=PCO_NOERROR)
    printf("PCO_GetGetTriggermode() Error 0x%x\n",err);
   else
    printf("cam%1d actual Triggermode: %d %s\n",i,triggermode,tmode[triggermode]);

   err=camera[i]->PCO_GetBinning(&binhorz,&binvert);
   if(err!=PCO_NOERROR)
    printf("PCO_GetBinning() Error 0x%x\n",err);
   else
    printf("cam%1d actual Binning: %dx%d\n",i,binhorz,binvert);
  
   err=camera[i]->PCO_GetROI(&wRoiX0, &wRoiY0, &wRoiX1, &wRoiY1);
   if(err!=PCO_NOERROR)
    printf("PCO_GetROI() Error 0x%x\n",err);
   else
    printf("cam%1d actual ROI: %d-%d, %d-%d\n",i,wRoiX0,wRoiX1,wRoiY0,wRoiY1);


   err=camera[i]->PCO_GetActualSize(&width[i],&height[i]);
   if(err!=PCO_NOERROR)
    printf("PCO_GetActualSize() Error 0x%x\n",err);

   printf("cam%1d Actual Resolution %d x %d\n",i,width[i],height[i]);

   err=grabber[i]->PostArm();
   if(err!=PCO_NOERROR)
    printf("grabber->PostArm() Error 0x%x\n",err);

/*is done from PostArm()
  err=grabber->SetBitAlignment(act_align);
  err=grabber->Set_Grabber_Size(width,height);
  if(err!=PCO_NOERROR)
   printf("Set_Grabber_Size() Error 0x%x\n",err);

*/

   err=camera[i]->PCO_SetRecordingState(1);
   if(err!=PCO_NOERROR)
    printf("PCO_SetRecordingState() Error 0x%x\n",err);
   
   printf("Get parameters camera %1d done\n\n",i); 
   
  }
  
  
/*  
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
   printf("\n");
   printf("0 to set recording state to OFF\n");
   printf("1 to set recording state to ON\n");
   printf("2 Single Acquire_Image (recording camera)\n");
   printf("3 Single Get_Image (recording camera or camera recorder buffer) \n");
   printf("4 Loop Acquire_Image\n");
   printf("5 Loop Wait_Next_Image\n");

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
     
     err=camera->PCO_SetBinning(new_binhorz,new_binvert);
     printf("Set binning %dx%d returned 0x%x\n",new_binhorz,new_binvert,err);

     if((description.wMaxHorzResStdDESC/new_binhorz)!=(wRoiX1-wRoiX0))
     {
      new_wRoiX0 = 1;
      new_wRoiX1 = description.wMaxHorzResStdDESC/new_binhorz;
     }
     if((description.wMaxVertResStdDESC/new_binvert)!=(wRoiY1-wRoiY0))
     {
      new_wRoiY0 = 1;
      new_wRoiY1 = description.wMaxVertResStdDESC/new_binvert;
     }
     err=camera->PCO_SetROI(new_wRoiX0, new_wRoiY0, new_wRoiX1, new_wRoiY1);
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
     
     
    }

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
    int Segment,image_number;
    char filename[300];

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

    printf("enter filename ...<CR>: \n");
    printf("if filename has suffix .b16 save image in b16 format\n");
    printf("if filename has suffix .tif save image in tif format\n");
    printf("if nothing is entered, no file will be saved\n");
    get_text(filename,300);

    if(strlen(filename))
     get_image(grabber,filename,Segment,image_number);
    else
     get_image(grabber,NULL,Segment,image_number);
    printf("\n");
   }
   else if(c=='4')
   {
    if(act_recstate==0)
     printf("\nStart Camera before grabbing\n");
    else
     grab_count_single(grabber,ima_count);
    printf("\n");
   }
   else if(c=='5')
   {
    if(act_recstate==0)
     printf("\nStart Camera before grabbing\n");
    else
     grab_count_wait(grabber,ima_count);
    printf("\n");
   }
  }
*/

  for(int i=0;i<MAXCAM;i++)
  {
   if(camnum[i]>=0)
   {
    if(grabber[i]!=NULL)
    {
     delete grabber[i];
    }
    if(camera[i]!=NULL) 
    {
     delete camera[i];
    }
   } 
  }      

  printf("Any key CR to close application\n");
  getchar();

  return 0;
}


DWORD grab_single(CPco_grab_clhs* grabber,char* filename)
{
 int err;
 unsigned int w,h,l;
 int ima_num; 
 WORD *picbuf;

 err=grabber->Get_actual_size(&w,&h,NULL);
 if(err!=PCO_NOERROR)
 {
  printf("\ngrab_single Get_actual_size error 0x%x\n",err);
  return err;
 }

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

DWORD get_image(CPco_grab_clhs* grabber,char* filename,WORD Segment,DWORD ImageNr)
{
 int err;
 unsigned int w,h,l;
 int ima_num; 
 WORD *picbuf;

 grabber->Get_actual_size(&w,&h,NULL);
 picbuf=(WORD*)malloc(w*h*sizeof(WORD));
 if(picbuf==NULL)
 {
  printf("\nget_image cannot allocate buffer\n");
  return PCO_ERROR_NOMEMORY | PCO_ERROR_APPLICATION;
 }

 err=grabber->Get_Image(Segment,ImageNr,picbuf);
 if(err!=PCO_NOERROR)
  printf("\nget_image Acquire_Image error 0x%x\n",err);

 if(err==PCO_NOERROR)
 {
  ima_num=image_nr_from_timestamp(picbuf,0);
  printf("\nget_image done successful, timestamp image_nr: %d\n",ima_num);
 
  if(filename!=NULL)
  {
   char *txt;
   int min,max,v;

   max=0;
   min=0xFFFF;
   l=w*20; //skip first lines with timestamp
   for(;l<w*h;l++)
   {
    v=*(picbuf+l);
    if(v<min)
     min=v;
    if(v>max)
    max=v;
   } 
   printf("get_image pixels min_value: %d max_value %d\n",min,max);

   do
   {
    txt=strchr(filename,'.');
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


DWORD grab_count_single(CPco_grab_clhs* grabber,int count)
{
 int err,i;
 int picnum,buf_nr,first_picnum,lost;
 unsigned int w,h,bp;
 WORD *picbuf[BUFNUM];
 double tim,freq;

 picnum=1;
 err=grabber->Get_actual_size(&w,&h,&bp);
 if(err!=PCO_NOERROR)
 {
  printf("\ngrab_count_single Get_actual_size error 0x%x\n",err);
  return err;
 }
 printf("\n");
 lost=first_picnum=0;

 memset(picbuf,0,sizeof(WORD*)*BUFNUM); 
 for(i=0;i< BUFNUM;i++)
 {
  picbuf[i]=(WORD*)malloc(w*h*sizeof(WORD));
  if(picbuf[i]==NULL)
  {
   printf("\nget_count_single image cannot allocate buffer %d\n",i);
   err=PCO_ERROR_NOMEMORY | PCO_ERROR_APPLICATION;
   break;
  }
 }

 for(i=0;i<count;i++)
 {
  buf_nr=i%BUFNUM;
  err=grabber->Acquire_Image(picbuf[buf_nr]);
  if(err!=PCO_NOERROR)
  {
   printf("\nAcquire_Image error 0x%x\n",err);
   break;
  }
  else
  {
   picnum=image_nr_from_timestamp(picbuf[buf_nr],0);
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

   if((count<=10)||(i<3))
    printf("\n");
   else
    printf("\r");
   fflush(stdout);
  }
 }
 i--;
 tim=mylog.stop_time_mess();
 freq=i*1000;
 freq/=tim;
 printf("\n%05d images grabbed time %dms time/pic: %.3fms lost %d freq: %.2fHz %.2fMB/sec",i+1,(int)tim,tim/i,lost,freq,(freq*w*h*2)/(1024*1024));


 for(i=0;i< BUFNUM;i++)
 {
  if(picbuf[i]!=NULL)
   free(picbuf[i]);
 }

 return err;
}


DWORD grab_count_wait(CPco_grab_clhs* grabber,int count)
{
 int err,i,timeout;
 int picnum,buf_nr,first_picnum,lost;
 unsigned int w,h,bp;
 WORD *picbuf[BUFNUM];
 double tim,freq;

 picnum=1;
 err=grabber->Get_actual_size(&w,&h,&bp);
 if(err!=PCO_NOERROR)
 {
  printf("\ngrab_count_wait Get_actual_size error 0x%x\n",err);
  return err;
 }
 printf("\n");
 lost=first_picnum=0;

 grabber->Get_Grabber_Timeout(&timeout);

 memset(picbuf,0,sizeof(WORD*)*BUFNUM); 
 for(i=0;i< BUFNUM;i++)
 {
  picbuf[i]=(WORD*)malloc(w*h*sizeof(WORD));
  if(picbuf[i]==NULL)
  {
   printf("\ngrab_count_wait cannot allocate buffer %d\n",i);
   err=PCO_ERROR_NOMEMORY | PCO_ERROR_APPLICATION;
   break;
  }
 }
 if(err!=PCO_NOERROR)
 {
  for(i=0;i< BUFNUM;i++)
  {
   if(picbuf[i]!=NULL)
    free(picbuf[i]);
  }
  return err;
 }

 err=grabber->Start_Acquire(); 
 if(err!=PCO_NOERROR)
 {
  printf("\ngrab_count_wait Start_Acquire error 0x%x\n",err);
  return err;
 }

 for(i=0;i<count;i++)
 {
  buf_nr=i%BUFNUM;
  err=grabber->Wait_For_Next_Image(picbuf[buf_nr],timeout);
  if(err!=PCO_NOERROR)
  {
   printf("\nWait_For_Next_Image error 0x%x\n",err);
   break;
  }
  else
  {
   picnum=image_nr_from_timestamp(picbuf[buf_nr],0);
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

   if((count<=10)||(i<3))
    printf("\n");
   else
    printf("\r");
   fflush(stdout);
  }
 }
  
 i--;
 tim=mylog.stop_time_mess();
 freq=i*1000;
 freq/=tim;
 printf("\n %05d images grabbed in %dms lost images %d\n",i+1,(int)tim,lost);
 printf(" freq: %.2fHz time/pic: %.3fms  %.2fMB/sec",freq,tim/i,(freq*w*h*2)/(1024*1024));

 err=grabber->Stop_Acquire(); 
 if(err!=PCO_NOERROR)
 {
  printf("\ngrab_count_wait Stop_Acquire error 0x%x\n",err);
 }

 for(i=0;i< BUFNUM;i++)
 {
  if(picbuf[i]!=NULL)
   free(picbuf[i]);
 }

 return err;
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
