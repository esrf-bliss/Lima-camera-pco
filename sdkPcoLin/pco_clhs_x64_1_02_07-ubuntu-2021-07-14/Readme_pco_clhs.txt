-------------------------------------------------------------------
                 P C O  AG
     TECHNICAL  INFORMATION  DOCUMENT
-------------------------------------------------------------------

README FOR SOFTWARE:  
pco_camera/pco_clhs

FOR PCO PRODUCT:
pco.edge_hs connected to CLHS grabber

DESCRIPTION:
This packages includes source files for a communication and grabber class for
pco.edge_clhs camera connected to a SiliconSoftware ME5 CLHS grabber.
The libpcoclhs must be linked to all programs, which use the pco classes. 
The sources for some simple example programs, which show the use of the class functions, are included.
All libraries and most of the example programs are available as static and dynamic version.


Attention:
To use the classes either the application 'pco_clhs_mgr' or the daemon 'pco_clhs_svc' must be started.
Both are supervising the clhs ports available on the installed grabber. If cameras are connected, the according links are established and
the camera is configured automaticly.
To close pco_clhs_mgr or pco_clhs_svc use the pkill command. For pco_clhs_mgr 'x' or ESC can also be used.
Ensure that all other applications, which use a clhs camera, are closed, before closing the manager application.
To show information about installed grabber and connected cameras pco_clhs_info application can be used.
The pco_clhs_info application could also be used to enable and disable distinct ports on the grabber.

VERSION 1
SUBVERSION 2
SUBAGE 7
LNX 64

CONTENTS:

Archive File Name: PCO_clhs_x64_1_02_xx.tar.gz

Installation Instructions:

Install driver and runtime from Silicon Software and check functionality of me5 board.
This PCO package does need the following packages from Silicon Software
runtime version:  siso-rt5-5.7.0.76321-linux-amd64
driver version:   menable_linuxdrv_src_4.2.6
applet version:   setup-mE5-MA-AF2-Applets-5.7.0-linux-amd64

The following applet must be flashed to the board
applet used:      libAcq_DualCLHSx1AreaRAW

Copy the pco_clhs_x64_1_02_xx.tar.gz to a distinct directory (e.g. PCO). 
Use "tar -xpvzf pco_clhs_1_02_xx.tar.gz" to get the sources.
After this you will find the following new directories and files as noted below.

Use "make" to compile all testprograms
Script symlink_pco creates the necessary links to the pco libraries in the bindyn directory.

If the programs can't find the siso libraries, you might have to add the following lines 
to /etc/environment (global) or /etc/bash.bashrc (shell only):

export SISODIR5="<path_to_siso_dir>"

export GENICAM_ROOT_V2_4=$SISODIR5/genicam
export GENICAM_CACHE_V2_4=$SISODIR5/genicam/cache
export GENICAM_LOG_CONFIG_V2_4=$SISODIR5/genicam/log/config/SisoLogging.properties

export PATH=$SISODIR5/bin/:$PATH
export LD_LIBRARY_PATH=$SISODIR5/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$GENICAM_ROOT_V2_4/bin/Linux64_x64:$LD_LIBRARY_PATH

and replace the <path_to_siso_dir> with the appropriate path, usually ending with SiliconSoftware.

The environment setting must also be included in the run environment of QT-Creator, if a QT-Application is started out of the QT-Creator. 

To use the dynamic libraries script 'symlink_pco' can be used to create symbolic links to the 
pco libraries in pco_camera/pco_common/pco_libdyn. 
'symlink_pco -b' will create the links in directory ./bindyn
'symlink_pco -u' will create the links in directory /usr/local/lib (write permissions needed)
'symlink_pco "path"' will create the links in user selected directory "path"
To update the linker cache '/sbin/ldconfig' should be used afterwards 



Filelist:
./pco_camera
parent directory

./pco_camera/pco_common
Files which are used also from other Interface Implementations

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

./pco_camera/pco_clhs/Makefile
Makefile to compile pco_clhs libraries and console testprograms.
Does also call 'symlink_pco -b'

./pco_camera/pco_clhs/pco_classes
Source code and header file for Class Cpco_com_clhs
Source code and header file for Class Cpco_grab_clhs

./pco_camera/pco_clhs/pco_ser_con
Source files and makefile for creating console testprograms
Program for testing the serial communication.
Program to switch pco.edge shutter mode

./pco_camera/pco_me4/pco_camera_grab
Source files and makefile for creating console testprograms
Programs for grabbing images from an pco.camera
pcox_   shows grabbed images in a X-Window

./pco_camera/pco_clhs/qt_pco_camera_clhs
Specific source files and project file for creating Qt based application

./pco_camera/PCO_clhs/bin
executable binaries linked to static libraries

./pco_camera/PCO_clhs/bindyn
executable binaries linked to dynamic libraries

SYSTEM_VERSION:
This package was successfully tested in a Debian 10 system with kernel 4.19
with the following driver and runtime versions from Silicon Software
 menable_linuxdrv_src_4.2.6 
 siso-rt5-5.7.0-linux-amd64
 

VERSION HISTORY:

ver01_02_07
Bugfix:
 pcoclhs library system freeze with specific image sequence 


ver01_02_06
Bugfix:
 pcoclhs library images are lost, when acquire is done with high system load


ver01_02_05
Bugfix:
 pco_clhs_mgr semaphore handling
 pcoclhs library acquire_status timeout

ver01_02_04
changes in symlink_pco and main Makefile
changes in some of the testprograms

ver01_02_03
bugfixes in libraries
pco_clhs_mgr can also handle multiport cameras

ver01_02_02
Makefiles changed to support dynamic libraries

ver01_02_01
adapted to runtime 5.4.4 and applet libAcq_DualCLHSx1AreaRAW

ver01_01_02
first beta release

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
