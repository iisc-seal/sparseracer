//--- list1/list_node.cpp  -- Fred Swartz 2001-12-13
#include <iostream>
using namespace std;

#include "list_node.h"

//=============================================== printList
void printList(ListNode* start) {
  for (; start!=NULL; start=start->next) {
    cout << start->data << " ";
  }
  cout << endl;
}//end printList


//============================================ readListLIFO
ListNode* readListLIFO(istream& input) {  
  ListNode* front = NULL;  // first element
  int x;

  while (input >> x) {
    ListNode* newnode = new ListNode();
    newnode->data = x;
    newnode->next = front;  // Points to rest of list

    front = newnode;        // Make this new head
  }
  return front;
}//end readListLIFO


//=============================================== readList
ListNode* readList(istream& input) {
  ListNode* front = NULL;  // first element
  ListNode* back  = NULL;  // last element
  int x;

  while (input >> x) {
    ListNode* newnode = new ListNode();
    newnode->data = x;
    newnode->next = NULL;   // this is the end

    if (front == NULL) {
      front = newnode;    // if this is the first node
      back  = newnode;    // it's both the front and back.
    }else{
      back->next = newnode; // prev end points to newnode
    }
    back = newnode;         // new end of list
  }
  return front;
}//end readList


//=============================================== clear
void clear(ListNode* somelist) {
  ListNode* temp;  // stores next pointer before delete
  for (ListNode* p = somelist; p != NULL; p = temp) {
    temp = p->next;  // must save before delete
    delete p;
  }
}//end clear
