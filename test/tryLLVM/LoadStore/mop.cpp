#include <cstdio>

void mop(int address, int size, int written){
  char *rw;
  if(written)
    rw = "written";
  else
    rw = "read";
  printf("%x of size %d %s \n", address, size, rw);
}
