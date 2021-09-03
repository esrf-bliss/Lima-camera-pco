//-----------------------------------------------------------------//
// Name        | CPco_grab_clhs.cpp          | Type: (*) source    //
//-------------------------------------------|       ( ) header    //
// Project     | pco.camera                  |       ( ) others    //
//-----------------------------------------------------------------//
// Platform    | Linux                                             //
//-----------------------------------------------------------------//
// Environment | gcc                                               //
//             |                                                   //
//-----------------------------------------------------------------//
// Purpose     | pco.camera - CLHS Image Grab                      //
//-----------------------------------------------------------------//
// Author      | MBL, PCO AG                                       //
//-----------------------------------------------------------------//
// Revision    | rev. 1.01 rel. 0.00                               //
//-----------------------------------------------------------------//
// Notes       | In this file are all functions and definitions,   //
//             | for grabbing from clhs camera                     //
//             |                                                   //
//             |                                                   //
//-----------------------------------------------------------------//
// (c) 2016 PCO AG * Donaupark 11 *                                //
// D-93309      Kelheim / Germany * Phone: +49 (0)9441 / 2005-0 *  //
// Fax: +49 (0)9441 / 2005-20 * Email: info@pco.de                 //
//-----------------------------------------------------------------//

#include "Cpco_grab_clhs.h"

#define PCO_CLHS_STARTED 0x0001
#define PCO_CLHS_BUFFER  0x0002


CPco_grab_clhs::CPco_grab_clhs(CPco_com_clhs *camera)
{
    clog = NULL;
    hgrabber = (PCO_HANDLE) NULL;

    cam = NULL;
    Cclhs_cam=NULL;


    act_bitpix = 0;
    act_width = 0;
    act_height = 0;
    act_align = 0;

    DataFormat = 0;
    ImageTimeout=0;
    aquire_flag=0;

//reset these settings
    camtype=0;
    serialnumber=0;
    cam_pixelrate=0;
    cam_timestampmode=0;
    cam_doublemode=0;
    cam_align=0;
    cam_noisefilter=0;
    cam_colorsensor=0;
    cam_width=cam_height=1000;
    cam_runtime=0;
    memset(&description,0,sizeof(description));

    if(camera != NULL)
     cam = camera;

}


CPco_grab_clhs::~CPco_grab_clhs()
{
  Close_Grabber();
}


void CPco_grab_clhs::SetLog(CPco_Log *elog)
{
   clog=elog;
}

void CPco_grab_clhs::writelog(DWORD lev,PCO_HANDLE hdriver,const char *str,...)
{
  if(clog)
  {
   va_list arg;
   va_start(arg,str);
   clog->writelog(lev,hdriver,str,arg);
   va_end(arg);
  }
}

int CPco_grab_clhs::Get_actual_size(unsigned int *width,unsigned int *height,unsigned int *bitpix)
{
  if(width)
   *width=act_width;
  if(height)
   *height=act_height;
  if(bitpix)
   *bitpix=act_bitpix;
  return PCO_NOERROR;
}

DWORD CPco_grab_clhs::Set_Grabber_Size(int width, int height)
{
  return Set_Grabber_Size(width,height,0);
}

DWORD CPco_grab_clhs::Set_Grabber_Size(int width,int height, int bitpix)
{
  writelog(PROCESS_M,hgrabber,"Set_Grabber_Size: start w:%d h:%d",width,height);

  act_height=height;
  act_width=width;

  if((bitpix == 0)&&(act_bitpix==0)&&(cam))
  {
   SC2_Camera_Description_Response description;
   cam->PCO_GetCameraDescriptor(&description);
   act_bitpix = description.wDynResDESC;
  }

  if(Cclhs_cam)
  {
   Cclhs_cam->Set_alignment(act_align);
   Cclhs_cam->Set_acquire_param(act_bitpix,0,0,cam_runtime);
   Cclhs_cam->Set_acquire_size(act_width,act_height);
  }
  writelog(PROCESS_M,hgrabber,"Set_Grabber_Size: done w:%d h:%d bitpix: %d align %d",act_width,act_height,act_bitpix,act_align);
  return PCO_NOERROR;
}


DWORD CPco_grab_clhs::Open_Grabber(int board)
{
  return Open_Grabber(board,0);
}

