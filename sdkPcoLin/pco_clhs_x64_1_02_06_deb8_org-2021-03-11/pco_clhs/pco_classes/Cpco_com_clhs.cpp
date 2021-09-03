//-----------------------------------------------------------------//
// Name        | Cpco_com_clhs.cpp           | Type: (*) source    //
//-------------------------------------------|       ( ) header    //
// Project     | pco.camera                  |       ( ) others    //
//-----------------------------------------------------------------//
// Platform    | Linux                                             //
//-----------------------------------------------------------------//
// Environment |                                                   //
//             |                                                   //
//-----------------------------------------------------------------//
// Purpose     | pco.camera - clhs communication                   //
//-----------------------------------------------------------------//
// Author      | MBL, PCO AG                                       //
//-----------------------------------------------------------------//
// Revision    | rev. 1.01                                         //
//-----------------------------------------------------------------//
// Notes       | In this file are all functions and definitions,   //
//             | for communication with CLHS grabbers              //
//             |                                                   //
//             |                                                   //
//-----------------------------------------------------------------//
// (c) 2016 - 2016 PCO AG                                          //
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

//-----------------------------------------------------------------//
// Revision History:                                               //
//-----------------------------------------------------------------//
// Rev.:     | Date:      | Changed:                               //
// --------- | ---------- | ---------------------------------------//
//  1.01     | 20.07.2016 | ported from USB                        //
//           |            |                                        //
// --------- | ---------- | ---------------------------------------//
//  0.0x     | xx.xx.2015 |                                        //
//           |            |                                        //
//-----------------------------------------------------------------//

#include "Cpco_com_clhs.h"


DWORD general_bits;

CPco_com_clhs::CPco_com_clhs() : CPco_com()
{
  Cclhs_cam=NULL;
}

CPco_com_clhs::~CPco_com_clhs()
{
  writelog(INIT_M,hdriver,"~CPco_com_clhs: before Close_Cam()");
  Close_Cam();
}

DWORD CPco_com_clhs::scan_camera()
{
  SC2_Simple_Telegram com;
  SC2_Camera_Type_Response resp;
  DWORD err=PCO_NOERROR;

  for(int i=0;i<5;i++)
  {
   writelog(INIT_M,hdriver,"scan_camera: scan with GET_CAMERA_TYPE %d",i);
   com.wCode=GET_CAMERA_TYPE;
   com.wSize=sizeof(SC2_Simple_Telegram);

   err=Control_Command(&com,sizeof(com),&resp,sizeof(SC2_Camera_Type_Response));
   if(err==PCO_NOERROR)
   {
    writelog(INIT_M,hdriver,"scan_camera: successful");
    break;
   }
  }
  return err;
}

CPco_clhs_cam* CPco_com_clhs::get_C_clhs_cam(void)
{
  return Cclhs_cam;
}




////////////////////////////////////////////////////////////////////////////////////////////
//
// OPEN/CLOSE FUNCTIONS
//
//
////////////////////////////////////////////////////////////////////////////////////////////


DWORD CPco_com_clhs::Open_Cam(DWORD num)
{
  return Open_Cam_Ext(num,NULL);
}

