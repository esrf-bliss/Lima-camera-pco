//-----------------------------------------------------------------//
// Name        | Cpco_cl_com.cpp             | Type: (*) source    //
//-------------------------------------------|       ( ) header    //
// Project     | pco.camera                  |       ( ) others    //
//-----------------------------------------------------------------//
// Platform    | WINDOWS 2000/XP                                   //
//-----------------------------------------------------------------//
// Environment | MS Visual Studio                                  //
//             |                                                   //
//-----------------------------------------------------------------//
// Purpose     | pco.camera - CameraLink Communication             //
//-----------------------------------------------------------------//
// Author      | MBL, PCO AG                                       //
//-----------------------------------------------------------------//
// Revision    | rev. 1.01 rel. 0.00                               //
//-----------------------------------------------------------------//
// Notes       | In this file are all functions and definitions,   //
//             | for commiunication with CameraLink grabbers       //
//             |                                                   //
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
// Free Software Foundation, Inc., 59 Temple Place- Suite 330,     //
// Boston, MA 02111-1307, USA                                      //
//-----------------------------------------------------------------//

//-----------------------------------------------------------------//
// Revision History:                                               //
//-----------------------------------------------------------------//
// Rev.:     | Date:      | Changed:                               //
// --------- | ---------- | ---------------------------------------//
//  1.01     | 08.10.2010 | ported from SDK                //
//           |            |                                        //
// --------- | ---------- | ---------------------------------------//
//  1.02     | 14.01.2011 | new functions:                         //
//           |            | PCO_GetTemperature                     //
//           |            | PCO_SetLut                             //
//           |            | PCO_ResetSettingsToDefault             //
//           |            | PCO_SetTriggerMode                     //
//           |            | PCO_ForceTrigger                       //
//           |            |                                        //
// --------- | ---------- | ---------------------------------------//
//  1.03     | 24.01.2012 | new functions:                         //
//           |            | PCO_GetCameraType                      //
//           |            | without log class                      //
//           |            |                                        //
//-----------------------------------------------------------------//
//  1.04     | 10.05.2012 | new functions:                         //
//           |            | PCO_GetSensorSignalStatus              //
//           |            | PCO_GetROI                             //
//           |            | PCO_SetROI                             //
//           |            | PCO_GetBinning                         //
//           |            | PCO_SetBinning                         //
//           |            |                                        //
//-----------------------------------------------------------------//
//  0.0x     | xx.xx.2009 |                                        //
//           |            |                                        //
//-----------------------------------------------------------------//

#include "Cpco_com_cl_me4.h"


////////////////////////////////////////////////////////////////////////////////////////////
//
//  Defines and globals
//
////////////////////////////////////////////////////////////////////////////////////////////

//HINSTANCE SerialLib=NULL;

