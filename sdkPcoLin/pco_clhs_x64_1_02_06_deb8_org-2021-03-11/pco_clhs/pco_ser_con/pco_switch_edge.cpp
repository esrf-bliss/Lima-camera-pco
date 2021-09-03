//-----------------------------------------------------------------//
// Name        | pco_switch_edge.cpp         | Type: (*) source    //
//-------------------------------------------|       ( ) header    //
// Project     | pco.camera                  |       ( ) others    //
//-----------------------------------------------------------------//
// Platform    | Linux                                             //
//-----------------------------------------------------------------//
// Environment |                                                   //
//             |                                                   //
//-----------------------------------------------------------------//
// Purpose     | pco.camera - pco.edge shutter mode                //
//-----------------------------------------------------------------//
// Author      | MBL, PCO AG                                       //
//-----------------------------------------------------------------//
// Notes       | console program to query and set shutter mode     //
//             | of pco.edge camera using class Cpco_com           //
//             |                                                   //
//             |                                                   //
//-----------------------------------------------------------------//
// (c) 2010 - 2017 PCO AG                                          //
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
//  1.01     | 08.11.2010 | ported from Windows SDK                //
//           |            |                                        //
// --------- | ---------- | ---------------------------------------//
//  1.02     | 14.01.2011 |                                        //
//           |            |                                        //
// --------- | ---------- | ---------------------------------------//
//  1.03     | 24.01.2012 | logging class from library             //
//           |            |                                        //
//-----------------------------------------------------------------//
//  1.04     | 28.07.2016 | clhs camera                             //
//           |            | use setup functions                    //
//           |            |                                        //
//-----------------------------------------------------------------//
//  1.05     | 24.04.2017 | use flag                               //
//           |            |  GENERALCAPS1_NO_GLOBAL_SHUTTER        //
//           |            |                                        //
//-----------------------------------------------------------------//
//  0.0x     | xx.xx.2012 |                                        //
//           |            |                                        //
//-----------------------------------------------------------------//


#include "Cpco_com.h"
#include "Cpco_com_clhs.h"


using namespace std;

#define WAIT_MS_100 100

int switch_camera(CPco_com_clhs* camera,int mode,int cam_num);

