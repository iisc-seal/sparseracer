export PATH=/home/anirudh/software/llvm/build/bin:$PATH
export INSTRUMENTDIRS="/home/anirudh/software/llvm/"
export LD_LIBRARY_PATH="/home/anirudh/software/llvm/test/tryLLVM/Basic/instrument"
clang++ -g -O0 -Xclang -load -Xclang /home/anirudh/software/llvm/build/lib/LLVMMemInstrument.so -g allocTest.cpp -c -o allocTest.o -rdynamic

clang++ -g allocTest.o -o allocTest -L/home/anirudh -L/home/anirudh/software/llvm/test/tryLLVM/Basic/instrument -linstrument -lunwind -rdynamic
