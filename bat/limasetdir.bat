@echo off
c:
set GITLIMADIR=c:\blissadm\git\Lima
set LIMAPCODIR=c:\blissadm\lima\pco\install_sinogram
IF NOT EXIST %LIMAPCODIR% mkdir %LIMAPCODIR%
IF not EXIST %GITLIMADIR%\install mkdir %GITLIMADIR%\install
IF not EXIST %GITLIMADIR%\install\lima mkdir %GITLIMADIR%\install\Lima
rem set GITLIMADIR="R:\dserver\classes\ccd\pco_rh\Lima\install\Lima"
@echo on
