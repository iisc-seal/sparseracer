LLVM_HOME=/home/anirudh/software/llvm
export PATH=$LLVM_HOME/build/bin:$PATH
export LD_LIBRARY_PATH=/home/anirudh/software/qtApps/instrument
#export PATH=SPATH:/home/anirudh/software/qtApps/instrument
LIB=$LLVM_HOME/build/lib

#clang -emit-llvm -S alloc.cpp -o alloc.ll
#opt -load $LIB/LLVMMemoryLogger.so -memlog < alloc.ll > /dev/null

clang -g -O0 -emit-llvm -S nestedObj.cpp -o nestedObj.ll
opt -S -load $LIB/LLVMMemInstrument.so -logAllocDealloc < nestedObj.ll > nestedObj.instrument.ll
clang++ -emit-llvm -S mop.cpp -o mop.ll
llvm-link mop.ll nestedObj.instrument.ll -S -o nestedObj.linked.ll
lli nestedObj.linked.ll
