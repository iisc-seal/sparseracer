#include <cstdio>
#include <syslog.h>
#include <cstdlib>

int main(){
  int *base = (int*)malloc(sizeof(int));
  int *rebase;
  printf("%p \n", base);
  *base = 12;
  rebase = (int*)realloc(base, 10*sizeof(int));
  rebase[0] = 4;
  int * pData;
  pData = (int*) calloc (10,sizeof(int));
  free(pData);
  printf("%p %p\n", rebase, pData);
}
