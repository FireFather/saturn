arch_cpu=x86-64-avx2
make --no-print-directory -j build ARCH=${arch_cpu} COMP=mingw
strip saturn.exe
mv saturn.exe saturn_x64_avx2.exe 
make clean
