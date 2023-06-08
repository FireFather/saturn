## New:
- NNUE (accepts any halfkp_256x2-32-32) eval
- Clang/reharper code optimizations

Support added for an external NNUE (halfkp_256x2-32-32) evaluation (nn.bin) via Daniel Shawul's nnue-probe library: https://github.com/dshawul/nnue-probe.

Visual Studio 2022 used...the project files have been included.
The executable has been produced using MSYS2 mingw-w64-x86_64-toolchain.

Any halfkp_256x2-32-32 NNUE can be used...see:

https://github.com/FireFather/halfkp_256x2-32-32-nets or

https://tests.stockfishchess.org/nns for a different net.

Compatible nets start on page 72-73 (approx.) with dates of 21-05-02 22:26:43 or earlier.

The nnue file size must = 20,530 KB (halfkp_256x2-32-32).

The progress information is located in progress/progress.md

## TODO
- Better evaluation, texel tuning
- Lazy SMP
- Better UCI support
- Better CLI
- Better history heuristic
- Make singular extension work
- Setup autotesting (e.g. using OpenBench)

## Building using cmake
Requires C++17 and popcnt intrisincs.
Go into source folder.
```
mkdir build
cd build
cmake ../
```
Then either:
```
cmake --build .
```
or go to the build folder and open the generated visual studio solution.

## Running
It is recommended to use a gui that supports the uci protocol (e.g. Arena, Cute Chess).
Alternatively, you can run it in console and type uci commands yourself.
