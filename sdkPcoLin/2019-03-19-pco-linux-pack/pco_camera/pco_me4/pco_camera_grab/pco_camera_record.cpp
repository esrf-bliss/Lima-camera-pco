//-----------------------------------------------------------------//
// Name        | pco_camera_record.cpp       | Type: (*) source    //
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
// Revision    | rev. 1.07                                         //
//-----------------------------------------------------------------//
// Notes       | main console program to show how to use           //
//             | class Cpco_camera and Cpco_com to get             //
//             | images from a pco camera with recorder buffer     //
//             |                                                   //
//-----------------------------------------------------------------//
// (c) 2010 - 2014 PCO AG                                          //
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
//  1.07     | 24.01.2014 | camera                                 //
//           |            |                                        //
//           |            |                                        //
//-----------------------------------------------------------------//
//  0.0x     | xx.xx.2012 |                                        //
//           |            |                                        //
//-----------------------------------------------------------------//

/*
#include <iostream>

#include "VersionNo.h"
#include "Cpco_com.h"
#include "Cpco_com_cl_me4.h"
#include "Cpco_grab_cl_me4.h"
#include "file12.h"

#define BUFNUM 4


int image_nr_from_timestamp(void *buf,int shift);
DWORD get_single(CPco_grab_cl_me4 *grabber,int shift);
DWORD get_single(CPco_grab_cl_me4 *grabber,char * filename,int shift);
DWORD get_count_single(CPco_grab_cl_me4 *grabber,int count,int shift);

void get_number(char *number,int len);
void get_text(char *text,int len);
void get_hexnumber(int *num,int len);


CPco_Log mylog("pco_camera_record.log");

int main(int argc, char* argv[])
{
  DWORD err;
  CPco_com_cl_me4 camera;
  CPco_grab_cl_me4 grabber(&camera);
*/




#include <iostream>

#include "VersionNo.h"
#include "Cpco_com.h"
#include "Cpco_com_cl_me4.h"
#include "Cpco_grab_cl_me4.h"
#include "file12.h"


int image_nr_from_timestamp(void *buf,int shift);
DWORD grab_single(CPco_grab_cl_me4 *grabber);
DWORD grab_single(CPco_grab_cl_me4 *grabber,char * filename);
DWORD grab_count(CPco_grab_cl_me4 *grabber,int ima_count);
DWORD grab_loop(CPco_grab_cl_me4 *grabber,int ima_count);

DWORD get_image(CPco_grab_cl_me4 *grabber,char * filename,WORD Segment,DWORD ImageNr);

void get_number(char *number,int len);
void get_text(char *text,int len);
void get_hexnumber(int *num,int len);



CPco_Log mylog("pco_camera_grab.log");

