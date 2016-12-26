/*
Copyright 2014-2016 Anirudh Santhiar, Shalini Kaleeswaran and Aditya
Kanade from the Software Engineering and Analysis Lab, Department of
Computer Science and Automation, Indian Institute of Science.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
/*
 * MultiStack.cpp
 *
 *  Created on: 31-May-2014
 *      Author: shalini
 */

#include <parser/MultiStack.h>

using namespace std;

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