DWORD CPco_com_clhs::Open_Cam_Ext(DWORD num,SC2_OpenStruct *open ATTRIBUTE_UNUSED)
{
  int err;

  writelog(INIT_M, (PCO_HANDLE)1,"%s: Common Version %02d.%02d.%02d",__FUNCTION__,COMMON_VERSION,COMMON_SUBVERSION,COMMON_SUBAGE);
  writelog(INIT_M, (PCO_HANDLE)1,"%s: CLHS libpcocom_clhs Version %02d.%02d.%02d",__FUNCTION__,VERSION,SUBVERSION,SUBAGE);

  initmode=num & ~0xFF;
  num=num&0xFF;

  if(num>MAXNUM_DEVICES)
  {
   writelog(ERROR_M,(PCO_HANDLE)1,"%s: No more entries left return NODRIVER",__FUNCTION__);
   return PCO_ERROR_DRIVER_NODRIVER | PCO_ERROR_DRIVER_CAMERALINK;
  }

  if(hdriver)
  {
   writelog(ERROR_M,(PCO_HANDLE)1,"%s: camera is already connected",__FUNCTION__);
   return PCO_ERROR_DRIVER_NODRIVER | PCO_ERROR_DRIVER_CAMERALINK;
  }

  if(Cclhs_cam==NULL)
  {
   Cclhs_cam= new CPco_clhs_cam();
   if(clog)
   {
    Cclhs_cam->SetLog(clog);
   }
   err=Cclhs_cam->Open(num);
   if(err!=PCO_NOERROR)
   {
    writelog(ERROR_M,hdriver,"%s: Cclhs_cam->Open failed 0x%x",__FUNCTION__,err);
    Cclhs_cam->Close();
    delete Cclhs_cam;
    Cclhs_cam=NULL;
    return err;
   }
  }

  if(err==PCO_NOERROR)
  {
   if(sem_init(&sMutex, 0, 1) == -1)
    writelog(PROCESS_M, (PCO_HANDLE)1,"%s: Could not get semaphore sMutex",__FUNCTION__);

   hdriver=(PCO_HANDLE)(0x100+num);
   camerarev=0;

   tab_timeout.command=PCO_SC2_COMMAND_TIMEOUT;
   tab_timeout.image=PCO_SC2_IMAGE_TIMEOUT_L;
   tab_timeout.transfer=PCO_SC2_COMMAND_TIMEOUT;

   boardnr=num;

 //check if camera is connected, error should be timeout
 //get camera descriptor to get maximal size of ccd
   err=scan_camera();
   if(err!=PCO_NOERROR)
    writelog(ERROR_M,hdriver,"%s: Control command failed with 0x%08x, no camera connected",__FUNCTION__,err);
  }


#ifndef BASE_FUNC_ONLY
  if(err==PCO_NOERROR)
  {
   err=get_description();
   if(err!=PCO_NOERROR)
    writelog(ERROR_M,hdriver,"%s: get_desription() failed with 0x%08x",__FUNCTION__,err);
  }

  if(err==PCO_NOERROR)
  {
   err=get_firmwarerev();
   if(err!=PCO_NOERROR)
    writelog(ERROR_M,hdriver,"%s: get_firmwarerev() failed with 0x%08x",__FUNCTION__,err);
  }

  if(err==PCO_NOERROR)
  {
   struct tm st;
#ifdef WIN32
   time_t curtime;
   curtime=time(NULL);
   localtime_s(&st,&curtime);
#else
   int timeoff;
   struct timeval tv;

   gettimeofday(&tv,NULL);
   timeoff=(1000000-tv.tv_usec)/1000;
   Sleep_ms(timeoff);
   gettimeofday(&tv,NULL);

   for(int i=0;i<10;i++)
   {
    gettimeofday(&tv,NULL);
    if(tv.tv_usec<100)
     break;
    usleep(20);
   }
   localtime_r(&tv.tv_sec,&st);
   writelog(INIT_M,hdriver,"%s: wait done set camera time to %02d.%02d.%04d %02d:%02d:%02d.%06d",__FUNCTION__,
             st.tm_mday,st.tm_mon+1,st.tm_year+1900,st.tm_hour,st.tm_min,st.tm_sec,tv.tv_usec);
#endif

   err=PCO_SetDateTime(&st);
   if(err!=PCO_NOERROR)
    writelog(ERROR_M,hdriver,"%s: PCO_SetDateTime() failed with 0x%08x",__FUNCTION__,err);
#endif
  }

  if(err!=PCO_NOERROR)
  {
   if(Cclhs_cam)
   {
    Cclhs_cam->Close();
    delete Cclhs_cam;
    Cclhs_cam=NULL;

    sem_close(&sMutex);
    sem_destroy(&sMutex);

    hdriver=(PCO_HANDLE)NULL;
    boardnr=-1;
   }
  }
  return err;
}

int CPco_com_clhs::IsOpen()
{
  if(hdriver!=(PCO_HANDLE)NULL)
   return true;
  else
   return false;
}