int main(int argc, char* argv[])
{
  DWORD err;
  CPco_com_cl_me4 camera;
  CPco_grab_cl_me4_camera grabber(&camera);

  int help=0;
  int board=0;
  char infostr[100];
  char number[20];

  int x;
  char c;
  int ima_count=100;
  int PicTimeOut=10000; //10 seconds
  int shift;
  WORD act_recstate,act_align;
  DWORD exp_time,delay_time,pixelrate;
  DWORD width,height,secs,nsecs;
  SC2_Camera_Description_Response description;
  double freq;
  SHORT ccdtemp,camtemp,pstemp;
  WORD camtype;
  DWORD serialnumber;
  int loglevel=0x0000F0FF;
  WORD triggermode;

  const char tmode[4][20]={"Auto","SW-Trig","Ext-Exp. Start","Ext-Exp. Ctrl"};

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

  if(loglevel>0)
  {
   camera.SetLog(&mylog);
   grabber.SetLog(&mylog);
  }

  printf("Try to open Camera\n");
  err=camera.Open_Cam(board);
  if(err!=PCO_NOERROR)
  {
   printf("error 0x%x in Open_Cam, close application",err);
   getchar();
   return -1;
  }

  err=camera.PCO_GetCameraType(&camtype,&serialnumber);
  if(err!=PCO_NOERROR))
  {
   printf("error 0x%x in PCO_GetCameraType",err);
   camera.Close_Cam();
   getchar();
   return -1;
  }

  if((camtype==CAMERATYPE_PCO_EDGE)||(camtype==CAMERATYPE_PCO_EDGE_42)||(camtype==CAMERATYPE_PCO_EDGE_GL))
  {
   printf("Wrong camera for this application");
   camera.Close_Cam();
   getchar();
   return -1;
  }

  printf("Try to open Grabber\n");
  err=grabber.Open_Grabber(board);
  if(err!=PCO_NOERROR)
  {
   printf("error 0x%x in Open_Grabber, close application",err);
   camera.Close_Cam();
   getchar();
   return -1;
  }

  err=grabber.Set_Grabber_Timeout(PicTimeOut);
  if(err!=PCO_NOERROR)
   printf("error 0x%x in Set_Grabber_Timeout",err);

  err=camera.PCO_GetCameraDescriptor(&description);
  if(err!=PCO_NOERROR)
   printf("PCO_GetCameraDescriptor() Error 0x%x\n",err);

  err=camera.PCO_GetInfo(1,infostr,sizeof(infostr));
  if(err!=PCO_NOERROR)
   printf("PCO_GetInfo() Error 0x%x\n",err);
  else
  {
   printf("Camera Name is: %s\n",infostr);
   printf("Camera Typ is : 0x%04x",camtype);
   printf("Camera Serial : %d",serialnumber);
  }

  err=camera.PCO_SetCameraToCurrentTime();
  if(err!=PCO_NOERROR)
   printf("PCO_SetCameraToCurrentTime() Error 0x%x\n",err);

  err=camera.PCO_GetTemperature(&ccdtemp,&camtemp,&pstemp);
  if(err!=PCO_NOERROR)
   printf("PCO_GetTemperature() Error 0x%x\n",err);
  else
  {
   printf("current temperatures\n");
   printf("Camera:      %d°C\n",camtemp);
   if(ccdtemp != (SHORT)(-32768))
    printf("Sensor:      %d.%02d°C\n",ccdtemp/10,ccdtemp%10);
   if(pstemp != (SHORT)(-32768))
    printf("PowerSupply: %d°C\n",pstemp);
  }


//set RecordingState to STOP
  err=camera.PCO_SetRecordingState(0);
  if(err!=PCO_NOERROR)
   printf("PCO_SetRecordingState() Error 0x%x\n",err);


//start from a known state
  err=camera.PCO_ResetSettingsToDefault();
  if(err!=PCO_NOERROR)
   printf("PCO_ResetSettingsToDefault() Error 0x%x\n",err);


  err=camera.PCO_SetTimestampMode(2);
  if(err!=PCO_NOERROR)
   printf("PCO_SetTimestampMode() Error 0x%x\n",err);

//set camera timebase to ms
  err=camera.PCO_SetTimebase(1,1);
  if(err!=PCO_NOERROR)
   printf("PCO_SetTimebase() Error 0x%x\n",err);

  exp_time=20000;
  delay_time=0;

  err=camera.PCO_SetDelayExposure(delay_time,exp_time);
  if(err!=PCO_NOERROR)
   printf("PCO_SetDelayExposure() Error 0x%x\n",err);

  if(description.wNumADCsDESC>1)
  {
   err=camera.PCO_SetADCOperation(2);
   if(err!=PCO_NOERROR)
    printf("PCO_SetADCOperation() Error 0x%x\n",err);
   else
    printf("PCO_SetADCOperation(2) done\n");
  }

  printf("possible PixelRates:\n");
  for(x=0;x<4;x++)
  {
   if(description.dwPixelRateDESC[x]!=0)
   {
    printf("%d: %d\n",x,description.dwPixelRateDESC[x]);
   }
  }

  err=camera.PCO_SetBitAlignment(BIT_ALIGNMENT_LSB);
  if(err!=PCO_NOERROR)
   printf("PCO_SetBitAlignment() Error 0x%x\n",err);


