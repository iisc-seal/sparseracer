/*
 * MultiStack.cpp
 *
 *  Created on: 31-May-2014
 *      Author: shalini
 */

#include <parser/MultiStack.h>

MultiStack::MultiStack() {
	bottom.opID = -1;
	bottom.opType = "";
	bottom.taskID = "";
	bottom.blockID = -1;
	bottom.threadID = -1;

	stack.push_back(bottom);
}

MultiStack::~MultiStack() {
}

void MultiStack::push(stackElementType element) {
	stack.push_back(element);
}

MultiStack::stackElementType MultiStack::peek(IDType thread) {
	for (list<stackElementType>::iterator it = stack.end(); it != stack.begin(); it--) {
		if (it->threadID == thread)
			return *it;
	}
	return bottom;
}

MultiStack::stackElementType MultiStack::pop(IDType thread) {
	for (list<stackElementType>::iterator it = stack.end(); it != stack.begin(); it--) {
		if (it->threadID == thread) {
			stackElementType temp = *it;
			stack.erase(it);
			return temp;
		}
	}
	return bottom;
}

void MultiStack::stackClear(IDType thread) {
	while (!isEmpty(thread)) {
		pop(thread);
	}
}

MultiStack::stackElementType MultiStack::pop(IDType thread, string task) {
	for (list<stackElementType>::iterator it = stack.end(); it != stack.begin(); it--) {
		if (it->threadID == thread && (it->taskID).compare(task) == 0) {
			stackElementType temp = *it;
			stack.erase(it);
			return temp;
		}
	}
	return bottom;
}

void MultiStack::stackClear(IDType thread, string task) {
	if (!isEmpty(thread)) {
		while (true) {
			MultiStack::stackElementType top = pop(thread, task);
			if (isBottom(top))
				break;
		}
	}
}

bool MultiStack::isBottom(MultiStack::stackElementType element) {
	if (element.opID == -1 && element.threadID == -1 &&
		element.opType.compare("") == 0 && element.taskID.compare("") == 0)
		return true;
	else
		return false;
}

bool MultiStack::isEmpty(IDType thread) {
	// If the top of the stack is bottom, stack is empty
	if (isBottom(peek(thread)))
		return true;
	else
		return false;
}
