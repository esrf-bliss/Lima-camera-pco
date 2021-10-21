#!/bin/bash
#python get_git_log.py 

cmake -Bbuild -H. -G "Ninja" -DLIMA_ENABLE_PYTHON=1 -DWITH_GIT_VERSION=1 -DCAMERA_ENABLE_TESTS=1 -DCMAKE_INSTALL_PREFIX=$PREFIX -DPYTHON_SITE_PACKAGES_DIR=$SP_DIR -DCMAKE_FIND_ROOT_PATH=$PREFIX
cmake --build build --target install
