//-----------------------------------------------------------------//
// Name        | Cpco_me4.cpp                | Type: (*) source    //
//-------------------------------------------|       ( ) header    //
// Project     | pco.camera                  |       ( ) others    //
//-----------------------------------------------------------------//
// Platform    | Linux                                             //
//-----------------------------------------------------------------//
// Environment |                                                   //
//             |                                                   //
//-----------------------------------------------------------------//
// Purpose     | pco.camera - CameraLink ME4 API                   //
//-----------------------------------------------------------------//
// Author      | MBL, PCO AG                                       //
//-----------------------------------------------------------------//
// Revision    | rev. 1.03                                         //
//-----------------------------------------------------------------//
// Notes       | In this file are all functions and definitions,   //
//             | for commiunication with CameraLink grabbers       //
//             |                                                   //
//             |                                                   //
//-----------------------------------------------------------------//
// (c) 2010 - 2012 PCO AG                                          //
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
// Free Software Foundation, Inc., 59 Temple Place                 //
// - Suite 330, Boston, MA 02111-1307, USA                         //
//-----------------------------------------------------------------//

//-----------------------------------------------------------------//
// Revision History:                                               //
//-----------------------------------------------------------------//
// Rev.:     | Date:      | Changed:                               //
// --------- | ---------- | ---------------------------------------//
//  1.01     | 08.10.2011 | ported from Windows SDK                //
//           |            |                                        //
//-----------------------------------------------------------------//
//  1.02     | 14.01.2011 | Set_DataFormat                         //
//           |            | use only standard hap-file             //
//           |            |                                        //
//-----------------------------------------------------------------//
//  1.03     | 24.01.2012 | use pcolog library                     //
//           |            | check transferred bytes                //
//           |            |                                        //
//-----------------------------------------------------------------//
//  0.0x     | xx.xx.2012 |                                        //
//           |            |                                        //
//-----------------------------------------------------------------//


#include "Cpco_grab_cl_me4.h"
#include "sisoboards.h"

static int sprintf_s(char* buf, int dwlen, const char* cp, ...)
{
    va_list arglist;
    va_start(arglist, cp);
    return vsnprintf(buf, dwlen, cp, arglist);
}

static int strcpy_s(char* buf, int dwlen ATTRIBUTE_UNUSED, const char* src)
{
    strcpy(buf,src);
    return 0;
}

/*
static int strcat_s(char* buf, int dwlen, const char* src)
{
  strcat(buf,src);
  return 0;
}
*/

CPco_grab_cl_me4::CPco_grab_cl_me4(CPco_com_cl_me4* camera)
{
    clog=NULL;
    hgrabber=(PCO_HANDLE)NULL;

    fg=NULL;
    pco_hap_loaded=0;
    act_width=act_height=100;
    act_linewidth=100;
    port=0;
    me_boardnr=-1;
    DataFormat=0;
    act_dmalength=100*100;

    buf_manager=0;
    pMem0=NULL;
    pMemInt=NULL;
    padr=NULL;
    size_alloc=0;
    nr_of_buffer=0;

    aquire_status=0;
    aquire_flag=0;
    last_picnr=0;

    ImageTimeout=10000;

    adr_line=NULL;
    line_alloc=0;

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
    memset(&clpar,0,sizeof(clpar));    
    memset(&description,0,sizeof(description));
    if(camera)
     cam=camera;
}

/*
CPco_grab_cl_me4::~CPco_grab_cl_me4()
{
    if(fg)
    {
        Free_Framebuffer();
        Close_Grabber();
    }
}
*/

void CPco_grab_cl_me4::SetLog(CPco_Log *elog)
{
    if(elog)
     clog=elog;
}

void CPco_grab_cl_me4::writelog(DWORD lev,PCO_HANDLE hdriver,const char *str,...)
{
    if(clog)
    {
        va_list arg;
        va_start(arg,str);
        clog->writelog(lev,hdriver,str,arg);
        va_end(arg);
    }
}



////////////////////////////////////////////////////////////////////////
// Fg_Error
//
// Read Error message and write to log
//
void CPco_grab_cl_me4::Fg_Error(Fg_Struct *fg)
{
    int error;
    PCO_HANDLE drv;
    error = Fg_getLastErrorNumber(fg);
    if(!fg)
        drv=(PCO_HANDLE)0x0001;
    else
        //drv=(PCO_HANDLE)fg;
        drv=(PCO_HANDLE)0x0002;
    writelog(ERROR_M,drv,"FG_Error %d, desc: %s",error,getErrorDescription(error));


}

DWORD CPco_grab_cl_me4::Get_actual_size(unsigned int *width,unsigned int *height,unsigned int *bitpix)
{
    if(width)
        *width=act_width;
    if(height)
        *height=act_height;
    if(bitpix)
        *bitpix=16;

    return PCO_NOERROR;
}

DWORD CPco_grab_cl_me4::Close_Grabber()
{
    //search if other port was opened before
    writelog(INIT_M,hgrabber,"close_grabber:");
    if(adr_line!=NULL)
    {
     free(adr_line);
     adr_line=NULL;
    }

    if(fg==NULL)
    {
        writelog(INIT_M,hgrabber,"close_grabber: Grabber was closed before");
        return PCO_NOERROR;
    }

    writelog(INIT_M,hgrabber,"close_grabber: Fg_FreeGrabber");
    Free_Framebuffer();
    Fg_FreeMemEx(fg, pMemInt);
    Fg_FreeGrabber(fg);
    writelog(INIT_M,hgrabber,"close_grabber: Grabber Free'd.");
    fg=NULL;
    me_boardnr=-1;

    return PCO_NOERROR;
}



DWORD CPco_grab_cl_me4::Set_Grabber_Timeout(int timeout)
{
    int err=PCO_NOERROR;

//the blocking functions support only timeouts in seconds
    ImageTimeout=timeout;

/*
    int val=timeout;
    if(Fg_setParameter(fg, FG_TIMEOUT,&val,port)<0)
    {
        Fg_Error(fg);
        writelog(ERROR_M,hgrabber,"Set_Grabber_Timeout: set FG_TIMEOUT failed");
        err=PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
    }
*/
    return err;
}

DWORD CPco_grab_cl_me4::Get_Grabber_Timeout(int *timeout)
{
    *timeout=ImageTimeout;
    return PCO_NOERROR;
}



DWORD CPco_grab_cl_me4::Allocate_Framebuffer(int bufnum)
{
    return Allocate_Framebuffer(bufnum,NULL);
}


DWORD CPco_grab_cl_me4::Allocate_Framebuffer(int bufnum,void *adr_in)
{
    int x,err;
    err=PCO_NOERROR;

    if(nr_of_buffer>0)
    {
        writelog(INTERNAL_1_M,hgrabber,"Allocate_Framebuffer: buffers already allocated");
        return PCO_ERROR_SDKDLL_BUFFERNOTVALID  | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
    }

    size_t total_buffer_size=act_dmalength*bufnum;

//    int picsize=act_width*act_height*sizeof(WORD);
//was    size_t total_buffer_size=picsize*bufnum;

#ifndef _M_X64
    if(bufnum>(int)(0x7FFFFFFF/act_dmalength))
        return PCO_ERROR_NOMEMORY  | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
#endif


    if((buf_manager&PCO_SC2_CL_INTERNAL_BUFFER)==PCO_SC2_CL_INTERNAL_BUFFER)
    {
        try
        {
            if((pMemInt = Fg_AllocMemEx(fg,total_buffer_size,bufnum))==NULL)
            {
                Fg_Error(fg);
                writelog(ERROR_M,hgrabber,"Allocate_Framebuffer: Fg_AllocMemEx failed");
                return PCO_ERROR_NOMEMORY | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
            }
        }
        catch(...)
        {
            return PCO_ERROR_NOMEMORY  | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
        }
        nr_of_buffer=bufnum;
        size_alloc=act_dmalength;

        writelog(BUFFER_M,hgrabber,"Allocate_Framebuffer: %d buffers size_alloc %d",nr_of_buffer,size_alloc);
        return err;
    }

    if((buf_manager&PCO_SC2_CL_EXTERNAL_BUFFER)==PCO_SC2_CL_EXTERNAL_BUFFER)
    {
        if((pMem0 = Fg_AllocMemHead(fg,total_buffer_size,bufnum))==NULL)
        {
            Fg_Error(fg);
            writelog(ERROR_M,hgrabber,"Allocate_Framebuffer: Fg_AllocMemHead failed");
            return PCO_ERROR_NOMEMORY  | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
        }

        if((buf_manager&PCO_SC2_CL_ONLY_BUFFER_HEAD)==0)
        {
            unsigned char **adr;
            if(adr_in==NULL)
            {
                x=0;
                adr=(unsigned char**)malloc(bufnum*sizeof(unsigned char*));
                if(adr!=NULL)
                {
                    padr=adr;
                    memset(padr,0,bufnum*sizeof(unsigned char*));
                    for(x=0;x<bufnum;x++)
                    {
                        padr[x]=(unsigned char*)malloc(act_dmalength);
                        if(padr[x]==NULL)
                            break;
                    }
                }
                else
                    err= PCO_ERROR_NOMEMORY | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
                nr_of_buffer=x;
            }
            else
            {
                padr=NULL;
                nr_of_buffer=bufnum;
                adr=(unsigned char**)adr_in;
            }

            for(x=0;x<nr_of_buffer;x++)
            {
                if(adr[x])
                {
                    if(Fg_AddMem(fg,adr[x],act_dmalength,x,pMem0)<FG_OK)
                    {
                        Fg_Error(fg);
                        writelog(ERROR_M,hgrabber,"Allocate_Framebuffer: Fg_AddMem failed");
                        err= PCO_ERROR_NOMEMORY | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
                        break;
                    }
                }
            }

            if(err!=PCO_NOERROR)
            {
                for(x=0;x<nr_of_buffer;x++)
                {
                    if(adr[x])
                    {
                        if(Fg_DelMem(fg,pMem0,x)!=FG_OK)
                        {
                            Fg_Error(fg);
                            writelog(ERROR_M,hgrabber,"Allocate_Framebuffer: Fg_DelMem failed");
                            err= PCO_ERROR_NOMEMORY  | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
                        }
                        if(padr)
                        {
                            free(adr[x]);
                        }
                    }
                }
                if(padr)
                    free(padr);
                padr=NULL;
                nr_of_buffer=0;

                if(Fg_FreeMemHead(fg,pMem0)!=FG_OK)
                {
                    Fg_Error(fg);
                    writelog(ERROR_M,hgrabber,"Allocate_Framebuffer: Fg_FreeMemHead failed");
                    err=PCO_ERROR_NOMEMORY | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
                }
            }
            else
                size_alloc=act_dmalength;
        }
        else
            nr_of_buffer=bufnum;
    }
    else
    {
        writelog(ERROR_M,hgrabber,"Allocate_Framebuffer: buf_manager_flag  0x%x not supported",buf_manager);
        return PCO_ERROR_WRONGVALUE | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
    }

    return err;
}

