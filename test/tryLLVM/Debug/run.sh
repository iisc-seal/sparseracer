export PATH=/home/anirudh/software/llvm/build/bin:$PATH
clang -g -O0 -emit-llvm -S b2.cpp -o b2.ll
opt -load /home/anirudh/software/llvm/build/lib/LLVMDebugInfoPrint.so -printDebug -instnamer < b2.ll > /dev/null
