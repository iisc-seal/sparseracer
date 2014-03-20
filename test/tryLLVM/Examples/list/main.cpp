//--- list1/main.cpp - Linked List example -- Fred Swartz 2001-12-13

#include <iostream>  
using namespace std;

#include "list_node.h"

//=============================================== main
int main() {

  ListNode* list1;

  list1 = readList(cin); // Read list elements
  cout << endl;
  printList(list1);      // Print list elements
  clear(list1);          // Delete list elements


  return 0;
}//end main