int main(int argc, char* argv[])
{
  DWORD err;
  int cam_num=0;
  int mode=0;
  char infostr[100];
  SC2_Camera_Description_Response description;
  WORD camtype;
  DWORD serialnumber;
  int x;

  for(x=1;x<argc;x++)
  {
   char *b;
    
   b=strchr(argv[1],'-');
   if(b!=NULL)
   {
    if(((b[1]=='G' )||(b[1]=='g'))&&((b[2]=='S' )||(b[2]=='s')))
     mode=2; 
    if(((b[1]=='R' )||(b[1]=='r'))&&((b[2]=='S' )||(b[2]=='s')))
     mode=1; 
    if(((b[1]=='G' )||(b[1]=='g'))&&((b[2]=='R' )||(b[2]=='r')))
     mode=3; 
    
    if((b[1]=='B' )||(b[1]=='b'))
    {
     cam_num=atoi(b+2);
     if((cam_num<0)||(cam_num>3))
      cam_num=0; 
    }
   }
  }

  CPco_com_clhs *cam;
  cam = new CPco_com_clhs();
  
  printf("Open Camera %d ",cam_num);
  fflush(stdout);
  err=cam->Open_Cam(cam_num);
  if(err!=PCO_NOERROR)
  {
   printf("failed with error 0x%x, CR to close application\n",err);
   getchar();
   return -1;
  }
  printf("done\n");
  fflush(stdout);

  err=cam->PCO_GetCameraType(&camtype,&serialnumber);
  err=cam->PCO_GetCameraDescriptor(&description);
  err=cam->PCO_GetInfo(1,infostr,sizeof(infostr));
  if(err!=PCO_NOERROR)
   printf("PCO_GetInfo() Error 0x%x\n",err);
  else
  {
   printf("Camera name is: %s\n",infostr);
   printf("Camera serialnumber is: %d\n",serialnumber);
  }

  if(camtype!=CAMERATYPE_PCO_EDGE_HS)
  {
   printf("No pco.edge camera found\n");
   err=cam->Close_Cam();
   if(err!=PCO_NOERROR)
    printf("CloseCam() Error 0x%x\n",err);
  
   printf("CR to exit\n");
   getchar();
   return 0;
  }

  WORD  setup_id=0;
  DWORD setup_flag[4];
  WORD  length=sizeof(setup_flag);

  err=cam->PCO_GetCameraSetup(&setup_id,setup_flag,&length);
  if(err!=PCO_NOERROR) 
  {
   printf("PCO_GetCameraSetup returned 0x%x\n",err);
   err=cam->Close_Cam();
   if(err!=PCO_NOERROR)
    printf("CloseCam() Error 0x%x\n",err);
  
   printf("CR to exit\n");
   getchar();
   return 0;
  }

  if(mode==0)
  {
   if(setup_flag[0]==PCO_EDGE_SETUP_ROLLING_SHUTTER)
   {
    printf("Camera in RollingShutter mode\n");
    if((description.dwGeneralCaps1&GENERALCAPS1_NO_GLOBAL_SHUTTER)==GENERALCAPS1_NO_GLOBAL_SHUTTER)
    {
     printf("no other setup type allowed\n");
    }
    else
    {
     printf("'pco_switch_edge -GS' to set camera to GlobalShutter mode\n");
     printf("'pco_switch_edge -GR' to set camera to GlobalReset mode\n");
    }
   }
   else if(setup_flag[0]==PCO_EDGE_SETUP_GLOBAL_SHUTTER)
   {
    printf("Camera in GlobalShutter mode\n");
    printf("'pco_switch_edge -RS' to set camera to RollingShutter mode\n");
    printf("'pco_switch_edge -GR' to set camera to GlobalReset mode\n");
   }
   else if(setup_flag[0]==PCO_EDGE_SETUP_GLOBAL_RESET)
   {
    printf("Camera in GlobalShutter mode\n");
    printf("'pco_switch_edge -RS' to set camera to RollingShutter mode\n");
    printf("'pco_switch_edge -GS' to set camera to GlobalShutter mode\n");
   }
  }
  else if(mode==1)
  {
   if(setup_flag[0]==PCO_EDGE_SETUP_ROLLING_SHUTTER)
   {
    printf("Camera is already in RollingShutter mode\n");
    if((description.dwGeneralCaps1&GENERALCAPS1_NO_GLOBAL_SHUTTER)==GENERALCAPS1_NO_GLOBAL_SHUTTER)
    {
     printf("no other setup type allowed\n");
    }
    else
    {
     printf("'pco_switch_edge -GS' to set camera to GlobalShutter mode\n");
     printf("'pco_switch_edge -GR' to set camera to GlobalReset mode\n");
    }
   }
   else
   {
    printf("Set camera to RollingShutter\n");
    err=switch_camera(cam,mode,cam_num);
    if(err==PCO_NOERROR)
     printf("done successfully\n");
   }
  }
  else if(mode==2)
  {
   if(setup_flag[0]==PCO_EDGE_SETUP_GLOBAL_SHUTTER)
   {
    printf("Camera is already in GlobalShutter mode\n");
    printf("'pco_switch_edge -RS' to set camera to RollingShutter mode\n");
    printf("'pco_switch_edge -GR' to set camera to GlobalReset mode\n");
   }
   else
   {
    if((description.dwGeneralCaps1&GENERALCAPS1_NO_GLOBAL_SHUTTER)==GENERALCAPS1_NO_GLOBAL_SHUTTER)
    {
     printf("GlobalShutter mode not supported from this camera\n");
    } 
    else
    {
     printf("Set camera to GlobalShutter\n");
     err=switch_camera(cam,mode,cam_num);
     if(err==PCO_NOERROR)
      printf("done successfully\n");
    }
   }
  }
  else if(mode==3)
  {
   if(setup_flag[0]==PCO_EDGE_SETUP_GLOBAL_RESET)
   {
    printf("Camera is already in GlobalReset mode\n");
    printf("'pco_switch_edge -RS' to set camera to RollingShutter mode\n");
    printf("'pco_switch_edge -GS' to set camera to GlobalShutter mode\n");
   }
   else
   {
    if((description.dwGeneralCaps1&GENERALCAPS1_NO_GLOBAL_SHUTTER)==GENERALCAPS1_NO_GLOBAL_SHUTTER)
    {
     printf("GlobalReset mode not supported from this camera\n");
    } 
    else
    {
     err=switch_camera(cam,mode,cam_num);
     if(err==PCO_NOERROR)
      printf("done successfully\n");
    }
   }
  }

  err=cam->Close_Cam();
  if(err!=PCO_NOERROR)
   printf("CloseCam() Error 0x%x\n",err);
  
  printf("CR to exit\n");
  getchar();

  return 0;
}