DWORD CPco_grab_clhs::Open_Grabber(int board,int initmode ATTRIBUTE_UNUSED)
{
  DWORD err=PCO_NOERROR;

  writelog(INIT_M, (PCO_HANDLE)1,"%s: CLHS libpcocam_clhs Version %02d.%02d.%02d",__FUNCTION__,VERSION,SUBVERSION,SUBAGE);

  if(hgrabber!=(PCO_HANDLE) NULL)
  {
   writelog(INIT_M,(PCO_HANDLE)1,"Open_Grabber: grabber was opened before");
   return PCO_NOERROR;
  }

  if(cam==NULL)
  {
   writelog(INIT_M,(PCO_HANDLE)1,"Open_Grabber: camera command interface must be available");
   return PCO_ERROR_DRIVER_NODRIVER | PCO_ERROR_DRIVER_CAMERALINK;
  }

  if(!cam->IsOpen())
  {
   writelog(INIT_M,(PCO_HANDLE)1,"Open_Grabber: camera command interface must be opened and initialized");
   return PCO_ERROR_DRIVER_NODRIVER | PCO_ERROR_DRIVER_CAMERALINK;
  }

  hgrabber=(PCO_HANDLE)0x200+board;

  ImageTimeout=10000;

  DataFormat=PCO_CL_DATAFORMAT_1x16;
  Cclhs_cam=cam->get_C_clhs_cam();

  return err;
}

DWORD CPco_grab_clhs::Close_Grabber()
{
  writelog(INIT_M,hgrabber,"Close_Grabber: ");

  if(hgrabber==(PCO_HANDLE)NULL)
  {
   writelog(INIT_M,hgrabber,"Close_Grabber: driver was closed before");
   return PCO_NOERROR;
  }
  hgrabber=(PCO_HANDLE)NULL;

  Cclhs_cam=NULL;



  act_width = 0;
  act_height = 0;
  act_bitpix = 0;
  DataFormat = 0;

  return PCO_NOERROR;
}

BOOL CPco_grab_clhs::IsOpen()
{
  if(hgrabber!=(PCO_HANDLE)NULL)
   return true;
  else
   return false;
}

void CPco_grab_clhs::SetBitAlignment(int align)
{
  act_align = align;
  if(Cclhs_cam)
   Cclhs_cam->Set_alignment(align);
}

DWORD CPco_grab_clhs::Set_DataFormat(DWORD dataformat)
{
  DataFormat=dataformat;
  return PCO_NOERROR;
}

DWORD CPco_grab_clhs::Set_Grabber_Timeout(int timeout)
{
  ImageTimeout=timeout;
  return PCO_NOERROR;
}

DWORD CPco_grab_clhs::Get_Grabber_Timeout(int *timeout)
{
  *timeout=ImageTimeout;
  return PCO_NOERROR;
}


DWORD CPco_grab_clhs::Get_Camera_Settings()
{
  DWORD err=PCO_NOERROR;
  DWORD secs,nsecs;

  if(cam&&cam->IsOpen())
  {
   if(description.wSensorTypeDESC==0)
   {
    cam->PCO_GetCameraDescriptor(&description);
    cam_colorsensor=description.wColorPatternTyp ? 1 : 0;
    writelog(INFO_M,hgrabber,"Get_Camera_Settings: cam_colorsensor       %d",cam_colorsensor);
   }

   err=cam->PCO_GetTimestampMode(&cam_timestampmode);
   if(err==PCO_NOERROR)
   {
    writelog(INFO_M,hgrabber,"Get_Camera_Settings: cam_timestampmode     %d",cam_timestampmode);
    err=cam->PCO_GetPixelRate(&cam_pixelrate);
   }
   if(err==PCO_NOERROR)
   {
    writelog(INFO_M,hgrabber,"Get_Camera_Settings: cam_pixelrate         %d",cam_pixelrate);
    err=cam->PCO_GetDoubleImageMode(&cam_doublemode);
    if((err&0xC000FFFF)==PCO_ERROR_FIRMWARE_NOT_SUPPORTED)
     err=PCO_NOERROR;
   }

   if(err==PCO_NOERROR)
   {
    writelog(INFO_M,hgrabber,"Get_Camera_Settings: cam_doublemode        %d",cam_doublemode);
    err=cam->PCO_GetNoiseFilterMode(&cam_noisefilter);
    if((err&0xC000FFFF)==PCO_ERROR_FIRMWARE_NOT_SUPPORTED)
     err=PCO_NOERROR;
   }

   if(err==PCO_NOERROR)
   {
    writelog(INFO_M,hgrabber,"Get_Camera_Settings: cam_noisefilter       %d",cam_noisefilter);
    err=cam->PCO_GetBitAlignment(&cam_align);
   }

   if(err==PCO_NOERROR)
   {
    writelog(INFO_M,hgrabber,"Get_Camera_Settings: cam_align             %d",cam_align);
    act_align=cam_align;
    err=cam->PCO_GetCOCRuntime(&secs,&nsecs);    
   }
   if(err==PCO_NOERROR)
   {
    cam_runtime=secs*1000+nsecs/1000000; 
    writelog(INFO_M,hgrabber,"Get_Camera_Settings: cam_runtime           %dms",cam_runtime);
    err=cam->PCO_GetActualSize(&cam_width,&cam_height);
   }
  
   if(err==PCO_NOERROR)
   {
    writelog(INFO_M,hgrabber,"Get_Camera_Settings: cam_width             %d",cam_width);
    writelog(INFO_M,hgrabber,"Get_Camera_Settings: cam_height            %d",cam_height);
   }
  }
  return err;
}