//prepare Camera for recording
  err=camera.PCO_ArmCamera();
  if(err!=PCO_NOERROR)
   printf("PCO_ArmCamera() Error 0x%x\n",err);

  err=camera.PCO_GetBitAlignment(&act_align);
  if(err!=PCO_NOERROR)
   printf("PCO_GetBitAlignment() Error 0x%x\n",err);

  grabber.SetBitAlignment(act_align);

  shift=0;
  if(act_align!=BIT_ALIGNMENT_LSB)
  {
   shift=16-description.wDynResDESC;
   printf("BitAlignment MSB shift %d\n",shift);
  }

  err=camera.PCO_GetPixelRate(&pixelrate);
  if(err!=PCO_NOERROR)
   printf("PCO_GetPixelrate() Error 0x%x\n",err);
  else
   printf("actual PixelRate: %d\n",pixelrate);

  err=camera.PCO_GetTriggerMode(&triggermode);
  if(err!=PCO_NOERROR)
   printf("PCO_GetGetTriggermode() Error 0x%x\n",err);
  else
   printf("actual Triggermode: %d %s\n",triggermode,tmode[triggermode]);


  err=camera.PCO_GetActualSize(&width,&height);
  if(err!=PCO_NOERROR)
   printf("PCO_GetActualSize() Error 0x%x\n",err);

  printf("Actual Resolution %d x %d\n",width,height);

  err=grabber.Set_Grabber_Size(width,height);
  if(err!=PCO_NOERROR)
   printf("Set_Grabber_Size() Error 0x%x\n",err);


  err=camera.PCO_SetRecordingState(1);
  if(err!=PCO_NOERROR)
   printf("PCO_SetRecordingState() Error 0x%x\n",err);

  c=' ';
  while(c!='x')
  {
   int ch;
   c=' ';

   printf("\n");
   camera.PCO_GetRecordingState(&act_recstate);
   camera.PCO_GetDelayExposure(&delay_time,&exp_time);
   camera.PCO_GetCOCRuntime(&secs,&nsecs);
   freq=nsecs;
   freq/=1000000000;
   freq+=secs;
   freq=1/freq;
   printf(" actual recording state %s actual freq: %.3lfHz time/pic: %.2lfms  datarate:%.2lfMB/sec \n",act_recstate ? "RUN" : "STOP",freq,1000/freq,(freq*width*height*2/(1024*1024)));
   printf("\n");
   printf("x to close camera and program\n");
   printf("c to set imagecount actual imagecount=%d\n",ima_count);
   printf("t to set picture timeout actual timeout=%d\n",PicTimeOut);
   printf("e to set exposure time actual exposuretime=%dus\n",exp_time);
   printf("d to set delay time actual delaytime=%dus\n",delay_time);
   printf("p to set pixelrate actual pixelrate %d\n",pixelrate);
   printf("a to set triggermode actual %s\n",tmode[triggermode]);
   printf("0 to set recording state to OFF\n");
   printf("1 to set recording state to ON\n");
   printf("2 Single Grab \n");
   printf("3 Loop Single Grab\n");
   printf("4 Single Grab ASYNC\n");
   printf("5 Loop Single ASYNC\n");

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
   else if(c=='t')
   {
    printf("enter picture timeout ...<CR>: ");
    get_number(number,10);
    if(strlen(number))
    {
     PicTimeOut=atoi(number);
     err=grabber.Set_Grabber_Timeout(PicTimeOut);
     if(err!=PCO_NOERROR)
      printf("error 0x%x in Set_Grabber_Timeout",err);
    }
   }
   else if(c=='e')
   {
    printf("enter new exposure time in us ...<CR>: ");
    get_number(number,10);
    if(strlen(number))
    {
     exp_time=atoi(number);
     if(act_recstate==1)
      camera.PCO_SetRecordingState(0);
     err=camera.PCO_SetDelayExposure(delay_time,exp_time);
     err=camera.PCO_GetDelayExposure(&delay_time,&exp_time);
     err=camera.PCO_ArmCamera();
     if(err!=PCO_NOERROR)
      printf("PCO_ArmCamera() Error 0x%x\n",err);
     if(act_recstate==1)
      camera.PCO_SetRecordingState(1);
    }
   }
   else if(c=='d')
   {
    printf("enter new delay time in us ...<CR>: ");
    get_number(number,10);
    if(strlen(number))
    {
     delay_time=atoi(number);
     if(act_recstate==1)
      camera.PCO_SetRecordingState(0);
     err=camera.PCO_SetDelayExposure(delay_time,exp_time);
     err=camera.PCO_GetDelayExposure(&delay_time,&exp_time);
     err=camera.PCO_ArmCamera();
     if(err!=PCO_NOERROR)
      printf("PCO_ArmCamera() Error 0x%x\n",err);
     if(act_recstate==1)
      camera.PCO_SetRecordingState(1);
    }
   }
   else if(c=='p')
   {
    printf("enter new pixelrate in MHz ...<CR>: ");
    get_number(number,4);
    if(strlen(number))
    {
     DWORD p;
     int x;

     p=atoi(number)*1000*1000;
     for(x=0;x<4;x++)
     {
      if((p<description.dwPixelRateDESC[x]+1000*1000)&&
         (p>description.dwPixelRateDESC[x]-1000*1000))
      {
       p=description.dwPixelRateDESC[x];
       break;
      }
     }
     if(x>=4)
     {
      printf("pixelrate %d not supported",p);
      continue;
     }

     if(act_recstate==1)
      camera.PCO_SetRecordingState(0);

     err=camera.PCO_SetPixelRate(p);
     if(err!=PCO_NOERROR)
      printf("PCO_SetPixelRate() Error 0x%x\n",err);

     err=camera.PCO_ArmCamera();
     if(err!=PCO_NOERROR)
      printf("PCO_ArmCamera() Error 0x%x\n",err);

     camera.PCO_GetPixelRate(&pixelrate);

     if(act_recstate==1)
      camera.PCO_SetRecordingState(1);
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
      camera.PCO_SetRecordingState(0);

     err=camera.PCO_SetTriggerMode(p);
     if(err!=PCO_NOERROR)
      printf("PCO_SetTriggerMode() Error 0x%x\n",err);

     err=camera.PCO_ArmCamera();
     if(err!=PCO_NOERROR)
      printf("PCO_ArmCamera() Error 0x%x\n",err);

     camera.PCO_GetTriggerMode(&triggermode);

     if(act_recstate==1)
      camera.PCO_SetRecordingState(1);
    }
   }
   else if(c=='0')
   {
    err=camera.PCO_SetRecordingState(0);
    if(err==PCO_NOERROR)
     printf("\nrecoding state is set to STOP\n");
   }
   else if(c=='1')
   {
    err=camera.PCO_SetRecordingState(1);
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
      grab_single(&grabber,filename,shift);
     else
      grab_single(&grabber,shift);
    }
    printf("\n");
   }
   else if(c=='3')
   {
    if(act_recstate==0)
     printf("\nStart Camera before grabbing\n");
    else
     grab_count_single(&grabber,ima_count,shift);
    printf("\n");
   }
   else if(c=='4')
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
      grab_single_async(&grabber,filename,shift);
     else
      grab_single_async(&grabber,shift);
    }
    printf("\n");
   }
   else if(c=='5')
   {
    if(act_recstate==0)
     printf("\nStart Camera before grabbing\n");
    else
     grab_count_single_async(&grabber,ima_count,shift);
    printf("\n");
   }
  }


  grabber.Close_Grabber();
  camera.Close_Cam();
  printf("Any key CR to close program\n");
  getchar();

  return 0;
}


