# One-dimensional quantum mechanics with Wigner functions
### Employing Schrodinger picture Liouville-von Neumann equation of motion
### To visualize purely quantum mechanical effects such as tunnelling, as well as macroscopic effects such as decoherence (aka measurement)
### Powered by C++ and Lua (and JavaScript / Python)

Setup

    git clone https://github.com/jbuchermn/onedse
    cd onedse
    git submodule init
    git submodule update --recursive
    cd cpp/dependencies && ./get.sh && cd ../..

Compile and run tests

    cd cpp && mkdir build && cd build && cmake .. && make && cd ../..
    ./cpp/build/test/onedse_test

Run a Lua script and plot results
    cd lua
    ../cpp/build/main/script/onedse ./decoherence.lua
    python3 ../python/plot.py decoherence/<result folder>
    cd ..

or run the webserver

    cd js && npm install && make && cd ..
    ./cpp/build/main/server/onedse_server

Open `localhost:8080` and try the examples in the `lua` folder.

## Troubleshooting / ToDos

 - [ ] `stdc++fs` on macOS / when using LLVM: Comment out the line in `CMakeLists.txt` --> experimental no longer necessary?
 - [ ] Update JS packages --> fix security issues
 - [ ] Improve build process
