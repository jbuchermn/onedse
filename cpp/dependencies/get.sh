#!/bin/bash

#Get alglib
wget http://www.alglib.net/translator/re/alglib-3.12.0.cpp.gpl.tgz
tar -xvf alglib-3.12.0.cpp.gpl.tgz
mv cpp/src alglib/alglib
rm -rf cpp
rm -f alglib-3.12.0.cpp.gpl.tgz
# Make alglib
cd alglib 
make
cd ..

#Get Eigen
wget https://gitlab.com/libeigen/eigen/-/archive/3.3.8/eigen-3.3.8.tar.gz
tar -xvf eigen-3.3.8.tar.gz
mv eigen-3.3.8 eigen
rm -f eigen-3.3.8.tar.gz