DWORD grab_single(CPco_grab_cl_me4 *grabber,int shift)
{
 int err;
 int picnum;
 unsigned int w,h,bp;
 WORD *adr;

 picnum=1;
 err=grabber->Get_actual_size(&w,&h,&bp);
 if(err!=PCO_NOERROR)
 {
  printf("\ngrab_single Get_actual_size error 0x%x\n",err);
  return err;
 }

 adr=(WORD*)malloc(w*h*((bp+7)/8));
 err=grabber->Acquire_Image(adr);
 if(err!=PCO_NOERROR)
  printf("\ngrab_single Acquire_Image error 0x%x\n",err);
 else
 {
  picnum=image_nr_from_timestamp(adr,shift);
  printf("\ngrab_single done successful, timestamp image_nr: %d\n",picnum);
 }

 free(adr);

 return err;
}

DWORD grab_single(CPco_grab_cl_me4 *grabber,char* filename,int shift)
{
 int err;
 int picnum;
 unsigned int w,h,bp,l;
 WORD *adr;

 picnum=1;
 err=grabber->Get_actual_size(&w,&h,&bp);
 if(err!=PCO_NOERROR)
 {
  printf("\ngrab_single Get_actual_size error 0x%x\n",err);
  return err;
 }


 adr=(WORD*)malloc(w*h*((bp+7)/8)*2);
 err=grabber->Acquire_Image(adr);
 if(err!=PCO_NOERROR)
  printf("\ngrab_single Acquire_Image error 0x%x\n",err);
 else
 {
  picnum=image_nr_from_timestamp(adr,shift);
  printf("\ngrab_single done successful, timestamp image_nr: %d\n",picnum);
 }

 if(err==PCO_NOERROR)
 {
  char *txt;
  int min,max,v;

  picnum=image_nr_from_timestamp(adr,shift);
  printf("grab_single done successful, timestamp image_nr: %d\n",picnum);
  max=0;
  min=0xFFFF;
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
   store_b16(filename,w,h,0,adr);
   printf("b16 image saved to %s\n",filename);
  }
  else if((txt)&&(strstr(txt,"tif")))
  {
   store_tif(filename,w,h,0,adr);
   printf("tif image saved to %s\n",filename);
  }

 }

 free(adr);
 return err;
}


