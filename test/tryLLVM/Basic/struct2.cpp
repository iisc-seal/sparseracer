#include<cstdio>
#include<syslog.h>

struct product{
  int serialNum;
  int *price;
};

int main(){
  struct product p;
  openlog ("exampleprog", LOG_CONS | LOG_PID, LOG_LOCAL1);
  p.serialNum = 10;
  p.price = new int;
  scanf("%d", p.price);
  printf("%d", *(p.price) * 100);
  (*p.price)++;
  closelog();
}