DWORD CPco_com_clhs::Close_Cam()
{
  if(hdriver==(PCO_HANDLE)NULL)
  {
   writelog(INIT_M,hdriver,"%s: driver was closed before",__FUNCTION__);
   return PCO_NOERROR;
  }

  if(Cclhs_cam)
  {
   writelog(INIT_M,hdriver,"%s: close Cclhs_cam",__FUNCTION__);
   Cclhs_cam->Close();
   delete Cclhs_cam;
   Cclhs_cam=NULL;

   writelog(INIT_M,hdriver, "%s: Close mutex 0x%x",__FUNCTION__, sMutex);
   sem_close(&sMutex);
   sem_destroy(&sMutex);
   Sleep_ms(100);

   hdriver=(PCO_HANDLE)NULL;
   boardnr=-1;
  }

  return PCO_NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
// CAMERA IO FUNCTION
//
//
////////////////////////////////////////////////////////////////////////////////////////////
DWORD CPco_com_clhs::Control_Command(void *buf_in,DWORD size_in,
                                    void *buf_out,DWORD size_out)
{
  DWORD err=PCO_NOERROR;
  unsigned char buffer[PCO_SC2_DEF_BLOCK_SIZE];
  int size;
  WORD com_in,com_out;
  CLHS_GENCP_REG reg;
  WORD *b; 

//wait for semaphore
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME,&ts);
  ts.tv_sec += 1;
  int s;
  while(((s = sem_timedwait(&sMutex, &ts)) == -1)&&(errno == EINTR))
   continue;
  if(s == -1)
  {
   if(errno == ETIMEDOUT)
    writelog(COMMAND_M,hdriver,"%s: mutex timed out",__FUNCTION__);
   else
    writelog(COMMAND_M,hdriver,"%s: some other error",__FUNCTION__);

   return PCO_ERROR_TIMEOUT | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
  }

  com_in=*((WORD*)buf_in);
  com_out=0x0000;
  {
   char buf[100] = "\0";
   ComToString(com_in,buf);
   writelog(COMMAND_M,hdriver,"%s: start com_in %s timeout %d",__FUNCTION__,buf,tab_timeout.command);
  }

  size=PCO_SC2_DEF_BLOCK_SIZE;
  memset(buffer,0,PCO_SC2_DEF_BLOCK_SIZE);

  size=size_in;
  err=build_checksum((unsigned char*)buf_in,(int*)&size);

  PCO_CONTROL_COMMAND(reg)
  reg.size=size;
  err=Cclhs_cam->write_cam(reg.adr,reg.size,buf_in);
  if(err!=PCO_NOERROR)
  {
   writelog(ERROR_M,hdriver,"%s: write_mem failed with 0x%x",__FUNCTION__,err);
   goto clhs_com_end;
  }

//we read 40byte because CLHS is fast enough and most telegram returns are smaller
//data after telegram length is random therefore we set it to 0
  PCO_CONTROL_COMMAND(reg)
  reg.size=40;
  err=Cclhs_cam->read_cam(reg.adr,&reg.size,buffer);
  if(err != 0)
  {
   writelog(ERROR_M,hdriver,"%s: read_mem failed with 0x%x",__FUNCTION__,err);
   goto clhs_com_end;
  }

  if(reg.size==40)
  {
   b=(WORD*)buffer; //size of packet is second WORD
   b++;
   reg.size=(int)*b;
   if(reg.size>PCO_SC2_DEF_BLOCK_SIZE)
    reg.size=PCO_SC2_DEF_BLOCK_SIZE;
  }
  if(reg.size>40)
  {
   PCO_CONTROL_COMMAND(reg)
   if(reg.size%sizeof(DWORD))
    reg.size=((reg.size/sizeof(DWORD))+1)*sizeof(DWORD);
   writelog(COMMAND_M,hdriver,"%s: read answer again with size %d",__FUNCTION__,reg.size);
   err=Cclhs_cam->read_cam(reg.adr,&reg.size,buffer);
   if(err != 0)
   {
    writelog(ERROR_M,hdriver,"%s: read_mem failed with 0x%x",__FUNCTION__,err);
    goto clhs_com_end;
   }
  }

  b=(WORD*)buffer;
  com_out=*b;
  if(com_in!=(com_out&0xFF3F))
  {
   writelog(ERROR_M,hdriver,"%s: comin  0x%04x != comout&0xFF3F 0x%04x",__FUNCTION__,com_in,com_out&0xFF3F);
   err=PCO_ERROR_DRIVER_IOFAILURE | PCO_ERROR_DRIVER_CAMERALINK;
   goto clhs_com_end;
  }

  if((com_out&RESPONSE_ERROR_CODE)==RESPONSE_ERROR_CODE)
  {
   SC2_Failure_Response resp;
   memcpy(&resp,buffer,sizeof(SC2_Failure_Response));
   err=resp.dwerrmess;
   if((err&0xC000FFFF)==PCO_ERROR_FIRMWARE_NOT_SUPPORTED)
    writelog(INTERNAL_1_M,hdriver,"%s: com 0x%x FIRMWARE_NOT_SUPPORTED",__FUNCTION__,com_in);
   else
    writelog(ERROR_M,hdriver,"%s: com 0x%x RESPONSE_ERROR_CODE error 0x%x",__FUNCTION__,com_in,err);
  }

  if(err==PCO_NOERROR)
  {
   if(com_out!=(com_in|RESPONSE_OK_CODE))
   {
    err=PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
    writelog(ERROR_M,hdriver,"%s: Data error com_out 0x%04x should be 0x%04x",__FUNCTION__,com_out,com_in|RESPONSE_OK_CODE);
   }
  }

  if(size_out > sizeof(SC2_Failure_Response))
   size=size_out;
  else
   size=sizeof(SC2_Failure_Response);

  writelog(INTERNAL_1_M,hdriver,"%s: before test_checksum read=0x%04x size %d",__FUNCTION__,com_out,size);
  if(test_checksum(buffer,&size)==PCO_NOERROR)
  {
   size-=1;
   if(size<(int)size_out)
    size_out=size;
   memcpy(buf_out,buffer,size_out);
  }
  else
   err=test_checksum(buffer,&size);


clhs_com_end:
  sem_post(&sMutex);
  writelog(COMMAND_M,hdriver,"%s: done com_in 0x%04x  com_out 0x%04x err 0x%08x",__FUNCTION__,com_in,com_out,err);
  return err;
}


