arch_cpu=x86-64-popc
make --no-print-directory -j build ARCH=${arch_cpu} COMP=mingw
strip saturn.exe
mv saturn.exe saturn-NN_x64_popc.exe
make clean 

arch_cpu=x86-64-avx2
make --no-print-directory -j build ARCH=${arch_cpu} COMP=mingw
strip saturn.exe
mv saturn.exe saturn-NN_x64_avx2.exe 
make clean 

arch_cpu=x86-64-bmi2
make --no-print-directory -j build ARCH=${arch_cpu} COMP=mingw
strip saturn.exe
mv saturn.exe saturn-NN_x64_bmi2.exe 
make clean 