DWORD CPco_grab_clhs::PostArm(int userset)
{
  DWORD err=PCO_NOERROR;
  writelog(PROCESS_M,hgrabber,"%s(%d)",__FUNCTION__,userset);

  if(err==PCO_NOERROR)
   err=Get_Camera_Settings();

  if((err==PCO_NOERROR)&&(userset==0))
  {
//     writelog(PROCESS_M,hgrabber,"PostArm: call Set_DataFormat(0x%x)",clpar.DataFormat);
//     err=Set_DataFormat(DataFormat);
   if(err==PCO_NOERROR)
   {
    writelog(PROCESS_M,hgrabber,"PostArm: call Set_Grabber_Size(%d,%d)",cam_width,cam_height);
    err=Set_Grabber_Size(cam_width,cam_height);
   }

/*
     if(err==PCO_NOERROR)
     {
      if((nr_of_buffer>0)&&(size_alloc!=act_dmalength))
      {
       int bufnum=nr_of_buffer;
       writelog(PROCESS_M,hgrabber,"PostArm: reallocate %d buffers",bufnum);
       Free_Framebuffer();
       err=Allocate_Framebuffer(bufnum);
      }
     }
*/
    }
    return err;
}


DWORD  CPco_grab_clhs::Allocate_Framebuffer(int nr_of_buffer ATTRIBUTE_UNUSED)
{
    DWORD err=PCO_NOERROR;
    return err;
}


DWORD  CPco_grab_clhs::Free_Framebuffer()
{
    DWORD err=PCO_NOERROR;
    return err;
}


DWORD CPco_grab_clhs::Acquire_Image(void *adr)
{
  DWORD err=PCO_NOERROR;
  err=Acquire_Image(adr,ImageTimeout);
  return err;
}

DWORD CPco_grab_clhs::Acquire_Image(void *adr,int timeout)
{
  DWORD err=PCO_NOERROR;
  DWORD imastat;
  BOOL run=started(); 
  if(!run)
  {
   err=Start_Acquire();
   writelog(PROCESS_M,hgrabber,"Acquire_Image: call Start returned 0x%x",err);
   if(err!=PCO_NOERROR)
    return err;
  }

  err=Cclhs_cam->Set_acquire_buffer(adr);
  writelog(PROCESS_M,hgrabber,"Acquire_Image: call Set returned 0x%x",err);
  if(err!=PCO_NOERROR)
   return err;

  err=Cclhs_cam->Wait_acquire_buffer(timeout);
  writelog(PROCESS_M,hgrabber,"Acquire_Image: call Wait returned 0x%x",err);

  if(!run)
  {
   DWORD e;
   e=Stop_Acquire();
   writelog(PROCESS_M,hgrabber,"Acquire_Image: call Stop returned 0x%x",e);
  }

  imastat=Cclhs_cam->Get_acquire_status();
  writelog(PROCESS_M,hgrabber,"Acquire_Image: call Status returned 0x%x",imastat);

  if(imastat==(PCO_ERROR_DRIVER_BUFFERS_PENDING | PCO_ERROR_DRIVER_CAMERALINK))
  {
   DWORD e;
   e=Cclhs_cam->Cancel_acquire_buffer();
   writelog(PROCESS_M,hgrabber,"Acquire_Image: call Cancel returned 0x%x",e);
  }
  return err;
}