DWORD CPco_grab_cl_me4::Free_Framebuffer()
{
    return Free_Framebuffer(NULL);
}


DWORD CPco_grab_cl_me4::Free_Framebuffer(void *adr_in)
{
    int err;

    err=PCO_NOERROR;

    if(nr_of_buffer==0)
        return PCO_NOERROR;

    if((buf_manager&PCO_SC2_CL_INTERNAL_BUFFER)==PCO_SC2_CL_INTERNAL_BUFFER)
    {
        if(Fg_FreeMemEx(fg,pMemInt)!=FG_OK)
        {
            Fg_Error(fg);
            writelog(ERROR_M,hgrabber,"Free_Framebuffer: Fg_FreeMemEx failed");
            return PCO_ERROR_NOMEMORY | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
        }
        nr_of_buffer=0;
        size_alloc=0;
        pMemInt=NULL;
        writelog(BUFFER_M,hgrabber,"Free_Framebuffer: done");
        return err;
    }

    if((buf_manager&PCO_SC2_CL_EXTERNAL_BUFFER)==PCO_SC2_CL_EXTERNAL_BUFFER)
    {
        if((buf_manager&PCO_SC2_CL_ONLY_BUFFER_HEAD)==0)
        {
            unsigned char** adr;
            if(adr_in==NULL)
                adr=padr;
            else
                adr=(unsigned char**)adr_in;

            for(int x=0;x<nr_of_buffer;x++)
            {
                if(adr[x])
                {
                    if(Fg_DelMem(fg,pMem0,x)!=FG_OK)
                    {
                        Fg_Error(fg);
                        writelog(ERROR_M,hgrabber,"Free_Framebuffer: Fg_DelMem failed");
                        err= PCO_ERROR_NOMEMORY | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
                    }
                    if(padr)
                    {
                        free(adr[x]);
                        adr[x]=0;
                    }
                }
            }

            if(padr)
                free(padr);
            padr=NULL;

            nr_of_buffer=0;
            size_alloc=0;
        }

        if(Fg_FreeMemHead(fg,pMem0)!=FG_OK)
        {
            Fg_Error(fg);
            writelog(ERROR_M,hgrabber,"Free_Framebuffer: Fg_FreeMemHead failed");
            err=PCO_ERROR_NOMEMORY | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
        }
    }
    else
    {
        writelog(ERROR_M,hgrabber,"Free_Framebuffer: buf_manager_flag  0x%x not supported",buf_manager);
        return PCO_ERROR_WRONGVALUE | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
    }

    return err;
}

DWORD CPco_grab_cl_me4::Get_Framebuffer()
{
    return nr_of_buffer;
}

DWORD CPco_grab_cl_me4::Acquire_Image(void *address)
{
    DWORD err;
    unsigned int w, h;
    WORD *adr;
    WORD *out;
    int picnum;

    writelog(INFO_M,hgrabber,"Acquire_Image: clpar.Transmit 0x%x",clpar.Transmit);

    out = (WORD*) address;
    picnum = 1;
    err = Start_Acquire(1);
    if((clpar.Transmit&0x01)==0)
    {
     if(cam&&cam->IsOpen())
      err=cam->PCO_RequestImage();
    }

    if(err==PCO_NOERROR)
     err = Wait_For_Next_Image(&picnum,ImageTimeout/1000);

    if(err==PCO_NOERROR)
     err=Check_DMA_Length(picnum);

    if(err==PCO_NOERROR)
    {
     err = Get_Framebuffer_adr(picnum, (void**)&adr);
     err = Unblock_buffer(picnum);
    }
    Stop_Acquire();

    if(err==PCO_NOERROR)
    {
     Get_actual_size(&w, &h, NULL);
     if(out)
     {
        Extract_Image(out, adr, w, h);
     }
    }
    return err;
}


DWORD CPco_grab_cl_me4::Get_Image(WORD Segment,DWORD ImageNr,void *address)
{
    int err;
    unsigned int w, h;
    WORD *adr;
    WORD *out;
    int picnum;

    writelog(INFO_M,hgrabber,"Get_Image: Segment %d ImageNr %d",Segment,ImageNr);

    out = (WORD*) address;
    picnum = 1;
    err = Start_Acquire(1);

    if((Segment==0)||(ImageNr==0))
    {
     if((clpar.Transmit&0x01)==0)
     {
      if(cam&&cam->IsOpen())
       err=cam->PCO_RequestImage();
     }
    }
    else
    {
     if(cam&&cam->IsOpen())
      err=cam->PCO_ReadImagesFromSegment(Segment,ImageNr,ImageNr);
     else
     {
      writelog(ERROR_M,hgrabber,"Get_Image: no associated camera found");
      return PCO_ERROR_DRIVER_NODRIVER | PCO_ERROR_DRIVER_CAMERALINK;
     }
    }

    if(err==PCO_NOERROR)
     err = Wait_For_Next_Image(&picnum,ImageTimeout/1000);

    if(err==PCO_NOERROR)
     err=Check_DMA_Length(picnum);

    if(err==PCO_NOERROR)
    {
     err = Get_Framebuffer_adr(picnum, (void**)&adr);
     err = Unblock_buffer(picnum);
    }
    Stop_Acquire();

    if(err==PCO_NOERROR)
    {
     Get_actual_size(&w, &h, NULL);
     if (out)
     {
      Extract_Image(out, adr, w, h);
     }
    }
    return err;
}


DWORD CPco_grab_cl_me4::Start_Acquire(int nr_of_ima)
{
    return Start_Acquire(nr_of_ima,0);
}

DWORD CPco_grab_cl_me4::Start_Acquire_NonBlock(int nr_of_ima)
{
    return Start_Acquire(nr_of_ima,PCO_SC2_CL_NON_BLOCKING_BUFFER);
}

DWORD CPco_grab_cl_me4::Start_Acquire(int nr_of_ima,int flag)
{
    int err;
    frameindex_t index;
    int pic_count;

    err=PCO_NOERROR;

    if(nr_of_buffer<=0)
    {
        writelog(ERROR_M,hgrabber,"Start_Aquire: buffers must be allocated before");
        return PCO_ERROR_WRONGVALUE | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
    }

    if(cam&&cam->IsOpen())
     cam->PCO_GetTransferParameter(&clpar,sizeof(clpar));

    aquire_flag&=~(PCO_SC2_CL_START_CONT|PCO_SC2_CL_NON_BLOCKING_BUFFER|PCO_SC2_CL_BLOCKING_BUFFER);

    if(nr_of_ima<=0)
        pic_count=GRAB_INFINITE;
    else
        pic_count=nr_of_ima;

    if((flag&PCO_SC2_CL_START_CONT)==PCO_SC2_CL_START_CONT)
    {
        pic_count=GRAB_INFINITE;
        aquire_flag|=PCO_SC2_CL_START_CONT;
    }

    dma_mem* pMem;

    if((buf_manager&PCO_SC2_CL_EXTERNAL_BUFFER)==PCO_SC2_CL_EXTERNAL_BUFFER)
        pMem=pMem0;
    else if((buf_manager&PCO_SC2_CL_INTERNAL_BUFFER)==PCO_SC2_CL_INTERNAL_BUFFER)
        pMem=pMemInt;
    else
    {
        writelog(ERROR_M,hgrabber,"Start_Aquire: buf_manager_flag  0x%x not supported",buf_manager);
        return PCO_ERROR_WRONGVALUE | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
    }

    if((flag&PCO_SC2_CL_NON_BLOCKING_BUFFER)==PCO_SC2_CL_NON_BLOCKING_BUFFER)
    {
        aquire_flag|=PCO_SC2_CL_NON_BLOCKING_BUFFER;
        writelog(PROCESS_M,hgrabber,"Start_Aquire: aquire_flag  PCO_SC2_CL_NON_BLOCKING_BUFFER");
        aquire_status=ACQ_STANDARD;
    }
    else //use  PCO_SC2_CL_BLOCKING_BUFFER as default
    {
        aquire_status=ACQ_BLOCK;
        aquire_flag|=PCO_SC2_CL_BLOCKING_BUFFER;
        writelog(PROCESS_M,hgrabber,"Start_Aquire: aquire_flag  PCO_SC2_CL_BLOCKING_BUFFER");
    }

    if(Fg_AcquireEx(fg,port,pic_count,aquire_status,pMem)!=FG_OK)
    {
        Fg_Error(fg);
        writelog(ERROR_M,hgrabber,"Start_Aquire: Fg_AcquireEx failed");
        err=PCO_ERROR_DRIVER_SYSERR  | PCO_ERROR_DRIVER_CAMERALINK;
    }
    index=Fg_getStatusEx(fg,NUMBER_OF_IMAGES_IN_PROGRESS,1,port,pMem);
    writelog(PROCESS_M,hgrabber,"Start_Aquire: Start IMAGES_IN_PROGRESS: %d",index);

    //set startflag
    if(err==PCO_NOERROR)
        aquire_flag|=PCO_SC2_CL_STARTED;

    return err;
}



