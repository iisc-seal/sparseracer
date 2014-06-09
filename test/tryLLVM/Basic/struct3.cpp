#include<cstdio>
#include <syslog.h>

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
  struct product *p = new product;
  printf("%x\n", p);
  p->serialNum = 10;
  p->nested.f1 = 10;
  p->nested.f2 = new int;
  printf("%x\n", p->nested.f2);
  *(p->nested.f2) = 3;
  delete p->nested.f2;
  closelog();
  //return 0;
}
