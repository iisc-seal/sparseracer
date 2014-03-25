#include <cstdio>

void mopDealloc(int address, int typeSize, char* type, char* debugLoc){
  printf("%p of type %s (typesize %d) bytes freed at %s\n", address, type, typeSize, debugLoc);
}

void mopAlloc(int address, int memsize, char* type, char* debugLoc){
  printf("allocated %d bytes of type %s at address %p (%s)\n", memsize, type, address, debugLoc);
}