DWORD CPco_grab_cl_me4::Stop_Acquire()
{
    int err,ret;
    frameindex_t index;

    err=PCO_NOERROR;

    dma_mem *pMem;
    if((buf_manager&PCO_SC2_CL_EXTERNAL_BUFFER)==PCO_SC2_CL_EXTERNAL_BUFFER)
        pMem=pMem0;
    else if((buf_manager&PCO_SC2_CL_INTERNAL_BUFFER)==PCO_SC2_CL_INTERNAL_BUFFER)
        pMem=pMemInt;
    else
    {
        writelog(ERROR_M,hgrabber,"Stop_Aquire: buf_manager_flag  0x%x not supported",buf_manager);
        return PCO_ERROR_WRONGVALUE | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
    }

    writelog(PROCESS_M,hgrabber,"Stop_Aquire: pMem %x port %d",pMem,port);

#ifdef _DEBUG
    index=Fg_getStatusEx(fg,NUMBER_OF_GRABBED_IMAGES,1,port,pMem);
    writelog(INTERNAL_1_M,hgrabber,"Stop_Aquire: GRABBED_IMAGES: %d",index);
    index=Fg_getStatusEx(fg,NUMBER_OF_LOST_IMAGES,1,port,pMem);
    writelog(INTERNAL_1_M,hgrabber,"Stop_Aquire:LOST_IMAGES: %d",index);
    index=Fg_getStatusEx(fg,NUMBER_OF_ACT_IMAGE,1,port,pMem);
    writelog(INTERNAL_1_M,hgrabber,"Stop_Aquire: ACTUAL_IMAGE: %d",index);
    index=Fg_getStatusEx(fg,NUMBER_OF_LAST_IMAGE,1,port,pMem);
    writelog(INTERNAL_1_M,hgrabber,"Stop_Aquire: LAST_IMAGE: %d",index);
    index=Fg_getStatusEx(fg,NUMBER_OF_NEXT_IMAGE,1,port,pMem);
    writelog(INTERNAL_1_M,hgrabber,"Stop_Aquire: NEXT_IMAGE: %d",index);
    index=Fg_getStatusEx(fg,NUMBER_OF_IMAGES_IN_PROGRESS,1,port,pMem);
    writelog(INTERNAL_1_M,hgrabber,"Stop_Aquire: IMAGES_IN_PROGRESS: %d",index);
#endif

    ret=Fg_stopAcquireEx(fg,port,pMem,STOP_ASYNC);
    if((ret!=FG_OK)&&(ret!=FG_TRANSFER_NOT_ACTIVE))
    {
        Fg_Error(fg);
        writelog(ERROR_M,hgrabber,"Stop_Acquire: Fg_stopAcquireEx failed");
        err=PCO_ERROR_DRIVER_SYSERR  | PCO_ERROR_DRIVER_CAMERALINK;
    }
    writelog(PROCESS_M,hgrabber,"Stop_Acquire: Fg_stopAcquireEx return 0x%x",err);

    index=Fg_getStatusEx(fg,NUMBER_OF_BLOCKED_IMAGES,1,port,pMem);
    writelog(INTERNAL_1_M,hgrabber,"Stop_Aquire: BLOCKED_IMAGES: %d",index);


    if(Fg_setStatusEx(fg,FG_UNBLOCK_ALL,nr_of_buffer,port,pMem)!=FG_OK)
    {
        Fg_Error(fg);
        writelog(ERROR_M,hgrabber,"Stop_Aquire: setEx FG_UNBLOCK_ALL failed");
    }

    if(err==PCO_NOERROR)
      aquire_flag&=~PCO_SC2_CL_STARTED;

    writelog(PROCESS_M,hgrabber,"Stop_Acquire: Fg_stopAquire()");
    return err;
}

BOOL CPco_grab_cl_me4::started()
{
    return aquire_flag&PCO_SC2_CL_STARTED;
}


//DWORD CPco_grab_cl_me4::Wait_For_Image(int *nr_of_pic,int timeout)
//{
//    int err;
//    frameindex_t index=-1;

//    err=PCO_NOERROR;

//    //  void* pMem;
//    dma_mem* pMem;
//    if((buf_manager&PCO_SC2_CL_EXTERNAL_BUFFER)==PCO_SC2_CL_EXTERNAL_BUFFER)
//        pMem=pMem0;
//    else if((buf_manager&PCO_SC2_CL_INTERNAL_BUFFER)==PCO_SC2_CL_INTERNAL_BUFFER)
//        pMem=pMemInt;
//    else
//    {
//        writelog(ERROR_M,hgrabber,"Wait_for_image: buf_manager_flag  0x%x not supported",buf_manager);
//        return PCO_ERROR_WRONGVALUE | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
//    }

//    if(aquire_status==ACQ_STANDARD)
//    {
//        //wait for picture done, with blocking function
//        index=Fg_getLastPicNumberBlockingEx(fg,*nr_of_pic,port,timeout,pMem);
//    }

//    if(index<0)
//    {
//        Fg_Error(fg);
//        writelog(ERROR_M,hgrabber,"Wait_for_image: timeout");
//        err=PCO_ERROR_TIMEOUT | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
//        *nr_of_pic=0;
//    }
//    else
//        *nr_of_pic=(int)index;

//    return err;
//}


DWORD CPco_grab_cl_me4::Wait_For_Next_Image(int *nr_of_pic,int timeout)
{
    int err;
    frameindex_t index;
    unsigned int imageTag=0;
    *nr_of_pic=0;

    err=PCO_NOERROR;
    if(timeout<2)
     timeout=2;
       
    index=Fg_getImageEx(fg,SEL_NEXT_IMAGE,0,port,timeout,pMemInt);
    if(index<0)
    {
     Fg_Error(fg);
     writelog(ERROR_M,hgrabber,"Wait_for_next_image: timeout index %d",index);
     err=PCO_ERROR_TIMEOUT | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
    }
    else
    {
     if(Fg_getParameterEx(fg,FG_IMAGE_TAG,&imageTag,port,pMemInt,index)!=0)
      Fg_Error(fg);
     *nr_of_pic=(int)index;
     writelog(PROCESS_M,hgrabber,"Wait_for_next_image: returned %d tag %d",index,imageTag&0xFFFF);
    }
    return err;
}

DWORD CPco_grab_cl_me4::Wait_For_Images(int nr_of_pic,int timeout)
{
    int err;
    frameindex_t index;
    unsigned int imageTag=0;

    err=PCO_NOERROR;
    if(nr_of_pic>nr_of_buffer)
    {
     writelog(ERROR_M,hgrabber,"Wait_for_images: nr_of_pic %d >nr_of_buffer %d",nr_of_pic,nr_of_buffer);
     err=PCO_ERROR_NOMEMORY | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
    }

    if(aquire_status==ACQ_STANDARD)
     index=Fg_getImageEx(fg,SEL_NUMBER,nr_of_pic,port,timeout,pMemInt);
    else
    {
     writelog(ERROR_M,hgrabber,"Wait_for_images: wrong aquire_status %d",aquire_status);
     return PCO_ERROR_WRONGVALUE | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
    }

    if(index<0)
    {
        Fg_Error(fg);
        writelog(ERROR_M,hgrabber,"Wait_for_images: timeout index %d ",index);
        err=PCO_ERROR_TIMEOUT | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
    }
    else
    {
     if(Fg_getParameterEx(fg,FG_IMAGE_TAG,&imageTag,port,pMemInt,index)!=0)
      Fg_Error(fg);
     writelog(PROCESS_M,hgrabber,"Wait_for_images: returned %d tag %d",index,imageTag&0xFFFF);
    }

    if(index<nr_of_pic)
    {
     err=PCO_ERROR_TIMEOUT | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
     writelog(ERROR_M,hgrabber,"Wait_for_images: timeout index %d < nr_of_pic %d",index,nr_of_pic);
    } 
    return err;
}




DWORD CPco_grab_cl_me4::Get_last_Image(int *nr_of_pic)
{
    int err;
    frameindex_t index=-1;

    err=PCO_NOERROR;

    //  void* pMem;
    dma_mem* pMem;
    if((buf_manager&PCO_SC2_CL_EXTERNAL_BUFFER)==PCO_SC2_CL_EXTERNAL_BUFFER)
        pMem=pMem0;
    else if((buf_manager&PCO_SC2_CL_INTERNAL_BUFFER)==PCO_SC2_CL_INTERNAL_BUFFER)
        pMem=pMemInt;
    else
    {
        writelog(ERROR_M,hgrabber,"Get_last_Image: buf_manager_flag  0x%x not supported",buf_manager);
        return PCO_ERROR_WRONGVALUE | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
    }

    if(aquire_status==ACQ_STANDARD)
    {
        //wait for picture done, with blocking function
        index=Fg_getLastPicNumberEx(fg,port,pMem);
    }

    if(index<0)
    {
        Fg_Error(fg);
        writelog(ERROR_M,hgrabber,"Get_last_Image: error");
        err=PCO_ERROR_TIMEOUT | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
        *nr_of_pic=0;
    }
    else
        *nr_of_pic=(int)index;

    return err;
}

