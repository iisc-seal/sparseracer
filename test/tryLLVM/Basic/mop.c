#include<stdio.h>

void mopInstrument(int address, int typeSize, char* type, char* debugLoc){
  printf("%p of type %s (typesize %d) at %s\n", address, type, typeSize, debugLoc);
}
