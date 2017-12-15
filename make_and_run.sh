#!/bin/bash
cd webapp && make && cd ..
cd server && mkdir -p build && cd build && CXX=clang++ cmake .. && make
./wigner_webd