DWORD CPco_grab_cl_me4::Get_Framebuffer_adr(int nr_of_pic,void **adr)
{
    int err;
    err=PCO_NOERROR;


    dma_mem* pMem;
    if((buf_manager&PCO_SC2_CL_EXTERNAL_BUFFER)==PCO_SC2_CL_EXTERNAL_BUFFER)
        pMem=pMem0;
    else if((buf_manager&PCO_SC2_CL_INTERNAL_BUFFER)==PCO_SC2_CL_INTERNAL_BUFFER)
        pMem=pMemInt;
    else
    {
        writelog(ERROR_M,hgrabber,"Get_Framebuffer_adr: buf_manager_flag  0x%x not supported",buf_manager);
        return PCO_ERROR_WRONGVALUE | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
    }

    if(nr_of_pic<=nr_of_buffer)
     *adr=(void*)Fg_getImagePtrEx(fg,nr_of_pic,port,pMem);
    else
    {
        *adr=NULL;
        writelog(ERROR_M,hgrabber,"Get_Framebuffer_adr: only %d buffers allocated",nr_of_buffer);
        return PCO_ERROR_WRONGVALUE | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
    }
    return err;
}


DWORD CPco_grab_cl_me4::Check_DMA_Length(int num)
{
    size_t dmaLength;
    DWORD err=PCO_NOERROR;

    dma_mem* pMem;
    if((buf_manager&PCO_SC2_CL_EXTERNAL_BUFFER)==PCO_SC2_CL_EXTERNAL_BUFFER)
        pMem=pMem0;
    else if((buf_manager&PCO_SC2_CL_INTERNAL_BUFFER)==PCO_SC2_CL_INTERNAL_BUFFER)
        pMem=pMemInt;
    else
    {
        writelog(ERROR_M,hgrabber,"Check_DMA_Length: buf_manager_flag  0x%x not supported",buf_manager);
        return PCO_ERROR_WRONGVALUE | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
    }

    if(pMem)
    {
        dmaLength = num;
        if(Fg_getParameterEx(fg,FG_TRANSFER_LEN,&dmaLength,port,pMem,num)!=FG_OK)
            Fg_Error(fg);
    }
    else
    {
        dmaLength = num;
        if(Fg_getParameter(fg, FG_TRANSFER_LEN,&dmaLength,port)!=FG_OK)
            Fg_Error(fg);
    }

    if(dmaLength<(size_t)act_dmalength)
    {
        writelog(ERROR_M,hgrabber,"picnum %d dmaLenght %d != %d set error on this buffer",num,dmaLength,act_dmalength);
        err=(PCO_ERROR_DRIVER_BUFFER_DMASIZE | PCO_ERROR_DRIVER_CAMERALINK);
    }
    else
        writelog(PROCESS_M,hgrabber,"dmaLenght %d act_dmalength %d",dmaLength,act_dmalength);



    return err;
}

DWORD CPco_grab_cl_me4::Unblock_buffer(int num)
{
    dma_mem* pMem;
    if((buf_manager&PCO_SC2_CL_EXTERNAL_BUFFER)==PCO_SC2_CL_EXTERNAL_BUFFER)
        pMem=pMem0;
    else if((buf_manager&PCO_SC2_CL_INTERNAL_BUFFER)==PCO_SC2_CL_INTERNAL_BUFFER)
        pMem=pMemInt;
    else
    {
        writelog(ERROR_M,hgrabber,"Get_image_buffer_adr: buf_manager_flag  0x%x not supported",buf_manager);
        return PCO_ERROR_WRONGVALUE | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
    }
    if(Fg_setStatusEx(fg,FG_UNBLOCK,num,port,pMem)!=FG_OK)
    {
        Fg_Error(fg);
        writelog(ERROR_M,hgrabber,"Unblock_buffer: error");
        return PCO_ERROR_WRONGVALUE | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
    }
    writelog(PROCESS_M,hgrabber,"Unblock_buffer: num %d done",num);
    return PCO_NOERROR;
}


DWORD CPco_grab_cl_me4::SetBitAlignment(int align)
{
  DWORD err=PCO_NOERROR;
  int val;

  if(align==0)
   val=FG_LEFT_ALIGNED;
  else
   val=FG_RIGHT_ALIGNED;
  if(Fg_setParameter(fg, FG_BITALIGNMENT,&val,port)<0)
  {
    Fg_Error(fg);
    writelog(ERROR_M,hgrabber,"SetBitAlignment: FG_BITALIGNMENT failed");
    err=PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
  }
  else
   writelog(INFO_M,hgrabber,"SetBitAlignment: FG_BITALIGNMENT %s val %d",val ? "FG_LEFT_ALIGNED" : "FG_RIGHT_ALIGNED",val);
  return err;
}

