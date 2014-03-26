export PATH=/home/anirudh/software/llvm/build/bin:$PATH
clang mop.c -c -o mop.o
clang allocDalloc.c -c -o allocDalloc.o
clang++ -O0 -Xclang -load -Xclang /home/anirudh/software/llvm/build/lib/LLVMLoadStoreInstrument.so -Xclang -load -Xclang /home/anirudh/software/llvm/build/lib/LLVMAllocFreeInstrument.so -Xclang -load -Xclang /home/anirudh/software/llvm/build/lib/LLVMFInstrument.so -g struct3.cpp -c -o struct3.o
clang++ mop.o allocDalloc.o struct3.o -o struct3

#-emit-llvm -S struct3.cpp -o struct3.bc
# opt -load /home/anirudh/software/llvm/build/lib/LLVMLoadStoreInstrument.so -offline -load /home/anirudh/software/llvm/build/lib/LLVMAllocFreeInstrument.so -logAllocDealloc < struct3.bc > struct3.instrument.bc
#clang++ -emit-llvm -S mop.cpp -o mop.bc
#clang++ -emit-llvm -S allocDalloc.cpp -o allocDalloc.bc
#llvm-link mop.bc allocDalloc.bc struct3.instrument.bc -S -o struct3.linked.bc
#lli struct3.linked.bc