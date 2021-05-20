-------------------------------------------------------------------
                 P C O  AG
     TECHNICAL  INFORMATION  DOCUMENT
-------------------------------------------------------------------

README FOR SOFTWARE:  
pco_camera/pco_me4

FOR PCO PRODUCT:
pco.edge,pco.edge42,pco.edgeGL or pco.camera connected to me4 grabber

DESCRIPTION:
This packages includes one communication and a few grabber classes for
pco.cameras connected to an SiliconSoftware ME4 grabber 
Additional there are some simple testprograms, which show the use of the class functions.


VERSION 1
SUBVERSION 1
SUBAGE 14
LNX 64

CONTENTS:

Archive File Name: PCO_me4_x64_1_01_xx.tar.gz

Installation Instructions:

Install driver and runtime from Silicon Software and check functionality of me4 board.

Copy the pco_me4_x64_1_01_xx.tar.gz to a distinct directory (e.g. PCO). 
Use "tar -xpvzf pco_me4_1_01_xx.tar.gz" to get the sources.
After this you will find the following new directories and files as noted below.

Use "make" to compile all testprograms

If the programs can't find the siso libraries, you might have to add the following lines 
to /etc/environment (global) or /etc/bash.bashrc (shell only):

export SISODIR5="<path_to_siso_dir>"

export PATH=$SISODIR5/bin/:$PATH
export LD_LIBRARY_PATH=$SISODIR5/lib:$LD_LIBRARY_PATH

and replace the <path_to_siso_dir> with the appropriate path, usually ending with SiliconSoftware.

If pco.edge GlobalShutter camera should be used copy 
the pco-hap file from ./pco_me4/pco_hap to $SISODIR5/Hardware Applets/mE4VD4-CL/ directory.

The environment setting must also be included in the run environment of QT-Creator, if a QT-Application is started out of the QT-Creator. 


To use the dynamic libraries script 'symlink_pco' can be used to create symbolic links to the 
pco libraries in pco_camera/pco_common/pco_libdyn. 
'symlink_pco -b' will create the links in directory 'current directory'/bindyn
'symlink_pco -u' will create the links in directory /usr/local/lib (write permissions needed)
'symlink_pco "path"' will create the links in user selected directory "path"
To update the linker cache '/sbin/ldconfig' should be used afterwards 


Filelist:
./pco_camera
parent directory

./pco_camera/pco_common
Files which are used also from other interface implementations

./pco_camera/pco_common/pco_classes
Source code and header files for Class Cpco_com
Communication to any PCO camera through PCO telegram structures

./pco_camera/pco_common/pco_include
PCO header files

./pco_camera/pco_common/pco_lib
static PCO library files (*.a)

./pco_camera/pco_common/pco_libdyn
dynamic PCO library files (*.so)

./pco_camera/pco_common/qt_pco_camera
Common source files for qt based application

./pco_camera/pco_me4/Makefile
Makefile to compile pco_me4 libraries and console testprograms.
Does also call 'symlink_pco -b'

./pco_camera/pco_me4/pco_classes
Source code and header file for Class Cpco_com_cl_me4
Source code and header file for Classes Cpco_grab_cl_me4_edge,Cpco_grab_cl_me4_edge42,Cpco_grab_cl_me4_camera
Source code and header file for Class Cpco_grab_cl_me4_edge_GL

./pco_me4/pco_hap
hap-file for pco_edge Global shutter camera

./pco_camera/pco_me4/pco_ser_con
Source files and makefile for creating console testprogram
Program for testing the serial communication
Program for switching operation mode for pco.edge (Rolling Shutter, Global Shutter, Global Reset)

./pco_camera/pco_me4/pco_edge_grab
Source files and makefile for creating console testprogram
Programs for grabbing images from an pco.edge Rolling Shutter camera
Programs for grabbing images from an pco.edge Rolling Shutter camera with external trigger
pcox_   shows grabbed images in an X-Window

./pco_camera/pco_me4/pco_edge_grab_gl
Source files and makefile for creating console testprograms
Programs for grabbing images from an pco.edge Global Shutter camera
pcox_   shows grabbed images in an X-Window

./pco_camera/pco_me4/pco_camera_grab
Source files and makefile for creating console testprograms
Programs for grabbing images from an pco.camera camera
pcox_   shows grabbed images in an X-Window

./pco_camera/pco_me4/qt_pco_camera_me4
Specific source files and project file for creating Qt based application

./pco_camera/PCO_clhs/bin
executable binaries linked to static libraries

./pco_camera/PCO_clhs/bindyn
executable binaries linked to dynamic libraries


SYSTEM_VERSION:
This package was successfully tested in a Debian 7.8 system with kernel 3.19 
with the following driver and runtime versions from Silicon Software
 menable_linuxdrv_src_4.1.7 
 siso-rt5-5.4.1.4-linux-amd64
 menable_linuxdrv_src_4.1.8 
 siso-rt5-5.4.4.1-linux-amd64


VERSION HISTORY:
ver01_01_14
added more camera commands
added dynamic libs
some minor bugfixes
changes in some of the testprograms


ver01_01_13
internal only
improved documentation

ver01_01_12
added some camera commands
added switch_edge program to select shutter mode of pco.edge
added camera_record program to show how to readout images from camera internal memory (CamRam)

ver01_01_11
added some camera commands

ver01_01_10
improved documentation

ver01_01_10
new directory layout ( pco_common added)
PostArm() function added to all grabber classes
support for pco.edge42 added
support for DataFormat 5x12L added

ver01_01_06
added Qt project (requires Qt5)
streamlined grabber names

ver01_01_05
added some camera commands
new headerfiles in PCO_Include
new pco libraries

ver01_01_04
added some camera commands
new headerfiles in PCO_Include
new pco libraries


ver01_01_03
with makefiles
added support for pco.edge GlobalShutter
new headerfiles in PCO_Include
new pco libraries


ver01_01_02
new example and new functions in class CPco_cl_com

ver01_01_01
initial project


KNOWN BUGS:

-------------------------------------------------------------------
 PCO AG
 DONAUPARK 11
 93309 KELHEIM / GERMANY
 PHONE +49 (9441) 20050
 FAX   +49 (9441) 200520
 info@pco.de, support@pco.de
 http://www.pco.de
-------------------------------------------------------------------
 DISCLAIMER
 THE ORIGIN OF THIS INFORMATION MAY BE INTERNAL OR EXTERNAL TO PCO.
 PCO MAKES EVERY EFFORT WITHIN ITS MEANS TO VERIFY THIS INFORMATION.
 HOWEVER, THE INFORMATION PROVIDED IN THIS DOCUMENT IS FOR YOUR
 INFORMATION ONLY. PCO MAKES NO EXPLICIT OR IMPLIED CLAIMS TO THE
 VALIDITY OF THIS INFORMATION.
-------------------------------------------------------------------
 Any trademarks referenced in this document are the property of
 their respective owners.
-------------------------------------------------------------------