DWORD CPco_grab_cl_me4::Get_Camera_Settings()
{
  DWORD err=PCO_NOERROR;

  if(cam&&cam->IsOpen())
  {
   cam->PCO_GetTransferParameter(&clpar,sizeof(clpar));
   writelog(INFO_M,hgrabber,"Get_Camera_Settings: clpar.Transmit        0x%x",clpar.Transmit);
   writelog(INFO_M,hgrabber,"Get_Camera_Settings: clpar.Dataformat      0x%x",clpar.DataFormat);
   writelog(INFO_M,hgrabber,"Get_Camera_Settings: clpar.Clockfrequency  %d",clpar.ClockFrequency);
   writelog(INFO_M,hgrabber,"Get_Camera_Settings: clpar.Baudrate        %d",clpar.baudrate);

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


DWORD CPco_grab_cl_me4::Set_Grabber_Param(int id,int value)
{
  switch(id)
  {
   default:
    return PCO_ERROR_WRONGVALUE | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;

   case ID_DOUBLEMODE:
    cam_doublemode=value;
    break;

   case ID_TIMESTAMP:
    cam_timestampmode=value;
    break;

   case ID_NOISEFILTER:
    cam_noisefilter=value;
    break;

   case ID_COLORSENSOR:
    cam_colorsensor=value;
    break;

   case ID_PIXELRATE:
    cam_pixelrate=value;
    break;

   case ID_ALIGNMENT:
    cam_align=value;
    break;
  }
  return PCO_NOERROR;
}


//-----------------------------------------------------------------//
// class CPco_me4_edge                                             //
//                                                                 //
// pco.edge55 in rolling shutter mode or global reset mode         //
//                                                                 //
// start                                                           //
//-----------------------------------------------------------------//

CPco_grab_cl_me4_edge::CPco_grab_cl_me4_edge(CPco_com_cl_me4* camera) : CPco_grab_cl_me4(camera)
{

}


DWORD CPco_grab_cl_me4_edge::Open_Grabber(int board)
{
    int err=PCO_NOERROR;

    if(fg!=NULL)
    {
        writelog(INIT_M,(PCO_HANDLE)1,"Open_Grabber: grabber was opened before");
        return PCO_NOERROR;
    }

    int num,type;

    //reset this settings
    me_boardnr=-1;
    port=PORT_A;
    num=0;

    for(int i=0;i<4;i++)
    {
        type=Fg_getBoardType(i);
        if(type == FG_ERROR)
        {
         writelog(ERROR_M,hgrabber,"open_grabber: Fg_getBoardType returned FG_ERROR");
	 continue;
        }
        writelog(INIT_M,hgrabber,"open_grabber: index %d type 0x%x",i,type);

        if(type<0)
        {
         writelog(ERROR_M,hgrabber,"open_grabber: Fg_getBoardType returned 0x%x",type);
         break;
        }

        switch(type)
        {
	 case PN_MICROENABLE4AD4CL:
	 case PN_MICROENABLE4VD4CL:
         {
          if(num==board)
          {
           me_boardnr=i;
           break;
          }
          num++;
         }
         default:
          continue; 
        } 
    }

    if(me_boardnr<0)
    {
        writelog(ERROR_M,hgrabber,"open_grabber: no board found, which does meet all requirements");
        return PCO_ERROR_DRIVER_NODRIVER | PCO_ERROR_DRIVER_CAMERALINK;
    }

    if(cam->IsOpen())
    {
     cam->PCO_GetCameraType(&camtype,&serialnumber);
     if((camtype!=CAMERATYPE_PCO_EDGE))
     {
      writelog(ERROR_M,hgrabber,"open_grabber: camtype is not pco_edge use other grabber class");
      return PCO_ERROR_DRIVER_NODRIVER | PCO_ERROR_DRIVER_CAMERALINK;
     }
    }
    hgrabber=(PCO_HANDLE)(0x1000+board);

    char buf[MAX_PATH];
    //  int hFile;
    //  char *sisodir;


    memset(buf,0,MAX_PATH);
    strcpy_s(buf,sizeof(buf),"FullAreaGray8_HS");
    pco_hap_loaded=0;

    writelog(INIT_M,hgrabber,"open_grabber: Fg_Init(%s,%d)",buf,me_boardnr);
    if((fg = Fg_Init(buf,me_boardnr)) == NULL)
    {
        err=Fg_getLastErrorNumber(NULL);
        writelog(ERROR_M,hgrabber,"open_grabber: Fg_Init(%s,%d) failed with err %d",buf,me_boardnr,err);
        if(err == FG_NO_VALID_LICENSE)
         writelog(ERROR_M,hgrabber,"open_grabber: Missing license for this mode");
        Fg_Error(fg);
        hgrabber=(PCO_HANDLE)NULL;
        me_boardnr=-1;
        return PCO_ERROR_DRIVER_NODRIVER | PCO_ERROR_DRIVER_CAMERALINK;
    }

    err=PCO_NOERROR;

//FG_CAMSTATUS is not supported from Applet FullAreaGray8_HS
//therefore the function call is discarded
    int val;

    if(err==PCO_NOERROR)
    {
         val=FG_CL_8BIT_FULL_10;
         if(Fg_setParameter(fg, FG_CAMERA_LINK_CAMTYP,&val,port)<0)
         {
             Fg_Error(fg);
             writelog(ERROR_M,hgrabber,"open_grabber: FG_CAMERA_LINK_CAMTYP failed");
             err=PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
         }
    }

    if(err==PCO_NOERROR)
    {
         val=FG_GRAY;
         if(Fg_setParameter(fg, FG_FORMAT,&val,port)<0)
         {
             Fg_Error(fg);
             writelog(ERROR_M,hgrabber,"open_grabber: FG_FORMAT failed");
             err=PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
         }
    }

    if(err==PCO_NOERROR)
    {
         val=FREE_RUN;
         if(Fg_setParameter(fg, FG_TRIGGERMODE,&val,port)<0)
         {
              Fg_Error(fg);
              writelog(ERROR_M,hgrabber,"init_grabber: set FG_TRIGGERMODE failed");
              err=PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
         }
    }

    if(err==PCO_NOERROR)
    {
        val=0x7FFFFFF; //(PCO_SC2_IMAGE_TIMEOUT_L*10)/1000;
        if(Fg_setParameter(fg, FG_TIMEOUT,&val,port)<0)
        {
            Fg_Error(fg);
            writelog(ERROR_M,hgrabber,"init_grabber: set FG_TIMEOUT failed");
            err=PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
        }
    }

//set default format and size
    DataFormat=SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER|PCO_CL_DATAFORMAT_5x16;
    if(err==PCO_NOERROR)
        err=Set_Grabber_Size(2560,2160);

    if(err!=PCO_NOERROR)
    {
        Fg_FreeGrabber(fg);
        fg=NULL;
        me_boardnr=-1;
        hgrabber=(PCO_HANDLE)NULL;
    }

    buf_manager=PCO_SC2_CL_INTERNAL_BUFFER;

    return err;
}


DWORD CPco_grab_cl_me4_edge::PostArm(int userset)
{

  DWORD err=PCO_NOERROR;
  DWORD format;
  WORD lut,lutpar;
  writelog(PROCESS_M,hgrabber,"%s(%d)",__FUNCTION__,userset);

  if(cam&&cam->IsOpen())
  {
   cam->PCO_GetTransferParameter(&clpar,sizeof(clpar));

   if(err==PCO_NOERROR)
    err=cam->PCO_GetActualSize(&cam_width,&cam_height);
   if(err==PCO_NOERROR)
    err=cam->PCO_GetPixelRate(&cam_pixelrate);    
   if((err==PCO_NOERROR)&&(userset==0))
   {
    format=DataFormat;

    clpar.DataFormat=DataFormat&~PCO_CL_DATAFORMAT_MASK;
    if((cam_width>1920)&&(cam_pixelrate>=286000000))
    {
     clpar.DataFormat|=PCO_CL_DATAFORMAT_5x12L;
     writelog(PROCESS_M,hgrabber,"PostArm: width>1920 %d && pixelrate >=286000000 %d Dataformat 0x%x",cam_width,cam_pixelrate,clpar.DataFormat);
     lut=0x1612;
     lutpar=0;
    }
    else
    {
     clpar.DataFormat|=PCO_CL_DATAFORMAT_5x16;
     writelog(PROCESS_M,hgrabber,"PostArm: width<=1920 %d || pixelrate<286000000 %d Dataformat 0x%x",cam_width,cam_pixelrate,clpar.DataFormat);
     lut=0;
     lutpar=0;
    }

    if(format!=clpar.DataFormat)
    {
     err=cam->PCO_SetLut(lut,lutpar);
     if(err==PCO_NOERROR)
      err=cam->PCO_SetTransferParameter(&clpar,sizeof(clpar));
     if(err==PCO_NOERROR)
      err=cam->PCO_ArmCamera();
    }
   }
  }

  if(err==PCO_NOERROR)
   err=Get_Camera_Settings();

  if((err==PCO_NOERROR)&&(userset==0))
  {

   writelog(PROCESS_M,hgrabber,"PostArm: call Set_DataFormat(0x%x)",clpar.DataFormat);
   err=Set_DataFormat(clpar.DataFormat);
   if(err==PCO_NOERROR)
   {
    writelog(PROCESS_M,hgrabber,"PostArm: call Set_Grabber_Size(%d,%d)",cam_width,cam_height);
    err=Set_Grabber_Size(cam_width,cam_height);
   }
   if(err==PCO_NOERROR)
   {
    if((nr_of_buffer>0)&&(size_alloc!=act_dmalength))
    {
     int bufnum=nr_of_buffer;
     writelog(PROCESS_M,hgrabber,"PostArm: call Free and Allocate_Framebuffer(%d)",bufnum);
     Free_Framebuffer();
     err=Allocate_Framebuffer(bufnum);
    }
   }
  }
  return err;
}


DWORD CPco_grab_cl_me4_edge::Set_Grabber_Size(DWORD width,DWORD height)
{
    int w;

    act_width=width;
    act_height=height;
    act_linewidth=width;

//use only default hapfile therefore all other pathes deleted
    if((DataFormat&PCO_CL_DATAFORMAT_MASK)==PCO_CL_DATAFORMAT_5x16)
    {
     w=width*2;
    }
    else if(  ((DataFormat&PCO_CL_DATAFORMAT_MASK)==PCO_CL_DATAFORMAT_5x12)
            ||((DataFormat&PCO_CL_DATAFORMAT_MASK)==PCO_CL_DATAFORMAT_5x12L)
            ||((DataFormat&PCO_CL_DATAFORMAT_MASK)==PCO_CL_DATAFORMAT_5x12R))
    {
     w=width*2*12;
     w/=16;
    }

    writelog(PROCESS_M,hgrabber,"Set_Grabber_Size: set FG_WIDTH:%d",w);
    if(Fg_setParameter(fg, FG_WIDTH,&w,port)<0)
    {
     Fg_Error(fg);
     writelog(ERROR_M,hgrabber,"Set_Grabber_Size: set FG_WIDTH failed");
     return PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
    }

    if(Fg_setParameter(fg, FG_HEIGHT,&height,port)<0)
    {
     Fg_Error(fg);
     writelog(ERROR_M,hgrabber,"Set_Grabber_Size: set FG_HEIGHT failed");
     return PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
    }

    act_dmalength=w*height;
    writelog(PROCESS_M,hgrabber,"Set_Grabber_Size: done w:%d h:%d lw %d dmalen %d",act_width,act_height,act_linewidth,act_dmalength);

    return PCO_NOERROR;
}


DWORD CPco_grab_cl_me4_edge::Set_DataFormat(DWORD format)
{
    DWORD err=PCO_NOERROR;

    if(DataFormat!=format)
    {
     writelog(PROCESS_M,hgrabber,"Data format changed to 0x%x",format);

     switch(format&PCO_CL_DATAFORMAT_MASK)
     {
       case PCO_CL_DATAFORMAT_5x16:
        err=PCO_NOERROR;
       break;
 

       case PCO_CL_DATAFORMAT_5x12:
       case PCO_CL_DATAFORMAT_5x12L:
       case PCO_CL_DATAFORMAT_5x12R:
        err=PCO_NOERROR;
       break;

       default:
        err=PCO_ERROR_WRONGVALUE | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
       break;
     }

     if(err==PCO_NOERROR)
     {
      switch(format&SCCMOS_FORMAT_MASK)
      {
       case SCCMOS_FORMAT_TOP_BOTTOM:
       case SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER:
       case SCCMOS_FORMAT_CENTER_TOP_CENTER_BOTTOM:
       case SCCMOS_FORMAT_CENTER_TOP_BOTTOM_CENTER:
       case SCCMOS_FORMAT_TOP_CENTER_CENTER_BOTTOM:
        err=PCO_NOERROR;
       break;

       default:
        err=PCO_ERROR_WRONGVALUE | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
       break;
      }
     }

     if(err==PCO_NOERROR)
      DataFormat=format;
    }   

    return err;
}


void CPco_grab_cl_me4_edge::Extract_Image(void *bufout, void *bufin, int width, int height,int line_width ATTRIBUTE_UNUSED)
{
    Extract_Image(bufout,bufin,width,height);
}

void CPco_grab_cl_me4_edge::Extract_Image(void *bufout,void *bufin,int width,int height)
{
//use only default hapfile therefore all other pathes deleted
    reorder_image(bufout,bufin,width,height,DataFormat);
    writelog(PROCESS_M,hgrabber,"Extract_Image: reorder_image() DataFormat 0x%x done",DataFormat);
}

void CPco_grab_cl_me4_edge::Get_Image_Line(void *bufout,void *bufin,int linenumber,int width,int height)
{
    get_image_line(bufout,bufin,linenumber,width,height,DataFormat);
    writelog(PROCESS_M,hgrabber,"Get_Image_Line: get_image_line() DataFormat 0x%x done",DataFormat);
}


DWORD CPco_grab_cl_me4_edge::Check_DRAM_Status(char *mess,int mess_size,int *fill)
{
    unsigned int Status1, Status2, Status3, Status4;
    int rcode;
    int Id1, Id2, Id3, Id4;


    if(pco_hap_loaded!=0x03)
    {
        *fill=0;
        return PCO_NOERROR;
    }

    Id1 = Fg_getParameterIdByName(fg, "Device1_Process0_DRAM1_Overflow");
    Id2 = Fg_getParameterIdByName(fg, "Device1_Process0_DRAM2_Overflow");

    rcode = Fg_getParameter(fg, Id1, &Status1, FG_PARAM_TYPE_UINT32_T);
    if(rcode)
    {
        if(mess)
            sprintf_s(mess,mess_size,"\nError read Status1 from board\n");
        return PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
    }
    rcode = Fg_getParameter(fg, Id2, &Status2, FG_PARAM_TYPE_UINT32_T);
    if(rcode)
    {
        if(mess)
            sprintf_s(mess,mess_size,"\nError read Status2 from board\n");
        return PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
    }

    Id3 = Fg_getParameterIdByName(fg, "Device1_Process0_DRAM1_FillLevel");
    rcode = Fg_getParameter(fg, Id3, &Status3, FG_PARAM_TYPE_UINT32_T);
    if(rcode)
    {
        if(mess)
            sprintf_s(mess,mess_size,"\nError read Status3 from board\n");
        return PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
    }
    *fill=Status3;

    if (Status1 || Status2)
    {
        Id4 = Fg_getParameterIdByName(fg, "Device1_Process0_DRAM2_FillLevel");
        rcode = Fg_getParameter(fg, Id3, &Status3, FG_PARAM_TYPE_UINT32_T);
        if(rcode)
        {
            if(mess)
                sprintf_s(mess,mess_size,"\nError read Status3 from board\n");
            return PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
        }

        rcode =	Fg_getParameter(fg, Id4, &Status4, FG_PARAM_TYPE_UINT32_T);
        if(rcode)
        {
            if(mess)
                sprintf_s(mess,mess_size,"\nError read Status4 from board\n");
            return PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
        }

        if(mess)
            sprintf_s(mess,mess_size,"\nOverflow detected: DRAM0 (%d - %d percent) DRAM1 (%d - %d percent)\n",
                      Status1, Status3, Status2, Status4);
        return PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
    }

    return PCO_NOERROR;
}


//-----------------------------------------------------------------//
// class CPco_me4_edge42                                           //
//                                                                 //
// pco.edge42 in rolling shutter mode or global reset mode         //
//                                                                 //
// start                                                           //
//-----------------------------------------------------------------//


CPco_grab_cl_me4_edge42::CPco_grab_cl_me4_edge42(CPco_com_cl_me4* camera) : CPco_grab_cl_me4(camera)
{

}


DWORD CPco_grab_cl_me4_edge42::Open_Grabber(int board)
{
    int err=PCO_NOERROR;

    if(fg!=NULL)
    {
        writelog(INIT_M,(PCO_HANDLE)1,"Open_Grabber: grabber was opened before");
        return PCO_NOERROR;
    }

    int num,type;

    //reset this settings
    me_boardnr=-1;
    port=PORT_A;
    num=0;

    for(int i=0;i<4;i++)
    {
        type=Fg_getBoardType(i);
        if(type == FG_ERROR)
        {
         writelog(ERROR_M,hgrabber,"open_grabber: Fg_getBoardType returned FG_ERROR");
	 continue;
        }
        writelog(INIT_M,hgrabber,"open_grabber: index %d type 0x%x",i,type);

        if(type<0)
        {
         writelog(ERROR_M,hgrabber,"open_grabber: Fg_getBoardType returned 0x%x",type);
         break;
        }

        switch(type)
        {
	 case PN_MICROENABLE4AD4CL:
	 case PN_MICROENABLE4VD4CL:
         {
          if(num==board)
          {
           me_boardnr=i;
           break;
          }
          num++;
         }
         default:
          continue; 
        } 
    }

    if(me_boardnr<0)
    {
        writelog(ERROR_M,hgrabber,"open_grabber: no board found, which does meet all requirements");
        return PCO_ERROR_DRIVER_NODRIVER | PCO_ERROR_DRIVER_CAMERALINK;
    }

    if(cam->IsOpen())
    {
     cam->PCO_GetCameraType(&camtype,&serialnumber);
     if((camtype!=CAMERATYPE_PCO_EDGE_42))
     {
      writelog(ERROR_M,hgrabber,"open_grabber: camtype is not pco_edge42 use other grabber class");
      return PCO_ERROR_DRIVER_NODRIVER | PCO_ERROR_DRIVER_CAMERALINK;
     }
    }
    hgrabber=(PCO_HANDLE)(0x1000+board);

    char buf[MAX_PATH];
    //  int hFile;
    //  char *sisodir;


    memset(buf,0,MAX_PATH);
    strcpy_s(buf,sizeof(buf),"FullAreaGray8_HS");
    pco_hap_loaded=0;

    writelog(INIT_M,hgrabber,"open_grabber: Fg_Init(%s,%d)",buf,me_boardnr);
    if((fg = Fg_Init(buf,me_boardnr)) == NULL)
    {
        err=Fg_getLastErrorNumber(NULL);
        writelog(ERROR_M,hgrabber,"open_grabber: Fg_Init(%s,%d) failed with err %d",buf,me_boardnr,err);
        if(err == FG_NO_VALID_LICENSE)
         writelog(ERROR_M,hgrabber,"open_grabber: Missing license for this mode");
        Fg_Error(fg);
        hgrabber=(PCO_HANDLE)NULL;
        me_boardnr=-1;
        return PCO_ERROR_DRIVER_NODRIVER | PCO_ERROR_DRIVER_CAMERALINK;
    }

    err=PCO_NOERROR;

//FG_CAMSTATUS is not supported from Applet FullAreaGray8_HS
//therefore the function call is discarded
    int val;
    if(err==PCO_NOERROR)
    {
         val=FG_CL_8BIT_FULL_10;
         if(Fg_setParameter(fg, FG_CAMERA_LINK_CAMTYP,&val,port)<0)
         {
             Fg_Error(fg);
             writelog(ERROR_M,hgrabber,"open_grabber: FG_CAMERA_LINK_CAMTYP failed");
             err=PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
         }
    }

    if(err==PCO_NOERROR)
    {
         val=FG_GRAY;
         if(Fg_setParameter(fg, FG_FORMAT,&val,port)<0)
         {
             Fg_Error(fg);
             writelog(ERROR_M,hgrabber,"open_grabber: FG_FORMAT failed");
             err=PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
         }
    }

    if(err==PCO_NOERROR)
    {
         val=FREE_RUN;
         if(Fg_setParameter(fg, FG_TRIGGERMODE,&val,port)<0)
         {
              Fg_Error(fg);
              writelog(ERROR_M,hgrabber,"init_grabber: set FG_TRIGGERMODE failed");
              err=PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
         }
    }

    if(err==PCO_NOERROR)
    {
        val=0x7FFFFFF; //(PCO_SC2_IMAGE_TIMEOUT_L*10)/1000;
        if(Fg_setParameter(fg, FG_TIMEOUT,&val,port)<0)
        {
            Fg_Error(fg);
            writelog(ERROR_M,hgrabber,"init_grabber: set FG_TIMEOUT failed");
            err=PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
        }
    }

//set default format and size
    DataFormat=SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER|PCO_CL_DATAFORMAT_5x16;
    if(err==PCO_NOERROR)
        err=Set_Grabber_Size(2060,2048);

    if(err!=PCO_NOERROR)
    {
        Fg_FreeGrabber(fg);
        fg=NULL;
        me_boardnr=-1;
        hgrabber=(PCO_HANDLE)NULL;
    }

    buf_manager=PCO_SC2_CL_INTERNAL_BUFFER;

    return err;
}


DWORD CPco_grab_cl_me4_edge42::PostArm(int userset)
{

  DWORD err=PCO_NOERROR;
  writelog(PROCESS_M,hgrabber,"%s(%d)",__FUNCTION__,userset);

  if(err==PCO_NOERROR)
   err=Get_Camera_Settings();

  if((err==PCO_NOERROR)&&(userset==0))
  {
   writelog(PROCESS_M,hgrabber,"PostArm: call Set_DataFormat(0x%x)",clpar.DataFormat);
   err=Set_DataFormat(clpar.DataFormat);
   if(err==PCO_NOERROR)
   {
    writelog(PROCESS_M,hgrabber,"PostArm: call Set_Grabber_Size(%d,%d)",cam_width,cam_height);
    err=Set_Grabber_Size(cam_width,cam_height);
   }
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
  }
  return err;
}



DWORD CPco_grab_cl_me4_edge42::Set_Grabber_Size(DWORD width,DWORD height)
{
    act_linewidth=width;
    act_width=width;
    act_height=height;

    if(width%8)
    {
     width=((width/8)+1)*8;
     act_linewidth=width;     
    }

//use only default hapfile therefore all other pathes deleted
    if((DataFormat&PCO_CL_DATAFORMAT_MASK)==PCO_CL_DATAFORMAT_5x16)
    {
     width=width*2;
    }

    writelog(PROCESS_M,hgrabber,"Set_Grabber_Size: set FG_WIDTH:%d",width);
    if(Fg_setParameter(fg, FG_WIDTH,&width,port)<0)
    {
     Fg_Error(fg);
     writelog(ERROR_M,hgrabber,"Set_Grabber_Size: set FG_WIDTH failed");
     return PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
    }

    if(Fg_setParameter(fg, FG_HEIGHT,&height,port)<0)
    {
     Fg_Error(fg);
     writelog(ERROR_M,hgrabber,"Set_Grabber_Size: set FG_HEIGHT failed");
     return PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
    }

    act_dmalength=act_linewidth*act_height*2;
    writelog(PROCESS_M,hgrabber,"Set_Grabber_Size: done w:%d h:%d lw %d dmalen %d",act_width,act_height,act_linewidth,act_dmalength);

    if((line_alloc<act_linewidth*2)||(adr_line==NULL))
    {
     if(adr_line)
      free(adr_line);  
     adr_line=(unsigned char*)malloc(act_linewidth*2*2);
     if(adr_line==NULL)
     {
      writelog(ERROR_M,hgrabber,"Extract_Image: allopcation of adr_line failed");
      return PCO_ERROR_NOMEMORY | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
     } 
     line_alloc=act_linewidth*2*2;
    }
    return PCO_NOERROR;
}

DWORD CPco_grab_cl_me4_edge42::Set_DataFormat(DWORD format)
{
    DWORD err=PCO_NOERROR;

    if(DataFormat!=format)
    {
     writelog(PROCESS_M,hgrabber,"Data format changed to 0x%x",format);

     if((format&PCO_CL_DATAFORMAT_MASK)!=PCO_CL_DATAFORMAT_5x16)
     {
      writelog(ERROR_M,hgrabber,"Wrong Data format 0x%x ",format);
      err=PCO_ERROR_WRONGVALUE | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
     }

     if(err==PCO_NOERROR)
     {
      switch(format&SCCMOS_FORMAT_MASK)
      {
       case SCCMOS_FORMAT_TOP_BOTTOM:
       case SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER:
       case SCCMOS_FORMAT_CENTER_TOP_CENTER_BOTTOM:
       case SCCMOS_FORMAT_CENTER_TOP_BOTTOM_CENTER:
       case SCCMOS_FORMAT_TOP_CENTER_CENTER_BOTTOM:
        err=PCO_NOERROR;
       break;

       default:
        err=PCO_ERROR_WRONGVALUE | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
       break;
      }
     }

     if(err==PCO_NOERROR)
      DataFormat=format;
    }
    return err;
}



void CPco_grab_cl_me4_edge42::Extract_Image(void *bufout, void *bufin, int width, int height,int line_width ATTRIBUTE_UNUSED)
{
    Extract_Image(bufout,bufin,width,height);
}

void CPco_grab_cl_me4_edge42::Extract_Image(void *bufout,void *bufin,int width,int height)
{
//use only default hapfile therefore all other pathes deleted
    if(act_linewidth!=act_width)
    {
     unsigned char* adr_in;
     unsigned char* adr_out;

     if(adr_line==NULL)
     {
      adr_line=(unsigned char*)malloc(act_linewidth*2*2);
      if(adr_line==NULL)
      {
       writelog(ERROR_M,hgrabber,"Extract_Image: allocation of adr_line failed");
       return; // PCO_ERROR_NOMEMORY | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
      } 
      line_alloc=act_linewidth*2*2;
     } 

     adr_in=(unsigned char*)bufin;
     adr_out=(unsigned char*)bufin;
     for(DWORD y=1;y<act_height;y++)
     {
      adr_in+=act_linewidth*2;
      adr_out+=act_width*2;
      memcpy(adr_line,adr_in,act_linewidth*2);
      memcpy(adr_out,adr_line,act_width*2);
     }
     writelog(PROCESS_M,hgrabber,"Extract_Image: sort_lines from act_linewidth %d -> act_width %d done",act_linewidth,act_width);
    }
    reorder_image(bufout,bufin,width,height,DataFormat);
    writelog(PROCESS_M,hgrabber,"Extract_Image: reorder_image() DataFormat 0x%x done",DataFormat);
}

void CPco_grab_cl_me4_edge42::Get_Image_Line(void *bufout,void *bufin,int linenumber,int width,int height)
{
    if(act_linewidth!=act_width)
    {
     DWORD y;
     unsigned char* adr_in;
     unsigned char* adr_out;

     if(adr_line==NULL)
     {
      adr_line=(unsigned char*)malloc(act_linewidth*2*2);
      if(adr_line==NULL)
      {
       writelog(ERROR_M,hgrabber,"Extract_Image: allopcation of adr_line failed");
       return; // PCO_ERROR_NOMEMORY | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
      } 
      line_alloc=act_linewidth*2*2;
     } 
     adr_in=(unsigned char*)bufin;
     adr_out=(unsigned char*)bufin;
     for(y=1;y<(DWORD)linenumber;y++)
     {
      adr_in+=act_linewidth*2;
      adr_out+=act_width*2;
      memcpy(adr_line,adr_in,act_linewidth*2);
      memcpy(adr_out,adr_line,act_width*2);
     }
     writelog(PROCESS_M,hgrabber,"Extract_Image: sort_lines %d from act_linewidth %d -> act_width %d done",y,act_linewidth,act_width);
    }
    get_image_line(bufout,bufin,linenumber,width,height,DataFormat);
    writelog(PROCESS_M,hgrabber,"Get_Image_Line: get_image_line() DataFormat 0x%x done",DataFormat);
}


//-----------------------------------------------------------------//
// class CPco_me4_edge42                                           //
//                                                                 //
// end                                                             //
//-----------------------------------------------------------------//



//-----------------------------------------------------------------//
// class CPco_me4_camera                                           //
//                                                                 //
// pco.1600, pco.2000, pco.4000, pco.dimax                         //
//                                                                 //
// start                                                           //
//-----------------------------------------------------------------//

CPco_grab_cl_me4_camera::CPco_grab_cl_me4_camera(CPco_com_cl_me4* camera) : CPco_grab_cl_me4(camera)
{
}


DWORD CPco_grab_cl_me4_camera::Open_Grabber(int board)
{
    int err=PCO_NOERROR;

    if(fg!=NULL)
    {
        writelog(INIT_M,hgrabber,"Open_Grabber: grabber was opened before");
        return PCO_NOERROR;
    }

    int num,type;

    //reset this settings
    me_boardnr=-1;
    port=PORT_A;

    //scan for me4_board and adjust me4_boardnr
    num=0;
    for(int i=0;i<4;i++)
    {
        type=Fg_getBoardType(i);
        if(type == FG_ERROR)
        {
         writelog(ERROR_M,hgrabber,"open_grabber: Fg_getBoardType returned FG_ERROR");
	 continue;
        }
        writelog(INIT_M,hgrabber,"open_grabber: index %d type 0x%x",i,type);

        if(type<0)
        {
         writelog(ERROR_M,hgrabber,"open_grabber: Fg_getBoardType returned 0x%x",type);
         break;
        }

        switch(type)
        {
         case PN_MICROENABLE4AD1CL:
	 case PN_MICROENABLE4AD4CL:
	 case PN_MICROENABLE4VD1CL:
	 case PN_MICROENABLE4VD4CL:
         {
          if(((num*2)==board)||((num*2)+1==board))
          {
           me_boardnr=i;
           break;
          }
          num++;
         }

         default:
          continue; 
        } 
    }

    if(me_boardnr<0)
    {
        writelog(ERROR_M,hgrabber,"open_grabber: no board found, which does meet all requirements");
        return PCO_ERROR_DRIVER_NODRIVER | PCO_ERROR_DRIVER_CAMERALINK;
    }


    if(cam->IsOpen())
    {
     cam->PCO_GetCameraType(&camtype,&serialnumber);
     if((camtype==CAMERATYPE_PCO_EDGE)||(camtype==CAMERATYPE_PCO_EDGE_42)||(camtype==CAMERATYPE_PCO_EDGE_GL))
     {
      writelog(ERROR_M,hgrabber,"open_grabber: camtype is pco_edge use specific grabber class");
      return PCO_ERROR_DRIVER_NODRIVER | PCO_ERROR_DRIVER_CAMERALINK;
     }
    }
    hgrabber=(PCO_HANDLE)(0x1000+board);


    char buf[40];
    memset(buf,0,40);

    strcpy_s(buf,sizeof(buf),"DualAreaGray16");
    writelog(INIT_M,hgrabber,"open_grabber: Fg_Init(%s,%d)",buf,me_boardnr);
    if((fg = Fg_Init(buf,me_boardnr)) == NULL)
    {
        writelog(ERROR_M,hgrabber,"open_grabber: Fg_Init(%s,%d) failed",buf,me_boardnr);
        if(Fg_getLastErrorNumber(NULL)== FG_NO_VALID_LICENSE)
            writelog(ERROR_M,hgrabber,"open_grabber: Missing license for this mode");
        Fg_Error(fg);
        hgrabber=(PCO_HANDLE)NULL;
        me_boardnr=-1;
        return PCO_ERROR_DRIVER_NODRIVER | PCO_ERROR_DRIVER_CAMERALINK;
    }

    err=PCO_NOERROR;
    if(Fg_getParameter(fg,FG_CAMSTATUS,&num,port)<0)
    {
        Fg_Error(fg);
        writelog(ERROR_M,hgrabber,"open_grabber: Fg_getParameter(,FG_CAMSTATUS,...) failed");
        err=PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
    }

    if((num==0)&&(err==PCO_NOERROR))
    {
        writelog(ERROR_M,hgrabber,"open_grabber: Fg_getParameter(,FG_CAMSTATUS,...) no camera connected. board: %d num: %d",board,num);
        err=PCO_ERROR_DRIVER_NOTINIT | PCO_ERROR_DRIVER_CAMERALINK;
    }
    int val;

    if(err==PCO_NOERROR)
    {
        val=0;
        if(Fg_setParameter(fg, FG_XOFFSET,&val,port)<0)
        {
            Fg_Error(fg);
            writelog(ERROR_M,hgrabber,"init_grabber: set FG_XOFFSET failed");
            err=PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
        }
    }
    if(err==PCO_NOERROR)
    {
        val=0;
        if(Fg_setParameter(fg, FG_YOFFSET,&val,port)<0)
        {
            Fg_Error(fg);
            writelog(ERROR_M,hgrabber,"init_grabber: set FG_YOFFSET failed");
            err=PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
        }
    }

    if(err==PCO_NOERROR)
    {
        val=FG_CL_SINGLETAP_16_BIT;
        if(Fg_setParameter(fg, FG_CAMERA_LINK_CAMTYP,&val,port)<0)
        {
            Fg_Error(fg);
            writelog(ERROR_M,hgrabber,"set_grabber_par: FG_CAMERA_LINK_CAMTYP failed");
            err=PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
        }
    }
    if(err==PCO_NOERROR)
    {
        val=FG_GRAY16;
        if(Fg_setParameter(fg, FG_FORMAT,&val,port)<0)
        {
            Fg_Error(fg);
            writelog(ERROR_M,hgrabber,"set_grabber_par: FG_FORMAT failed");
            err=PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
        }
    }

    if(err==PCO_NOERROR)
    {
        val=FREE_RUN;
        if(Fg_setParameter(fg, FG_TRIGGERMODE,&val,port)<0)
        {
            Fg_Error(fg);
            writelog(ERROR_M,hgrabber,"init_grabber: set FG_TRIGGERMODE failed");
            err=PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
        }
    }

    if(err==PCO_NOERROR)
    {
        val=0x7FFFFFF; //(PCO_SC2_IMAGE_TIMEOUT_L*10)/1000;
        if(Fg_setParameter(fg, FG_TIMEOUT,&val,port)<0)
        {
            Fg_Error(fg);
            writelog(ERROR_M,hgrabber,"init_grabber: set FG_TIMEOUT failed");
            err=PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
        }
    }

    DataFormat=PCO_CL_DATAFORMAT_1x16;

    //set default sizes
    if(err==PCO_NOERROR)
        err=Set_Grabber_Size(1280,1024);

    if(err!=PCO_NOERROR)
    {
        Fg_FreeMemEx(fg, pMemInt);
        Fg_FreeGrabber(fg);
        fg=NULL;
        me_boardnr=-1;
        hgrabber=(PCO_HANDLE)NULL;
    }

    buf_manager=PCO_SC2_CL_INTERNAL_BUFFER;

    return err;
}

DWORD CPco_grab_cl_me4_camera::PostArm(int userset)
{

  DWORD err=PCO_NOERROR;
  writelog(PROCESS_M,hgrabber,"%s(%d)",__FUNCTION__,userset);

  if(err==PCO_NOERROR)
   err=Get_Camera_Settings();

  if((err==PCO_NOERROR)&&(userset==0))
  {
   writelog(PROCESS_M,hgrabber,"PostArm: call SetBitAlignment(%d)",cam_align);
   err=SetBitAlignment(cam_align);

   if(err==PCO_NOERROR)
   {
    writelog(PROCESS_M,hgrabber,"PostArm: call Set_DataFormat(0x%x)",clpar.DataFormat);
    err=Set_DataFormat(clpar.DataFormat);
   }

   if(err==PCO_NOERROR)
   {
    writelog(PROCESS_M,hgrabber,"PostArm: call Set_Grabber_Siz(%d,%d)",cam_width,cam_height);
    err=Set_Grabber_Size(cam_width,cam_height);
   }
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
  }
  return err;
}


DWORD CPco_grab_cl_me4_camera::Set_Grabber_Size(DWORD width,DWORD height)
{

    act_width=width;
    act_height=height;
    act_linewidth=width;


    writelog(PROCESS_M,hgrabber,"Set_Grabber_Size: set FG_WIDTH:%d",width);
    if(Fg_setParameter(fg, FG_WIDTH,&width,port)<0)
    {
        Fg_Error(fg);
        writelog(ERROR_M,hgrabber,"Set_Grabber_Size: set FG_WIDTH failed");
        return PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
    }

    if(Fg_setParameter(fg, FG_HEIGHT,&height,port)<0)
    {
        Fg_Error(fg);
        writelog(ERROR_M,hgrabber,"Set_Grabber_Size: set FG_HEIGHT failed");
        return PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
    }

    act_dmalength=width*height*sizeof(WORD);
    writelog(PROCESS_M,hgrabber,"Set_Grabber_Size: done w:%d h:%d lw %d dmalen %d",act_width,act_height,act_linewidth,act_dmalength);


    return PCO_NOERROR;
}

DWORD CPco_grab_cl_me4_camera::Set_DataFormat(DWORD format)
{
    DWORD err=PCO_NOERROR;
    int val;

    if(DataFormat!=format)
    {
     writelog(PROCESS_M,hgrabber,"Data format changed to 0x%x",format);

     if((format&PCO_CL_DATAFORMAT_MASK)==PCO_CL_DATAFORMAT_1x16)
     {
      if(err==PCO_NOERROR)
      {
       val=FG_CL_SINGLETAP_16_BIT;
       if(Fg_setParameter(fg, FG_CAMERA_LINK_CAMTYP,&val,port)<0)
       {
        Fg_Error(fg);
        writelog(ERROR_M,hgrabber,"Set_DataFormat: FG_CAMERA_LINK_CAMTYP failed");
        err=PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
       }
       writelog(INIT_M,hgrabber,"Set_DataFormat: FG_CL_SINGLETAP_16_BIT");
      }
      if(err==PCO_NOERROR)
      {
        val=FG_GRAY16;
        if(Fg_setParameter(fg, FG_FORMAT,&val,port)<0)
        {
            Fg_Error(fg);
            writelog(ERROR_M,hgrabber,"Set_DataFormat: FG_FORMAT failed");
            err=PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
        }
      }
     }
     else if((format&PCO_CL_DATAFORMAT_MASK)==PCO_CL_DATAFORMAT_2x12)
     {
      if(err==PCO_NOERROR)
      {
       val=FG_CL_DUALTAP_12_BIT;
       if(Fg_setParameter(fg, FG_CAMERA_LINK_CAMTYP,&val,port)<0)
       {
        Fg_Error(fg);
        writelog(ERROR_M,hgrabber,"Set_DataFormat: FG_CAMERA_LINK_CAMTYP failed");
        err=PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
       }
       writelog(INIT_M,hgrabber,"Set_DataFormat: FG_CL_DUALTAP_12_BIT");
      }
      if(err==PCO_NOERROR)
      {
        val=FG_GRAY16;
        if(Fg_setParameter(fg, FG_FORMAT,&val,port)<0)
        {
            Fg_Error(fg);
            writelog(ERROR_M,hgrabber,"Set_DataFormat: FG_FORMAT failed");
            err=PCO_ERROR_DRIVER_DATAERROR | PCO_ERROR_DRIVER_CAMERALINK;
        }
      }
     }
     else
      err=PCO_ERROR_WRONGVALUE | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;

     if(err==PCO_NOERROR)
     {
      switch(format&SCCMOS_FORMAT_MASK)
      {
       case SCCMOS_FORMAT_TOP_BOTTOM:
        err=PCO_NOERROR;
       break;

       case SCCMOS_FORMAT_TOP_CENTER_BOTTOM_CENTER:
       case SCCMOS_FORMAT_CENTER_TOP_CENTER_BOTTOM:
       case SCCMOS_FORMAT_CENTER_TOP_BOTTOM_CENTER:
       case SCCMOS_FORMAT_TOP_CENTER_CENTER_BOTTOM:
        if(camtype==CAMERATYPE_PCO_EDGE_MT)
         err=PCO_NOERROR;
        else
         err=PCO_ERROR_WRONGVALUE | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
       break;

       default:
        err=PCO_ERROR_WRONGVALUE | PCO_ERROR_DRIVER | PCO_ERROR_DRIVER_CAMERALINK;
       break;
      }
     }

     Fg_getParameter(fg, FG_PIXELDEPTH,&val,port);
     writelog(INIT_M,hgrabber,"Set_DataFormat: FG_PIXELDEPTH %d",val);
    }

    if(err==PCO_NOERROR)
     DataFormat=format;
    return err;
}


void CPco_grab_cl_me4_camera::Extract_Image(void *bufout, void *bufin, int width, int height,int line_width ATTRIBUTE_UNUSED)
{
    Extract_Image(bufout,bufin,width,height);
}


void CPco_grab_cl_me4_camera::Extract_Image(void *bufout,void *bufin,int width,int height)
{
    if(DataFormat&SCCMOS_FORMAT_MASK)
    {
     reorder_image(bufout,bufin,width,height,DataFormat);
     writelog(PROCESS_M,hgrabber,"Extract_Image: reorder_image() DataFormat 0x%x done",DataFormat);
    }
    else
    {
     memcpy(bufout,bufin,width*height*sizeof(WORD));
     writelog(PROCESS_M,hgrabber,"Extract_Image: memcpy done");

//workaround for pco.1200 DUAL_TAP set first 2 pixels to 0
     if(camtype==CAMERATYPE_PCO1200HS)
     {
      if((DataFormat&PCO_CL_DATAFORMAT_MASK)==PCO_CL_DATAFORMAT_2x12)
      {
       if((cam_timestampmode == TIMESTAMP_MODE_BINARY)||(cam_timestampmode == TIMESTAMP_MODE_BINARYANDASCII))
       {
        writelog(PROCESS_M,hgrabber,"timestamp correction");

        DWORD *adr=(DWORD *)bufout;
        *adr=0;
       } 
      }
     }
    }

}

void CPco_grab_cl_me4_camera::Get_Image_Line(void *bufout,void *bufin,int linenumber,int width,int height ATTRIBUTE_UNUSED)
{
    WORD *buf;
    buf=(WORD*)bufin;
    buf+=(linenumber-1)*width;

    if(DataFormat&SCCMOS_FORMAT_MASK)
    {
     get_image_line(bufout,bufin,linenumber,width,height,DataFormat);
     writelog(PROCESS_M,hgrabber,"Get_Image_Line: get_image_line() DataFormat 0x%x done",DataFormat);
    }
    else
    {
//workaround for pco.1200 DUAL_TAP set first 2 pixels to 0
     if((camtype==CAMERATYPE_PCO1200HS)&&(linenumber==1))
     {
      if((DataFormat&PCO_CL_DATAFORMAT_MASK)==PCO_CL_DATAFORMAT_2x12)
      {
       if((cam_timestampmode == TIMESTAMP_MODE_BINARY)||(cam_timestampmode == TIMESTAMP_MODE_BINARYANDASCII))
       {
        writelog(PROCESS_M,hgrabber,"timestamp correction");
 
        DWORD *adr=(DWORD *)buf;
        *adr=0;
       } 
      }
     }
     memcpy(bufout,buf,width*sizeof(WORD));
     writelog(PROCESS_M,hgrabber,"Get_Image_Line: memcpy done");
    }
}






