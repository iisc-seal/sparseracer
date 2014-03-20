#include <cstdio>

void mop(int address, char* type, int size){
  printf("%x of type %s and size %d bytes freed \n", address, type, size);
}
