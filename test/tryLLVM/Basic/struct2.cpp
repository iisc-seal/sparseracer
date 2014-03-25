#include<cstdio>
struct product{
  int serialNum;
  int *price;
};

int main(){
  struct product p;
  p.serialNum = 10;
  p.price = new int;
  scanf("%d", p.price);
  //printf("%d", *(p.price) * 100);
  (*p.price)++;
}
