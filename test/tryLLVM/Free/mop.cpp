#include <cstdio>

void mopDealloc(int address, int typeSize, char* type, char* debugLoc){
  printf("%p of type %s (typesize %d) bytes freed at %s\n", address, type, typeSize, debugLoc);
}

void mopAlloc(int address, int memsize, char* type, char* debugLoc){
  printf("%p of type %s allocated %d bytes at %s\n", address, type, memsize, debugLoc);
}
