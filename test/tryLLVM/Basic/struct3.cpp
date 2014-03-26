#include<cstdio>
struct product{
  int serialNum;
  int price;
  struct node{
    int f1;
    int *f2;
  }nested;
};

int main(){
  struct product *p = new product;
  p->serialNum = 10;
  p->nested.f1 = 10;
  p->nested.f2 = new int;
  printf("%p\n", p->nested.f2);
  *(p->nested.f2) = 3;
  //return 0;
}
