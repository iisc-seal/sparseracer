#include<cstdio>
#include<syslog.h>

struct product{
  int serialNum;
  int *price;
};

int bar(){
  return 42;
}

int foo(int k){
  if(k > 100)
    return 0;

  if(k > 20 && k < 40)
    return bar();

  return 56;
}

int main(){
  struct product p;
  openlog ("exampleprog", LOG_CONS | LOG_PID, LOG_LOCAL1);
  p.serialNum = 10;
  p.price = new int;
  scanf("%d", p.price);
  printf("%d", *(p.price) * 100);
  (*p.price)++;
  foo(0);
  closelog();
}