DWORD grab_count_single(CPco_grab_cl_me4 *grabber,int count,int shift)
{
 int err,i;
 int picnum,buf_nr,first_picnum,lost;
 unsigned int w,h,bp;
 WORD *adr[BUFNUM];
 double tim,freq;

 picnum=1;
 err=grabber->Get_actual_size(&w,&h,&bp);
 if(err!=PCO_NOERROR)
 {
  printf("\ngrab_single Get_actual_size error 0x%x\n",err);
  return err;
 }
 printf("\n");
 lost=first_picnum=0;

 for(i=0;i< BUFNUM;i++)
  adr[i]=(WORD*)malloc(w*h*((bp+7)/8));

 for(i=0;i<count;i++)
 {
  buf_nr=i%BUFNUM;
  err=grabber->Acquire_Image(adr[buf_nr]);
  if(err!=PCO_NOERROR)
  {
   printf("\nAcquire_Image error 0x%x\n",err);
   break;
  }
  else
  {
   picnum=image_nr_from_timestamp(adr[buf_nr],shift);
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
  free(adr[i]);

 return err;
}


DWORD grab_single_async(CPco_grab_cl_me4 *grabber,int shift)
{
 int err;
 int picnum;
 unsigned int w,h,bp;
 WORD *adr;

 picnum=1;
 err=grabber->Get_actual_size(&w,&h,&bp);
 if(err!=PCO_NOERROR)
 {
  printf("\ngrab_single Get_actual_size error 0x%x\n",err);
  return err;
 }

 adr=(WORD*)malloc(w*h*((bp+7)/8));
 err=grabber->Acquire_Image_Async_wait(adr);
 if(err!=PCO_NOERROR)
  printf("\ngrab_single Aquire_Image_Async error 0x%x\n",err);
 else
 {
  picnum=image_nr_from_timestamp(adr,shift);
  printf("\ngrab_single done successful, timestamp image_nr: %d\n",picnum);
 }

 free(adr);

 return err;
}

DWORD grab_single_async(CPco_grab_cl_me4 *grabber,char* filename,int shift)
{
 int err;
 int picnum;
 unsigned int w,h,bp,l;
 WORD *adr;

 picnum=1;
 err=grabber->Get_actual_size(&w,&h,&bp);
 if(err!=PCO_NOERROR)
 {
  printf("\ngrab_single Get_actual_size error 0x%x\n",err);
  return err;
 }


 adr=(WORD*)malloc(w*h*((bp+7)/8));

  printf("\ngrab_single: Buffer size is %i",w*h*((bp+7)/8));

 err=grabber->Acquire_Image_Async_wait(adr);
 if(err!=PCO_NOERROR)
  printf("\ngrab_single Acquire_Image_Async error 0x%x\n",err);
 else
 {
  picnum=image_nr_from_timestamp(adr,shift);
  printf("\ngrab_single done successful, timestamp image_nr: %d\n",picnum);
 }

 if(err==PCO_NOERROR)
 {
  char *txt;
  int min,max,v;

  picnum=image_nr_from_timestamp(adr,shift);
  printf("grab_single done successful, timestamp image_nr: %d\n",picnum);
  max=0;
  min=0xFFFF;
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
   store_b16(filename,w,h,0,adr);
   printf("b16 image saved to %s\n",filename);
  }
  else if((txt)&&(strstr(txt,"tif")))
  {
   store_tif(filename,w,h,0,adr);
   printf("tif image saved to %s\n",filename);
  }
 }

 free(adr);
 return err;
}


DWORD grab_count_single_async(CPco_grab_cl_me4 *grabber,int count,int shift)
{
 int err,i;
 int picnum,buf_nr,first_picnum,lost;
 unsigned int w,h,bp;
 WORD *adr[BUFNUM];
 double tim,freq;

 picnum=1;
 err=grabber->Get_actual_size(&w,&h,&bp);
 if(err!=PCO_NOERROR)
 {
  printf("\ngrab_single Get_actual_size error 0x%x\n",err);
  return err;
 }
 printf("\n");
 lost=first_picnum=0;

 for(i=0;i< BUFNUM;i++)
  adr[i]=(WORD*)malloc(w*h*((bp+7)/8));

 for(i=0;i<count;i++)
 {
     buf_nr=i%BUFNUM;
     if(i == count)
         err=grabber->Acquire_Image_Async_wait(adr[buf_nr]);
     else

         err=grabber->Acquire_Image_Async(adr[buf_nr]);
     if(err!=PCO_NOERROR)
     {
   printf("\nAcquire_Image_Async error 0x%x\n",err);
   break;
  }
  else
  {
   picnum=image_nr_from_timestamp(adr[buf_nr],shift);
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


 for(i=0;i< BUFNUM;i++)
  free(adr[i]);

 return err;
}


int image_nr_from_timestamp(void *buf,int shift)
{
  unsigned short *b;
  int y;
  int image_nr=0;
  b=(unsigned short *)(buf);
  //printf("raw image_nr data: %d %d %d %d %d       ",b[0],b[1],b[2],b[3],b[4]);

  y=100*100*100;
  for(;y>0;y/=100)
  {
   image_nr+= ((((*b>>shift)&0x00F0)>>4)*10 + (*b>>shift&0x000F))*y;
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
