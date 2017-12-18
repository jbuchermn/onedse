#!/bin/bash
mkdir -p build
cd build && cmake .. && make -j3

if [ "$1" == "run" ]; then
    ./wigner_webd
fi
