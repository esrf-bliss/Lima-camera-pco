echo "... [camera/bld.bat] enter"

REM python get_git_log.py 

echo "... [camera/bld.bat] cmake compile"
cmake -Bbuild -H. -G "Ninja" -DLIMA_ENABLE_PYTHON=1 -DWITH_GIT_VERSION=1 -DLIMA_ENABLE_DEBUG=1 -DCAMERA_ENABLE_TESTS=1 -DCMAKE_INSTALL_PREFIX=%LIBRARY_PREFIX% -DPYTHON_SITE_PACKAGES_DIR=%SP_DIR% -DCMAKE_FIND_ROOT_PATH=%LIBRARY_PREFIX%
IF %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%


echo "... [camera/bld.bat] cmake build"
cmake --build build --config Release --target install
IF %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

echo "... [camera/bld.bat] exit"