DWORD CPco_grab_clhs::Get_Image(WORD Segment,DWORD ImageNr,void *adr)
{
  DWORD err=PCO_NOERROR;
  DWORD imastat;
  BOOL run=started(); 
  
  if(!run)
  {
   err=Start_Acquire();
   writelog(PROCESS_M,hgrabber,"Get_Image: call Start returned 0x%x",err);
  }
  err=Cclhs_cam->Set_acquire_buffer(adr);
  writelog(PROCESS_M,hgrabber,"Get_Image: call Set returned 0x%x",err);

  if((Segment!=0)&&(ImageNr!=0))
  {
   if(cam&&cam->IsOpen())
    err=cam->PCO_ReadImagesFromSegment(Segment,ImageNr,ImageNr);
   else
   {
    writelog(ERROR_M,hgrabber,"Get_Image: no associated camera found");
    return PCO_ERROR_DRIVER_NODRIVER | PCO_ERROR_DRIVER_CAMERALINK;
   }
  }
  err=Cclhs_cam->Wait_acquire_buffer(ImageTimeout);
  writelog(PROCESS_M,hgrabber,"Get_Image: call Wait returned 0x%x",err);
  
  if(!run)
  {
   DWORD e;
   e=Stop_Acquire();
   writelog(PROCESS_M,hgrabber,"Get_Image: call Stop returned 0x%x",e);
  }
  
  imastat=Cclhs_cam->Get_acquire_status();
  writelog(PROCESS_M,hgrabber,"Get_Image: call Status returned 0x%x",err);
  
  if(imastat==(PCO_ERROR_DRIVER_BUFFERS_PENDING | PCO_ERROR_DRIVER_CAMERALINK))
  {
   DWORD e;
   e=Cclhs_cam->Cancel_acquire_buffer();
   writelog(PROCESS_M,hgrabber,"Get_Image: call Cancel returned 0x%x",e);
  }
  return err;
}


DWORD CPco_grab_clhs::Start_Acquire()
{
  DWORD err=PCO_NOERROR;
  err=Cclhs_cam->Start_acquisition();
  if(err==PCO_NOERROR)
   aquire_flag|=PCO_CLHS_STARTED;
  else
   writelog(ERROR_M,hgrabber,"Start_Acquire: call Start returned 0x%x",err);
  return err;
}

DWORD CPco_grab_clhs::Stop_Acquire()
{
  DWORD err=PCO_NOERROR;
  err=Cclhs_cam->Stop_acquisition();
  if(err==PCO_NOERROR)
   aquire_flag&=~PCO_CLHS_STARTED;
  else
   writelog(ERROR_M,hgrabber,"Stop_Acquire: call Stop returned 0x%x",err);

  if(aquire_flag&PCO_CLHS_BUFFER)
  {
   Cclhs_cam->Cancel_acquire_buffer();
   aquire_flag&=~PCO_CLHS_BUFFER;
  }
  return err;
}

BOOL CPco_grab_clhs::started()
{
  return aquire_flag&PCO_CLHS_STARTED;
}

DWORD CPco_grab_clhs::Wait_For_Next_Image(void* adr,int timeout)
{
  DWORD err=PCO_NOERROR;
  err=Cclhs_cam->Set_acquire_buffer(adr);
  if(err==PCO_NOERROR)
  {
   aquire_flag|=PCO_CLHS_BUFFER;
   writelog(PROCESS_M,hgrabber,"Wait_For_Next_Image: call Set_Buffer done");
  }
  else
   writelog(ERROR_M,hgrabber,"Wait_For_Next_Image: call Set_Buffer failed err 0x%x",err);

  if(err==PCO_NOERROR)
  {
   err=Cclhs_cam->Wait_acquire_buffer(timeout);
   writelog(PROCESS_M,hgrabber,"Wait_For_Next_Image: call Wait_Buffer returned 0x%x",err);
   if(err==PCO_NOERROR)
   {
    aquire_flag&=~PCO_CLHS_BUFFER;

    err=Cclhs_cam->Get_acquire_status();
    writelog(PROCESS_M,hgrabber,"Wait_For_Next_Image: call Get_Status returned 0x%x",err);
   }
  }
  return err;
}
