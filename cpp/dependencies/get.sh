#!/bin/bash

#Get alglib
wget http://www.alglib.net/translator/re/alglib-3.12.0.cpp.gpl.tgz
tar -xvf alglib-3.12.0.cpp.gpl.tgz
mv cpp/src alglib/alglib
rm -rf cpp
rm -f alglib-3.12.0.cpp.gpl.tgz

#Make alglib
cd alglib && make && cd ..

#Get Eigen
wget http://bitbucket.org/eigen/eigen/get/3.3.4.tar.gz
tar -xvf 3.3.4.tar.gz
mv eigen-eigen-5a0156e40feb eigen
rm -f 3.3.4.tar.gz
