#include <cstdlib>

struct node{
  int data;
  node* next;
};

int main(){
  node *head = new node;
  head->data = 5;
  head->next = NULL;
}
