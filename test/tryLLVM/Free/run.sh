LLVM_HOME=/home/anirudh/software/llvm
export PATH=$LLVM_HOME/build/bin:$PATH
LIB=$LLVM_HOME/build/lib

#clang -emit-llvm -S alloc.cpp -o alloc.ll
#opt -load $LIB/LLVMMemoryLogger.so -memlog < alloc.ll > /dev/null

clang -g -O0 -emit-llvm -S free.cpp -o free.ll
opt -S -load $LIB/LLVMAllocFreeInstrument.so -logAllocDealloc < free.ll > free.instrument.ll
clang++ -emit-llvm -S mop.cpp -o mop.ll
llvm-link mop.ll free.instrument.ll -S -o test.linked.ll
lli test.linked.ll
