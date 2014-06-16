#include<cstdio>
#include <syslog.h>
#include <cstdlib>
extern void mopInstrument(int address, int typeSize, char* type, char* debugLoc, char *fName);

struct product{
  int serialNum;
  int price;
  struct node{
    int f1;
    int *f2;
  }nested;
};

int main(){
  openlog ("struct3", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
  int *base = (int*)malloc(sizeof(int));
  *base = 12;
  struct product *p = new product;
  printf("%x\n", p);
  p->serialNum = 10;
  p->nested.f1 = 10;
  p->nested.f2 = new int;
  base = (int*)realloc(base, 10*sizeof(int));
  base[0] = 4;
  printf("%x\n", p->nested.f2);
  *(p->nested.f2) = 3;
  delete p->nested.f2;
  free(p);
  printf("%d", sizeof(struct product));
  closelog();
  //return 0;
}
