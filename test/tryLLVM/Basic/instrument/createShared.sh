export PATH=/mnt/hdd2/anirudh/software/llvm-ff/build/bin:$PATH
clang -Wall -g -c -fPIC -I/usr/include/c++/4.8/ -I/usr/include/x86_64-linux-gnu/c++/4.8/ allocDalloc.c mop.c 
ar -cvq libinstrument.a mop.o allocDalloc.o
clang -shared -Wl,-soname,libinstrument.so.1 -o libinstrument.so.1.0   *.o -lstdc++
ln -sf libinstrument.so.1.0 libinstrument.so.1
ln -sf libinstrument.so.1.0 libinstrument.so