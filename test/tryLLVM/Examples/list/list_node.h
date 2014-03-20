//--- list1/list_node.h -- Fred Swartz 2001-12-13
// There should really be a list class with member functions.
// But that is for a later example.

#ifndef LIST_NODE_H
#define LIST_NODE_H

#include <iostream>
using namespace std;

struct ListNode {
  ListNode* next;
  int       data;
};
     

//=============================================== prototypes
void      clear(ListNode* somelist);
void      printList(ListNode* start);
ListNode* readList(istream& input); 
ListNode* readListLIFO(istream& input);

#endif