int switch_camera(CPco_com_clhs* cam,int mode,int cam_num)
{
  DWORD err=PCO_NOERROR;
  int count;
  WORD  setup_id=0;
  DWORD setup_flag[4];
  WORD  length=sizeof(setup_flag);
  char infostr[100];

  cam->PCO_SetRecordingState(0);

  DWORD comtime=2000;
  cam->Set_Timeouts(&comtime,sizeof(comtime));
  if(err!=PCO_NOERROR)
  {
   printf("Set_Timeouts error 0x%x\n",err);
   return err;
  }

  if(mode==1)
   setup_flag[0]=PCO_EDGE_SETUP_ROLLING_SHUTTER;
  else if(mode==2)
   setup_flag[0]=PCO_EDGE_SETUP_GLOBAL_SHUTTER;
  else if(mode==3)
   setup_flag[0]=PCO_EDGE_SETUP_GLOBAL_RESET;

  setup_flag[1]=0;
  setup_flag[2]=0;
  setup_flag[3]=0;

  err=cam->PCO_SetCameraSetup(setup_id,setup_flag,length);
  if(err!=PCO_NOERROR)
  {
   printf("PCO_SetCameraSetup error 0x%x\n",err);
   return err;
  }

  err=cam->PCO_RebootCamera();  
  if(err!=PCO_NOERROR)
  {
   printf("PCO_RebootCamera error 0x%x\n",err);
   return err;
  }

  printf("Close camera and wait until it is reconnected\n");
  err=cam->Close_Cam();
  if(err!=PCO_NOERROR)
  {
   printf("Close error 0x%x\n",err);
   return err;
  }

  printf("Camera rebooting wait ");
  fflush(stdout);
  for(count=8000/WAIT_MS_100;count>0;count--)
  {
   cam->Sleep_ms(WAIT_MS_100);
   if((count%4)==0)
   {
    printf(".");
    fflush(stdout);
   }
  }
   
  printf("\n");
  printf("Camera reconnect wait ");
  count=6000/WAIT_MS_100;   
  fflush(stdout);
  do
  {
   err=cam->Open_Cam(cam_num);
   cam->Sleep_ms(WAIT_MS_100);
   if((count%2)==0)
   {
    printf(".");
    fflush(stdout);
   }
  }
  while((err!=PCO_NOERROR)&&(count-- > 0));
  
  printf("\n");
  if(err!=PCO_NOERROR)
  {
   printf("Error reconnecting camera 0x%x",err);
   return err;
  }

  err=cam->PCO_GetInfo(1,infostr,sizeof(infostr));
  if(err!=PCO_NOERROR)
   printf("PCO_GetInfo() Error 0x%x\n",err);
  else
   printf("Camera name is: %s\n",infostr);
  
  return err;
}

