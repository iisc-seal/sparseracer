#include<cstdlib>
#include<cstdio>

struct node{
  int data;
  node* next;
};

int main(){
  struct node *head = (node *)malloc(sizeof(node));
  printf("head @ %p \n", head);
  free(head);

  int *a = new int;
  printf("a @ %p \n", a);
  delete a;

  int *array = new int[10];
  printf("array @ %p \n", array);
  delete[] array;
}
