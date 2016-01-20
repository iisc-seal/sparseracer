/*
 * MultiStack.h
 *
 *  Created on: 31-May-2014
 *      Author: shalini
 */

#include <list>
#include <string>

#include <config.h>

#ifndef MULTISTACK_H_
#define MULTISTACK_H_

class MultiStack {
public:
	MultiStack();
	virtual ~MultiStack();

	struct stackElementType {
		std::string opType;
		IDType opID;
		IDType blockID;
		IDType nodeID;
		IDType threadID;
		std::string taskID;
	};

	// normal push operation
	void push(stackElementType element);

	// peek the top most element of thread t
	stackElementType peek(IDType t);

	// pop top most element of thread t
	stackElementType pop(IDType t);

	// remove those entries in the stack that has thread t
	void stackClear(IDType t);

	// pop top most element of thread t and task tt
	stackElementType pop(IDType t, std::string tt);

	// remove those entries in the stack that has thread t and task tt
	void stackClear(IDType t, std::string tt);

	bool isBottom(stackElementType element);

	// Check if stack is empty for thread t
	bool isEmpty(IDType t);

private:
	// Stack - each element is a pair of threadID and taskID.
	std::list<stackElementType> stack;

	// element to represent bottom
	stackElementType bottom;
};

#endif /* MULTISTACK_H_ */
