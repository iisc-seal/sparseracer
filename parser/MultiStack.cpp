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
	bottom.threadID = -1;

	stack.push_back(bottom);
}

MultiStack::~MultiStack() {
}

void MultiStack::push(stackElementType element) {
	stack.push_back(element);
}

MultiStack::stackElementType MultiStack::peek(long long thread) {
	for (list<stackElementType>::iterator it = stack.end(); it != stack.begin(); it--) {
		if (it->threadID == thread)
			return *it;
	}
	return bottom;
}

MultiStack::stackElementType MultiStack::pop(long long thread) {
	for (list<stackElementType>::iterator it = stack.end(); it != stack.begin(); it--) {
		if (it->threadID == thread) {
			stackElementType temp = *it;
			stack.erase(it);
			return temp;
		}
	}
	return bottom;
}

MultiStack::stackElementType MultiStack::pop(long long thread, string task) {
	for (list<stackElementType>::iterator it = stack.end(); it != stack.begin(); it--) {
		if (it->threadID == thread && (it->taskID).compare(task) == 0) {
			stackElementType temp = *it;
			stack.erase(it);
			return temp;
		}
	}
	return bottom;
}

bool MultiStack::isBottom(MultiStack::stackElementType element) {
	if (element.opID == -1 && element.threadID == -1 &&
		element.opType.compare("") == 0 && element.taskID.compare("") == 0)
		return true;
	else
		return false;
}
