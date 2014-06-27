/*
 * MultiStack.h
 *
 *  Created on: 31-May-2014
 *      Author: shalini
 */

#include <list>
#include <string>

#include <config.h>

using namespace std;

#ifndef MULTISTACK_H_
#define MULTISTACK_H_

class MultiStack {
public:
	MultiStack();
	virtual ~MultiStack();

	struct stackElementType {
		string opType;
		IDType opID;
		IDType blockID;
		IDType threadID;
		string taskID;
	};

	// normal push operation
	void push(stackElementType element);

	// peek the top most element of thread t
	stackElementType peek(IDType t);

	// pop top most element of thread t
	stackElementType pop(IDType t);

	// pop top most element of thread t and task tt
	stackElementType pop(IDType t, string tt);

	bool isBottom(stackElementType element);

	// Check if stack is empty for thread t
	bool isEmpty(IDType t);

private:
	// Stack - each element is a pair of threadID and taskID.
	list<stackElementType> stack;

	// element to represent bottom
	stackElementType bottom;
};

#endif /* MULTISTACK_H_ */
