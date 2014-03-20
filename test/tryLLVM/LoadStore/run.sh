export PATH=/home/anirudh/software/llvm/build/bin:$PATH
clang++ -emit-llvm -S test.cpp -o test.bc
opt -load /home/anirudh/software/llvm/build/lib/LLVMLoadStoreInstrument.so -offline < test.bc > test.instrument.bc
clang++ -emit-llvm -S mop.cpp -o mop.bc
llvm-link mop.bc test.instrument.bc -S -o test.linked.bc
lli test.linked.bc