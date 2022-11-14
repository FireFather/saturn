## New:
- NNUE (accepts any halfkp_256x2-32-32) eval
- Clang/reharper code optimizations

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