void close_SerialLib()
{
    /*
  if(SerialLib)
  {
   FreeLibrary(SerialLib);
   SerialLib=NULL;
  }
  */
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//  LOCAL FUNCTIONS
//
////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#if !defined _byteswap_ulong
inline unsigned long _byteswap_ulong(unsigned long val)
{
    int x;
    unsigned char b[4];

    for(x=0;x<4;x++)
        b[3-x]=*((unsigned char*)&val+x);

    return (unsigned long)(*(unsigned long*)b);
}
#endif
#endif



CPco_com_cl_me4::CPco_com_cl_me4()
{
//    log=NULL;
    serialRef=NULL;
    hdriver=(PCO_HANDLE)NULL;

    //common cameralink
    initmode=0;
    boardnr=-1;
    scan=FALSE;
}

CPco_com_cl_me4::~CPco_com_cl_me4()
{
    Close_Cam();
}


DWORD CPco_com_cl_me4::scan_camera()
{
    SC2_Simple_Telegram com;
    SC2_Camera_Type_Response resp;
    int baudrate;
    int b;
//    unsigned int buf[10]={1,2,3,4,5,6,7,8,9,0};
    DWORD err=PCO_NOERROR;
   
    b=0;
    baudrate=baud[b];

    com.wCode=GET_CAMERA_TYPE;
    com.wSize=sizeof(SC2_Simple_Telegram);
//    writelog(INIT_M,hdriver,"sleep1");
//    Sleep_ms(150);
//    writelog(INIT_M,hdriver,"sleep2");
//    clGetSupportedBaudRates(serialRef,buf);
//    writelog(INIT_M,hdriver,"getbaudrates %d %d",buf[0],buf[1]);

 

    err=Control_Command(&com,sizeof(com),&resp,sizeof(SC2_Camera_Type_Response));
    writelog(INIT_M,hdriver,"scan_camera: 1.scan with baudrate %d done, returned 0x%x",baudrate,err);
    if(err!=PCO_NOERROR)
    {
     baudrate=115200;
     err=clSetBaudRate(serialRef,CL_BAUDRATE_115200);
     Sleep_ms(150);
     err=Control_Command(&com,sizeof(com),&resp,sizeof(SC2_Camera_Type_Response));
     writelog(INIT_M,hdriver,"scan_camera: 2.scan with baudrate %d done, returned 0x%x",baudrate,err);
    }

    while((err!=PCO_NOERROR)&&(baudrate<115200))
    {
     b++;
     baudrate=baud[b];
        //if(clSetBaudRate!=NULL)
     {
      err=clSetBaudRate(serialRef,1<<b);
      Sleep_ms(150);
     }
     err=Control_Command(&com,sizeof(com),&resp,sizeof(SC2_Camera_Type_Response));
    }

    if(err==PCO_NOERROR)
     transferpar.baudrate=baudrate;

    writelog(INIT_M,hdriver,"scan_camera: baudrate set to %d",transferpar.baudrate);

    return err;
}

DWORD CPco_com_cl_me4::set_baudrate(int baudrate)
{
    // return PCO_ERROR_DRIVER_NOFUNCTION | PCO_ERROR_DRIVER_CAMERALINK;
    DWORD err;
    int b;

    // if(clSetBaudRate==NULL)
    // {
    //  writelog(ERROR_M,hdriver,"set_baudrate: clSetBaudRate not found in clser Dll");
    //  return PCO_ERROR_DRIVER_NOFUNCTION | PCO_ERROR_DRIVER_CAMERALINK;
    // }

    SC2_Simple_Telegram com;
    SC2_Get_CL_Baudrate_Response resp;

    com.wCode=GET_CL_BAUDRATE;
    com.wSize=sizeof(com);
    err=Control_Command(&com,sizeof(com),&resp,sizeof(SC2_Get_CL_Baudrate_Response));

    if(err!=PCO_NOERROR)
    {
        writelog(ERROR_M,hdriver,"set_baudrate: GET_CL_BAUDRATE failed with 0x%x",err);
        return err;
    }

    switch(baudrate)
    {
    case   9600:
        b=CL_BAUDRATE_9600;
        break;

    case  19200:
        b=CL_BAUDRATE_19200;
        break;

    case  38400:
        b=CL_BAUDRATE_38400;
        break;

    case  57600:
        b=CL_BAUDRATE_57600;
        break;

    case 115200:
        b=CL_BAUDRATE_115200;
        break;

    default:
        writelog(ERROR_M,hdriver,"set_baudrate: baudrate %d not supported",baudrate);
        return PCO_ERROR_WRONGVALUE | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;;
    }

    SC2_Set_CL_Baudrate setbaud;

    setbaud.wCode=SET_CL_BAUDRATE;
    setbaud.wSize=sizeof(SC2_Set_CL_Baudrate);
    setbaud.dwBaudrate=baudrate;

    err=Control_Command(&setbaud,sizeof(SC2_Set_CL_Baudrate),&resp,sizeof(SC2_Get_CL_Baudrate_Response));
    if(err!=PCO_NOERROR)
    {
        writelog(ERROR_M,hdriver,"set_baudrate: SET_CL_BAUDRATE failed with 0x%x",err);
        return err;
    }
    Sleep_ms(200);

    clSetBaudRate(serialRef,b);

    com.wCode=GET_CL_BAUDRATE;
    com.wSize=sizeof(com);
    err=Control_Command(&com,sizeof(com),&resp,sizeof(SC2_Get_CL_Baudrate_Response));

    //switch back to default baudrate of 9600
    if(err!=PCO_NOERROR)
    {
        writelog(ERROR_M,hdriver,"set_baudrate: GET_CL_BAUDRATE failed with 0x%x",err);
        b=1;
        clSetBaudRate(serialRef,b);
        transferpar.baudrate=9600;
    }
    transferpar.baudrate=baudrate;
    writelog(INIT_M,hdriver,"set_baudrate: baudrate set to %d",transferpar.baudrate);
    return err;
}

DWORD CPco_com_cl_me4::get_actual_cl_config()
{
    SC2_Simple_Telegram com;
    SC2_Get_CL_Configuration_Response resp;
    SC2_Get_Interface_Output_Format com_get_if;
    SC2_Get_Interface_Output_Format_Response resp_if;
    DWORD err=PCO_NOERROR;

    com.wCode=GET_CL_CONFIGURATION;
    com.wSize=sizeof(SC2_Simple_Telegram);
    err=Control_Command(&com,sizeof(com),&resp,sizeof(resp));

    if(err!=PCO_NOERROR)
        writelog(ERROR_M,hdriver,"get_actual_cl_config: GET_CL_CONFIGURATION failed with 0x%x",err);
    else
    {
        transferpar.ClockFrequency=resp.dwClockFrequency;
        transferpar.CCline=resp.bCCline;
        transferpar.Transmit=resp.bTransmit;
        transferpar.DataFormat=resp.bDataFormat;
    }

//check for sensor type removed here
    {
        com_get_if.wCode=GET_INTERFACE_OUTPUT_FORMAT;
        com_get_if.wSize=sizeof(com_get_if);
        com_get_if.wInterface=INTERFACE_CL_SCCMOS;
        err=Control_Command(&com_get_if,sizeof(com_get_if),
                            &resp_if,sizeof(resp_if));
        if(err!=PCO_NOERROR)
            resp_if.wFormat=0;
        else
            transferpar.DataFormat|=resp_if.wFormat;
        writelog(INIT_M,hdriver,"get_actual_cl_config: GET_INTERFACE_OUTPUT_FORMAT err 0x%x format 0x%x",err,resp_if.wFormat);
    }
    return PCO_NOERROR;
}

DWORD CPco_com_cl_me4::set_cl_config(PCO_SC2_CL_TRANSFER_PARAM cl_par)
{
    SC2_Set_CL_Configuration cl_com;
    SC2_Get_CL_Configuration_Response cl_resp;
    DWORD err=PCO_NOERROR;

    cl_com.wCode=SET_CL_CONFIGURATION;
    cl_com.wSize=sizeof(cl_com);
    cl_com.dwClockFrequency=cl_par.ClockFrequency;
    cl_com.bTransmit=cl_par.Transmit&0xFF;
    cl_com.bCCline=cl_par.CCline&0xFF;
    cl_com.bDataFormat=cl_par.DataFormat&0xFF;
    writelog(INIT_M,hdriver,"set_cl_config: freq %dMHz data 0x%08x  line 0x%04x transmit 0x%08x", cl_com.dwClockFrequency/1000000,cl_com.bDataFormat,cl_com.bCCline,cl_com.bTransmit);

    //send set CL_config
    err=Control_Command(&cl_com,sizeof(cl_com),
                        &cl_resp,sizeof(cl_resp));
    if(err!=PCO_NOERROR)
    {
        writelog(ERROR_M,hdriver,"set_cl_config: SET_CL_CONFIGURATION failed with 0x%x",err);
        return err;
    }

//check for sensor type removed here
    {
        SC2_Set_Interface_Output_Format com_set_if;
        SC2_Set_Interface_Output_Format_Response resp_if;

        com_set_if.wCode=SET_INTERFACE_OUTPUT_FORMAT;
        com_set_if.wSize=sizeof(com_set_if);
        com_set_if.wFormat=cl_par.DataFormat&SCCMOS_FORMAT_MASK;
        com_set_if.wInterface=INTERFACE_CL_SCCMOS;
        com_set_if.wReserved1=0;
        err=Control_Command(&com_set_if,sizeof(com_set_if),
                            &resp_if,sizeof(resp_if));

        if((err&0xF000FFFF)==PCO_ERROR_FIRMWARE_NOT_SUPPORTED) 
         err=PCO_NOERROR;

        if(err!=PCO_NOERROR)
         writelog(ERROR_M,hdriver,"set_cl_config: SCCMOS SET_INTERFACE_OUTPUT_FORMAT failed with 0x%x",err);
    }
    return err;
}

////////////////////////////////////////////////////////////////////////
// Openlibrary clser... and get serial functions
//

DWORD CPco_com_cl_me4::Get_Serial_Functions()
{
    // set function pointers

    pclSerialInit = clSerialInit;
    pclSerialRead = clSerialRead;
    pclSerialWrite = clSerialWrite;
    pclSerialClose = clSerialClose;
    pclSetBaudRate = clSetBaudRate;
    pclGetSupportedBaudRates = clGetSupportedBaudRates;
    pclGetNumBytesAvail = clGetNumBytesAvail;
    pclFlushPort = clFlushPort;
    pclSetParity = clSetParity;
    pclGetNumSerialPorts = clGetNumSerialPorts;
    pclGetManufacturerInfo = clGetManufacturerInfo;
    pclGetSerialPortIdentifier = clGetSerialPortIdentifier;

    return PCO_NOERROR;
}



////////////////////////////////////////////////////////////////////////////////////////////
//
// OPEN/CLOSE FUNCTIONS
//
//
////////////////////////////////////////////////////////////////////////////////////////////


DWORD CPco_com_cl_me4::Open_Cam(DWORD num)
{
  return Open_Cam_Ext(num,NULL);
}

DWORD CPco_com_cl_me4::Open_Cam_Ext(DWORD num,SC2_OpenStruct *open ATTRIBUTE_UNUSED)
{
  int err;
  initmode=num & ~0xFF;
  num=num&0xFF;

  if(num>MAXNUM_DEVICES)
  {
   writelog(ERROR_M,(PCO_HANDLE)1,"Open_Cam_Ext: No more entries left return NODRIVER");
   return PCO_ERROR_DRIVER_NODRIVER | PCO_ERROR_DRIVER_CAMERALINK;
  }

  if(GetConnectionStatus()&(1<<num))
  {
   writelog(ERROR_M,(PCO_HANDLE)1,"Open_Cam_Ext: camera is already connected");
   return PCO_ERROR_DRIVER_NODRIVER | PCO_ERROR_DRIVER_CAMERALINK;
  }


  hdriver=(PCO_HANDLE)(0x100+num);
  err=Get_Serial_Functions();
  if(err!=PCO_NOERROR)
  {
   writelog(ERROR_M,(PCO_HANDLE)1,"Open_Cam_Ext: Cannot get serial Functions");
   hdriver=(PCO_HANDLE)NULL;
   return PCO_ERROR_DRIVER_NODRIVER | PCO_ERROR_DRIVER_CAMERALINK;
  }

  unsigned int num_port=0;
  {
   char manufacturerName[500];
   unsigned int bufferSize;
   unsigned int version;
   char type[500];
   unsigned int buffersize;

   bufferSize=sizeof(manufacturerName);
   buffersize=sizeof(type);

   if(pclGetNumSerialPorts)
   {
    pclGetNumSerialPorts(&num_port);
    if(num>=num_port)
    {
     hdriver=(PCO_HANDLE)NULL;
     writelog(ERROR_M,(PCO_HANDLE)1,"Open_Cam_Ext: board %d reqested, only %d ports found return NODRIVER",num,num_port);
     return PCO_ERROR_DRIVER_NODRIVER | PCO_ERROR_DRIVER_CAMERALINK;
    }
   }
   if(pclGetManufacturerInfo)
   {
    pclGetManufacturerInfo(manufacturerName, &bufferSize,&version);
    writelog(INIT_M,(PCO_HANDLE)1,"Open_Cam_Ext: ManufacturerName %s",manufacturerName);
   }

   if(pclGetSerialPortIdentifier)
   {
    for(unsigned int i=0;i<num_port;i++)
    {
     pclGetSerialPortIdentifier(i,type, &buffersize);
     writelog(INIT_M,(PCO_HANDLE)1,"Open_Cam_Ext: PortIdentifier %d: %s",i,type);
    }
   }
  }

  if(pclSerialInit!=NULL)
  {
   err=pclSerialInit(num,(void**)&serialRef);
   if(err<0)
   {
    writelog(ERROR_M,(PCO_HANDLE)1,"Open_Cam_Ext: Cannot open serial Cameralink Device %d %d",num,err);
    hdriver=(PCO_HANDLE)NULL;
    serialRef=NULL;
    return PCO_ERROR_DRIVER_NODRIVER | PCO_ERROR_DRIVER_CAMERALINK;
   }
  }


  sem_init(&sComMutex, 0, 1);
  camerarev=0;

  tab_timeout.command=PCO_SC2_COMMAND_TIMEOUT;
  tab_timeout.image=PCO_SC2_IMAGE_TIMEOUT_L;
  tab_timeout.transfer=PCO_SC2_COMMAND_TIMEOUT;

  transferpar.baudrate=9600;
  transferpar.DataFormat=PCO_CL_DATAFORMAT_5x12;

  boardnr=num;

//check if camera is connected, error should be timeout
//get camera descriptor to get maximal size of ccd
//scan for other baudrates than default 9600baud
//if scan is TRUE error output is disabled

  scan=TRUE;
  err=PCO_NOERROR;
  err=scan_camera();
  if(err!=PCO_NOERROR)
  {
   writelog(ERROR_M,hdriver,"Open_Cam_Ext: Control command failed with 0x%x, no camera connected",err);

   if(pclSerialClose!=NULL)
    pclSerialClose(serialRef);

   boardnr=-1;
   hdriver=(PCO_HANDLE)NULL;
   serialRef=NULL;

   return PCO_ERROR_DRIVER_NOTINIT  | PCO_ERROR_DRIVER_CAMERALINK;
  }

  if(err==PCO_NOERROR)
  {
   err=get_description();
   if(err!=PCO_NOERROR)
    writelog(ERROR_M,hdriver,"%s: get_desription() failed with 0x%08x",__FUNCTION__,err);
  }
  if(err==PCO_NOERROR)
  {
   get_actual_cl_config();
   get_lut_info();
  }
  scan=FALSE;


#ifndef BASE_FUNC_ONLY
  if(err==PCO_NOERROR)
  {
   err=get_firmwarerev();
   if(err!=PCO_NOERROR)
    writelog(ERROR_M,hdriver,"%s: get_firmwarerev() failed with 0x%08x",__FUNCTION__,err);
  }

  if(err==PCO_NOERROR)
  {
   struct tm st;
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

   err=PCO_SetDateTime(&st);
   if(err!=PCO_NOERROR)
    writelog(ERROR_M,hdriver,"%s: PCO_SetDateTime() failed with 0x%08x",__FUNCTION__,err);
  }
#endif

  set_baudrate(115200);


/*
  com.wCode=GET_CAMERA_DESCRIPTION;
    com.wSize=sizeof(SC2_Simple_Telegram);
    err=Control_Command(&com,sizeof(com),
                        &description,sizeof(SC2_Camera_Description_Response));

#ifndef BASE_FUNC_ONLY
    if(err!=PCO_NOERROR)
        writelog(ERROR_M,hdriver,"Open_Cam_Ext: GET_CAMERA_DESCRIPTION failed with 0x%x",err);

    get_actual_cl_config();
    get_firmwarerev();
    get_lut_info();
*/

    int connected = GetConnectionStatus();
    SetConnectionStatus(connected|=(1<<boardnr));

    return PCO_NOERROR;
}

BOOLEAN CPco_com_cl_me4::IsOpen()
{
    if(hdriver!=(PCO_HANDLE)NULL)
        return true;
    else
        return false;
}


DWORD CPco_com_cl_me4::Close_Cam()
{
    if(hdriver==(PCO_HANDLE)NULL)
    {
     writelog(INIT_M,hdriver,"Close_Cam: driver was closed before");
     return PCO_NOERROR;
    }

    {
     writelog(INIT_M,hdriver,"Close_Cam: Close Mutex ");
     sem_close(&sComMutex);
     sem_destroy(&sComMutex);
    }

    if((pclSerialClose!=NULL)&&(serialRef!=NULL))
     pclSerialClose(serialRef);

    boardnr=-1;
    serialRef=NULL;
    hdriver=(PCO_HANDLE)NULL;
    int connected = GetConnectionStatus();
    SetConnectionStatus(connected&=(1<<boardnr));

    return PCO_NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
// CAMERA IO FUNCTION
//
//
////////////////////////////////////////////////////////////////////////////////////////////
DWORD CPco_com_cl_me4::Control_Command(void *buf_in,DWORD size_in,
                                       void *buf_out,DWORD size_out)
{
    int err=PCO_NOERROR;
    unsigned char buffer[PCO_SC2_DEF_BLOCK_SIZE];
    unsigned int size;
    WORD com_in,com_out;

    sem_wait(&sComMutex);

    if(pclFlushPort)
    {
        err=pclFlushPort(serialRef);
        if(err<0)
        {
            writelog(ERROR_M,hdriver,"Control_Command: clSerialFlush error = %d",err);
        }
    }

    com_out=0;
    com_in = *((WORD*)buf_in);

    {
     char strbuf[50] = "\0";
     ComToString(com_in, strbuf);
     writelog(COMMAND_M, hdriver, "Control_Command: start= %s serialRef %x timeout %d", strbuf, serialRef, tab_timeout.command);
    }


    size=MAX(size_out,sizeof(SC2_Failure_Response));
    size=PCO_SC2_DEF_BLOCK_SIZE;
    memset(buffer,0,PCO_SC2_DEF_BLOCK_SIZE);

    size=size_in;
    err=build_checksum((unsigned char*)buf_in,(int*)&size);

    err=pclSerialWrite(serialRef,(char*)buf_in,&size,tab_timeout.command);
    if(err<0)
    {
        switch (err) {
        case CL_ERR_INVALID_REFERENCE:
            writelog(ERROR_M,hdriver,"Control_Command: clSerialWrite error = %d INVALID_REFERENCE",err);
            break;
        case CL_ERR_TIMEOUT:
            writelog(ERROR_M,hdriver,"Control_Command: clSerialWrite error = %d TIMEOUT",err);
            break;
        default:
            writelog(ERROR_M,hdriver,"Control_Command: clSerialWrite error = %d %s",err);
            break;
        }
        err=PCO_ERROR_DRIVER_IOFAILURE | PCO_ERROR_DRIVER_CAMERALINK;
        goto SERCOM_OUT;
//     sercom_out(0,err);
//     return err;
    }

    size=sizeof(WORD)*2;

    err=pclSerialRead(serialRef,(char*)&buffer[0],&size,tab_timeout.command*2);

    if(err<0)
    {
     if(scan==FALSE)
     {
      switch (err)
      {
        case CL_ERR_INVALID_REFERENCE:
            writelog(ERROR_M,hdriver,"Control_Command: clSerialRead error = %d INVALID_REFERENCE",err);
            break;
        case CL_ERR_TIMEOUT:
            writelog(ERROR_M,hdriver,"Control_Command: clSerialRead error = %d TIMEOUT",err);
            break;
        default:
            writelog(ERROR_M,hdriver,"Control_Command: clSerialRead error = %d ",err);
            break;
      }
     }
     err=PCO_ERROR_DRIVER_IOFAILURE | PCO_ERROR_DRIVER_CAMERALINK;
     goto SERCOM_OUT;
//     sercom_out(0,err);
//     return err;
    }

    com_out=buffer[0]+buffer[1]*0x100;

    WORD *b;
    b=(WORD *)buffer; //size of packet is second WORD
    b++;
    size=(int)*b;
    if(size>PCO_SC2_DEF_BLOCK_SIZE)
     size=PCO_SC2_DEF_BLOCK_SIZE;
    size-=sizeof(WORD)*2;

    writelog(INTERNAL_1_M,hdriver,"Control_Command: before read=0x%04x size %d",com_out,size);
    if((int)size<0)
    {
     writelog(ERROR_M,hdriver,"Control_Command: clSerialRead remaining size<0 %d err %d com:0x%04x",size,err,com_out);
     err=PCO_ERROR_DRIVER_IOFAILURE | PCO_ERROR_DRIVER_CAMERALINK;
     goto SERCOM_OUT;
//     sercom_out(0,err);
//     return err;
    }
    if(com_in!=(com_out&0xFF3F))
    {
     writelog(ERROR_M,hdriver,"Control_Command: clSerialRead comin  0x%04x != comout&0xFF3F 0x%04x",com_in,com_out&0xFF3F);
     err=PCO_ERROR_DRIVER_IOFAILURE | PCO_ERROR_DRIVER_CAMERALINK;
     goto SERCOM_OUT;
//     sercom_out(0,err);
//     return err;
    }

    err=pclSerialRead(serialRef,(char*)&buffer[sizeof(WORD)*2],&size,tab_timeout.command*2);
    if((int)err<0)
    {
        writelog(ERROR_M,hdriver,"Control_Command: clSerialRead size:%d back %d error = %d",*b,size,err);
        err=PCO_ERROR_DRIVER_IOFAILURE | PCO_ERROR_DRIVER_CAMERALINK;
     goto SERCOM_OUT;
//     sercom_out(0,err);
//     return err;
    }

    err=PCO_NOERROR;

    com_out=buffer[0]+buffer[1]*0x100;

    if((com_out&RESPONSE_ERROR_CODE)==RESPONSE_ERROR_CODE)
    {
     SC2_Failure_Response resp;
     memcpy(&resp,buffer,sizeof(SC2_Failure_Response));
     err=resp.dwerrmess;
     if(scan==FALSE)
     {
      if((err&0xC000FFFF)==PCO_ERROR_FIRMWARE_NOT_SUPPORTED)
       writelog(INTERNAL_1_M,hdriver,"Control_Command: com 0x%x FIRMWARE_NOT_SUPPORTED",com_in);
      else
       writelog(ERROR_M,hdriver,"Control_Command: com 0x%x RESPONSE_ERROR_CODE error 0x%x",com_in,err);
     }
    }

    if(err==PCO_NOERROR)
    {
     if(com_out!=(com_in|RESPONSE_OK_CODE))
     {
      err=PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
      writelog(ERROR_M,hdriver,"Control_Command: Data error com_out 0x%04x should be 0x%04x",com_out,com_in|RESPONSE_OK_CODE);
     }
    }

    size=MAX(size_out,sizeof(SC2_Failure_Response));

    writelog(INTERNAL_1_M,hdriver,"Control_Command: before test_checksum read=0x%04x size %d",com_out,size);
    if(test_checksum(buffer,(int*)&size)==PCO_NOERROR)
    {
     size-=1;
     if(size<size_out)
      size_out=size;
     if(buf_out)
      memcpy(buf_out,buffer,size_out);
    }
    else
     err=test_checksum(buffer,(int*)&size);

SERCOM_OUT:
//    sercom_out(com_out,err);
    sem_post(&sComMutex);
//    writelog(COMMAND_M,hdriver,"Control_Command: Release Mutex done");
    writelog(COMMAND_M,hdriver,"Control_Command: Control_command end 0x%04x err 0x%x",com_out,err);
    return err;
}

void CPco_com_cl_me4::sercom_out(WORD com_out,DWORD err)
{
    sem_post(&sComMutex);
    writelog(COMMAND_M,hdriver,"Control_Command: Release Mutex 0x%x done",sComMutex);
    writelog(COMMAND_M,hdriver,"Control_Command: Control_command end 0x%04x err 0x%x",com_out,err);
}

DWORD CPco_com_cl_me4::PCO_GetTransferParameter(void* buf,int length)
{
    memcpy(buf,&transferpar,length);
    return PCO_NOERROR;
}

DWORD CPco_com_cl_me4::PCO_SetTransferParameter(void* buf,int length)
{
    PCO_SC2_CL_TRANSFER_PARAM par;
    DWORD err=PCO_NOERROR;

    par.ClockFrequency=transferpar.ClockFrequency;
    par.CCline=transferpar.CCline;
    par.Transmit=transferpar.Transmit;
    par.DataFormat=transferpar.DataFormat;

    memcpy(&par,buf,length);

    if(par.baudrate!=transferpar.baudrate)
        err=set_baudrate(par.baudrate);

    if(err==PCO_NOERROR)
        err=set_cl_config(par);

    err=get_actual_cl_config();

    return err;
}
