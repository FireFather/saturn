arch_cpu=x86-64-bmi2
make --no-print-directory -j build ARCH=${arch_cpu} COMP=mingw
strip saturn.exe
mv saturn.exe saturn_x64_bmi2.exe 
make clean
