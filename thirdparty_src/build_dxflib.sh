#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

tar xzf TARBALLS/dxflib-2.7.5.tar.gz && cd dxflib-2.7.5 && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 mkdir build
 cd build
 cmake ..  -DCMAKE_INSTALL_PREFIX=$INSTALLDIR 
 make -j12
 make install
)
