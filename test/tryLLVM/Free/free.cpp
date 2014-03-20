#include<cstdlib>
#include<cstdio>

struct node{
  int data;
  node* next;
};

int main(){
  struct node *head = (node *)malloc(sizeof(node));
  printf("head @ %x \n", head);
  free(head);
}
