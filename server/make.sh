#!/bin/bash
mkdir -p build
cd build && CXX=g++-7 cmake .. && make -j3

if [ "$1" == "run" ]; then
    ./wigner_webd
fi

cd ..
