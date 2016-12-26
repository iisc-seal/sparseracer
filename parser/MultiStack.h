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
