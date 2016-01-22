/*
 * TraceParser.cpp
 *
 *  Created on: 29-May-2014
 *      Author: shalini
 */

#include "TraceParser.h"
#include <iostream>
#include <cstring>
#include <cassert>
#include <boost/regex.hpp>
#include "MultiStack.h"

#include <debugconfig.h>

using namespace std;

/*
 * Constructor for TraceParser class.
 * Takes as argument name of the tracefile
 * Creates regex for a valid operation.
 */
//TraceParser::TraceParser(string traceFileName, Logger *logger) {
TraceParser::TraceParser(string traceFileName) {
	traceFile.open(traceFileName.c_str(), ios_base::in);
	cout << traceFileName << endl;
	if (!traceFile.is_open()) {
		cout << "Cannot open trace file\n";
	}

	traceName = traceFileName;

	opCount = 0;
	nodeCount = 0;
	// The prefix regular expression
	//prefixRegEx = " *";
	prefixRegEx = "\\s*";

	posIntRegEx = "[0-9]+";
	intRegEx = "[-]?[0-9]+";
	hexRegEx = "0[xX][0-9a-fA-F]+";

	// The operation regular expression.
	opRegEx = " *(threadinit) *\\( *(" + posIntRegEx + ") *\\) *" + "|" +
			  " *(threadexit) *\\( *(" + posIntRegEx + ") *\\) *" + "|" +
			  " *(fork) *\\( *("       + posIntRegEx + ") *, *(" 	   + posIntRegEx + ") *\\) *" + "|" +
			  " *(join) *\\( *("       + posIntRegEx + ") *, *(" 	   + posIntRegEx + ") *\\) *" + "|" +
			  " *(enterloop) *\\( *("  + posIntRegEx + ") *\\) *" + "|" +
			  " *(exitloop) *\\( *("   + posIntRegEx + ") *\\) *" + "|" +
			  " *(enq) *\\( *("        + posIntRegEx + ") *, *(" 	   + posIntRegEx + ") *, *("
			  	  	  	  	  	  	   + posIntRegEx + ") *, *("       + intRegEx + ") *\\) *" + "|" +
			  " *(deq) *\\( *(" 	   + posIntRegEx + ") *, *(" 	   + posIntRegEx + ") *\\) *" + "|" +
			  " *(end) *\\( *(" 	   + posIntRegEx + ") *, *(" 	   + posIntRegEx + ") *\\) *" + "|" +
#ifdef PERMIT
			  " *(permit) *\\( *(" 	   + posIntRegEx + ") *, *(" 	   + posIntRegEx + ") *, *("
			  	  	  	  	  	  	                               + hexRegEx + ") *\\) *" + "|" +
			  " *(revoke) *\\( *(" 	   + posIntRegEx + ") *, *(" 	   + posIntRegEx + ") *, *("
			  	  	  	  	  	  	                               + hexRegEx + ") *\\) *" + "|" +
#else
			  " *(pause) *\\( *(" 	   + posIntRegEx + ") *, *(" 	   + posIntRegEx + ") *, *("
			  	  	  	  	  	  	     			  	  	  	   + hexRegEx + ") *\\) *" + "|" +
			  " *(resume) *\\( *(" 	   + posIntRegEx + ") *, *(" 	   + posIntRegEx + ") *, *("
			  	  	  	  	  	  	     			  	  	  	   + hexRegEx + ") *\\) *" + "|" +
#endif
			  " *(reset) *\\( *("	   + posIntRegEx + ") *, *("      + hexRegEx + ") *\\) *" + "|" +
			  " *(alloc) *\\( *(" 	   + posIntRegEx + ") *, *(" 	   + hexRegEx + ") *, *("
			  	  	  	  	  	  	  	  	  	    		  	   + posIntRegEx + ") *\\) *" + "|" +
			  " *(free) *\\( *(" 	   + posIntRegEx + ") *, *(" 	   + hexRegEx + ") *, *("
			  	  	  	  	  	  	  	  	  	   	   	   	  	   + posIntRegEx + ") *\\) *" + "|" +
			  " *(inc) *\\( *(" 	   + posIntRegEx + ") *, *\\( *(" + hexRegEx + ") *, *("
			  	  	  	  	  	  	  	  	  	  	   	   	  	   + posIntRegEx + ") *\\) *\\) *" + "|" +
			  " *(dec) *\\( *(" 	   + posIntRegEx + ") *, *\\( *(" + hexRegEx + ") *, *("
			  	  	  	  	  	  	  	  	  	  	   	   	   	   + posIntRegEx + ") *\\) *\\) *" + "|" +
#ifdef LOCKS
			  " *(wait) *\\( *(" 	   + posIntRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *" + "|" +
			  " *(notify) *\\( *(" 	   + posIntRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *" + "|" +
			  " *(notifyall) *\\( *("  + posIntRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *" + "|" +
#endif
			  " *(read) *\\( *(" 	   + posIntRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *" + "|" +
			  " *(write) *\\( *(" 	   + posIntRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *";

	suffixRegEx = "\\s*";
	// Regular expression for the entire line
	finalRegEx = "^" + prefixRegEx + "(" + opRegEx + ")" + suffixRegEx + "$";
}

TraceParser::~TraceParser() {
}

//int TraceParser::parse(UAFDetector &detector, Logger *logger) {
int TraceParser::parse(UAFDetector &detector) {
	string line;
	boost::regex reg;
	boost::cmatch matches;

	try {
		// create boost regex from finalRegEx, ignoring case
		reg.assign(finalRegEx, boost::regex_constants::icase);
	}
	catch (boost::regex_error& e) {
		cout << finalRegEx << " is not a valid regular expression: \""
			 << e.what() << "\"" << endl;
		return -1;
	}

	long long typePos, threadPos;

	MultiStack stackForThreadOrder;
	MultiStack stackForTaskOrder;
	MultiStack stackForNestingOrder;
	MultiStack stackForGlobalLoop; // To record whether we saw enterloop for each thread.

	while (getline(traceFile, line)) {
		// Check whether the line is a valid line according to finalRegEx
		if (!boost::regex_match(line.c_str(), matches, reg)) {
			cout << "ERROR: Line in trace file is not valid\n";
			cout << line << endl;
			return -1;
		}
		else {
			opCount++;
			cout << opCount << " " << line << endl;

			// Temp variables to store details of current operation/block/task/thread.
			UAFDetector::opDetails opdetails;

			IDType threadID;	// stores the threadID of current operation
			MultiStack::stackElementType stackElement; // stores the details of current operation

			// matches[] contains the substrings that matched subexpressions of the regex.
			// matches[1] is the complete operation (without prefix or suffix), for e.g., threadinit(0)
			string OpString(matches[1].first, matches[1].second);

			for (unsigned i=2; i < matches.size(); i++) {
				string match(matches[i].first, matches[i].second);
				// find the next non-empty matches[i] after matches[1] - this would be the type of operation.
				if (!match.empty() && match.compare(" ") != 0) {
					typePos = i;

					for (unsigned j=typePos+1; j < matches.size(); j++) {
						string m1(matches[j].first, matches[j].second);
						// find the next non-empty matches[i] after type of operation.
						if (!m1.empty() && m1.compare(" ") != 0) {
							threadID = atoi(m1.c_str());
							threadPos = j;
							break;
						}
					}

					// Obtain opType and threadID of the current operation
					opdetails.opType = match;
					opdetails.threadID = threadID;

					// Populate details for stack entry
					stackElement.opID = opCount;
					stackElement.opType = match;
					stackElement.threadID = threadID;

					if (match.compare("threadinit") == 0) {
						nodeCount++;
						opdetails.nodeID = nodeCount;
						stackElement.nodeID = nodeCount;

						if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
							UAFDetector::setOfOps set;
							set.opSet.insert(opCount);
							detector.nodeIDMap[nodeCount] = set;
						} else {
							cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
							cout << "ERROR: Existing entry:\n";
							detector.nodeIDMap[nodeCount].printDetails();
							return -1;
						}
#ifdef SANITYCHECK
						// Sanity check: stack should be empty for threadID
						assert(stackForThreadOrder.isEmpty(threadID));
						assert(stackForTaskOrder.isEmpty(threadID));
#endif

						// Obtain details of current thread

						if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
							// If there is no entry for this thread in the threadIDMap, then we did not see
							// a fork op for this thread.

							UAFDetector::threadDetails threaddetails;
							threaddetails.firstOpID = opCount;
							threaddetails.threadinitOpID = opCount;
							detector.threadIDMap[threadID] = threaddetails;
						} else {
							UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
							// Sanity check: if there already exists an entry for thread, then this must
							// have been set when we saw a fork op for this thread. That is, the threadinit
							// op id has not been set, same with firstOpID
							assert(existingEntry.threadinitOpID == -1);
							assert(existingEntry.firstOpID == -1);
#endif
							existingEntry.threadinitOpID = opCount;
							existingEntry.firstOpID = opCount;
							detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
							detector.threadIDMap[threadID] = existingEntry;
						}

						stackForThreadOrder.push(stackElement);
					} else if (match.compare("threadexit") == 0) {
						nodeCount++;
						opdetails.nodeID = nodeCount;
						stackElement.nodeID = nodeCount;

						if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
							UAFDetector::setOfOps set;
							set.opSet.insert(opCount);
							detector.nodeIDMap[nodeCount] = set;
						} else {
							cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
							cout << "ERROR: Existing entry:\n";
							detector.nodeIDMap[nodeCount].printDetails();
							return -1;
						}

						// Get the stack top to obtain the previous op in thread.
						if (stackForThreadOrder.isEmpty(threadID)) {
							cout << "WARNING: No previous op found for threadexit " << opCount
								 << " on stackForThreadOrder\n";
							cout << "WARNING: This is fine if this is the first op in the thread\n";

#ifdef SANITYCHECK
							// Sanity check: task order stack needs to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
#endif

							// Update firstop, firstblock, lastblock and threadexitopID for the current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.threadexitOpID = opCount;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								// If there is an entry for this thread in threadIDMap, we had seen the fork op
								// of this thread and the forkOpID is set.
								// But in this case, threadexit cannot be the first op in the thread. We must have
								// seen a threadinit.
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.threadexitOpID == -1);
								assert(existingEntry.firstOpID == -1);

								if (existingEntry.forkOpID == -1) {
									cout << "ERROR: We did not see a fork op for thread " << threadID
										 << " but there already exists an entry for this thread in threadIDMap\n";
									cout << "ERROR: Existing entry details:\n";
									existingEntry.printThreadDetails();
									cout << "\n";
									return -1;
								}
#endif
								existingEntry.threadexitOpID = opCount;
								existingEntry.firstOpID = opCount;
								existingEntry.lastOpInThreadID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadOrder.peek(threadID);

							// Set current op as next-op for the stack top
							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for " << previousOpInThread.opID << " in opIDMap\n";
								cout << "ERROR: While examining op " << opCount << "\n";
								cout << "ERROR: Trying to set op " << opCount << " as next-op of op "
									 << previousOpInThread.opID << "\n";
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID in stack and opIDMap
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								existingEntry.nextOpInThread = opCount;
								opdetails.prevOpInThread = previousOpInThread.opID;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							// Update threadexit-opID and lastblock-ID for the current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								cout << "ERROR: Cannot find entry for thread " << threadID << " in threadIDMap\n";
								cout << "ERROR: This is an error since threadexit is not the first op "
								     << "in this thread according to the ThreadOrder stack, this means "
								     << "we should find an entry for this thread in threadIDMap\n";
								return -1;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
								existingEntry.threadexitOpID = opCount;
								existingEntry.lastOpInThreadID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							// Clear all the ops for this thread from the stack.
							stackForThreadOrder.stackClear(threadID);
						}
					} else if (match.compare("enterloop") == 0) {
						nodeCount++;
						opdetails.nodeID = nodeCount;
						stackElement.nodeID = nodeCount;

						if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
							UAFDetector::setOfOps set;
							set.opSet.insert(opCount);
							detector.nodeIDMap[nodeCount] = set;
						} else {
							cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
							cout << "ERROR: Existing entry:\n";
							detector.nodeIDMap[nodeCount].printDetails();
							return -1;
						}

						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for enterloop " << opCount
								 << " on stackForThreadOrder\n";
							cout << "WARNING: This is fine if this is the first op in the thread\n";

#ifdef SANITYCHECK
							// Sanity check: task order stack needs to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread

							// Update firstop and enterloopID for current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								threaddetails.enterloopID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								// If there is an entry for this thread in threadIDMap, we had seen the fork op
								// of this thread and the forkOpID is set.
								// But in this case, enterloop cannot be the first op in the thread. We must have
								// seen a threadinit.
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstOpID == -1);

								if (existingEntry.forkOpID == -1) {
									cout << "ERROR: We did not see a fork op for thread " << threadID
										 << " but there already exists an entry for this thread in threadIDMap\n";
									cout << "ERROR: Existing entry details:\n";
									existingEntry.printThreadDetails();
									cout << "\n";
									return -1;
								}
#endif

								existingEntry.firstOpID = opCount;
								existingEntry.enterloopID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}


							stackForThreadOrder.push(stackElement);
							stackForGlobalLoop.push(stackElement);

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadOrder.peek(threadID);

							// Set current op as next-op for the stack top
							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for " << previousOpInThread.opID << " in opIDMap\n";
								cout << "ERROR: While examining op " << opCount << "\n";
								cout << "ERROR: Trying to set op " << opCount << " as next-op of op "
									 << previousOpInThread.opID << "\n";
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev-op has same threadID as current op in stack and opIDMap
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								existingEntry.nextOpInThread = opCount;
								opdetails.prevOpInThread = previousOpInThread.opID;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							// Set enterloopID for the current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								cout << "ERROR: Cannot find entry for thread " << threadID << " in threadIDMap\n";
								return -1;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
								existingEntry.enterloopID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							stackForThreadOrder.push(stackElement);
							stackForGlobalLoop.push(stackElement);
						}
					} else if (match.compare("exitloop") == 0) {
						nodeCount++;
						opdetails.nodeID = nodeCount;
						stackElement.nodeID = nodeCount;

						if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
							UAFDetector::setOfOps set;
							set.opSet.insert(opCount);
							detector.nodeIDMap[nodeCount] = set;
						} else {
							cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
							cout << "ERROR: Existing entry:\n";
							detector.nodeIDMap[nodeCount].printDetails();
							return -1;
						}

						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for exitloop " << opCount
								 << " on stack\n";
							cout << "WARNING: This is fine if this is the first op in the thread\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							// Update firstop for current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								threaddetails.exitloopID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstOpID == -1);

								if (existingEntry.forkOpID == -1) {
									cout << "ERROR: We did not see a fork op for thread " << threadID
										 << " but there already exists an entry for this thread in threadIDMap\n";
									cout << "ERROR: Existing entry details:\n";
									existingEntry.printThreadDetails();
									cout << "\n";
									return -1;
								}
#endif
								existingEntry.exitloopID = opCount;
								existingEntry.firstOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}


							stackForThreadOrder.push(stackElement);

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadOrder.peek(threadID);

							// Set current op as next-op for stack top
							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev-op has same thread as current op in stack and opIDMap
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								existingEntry.nextOpInThread = opCount;
								opdetails.prevOpInThread = previousOpInThread.opID;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							// Set exitloopID for the current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								cout << "ERROR: Cannot find entry for thread " << threadID << " in threadIDMap\n";
								return -1;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
								existingEntry.exitloopID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							stackForThreadOrder.push(stackElement);
							stackForGlobalLoop.stackClear(threadID);
							stackForGlobalLoop.push(stackElement); // pushing exitloop
						}
					} else if (match.compare("enq") == 0) {
						nodeCount++;
						opdetails.nodeID = nodeCount;
						stackElement.nodeID = nodeCount;

						if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
							UAFDetector::setOfOps set;
							set.opSet.insert(opCount);
							detector.nodeIDMap[nodeCount] = set;
						} else {
							cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
							cout << "ERROR: Existing entry:\n";
							detector.nodeIDMap[nodeCount].printDetails();
							return -1;
						}

						// Obtain 2nd, 3rd and 4th argument (i.e task enqueued, target threadID & priority) of enq op
						string taskEnqueued;
						IDType targetThread;
						IDType priority;
						unsigned j;
						for (j=threadPos+1; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							// find the next non-empty matches[i] after threadID of operation.
							if (!m1.empty() && m1.compare(" ") != 0) {
								taskEnqueued = m1;
								break;
							}
						}
						for (j=j+1; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							if (!m1.empty() && m1.compare(" ") != 0) {
								targetThread = atoi(m1.c_str());
								break;
							}
						}
						for (j=j+1; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							if (!m1.empty() && m1.compare(" ") != 0) {
								priority = atoi(m1.c_str());
								break;
							}
						}

						if (detector.taskIDMap.find(taskEnqueued) == detector.taskIDMap.end()) {
							UAFDetector::taskDetails taskdetails;
							taskdetails.enqOpID = opCount;
							taskdetails.priority = priority;
							detector.taskIDMap[taskEnqueued] = taskdetails;
						} else {
							cout << "ERROR: Found duplicate entry for task " << taskEnqueued << " in taskIDMap\n";
							cout << "While examining op " << opCount;
							cout << "\nDuplicate entry:\n";
							detector.taskIDMap[taskEnqueued].printTaskDetails();
							cout << endl;
							return -1;
						}

						UAFDetector::enqOpDetails enqdetails;
						enqdetails.targetThread = targetThread;
						enqdetails.taskEnqueued = taskEnqueued;
						enqdetails.priority = priority;

						// Map enq op to its arguments;
						if (detector.enqToTaskEnqueued.find(opCount) == detector.enqToTaskEnqueued.end()) {
							detector.enqToTaskEnqueued[opCount] = enqdetails;
						} else {
							cout << "ERROR: Found duplicate entry for enq op " << opCount << " in enqToTaskEnqueued\n";
							cout << "Duplicate entry:\n";
							detector.enqToTaskEnqueued[opCount].printEnqDetails();
							cout << endl;
							return -1;
						}

						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for enq " << opCount
								 << " on stack\n";
							cout << "WARNING: This is fine if this is the first op in the thread\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							// Update firstop for current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							stackForThreadOrder.push(stackElement);

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadOrder.peek(threadID);
							// In the case of an enq operation, the previous op in the thread need not be in the same task as enq.
							// The use case for this is when the task is a nested task and the enq op happens within the nesting
							// loop, but outside any of the child tasks.
							MultiStack::stackElementType previousOpInTask = stackForTaskOrder.peek(threadID);

							// If the previous op in task stack is the same as the previous op in thread stack, then we are not
							// in the middle of a nesting loop.
							bool firstOpInsideNestingLoop;
							bool firstOpInsideGlobalLoop = false;
							if (previousOpInThread.opID == -1) {
								cout << "ERROR: ThreadOrder stack is not empty, but a peek operation returns invalid element\n";
								return -1;
							}
							if (!stackForTaskOrder.isEmpty(threadID)) {
								if (previousOpInTask.opID != previousOpInThread.opID)
									firstOpInsideNestingLoop = true;
								else {
									// If the previous op in task is a pause, then we are in a new block
#ifdef PERMIT
									if (detector.opIDMap[previousOpInTask.opID].opType.compare("permit") == 0)
#else
									if (detector.opIDMap[previousOpInTask.opID].opType.compare("pause") == 0)
#endif
										firstOpInsideNestingLoop = true;
									else
										firstOpInsideNestingLoop = false;
								}
							} else {
								// Task order stack is empty, this means we are not inside a nesting loop
								firstOpInsideNestingLoop = false;

								if (stackForNestingOrder.isEmpty(threadID) && !stackForGlobalLoop.isEmpty(threadID)) {
									// We are inside the global loop.
									MultiStack::stackElementType topOfGlobalLoopStack = stackForGlobalLoop.peek(threadID);
									if (topOfGlobalLoopStack.opType.compare("enterloop") == 0
											|| topOfGlobalLoopStack.opType.compare("exitloop") == 0) {
										if (previousOpInThread.opType.compare("exitloop") == 0)
											firstOpInsideGlobalLoop = false;
										else
											firstOpInsideGlobalLoop = true;
									}
								} else if (!stackForNestingOrder.isEmpty(threadID)) {
									cout << "ERROR: TaskOrder stack is empty but NestingOrder stack is not: for thread "
										 << threadID << "\n";
									cout << "ERROR: Top element in NestingOrder stack: "
										 << stackForNestingOrder.peek(threadID).opID
										 << "\n";
									return -1;
								}
							}

							// previous-op-in-thread need not be the previous-op-in-task or the previous-op-in-block
							if (!firstOpInsideNestingLoop) {
								if (firstOpInsideGlobalLoop) {
								} else {
									opdetails.taskID = previousOpInThread.taskID;
								}
							} else {
								// If this is the first op inside the nesting loop, then we are in a new block
								opdetails.taskID = previousOpInTask.taskID;
							}

							// Set current op as next-op for stack top
							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev-op has same thread as current op in stack and opIDMap
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								existingEntry.nextOpInThread = opCount;
								opdetails.prevOpInThread = previousOpInThread.opID;

								// previous-op-in-thread need not be the previous-op-in-task or the previous-op-in-block
								if (!firstOpInsideNestingLoop && !firstOpInsideGlobalLoop) {
									if (previousOpInThread.taskID.compare("") != 0)
										existingEntry.nextOpInTask = opCount;
								}

								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							if (firstOpInsideNestingLoop) {
								if (detector.opIDMap.find(previousOpInTask.opID) == detector.opIDMap.end()) {
									cout << "ERROR: Cannot find entry for op " << previousOpInTask.opID << " in opIDMap\n";
									return -1;
								} else {
									UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInTask.opID];
#ifdef SANITYCHECK
									// Sanity check: prev-op has same thread as current op in stack and opIDMap
									assert(previousOpInTask.threadID == threadID);
									assert(existingEntry.threadID == threadID);
#endif
									opdetails.taskID = previousOpInTask.taskID;
									opdetails.prevOpInThread = previousOpInThread.opID;
									existingEntry.nextOpInTask = opCount;
									detector.opIDMap.erase(detector.opIDMap.find(previousOpInTask.opID));
									detector.opIDMap[previousOpInTask.opID] = existingEntry;
								}
							}

							// Add enq to the enqSet for current block
							if (!firstOpInsideNestingLoop && !firstOpInsideGlobalLoop) {

							} else if (!firstOpInsideNestingLoop && firstOpInsideGlobalLoop) {

							} else {

							}

							if (!firstOpInsideNestingLoop && !firstOpInsideGlobalLoop) {
								stackElement.taskID = previousOpInThread.taskID;
							} else if (!firstOpInsideNestingLoop && firstOpInsideGlobalLoop) {
							} else {
								stackElement.taskID = previousOpInTask.taskID;
							}
							stackForThreadOrder.push(stackElement);
							if (!stackForTaskOrder.isEmpty(threadID))
								stackForTaskOrder.push(stackElement);
							else {
								if (stackForNestingOrder.isEmpty(threadID) && !stackForGlobalLoop.isEmpty(threadID)) {
									stackForGlobalLoop.push(stackElement);
								}
							}
						}
					} else if (match.compare("deq") == 0) {
						nodeCount++;
						opdetails.nodeID = nodeCount;
						stackElement.nodeID = nodeCount;

						if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
							UAFDetector::setOfOps set;
							set.opSet.insert(opCount);
							detector.nodeIDMap[nodeCount] = set;
						} else {
							cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
							cout << "ERROR: Existing entry:\n";
							detector.nodeIDMap[nodeCount].printDetails();
							return -1;
						}

						// Obtain 2nd argument (i.e., task dequeued) of deq
						string taskDequeued;
						for (unsigned j=threadPos+1; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							// find the next non-empty matches[i] after threadID of operation.
							if (!m1.empty() && m1.compare(" ") != 0) {
								taskDequeued = m1;
								break;
							}
						}
						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for deq " << opCount
								 << " on stack\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							opdetails.taskID = taskDequeued;

							if (detector.opIDMap.find(opCount) == detector.opIDMap.end()) {
								detector.opIDMap[opCount] = opdetails;
							} else {
								cout << "ERROR: Found duplicate entry for op " << opCount << " in opIDMap\n";
								cout << "ERROR: Duplicate entry:\n";
								detector.opIDMap[opCount].printOpDetails();
								return -1;
							}

							// If there exists an entry for this tak in taskIDMap, 
							// then we saw an enq of this task
							if (detector.taskIDMap.find(taskDequeued) == detector.taskIDMap.end()) {
								UAFDetector::taskDetails taskdetails;
								taskdetails.deqOpID = opCount;
								taskdetails.firstOpInTaskID = opCount;
								detector.taskIDMap[taskDequeued] = taskdetails;
							} else {
								// We saw an enq of this task before
								UAFDetector::taskDetails existingEntry = detector.taskIDMap[taskDequeued];
#ifdef SANITYCHECK
								assert(existingEntry.enqOpID != -1);
#endif
								existingEntry.deqOpID = opCount;
								existingEntry.firstOpInTaskID = opCount;
								detector.taskIDMap.erase(detector.taskIDMap.find(taskDequeued));
								detector.taskIDMap[taskDequeued] = existingEntry;
							}

							// Update firstop, firstblock and exitloopblock for current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							stackElement.taskID = taskDequeued;
							stackForThreadOrder.push(stackElement);
							stackForTaskOrder.push(stackElement);
						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadOrder.peek(threadID);

							// deq is the beginning of a block
							opdetails.taskID = taskDequeued;

							// Set current op as next-op for stack top
							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev-op has same thread as current op in stack and opIDMap
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
								assert(previousOpInThread.taskID.compare(taskDequeued) != 0);
								assert(existingEntry.taskID.compare(taskDequeued) != 0);
#endif

								existingEntry.nextOpInThread = opCount;
								opdetails.prevOpInThread = previousOpInThread.opID;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							// Map deqopID, parentTask, etc of current task
							if (detector.taskIDMap.find(taskDequeued) == detector.taskIDMap.end()) {
								// There is no entry for this task in taskIDMap, this means we did not see an enq of this op.
								// This happens when the enq is from a thread we ignored (or something like that!)

								UAFDetector::taskDetails taskdetails;
								taskdetails.deqOpID = opCount;
								taskdetails.firstOpInTaskID = opCount;

								// Find if there was a pause of some task when this task is dequeued.
								if (!stackForNestingOrder.isEmpty(threadID)) {
									MultiStack::stackElementType topOfNestingStack = stackForNestingOrder.peek(threadID);

#ifdef PERMIT
									if (topOfNestingStack.opType.compare("permit") == 0) {
#else
									if (topOfNestingStack.opType.compare("pause") == 0) {
#endif
										if (detector.taskIDMap.find(topOfNestingStack.taskID) == detector.taskIDMap.end()) {
											cout << "ERROR: Cannot find entry for task " << topOfNestingStack.taskID << " in taskIDMap\n";
											cout << "ERROR: While examining op " << opCount << "\n";
											return -1;
										}
										taskdetails.parentTask = topOfNestingStack.taskID;
									}
								}

								detector.taskIDMap[taskDequeued] = taskdetails;
							} else {
								UAFDetector::taskDetails existingEntry = detector.taskIDMap[taskDequeued];
								existingEntry.deqOpID = opCount;
								existingEntry.firstOpInTaskID = opCount;

								// Find if there was a pause of some task when this task is dequeued.
								if (!stackForNestingOrder.isEmpty(threadID)) {
									MultiStack::stackElementType topOfNestingStack = stackForNestingOrder.peek(threadID);
#ifdef PERMIT
									if (topOfNestingStack.opType.compare("permit") == 0) {
#else
									if (topOfNestingStack.opType.compare("pause") == 0) {
#endif
									    if (detector.taskIDMap.find(topOfNestingStack.taskID) == detector.taskIDMap.end()) {
										cout << "ERROR: Cannot find entry for task " << topOfNestingStack.taskID << " in taskIDMap\n";
										cout << "While examining op " << opCount << "\n";
										return -1;
									    }
									    existingEntry.parentTask = topOfNestingStack.taskID;
									}
								}

								detector.taskIDMap.erase(detector.taskIDMap.find(taskDequeued));
								detector.taskIDMap[taskDequeued] = existingEntry;
							}

							stackElement.taskID = taskDequeued;
							stackForThreadOrder.push(stackElement);
							stackForTaskOrder.push(stackElement);
							stackForNestingOrder.push(stackElement);

							if (!stackForGlobalLoop.isEmpty(threadID)) {
								while (stackForGlobalLoop.peek(threadID).opType.compare("enterloop") != 0 &&
										stackForGlobalLoop.peek(threadID).opType.compare("exitloop") != 0)
									stackForGlobalLoop.pop(threadID);
							}
						}
#ifdef PERMIT
					} else if (match.compare("permit") == 0) {
#else
					} else if (match.compare("pause") == 0) {
#endif
						nodeCount++;
						opdetails.nodeID = nodeCount;
						stackElement.nodeID = nodeCount;

						if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
							UAFDetector::setOfOps set;
							set.opSet.insert(opCount);
							detector.nodeIDMap[nodeCount] = set;
						} else {
							cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
							cout << "ERROR: Existing entry:\n";
							detector.nodeIDMap[nodeCount].printDetails();
							return -1;
						}

						// Obtain 2nd & 3rd argument (i.e., current task & shared variable) of pause
						string task, sharedVariable;
						unsigned j;
						for (j=threadPos+1; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							// find the next non-empty matches[i] after threadID of operation.
							if (!m1.empty() && m1.compare(" ") != 0) {
								task = m1;
								break;
							}
						}
						for (j=j+1; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							if (!m1.empty() && m1.compare(" ") != 0) {
								sharedVariable = m1;
								break;
							}
						}

						if (detector.pauseResumeResetOps.find(opCount) == detector.pauseResumeResetOps.end()) {
							detector.pauseResumeResetOps[opCount] = sharedVariable;
						} else {
							cout << "ERROR: Found duplicate entry for op " << opCount << " in pauseResumeResetOps\n";
							cout << "Duplicate entry: " << detector.pauseResumeResetOps[opCount];
							cout << endl;
							return -1;
						}

						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadOrder.isEmpty(threadID)) {
							cout << "WARNING: No previous op found for pause " << opCount
								 << " on stack\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							opdetails.taskID = task;

							// Update firstop for current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstOpID == -1);
								if (existingEntry.forkOpID == -1) {
								    cout << "ERROR: We did not see a fork op, but there already exists an entry for thread "
									 << threadID << "\n";
								    cout << "ERROR: Existing entry:\n";
								    existingEntry.printThreadDetails();
								}
#endif
								existingEntry.firstOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							if (detector.taskIDMap.find(task) == detector.taskIDMap.end()) {
								UAFDetector::taskDetails taskdetails;
								taskdetails.firstPauseOpID = opCount;
								taskdetails.atomic = false;
								taskdetails.firstOpInTaskID = opCount;
								UAFDetector::pauseResumeResetTuple pauseResumeResetDetails;
								pauseResumeResetDetails.pauseOp = opCount;
								taskdetails.pauseResumeResetSequence.push_back(pauseResumeResetDetails);
								detector.taskIDMap[task] = taskdetails;
							} else {
								UAFDetector::taskDetails existingEntry = detector.taskIDMap[task];
								existingEntry.atomic = false;
								existingEntry.firstPauseOpID = opCount;
								existingEntry.firstOpInTaskID = opCount;
								UAFDetector::pauseResumeResetTuple pauseResumeResetDetails;
								pauseResumeResetDetails.pauseOp	= opCount;
								existingEntry.pauseResumeResetSequence.push_back(pauseResumeResetDetails);
								detector.taskIDMap.erase(detector.taskIDMap.find(task));
								detector.taskIDMap[task] = existingEntry;
							}

							if (detector.nestingLoopMap.find(sharedVariable) == detector.nestingLoopMap.end()) {
								UAFDetector::nestingLoopDetails loopdetails;
								UAFDetector::pauseResumeResetTuple prr;
								prr.pauseOp = opCount;
								loopdetails.pauseResumeResetSet.push_back(prr);
								detector.nestingLoopMap[sharedVariable] = loopdetails;
							} else {
#if 0
								cout << "ERROR: Found duplicate entry for shared variable " << sharedVariable << " in nestingLoopMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.nestingLoopMap[sharedVariable].printNestingLoopDetails();
								cout << endl;
								return -1;
#endif
								UAFDetector::nestingLoopDetails existingEntry = detector.nestingLoopMap[sharedVariable];
								for (vector<UAFDetector::pauseResumeResetTuple>::iterator it =
								    existingEntry.pauseResumeResetSet.begin();
								    it != existingEntry.pauseResumeResetSet.end(); it++) {
									if (it->pauseOp == opCount) {
										cout << "ERROR: Found duplicate enry for pause op " << opCount
											 << " in the set of shared variable " << sharedVariable << " in nestingLoopMap\n";
										cout << "ERROR: While examining op " << opCount << "\n";
										cout << "ERROR: Duplicate entry:\n";
										detector.nestingLoopMap[sharedVariable].printNestingLoopDetails();
										cout << "\n";
										return -1;
									}
								}
								UAFDetector::pauseResumeResetTuple prr;
								prr.pauseOp = opCount;
								existingEntry.pauseResumeResetSet.push_back(prr);
								detector.nestingLoopMap.erase(detector.nestingLoopMap.find(sharedVariable));
								detector.nestingLoopMap[sharedVariable] = existingEntry;
							}
							stackElement.taskID = task;
							stackForThreadOrder.push(stackElement);
							stackForTaskOrder.push(stackElement);

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadOrder.peek(threadID);
							// In the case of pause, the previous op in thread is in the same task as pause.
							// So no need to look separately in the task stack.

							opdetails.taskID = task;
							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								cout << "While examining op " << opCount;
								cout << endl;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
								assert(previousOpInThread.taskID.compare(task) == 0);
								assert(existingEntry.taskID.compare(task) == 0);
#endif

								existingEntry.nextOpInTask = opCount;
								existingEntry.nextOpInThread = opCount;
								opdetails.prevOpInThread = previousOpInThread.opID;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							if (detector.taskIDMap.find(task) == detector.taskIDMap.end()) {
								cout << "ERROR: Cannot find entry for task " << task << " in taskIDMap\n";
								cout << "While examining op " << opCount;
								cout << endl;
								return -1;
							} else {
								UAFDetector::taskDetails existingEntry = detector.taskIDMap[task];
								if (!stackForNestingOrder.isEmpty(threadID)) {
									MultiStack::stackElementType topOfNestingStack = stackForNestingOrder.peek(threadID);
									if (topOfNestingStack.opType.compare("deq") == 0) {
										existingEntry.firstPauseOpID = opCount;
										existingEntry.atomic = false;
									}
								} 
#if 0
								else {
									cout << "ERROR: stackForNesting Order is empty\n";
									cout << "Expected to find the deq/previous-resume of task " << task << endl;
									cout << "While examining pause op " << opCount << endl;
									return -1;
								}
#endif

								UAFDetector::pauseResumeResetTuple prr;
								prr.pauseOp = opCount;
								existingEntry.pauseResumeResetSequence.push_back(prr);

								detector.taskIDMap.erase(detector.taskIDMap.find(task));
								detector.taskIDMap[task] = existingEntry;
							}

							if (detector.nestingLoopMap.find(sharedVariable) == detector.nestingLoopMap.end()) {
								UAFDetector::nestingLoopDetails loopdetails;
								UAFDetector::pauseResumeResetTuple prr;
								prr.pauseOp = opCount;
								loopdetails.pauseResumeResetSet.push_back(prr);
								detector.nestingLoopMap[sharedVariable] = loopdetails;
							} else {
								UAFDetector::nestingLoopDetails existingEntry = detector.nestingLoopMap[sharedVariable];
								for (vector<UAFDetector::pauseResumeResetTuple>::iterator it =
								    existingEntry.pauseResumeResetSet.begin();
								    it != existingEntry.pauseResumeResetSet.end(); it++) {
								    if (it->pauseOp == opCount) {
									cout << "ERROR: Found existing entry for pause op " << opCount 
									     << " in nestingLoopMap\n";
									return -1;
								    }
								}
								UAFDetector::pauseResumeResetTuple prr;
								prr.pauseOp = opCount;
								existingEntry.pauseResumeResetSet.push_back(prr);
								detector.nestingLoopMap.erase(detector.nestingLoopMap.find(sharedVariable));
								detector.nestingLoopMap[sharedVariable] = existingEntry;
							}

							stackElement.taskID = task;
							stackForThreadOrder.push(stackElement);
							stackForTaskOrder.push(stackElement);
							stackForNestingOrder.push(stackElement);

							if (!stackForGlobalLoop.isEmpty(threadID)) {
//								while (stackForGlobalLoop.peek(threadID).opType.compare("enterloop") != 0)
								while (stackForGlobalLoop.peek(threadID).opType.compare("enterloop") != 0 &&
										stackForGlobalLoop.peek(threadID).opType.compare("exitloop") != 0)
									stackForGlobalLoop.pop(threadID);
							}
						}
					} else if (match.compare("reset") == 0) {
						nodeCount++;
						opdetails.nodeID = nodeCount;
						stackElement.nodeID = nodeCount;

						if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
							UAFDetector::setOfOps set;
							set.opSet.insert(opCount);
							detector.nodeIDMap[nodeCount] = set;
						} else {
							cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
							cout << "ERROR: Existing entry:\n";
							detector.nodeIDMap[nodeCount].printDetails();
							return -1;
						}

						// Obtain 2nd & 3rd argument (i.e., current task & shared variable) of reset
						string sharedVariable;
						unsigned j;
						for (j=threadPos+1; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							// find the next non-empty matches[i] after threadID of operation.
							if (!m1.empty() && m1.compare(" ") != 0) {
								sharedVariable = m1;
								break;
							}
						}

						if (detector.pauseResumeResetOps.find(opCount) == detector.pauseResumeResetOps.end()) {
							detector.pauseResumeResetOps[opCount] = sharedVariable;
						} else {
							cout << "ERROR: Found duplicate entry for op " << opCount << " in pauseResumeResetOps\n";
							cout << "Duplicate entry: " << detector.pauseResumeResetOps[opCount];
							cout << endl;
							return -1;
						}

						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for reset " << opCount
								 << " on stack\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread

							// Update firstop, firstblock and exitloopblock for current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							if (detector.nestingLoopMap.find(sharedVariable) == detector.nestingLoopMap.end()) {
								UAFDetector::nestingLoopDetails loopdetails;
								UAFDetector::pauseResumeResetTuple prr;
								prr.resetOp = opCount;
								loopdetails.pauseResumeResetSet.push_back(prr);
								detector.nestingLoopMap[sharedVariable] = loopdetails;
							} else {
								UAFDetector::nestingLoopDetails existingEntry = detector.nestingLoopMap[sharedVariable];
								for (vector<UAFDetector::pauseResumeResetTuple>::iterator it =	
								    existingEntry.pauseResumeResetSet.begin(); it !=
								    existingEntry.pauseResumeResetSet.end();
it++) {
								    if (it->resetOp == opCount) {
									cout << "ERROR: Found duplicate entry for reset op " << opCount
									     << " in nestingLoopMap\n";
									return -1;
								    }
								}
								UAFDetector::pauseResumeResetTuple prr;
								prr.resetOp = opCount;
								existingEntry.pauseResumeResetSet.push_back(prr);
								detector.nestingLoopMap.erase(detector.nestingLoopMap.find(sharedVariable));
								detector.nestingLoopMap[sharedVariable] = existingEntry;
							}
							stackForThreadOrder.push(stackElement);
//							stackForTaskOrder.push(stackElement);
							stackForGlobalLoop.push(stackElement);

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadOrder.peek(threadID);
							// In the case of reset, the previous op in thread is not the previous op in the task.
							// So, get top of task stack to obtain prev op in task.
							MultiStack::stackElementType previousOpInTask = stackForTaskOrder.peek(threadID);
							// If the previous op in task stack is the same as the previous op in thread stack, then we are not
							// in the middle of a nesting loop.
							bool firstOpInsideNestingLoop;
							bool firstOpInsideGlobalLoop = false;
							if (previousOpInThread.opID == -1) {
								cout << "ERROR: ThreadOrder stack is not empty, but a peek operation returns invalid element\n";
								return -1;
							}
							if (!stackForTaskOrder.isEmpty(threadID)) {
								if (previousOpInTask.opID != previousOpInThread.opID)
									firstOpInsideNestingLoop = true;
								else {
									// If the previous op in task is a pause, then we are in a new block
#ifdef PERMIT
									if (detector.opIDMap[previousOpInTask.opID].opType.compare("permit") == 0)
#else
									if (detector.opIDMap[previousOpInTask.opID].opType.compare("pause") == 0)
#endif
										firstOpInsideNestingLoop = true;
									else
										firstOpInsideNestingLoop = false;
								}
							} else {
								// Task order stack is empty, this means we are not inside a nesting loop
								firstOpInsideNestingLoop = false;

								if (stackForNestingOrder.isEmpty(threadID) && !stackForGlobalLoop.isEmpty(threadID)) {
									// We are inside the global loop.
									MultiStack::stackElementType topOfGlobalLoopStack = stackForGlobalLoop.peek(threadID);
//									if (topOfGlobalLoopStack.opType.compare("enterloop") == 0)
									if (topOfGlobalLoopStack.opType.compare("enterloop") == 0
											|| topOfGlobalLoopStack.opType.compare("exitloop") == 0) {
										if (previousOpInThread.opType.compare("exitloop") == 0)
											firstOpInsideGlobalLoop = false;
										else
											firstOpInsideGlobalLoop = true;
									}
								} else if (!stackForNestingOrder.isEmpty(threadID)) {
									cout << "ERROR: TaskOrder stack is empty but NestingOrder stack is not: for thread " << threadID << "\n";
									cout << "ERROR: Top element in NestingOrder stack: " << stackForNestingOrder.peek(threadID).opID
										 << "\n";
									return -1;
								}
							}

							// previous-op-in-thread need not be the previous-op-in-task or the previous-op-in-block
							if (!firstOpInsideNestingLoop) {
								if (firstOpInsideGlobalLoop) {
								} else {
									opdetails.taskID = previousOpInThread.taskID;
								}
							} else {
								// If this is the first op inside the nesting loop, then we are in a new block
								opdetails.taskID = previousOpInTask.taskID;
							}

							// Set current op as next-op for stack top
							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev-op has same thread as current op in stack and opIDMap
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								existingEntry.nextOpInThread = opCount;
								opdetails.prevOpInThread = previousOpInThread.opID;

								// previous-op-in-thread need not be the previous-op-in-task or the previous-op-in-block
								if (!firstOpInsideNestingLoop) {
									if (previousOpInThread.taskID.compare("") != 0)
										existingEntry.nextOpInTask = opCount;
								}

								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							if (firstOpInsideNestingLoop) {
								if (detector.opIDMap.find(previousOpInTask.opID) == detector.opIDMap.end()) {
									cout << "ERROR: Cannot find entry for op " << previousOpInTask.opID << " in opIDMap\n";
									return -1;
								} else {
									UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInTask.opID];
#ifdef SANITYCHECK
									// Sanity check: prev-op has same thread as current op in stack and opIDMap
									assert(previousOpInTask.threadID == threadID);
									assert(existingEntry.threadID == threadID);
#endif
									opdetails.taskID = previousOpInTask.taskID;
									existingEntry.nextOpInTask = opCount;
									detector.opIDMap.erase(detector.opIDMap.find(previousOpInTask.opID));
									detector.opIDMap[previousOpInTask.opID] = existingEntry;
								}
							}

							if (firstOpInsideNestingLoop || firstOpInsideGlobalLoop) {

							}

							if (firstOpInsideNestingLoop) {

							} else if (firstOpInsideGlobalLoop) {

							}

							if (detector.nestingLoopMap.find(sharedVariable) == detector.nestingLoopMap.end()) {
								UAFDetector::nestingLoopDetails loopdetails;
								UAFDetector::pauseResumeResetTuple prr;
								prr.resetOp = opCount;
								loopdetails.pauseResumeResetSet.push_back(prr);
								detector.nestingLoopMap[sharedVariable] = loopdetails;
							} else {
								UAFDetector::nestingLoopDetails existingEntry = detector.nestingLoopMap[sharedVariable];
								vector<UAFDetector::pauseResumeResetTuple>::reverse_iterator it;
								for (it = existingEntry.pauseResumeResetSet.rbegin(); it !=
								    existingEntry.pauseResumeResetSet.rend();
it++) {
								    if (it->resetOp == opCount) {
									cout << "ERROR: Found duplicate entry for reset op " << opCount
									     << " in nestingLoopMap\n";
									return -1;
								    }
								    if (it->resetOp == -1 && it->pauseOp != -1 && it->resumeOp == -1)
								    	break;
								}
								if (it != existingEntry.pauseResumeResetSet.rend()) {
									it->resetOp = opCount;

									// Add this reset op to taskIDMap of the appropriate task.
									IDType pauseOp = it->pauseOp;
									std::string pauseTask = detector.opIDMap[pauseOp].taskID;
									if (pauseTask.compare("") != 0) {
										UAFDetector::taskDetails taskExistingEntry = detector.taskIDMap[pauseTask];
										vector<UAFDetector::pauseResumeResetTuple>::iterator it;
										for (it = taskExistingEntry.pauseResumeResetSequence.begin();
												it != taskExistingEntry.pauseResumeResetSequence.end();
												it++) {
											if (it->pauseOp == pauseOp && it->resetOp == -1)
												break;
										}
										if (it != taskExistingEntry.pauseResumeResetSequence.end()) {
											it->resetOp = opCount;
										} else {
											UAFDetector::pauseResumeResetTuple prr;
											prr.resetOp = opCount;
											existingEntry.pauseResumeResetSet.push_back(prr);
										}
										detector.taskIDMap.erase(detector.taskIDMap.find(pauseTask));
										detector.taskIDMap[pauseTask] = taskExistingEntry;
									}
								} else {
									UAFDetector::pauseResumeResetTuple prr;
									prr.resetOp = opCount;
									existingEntry.pauseResumeResetSet.push_back(prr);
								}
								detector.nestingLoopMap.erase(detector.nestingLoopMap.find(sharedVariable));
								detector.nestingLoopMap[sharedVariable] = existingEntry;
							}

							if (!firstOpInsideNestingLoop && !firstOpInsideGlobalLoop) {
								stackElement.taskID = previousOpInThread.taskID;
							} else if (!firstOpInsideNestingLoop && firstOpInsideGlobalLoop) {
							} else {
								stackElement.taskID = previousOpInTask.taskID;
							}
							stackForThreadOrder.push(stackElement);
							if (!stackForTaskOrder.isEmpty(threadID))
								stackForTaskOrder.push(stackElement);
							else {
								if (stackForNestingOrder.isEmpty(threadID) && !stackForGlobalLoop.isEmpty(threadID)) {
									stackForGlobalLoop.push(stackElement);
								}
							}
						}
#ifdef PERMIT
					} else if (match.compare("revoke") == 0) {
#else
					} else if (match.compare("resume") == 0) {
#endif
						nodeCount++;
						opdetails.nodeID = nodeCount;
						stackElement.nodeID = nodeCount;

						if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
							UAFDetector::setOfOps set;
							set.opSet.insert(opCount);
							detector.nodeIDMap[nodeCount] = set;
						} else {
							cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
							cout << "ERROR: Existing entry:\n";
							detector.nodeIDMap[nodeCount].printDetails();
							return -1;
						}

						// Obtain 2nd & 3rd argument (i.e., current task & shared variable) of resume
						string task, sharedVariable;
						unsigned j;
						for (j=threadPos+1; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							// find the next non-empty matches[i] after threadID of operation.
							if (!m1.empty() && m1.compare(" ") != 0) {
								task = m1;
								break;
							}
						}
						for (j=j+1; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							// find the next non-empty matches[i] after threadID of operation.
							if (!m1.empty() && m1.compare(" ") != 0) {
								sharedVariable = m1;
								break;
							}
						}

						if (detector.pauseResumeResetOps.find(opCount) == detector.pauseResumeResetOps.end()) {
							detector.pauseResumeResetOps[opCount] = sharedVariable;
						} else {
							cout << "ERROR: Found duplicate entry for op " << opCount << " in pauseResumeResetOps\n";
							cout << "Duplicate entry: " << detector.pauseResumeResetOps[opCount];
								cout << endl;
							return -1;
						}

						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for resume " << opCount
								 << " on stack\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							opdetails.taskID = task;

							if (detector.taskIDMap.find(task) == detector.taskIDMap.end()) {
								UAFDetector::taskDetails taskdetails;
								taskdetails.atomic = false;
								taskdetails.firstOpInTaskID = opCount;

								UAFDetector::pauseResumeResetTuple prr;
								prr.resumeOp = opCount;
								taskdetails.pauseResumeResetSequence.push_back(prr);

								detector.taskIDMap[task] = taskdetails;
							} else {
								// Resume is the first op in the thread and the task.
								// This means the entry in taskIDMap contains only the information about the 
								// enq op of this task
								UAFDetector::taskDetails existingEntry = detector.taskIDMap[task];
								existingEntry.atomic = false;
								existingEntry.firstOpInTaskID = opCount;
								UAFDetector::pauseResumeResetTuple prr;
								prr.resumeOp = opCount;
								existingEntry.pauseResumeResetSequence.push_back(prr);
								detector.taskIDMap.erase(detector.taskIDMap.find(task));
								detector.taskIDMap[task] = existingEntry;
							}

							if (detector.nestingLoopMap.find(sharedVariable) == detector.nestingLoopMap.end()) {
								UAFDetector::nestingLoopDetails loopdetails;
								UAFDetector::pauseResumeResetTuple prr;
								prr.resumeOp = opCount;
								loopdetails.pauseResumeResetSet.push_back(prr);
								detector.nestingLoopMap[sharedVariable] = loopdetails;
							} else {
								UAFDetector::nestingLoopDetails existingEntry = detector.nestingLoopMap[sharedVariable];
								for (vector<UAFDetector::pauseResumeResetTuple>::iterator it =	
								    existingEntry.pauseResumeResetSet.begin(); it !=
								    existingEntry.pauseResumeResetSet.end(); it++) {
								    if (it->resumeOp == opCount) {
								    	cout << "ERROR: Found duplicate entry for resume op " << opCount
								    		 << " in nestingLoopMap\n";
								    	cout << "ERROR: Duplicate entry:\n";
								    	it->printPauseResumeResetTupleDetails();
								    	return -1;
								    }
								}
								UAFDetector::pauseResumeResetTuple prr;
								prr.resumeOp = opCount;
								existingEntry.pauseResumeResetSet.push_back(prr);
								detector.nestingLoopMap.erase(detector.nestingLoopMap.find(sharedVariable));
								detector.nestingLoopMap[sharedVariable] = existingEntry;
							}

							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							stackElement.taskID = task;
							stackForThreadOrder.push(stackElement);
							stackForTaskOrder.push(stackElement);
							stackForNestingOrder.push(stackElement);

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadOrder.peek(threadID);
							// In the case of resume, the previous op in thread is not the previous op in the task.
							// So, get top of task stack to obtain prev op in task.
							MultiStack::stackElementType previousOpInTask = stackForTaskOrder.peek(threadID);

							// resume is the beginning of a block
							opdetails.taskID = task;

							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								cout << "While examining op " << opCount;
								cout << endl;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif
								existingEntry.nextOpInThread = opCount;
								opdetails.prevOpInThread = previousOpInThread.opID;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;

							}

							if (detector.opIDMap.find(previousOpInTask.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInTask.opID << " in opIDMap\n";
								cout << "While examining op " << opCount << endl;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInTask.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op in task has same threadID and taskID as resume
								assert(previousOpInTask.threadID == threadID);
								assert(existingEntry.threadID == threadID);
								assert(previousOpInTask.taskID.compare(task) == 0);
								assert(existingEntry.taskID.compare(task) == 0);
#endif

								existingEntry.nextOpInTask = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInTask.opID));
								detector.opIDMap[previousOpInTask.opID] = existingEntry;
							}

							if (detector.taskIDMap.find(task) == detector.taskIDMap.end()) {
								cout << "WARNING: Cannot find entry for task " << task << " in taskIDMap\n";
								cout << "WARNING: While examining op " << opCount;
								UAFDetector::taskDetails taskdetails;
								taskdetails.atomic = false;
								taskdetails.firstOpInTaskID = opCount;

								// We cannot obtain parent task if resume is the first op we have seen in its own task.
								// Also there cannot be a pause op of this task either.
								vector<UAFDetector::pauseResumeResetTuple>::reverse_iterator it;
								for (it = detector.nestingLoopMap[sharedVariable].pauseResumeResetSet.rbegin();
										it != detector.nestingLoopMap[sharedVariable].pauseResumeResetSet.rend();
										it++) {
										if (it->resetOp != -1 && it->resumeOp == -1 && it->pauseOp == -1) {
											cout << "DEBUG: Matching reset op " << it->resetOp << " with resume op "
												 << opCount << "\n";
											UAFDetector::pauseResumeResetTuple prr;
											prr.resetOp = it->resetOp;
											prr.resumeOp = opCount;
											taskdetails.pauseResumeResetSequence.push_back(prr);
											break;
										}
								}
								detector.taskIDMap[task] = taskdetails;
							} else {
								UAFDetector::taskDetails existingEntry = detector.taskIDMap[task];
								if (existingEntry.pauseResumeResetSequence.size() == 0) {
									// There was no pause op of this task. But we must have seen an enq op.
									for (vector<UAFDetector::pauseResumeResetTuple>::reverse_iterator it =
											detector.nestingLoopMap[sharedVariable].pauseResumeResetSet.rbegin();
											it != detector.nestingLoopMap[sharedVariable].pauseResumeResetSet.rend();
											it++) {
											if (it->resetOp != -1 && it->resumeOp == -1 && it->pauseOp == -1) {
												cout << "DEBUG: Matching reset op " << it->resetOp << " with resume op "
													 << opCount << "\n";
												UAFDetector::pauseResumeResetTuple prr;
												prr.resetOp = it->resetOp;
												prr.resumeOp = opCount;
												existingEntry.pauseResumeResetSequence.push_back(prr);
												break;
											}
									}
								} else {
									for (vector<UAFDetector::pauseResumeResetTuple>::reverse_iterator prIt =
											existingEntry.pauseResumeResetSequence.rbegin();
											prIt != existingEntry.pauseResumeResetSequence.rend(); prIt++) {
										if (prIt->resumeOp == -1) {
											IDType pauseOp = prIt->pauseOp;
											if (pauseOp != -1 &&
													detector.pauseResumeResetOps.find(pauseOp) == detector.pauseResumeResetOps.end()) {
												cout << "ERROR: Cannot find shared variable of pause op " << pauseOp << " in pauseResumeResetOps\n";
												cout << "While examining op " << opCount << endl;
												return -1;
											} else if (pauseOp != -1) {
												std::string pauseVariable = detector.pauseResumeResetOps[pauseOp];
												if (pauseVariable.compare(sharedVariable) == 0) {
													prIt->resumeOp = opCount;
													break;
												}
											}
										}
									}
								}

								detector.taskIDMap.erase(detector.taskIDMap.find(task));
								detector.taskIDMap[task] = existingEntry;
							}

							if (detector.nestingLoopMap.find(sharedVariable) == detector.nestingLoopMap.end()) {
								UAFDetector::nestingLoopDetails loopdetails;
								UAFDetector::pauseResumeResetTuple prr;
								prr.resumeOp = opCount;
								loopdetails.pauseResumeResetSet.push_back(prr);
								detector.nestingLoopMap[sharedVariable] = loopdetails;
							} else {
								UAFDetector::nestingLoopDetails existingEntry = detector.nestingLoopMap[sharedVariable];
								if (existingEntry.pauseResumeResetSet.size() == 0) {
									UAFDetector::pauseResumeResetTuple prr;
									prr.resumeOp = opCount;
									existingEntry.pauseResumeResetSet.push_back(prr);
								} else {
									vector<UAFDetector::pauseResumeResetTuple>::reverse_iterator prIt;
									for (prIt = existingEntry.pauseResumeResetSet.rbegin();
											prIt != existingEntry.pauseResumeResetSet.rend(); prIt++) {
										if (prIt->resumeOp == -1 && (prIt->pauseOp != -1 || prIt->resetOp != -1))
											break;
									}
									if (prIt != existingEntry.pauseResumeResetSet.rend()) {
										prIt->resumeOp = opCount;
									}
								}
								detector.nestingLoopMap.erase(detector.nestingLoopMap.find(sharedVariable));
								detector.nestingLoopMap[sharedVariable] = existingEntry;
							}


							stackElement.taskID = task;
							stackForThreadOrder.push(stackElement);
							stackForTaskOrder.push(stackElement);
							stackForNestingOrder.push(stackElement);

							if (!stackForGlobalLoop.isEmpty(threadID)) {
//								while (stackForGlobalLoop.peek(threadID).opType.compare("enterloop") != 0)
								while (stackForGlobalLoop.peek(threadID).opType.compare("enterloop") != 0 &&
										stackForGlobalLoop.peek(threadID).opType.compare("exitloop") != 0)
									stackForGlobalLoop.pop(threadID);
							}
						}
					} else if (match.compare("end") == 0) {
						nodeCount++;
						opdetails.nodeID = nodeCount;
						stackElement.nodeID = nodeCount;

						if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
							UAFDetector::setOfOps set;
							set.opSet.insert(opCount);
							detector.nodeIDMap[nodeCount] = set;
						} else {
							cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
							cout << "ERROR: Existing entry:\n";
							detector.nodeIDMap[nodeCount].printDetails();
							return -1;
						}

						// Obtain 2nd argument (i.e., current task) of end
						string task;
						for (unsigned j=threadPos+1; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							// find the next non-empty matches[i] after threadID of operation.
							if (!m1.empty() && m1.compare(" ") != 0) {
								task = m1;
								break;
							}
						}

						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for end " << opCount
								 << " on stackForThreadOrder\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							opdetails.taskID = task;

							if (detector.taskIDMap.find(task) == detector.taskIDMap.end()) {
								UAFDetector::taskDetails taskdetails;
								taskdetails.endOpID = opCount;
								taskdetails.firstOpInTaskID = opCount;
								taskdetails.lastOpInTaskID = opCount;
								detector.taskIDMap[task] = taskdetails;
							} else {
								UAFDetector::taskDetails existingEntry = detector.taskIDMap[task];
#ifdef SANITYCHECK
								// If there already exists an entry for this task in taskIDMap, we had seen
								// an enq op.
								assert(existingEntry.endOpID == -1);
								assert(existingEntry.enqOpID != -1);
#endif
								existingEntry.endOpID = opCount;
								existingEntry.firstOpInTaskID = opCount;
								existingEntry.lastOpInTaskID = opCount;
								detector.taskIDMap.erase(detector.taskIDMap.find(task));
								detector.taskIDMap[task] = existingEntry;
							}

							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}


							stackElement.taskID = task;
							stackForThreadOrder.push(stackElement);

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadOrder.peek(threadID);
							// In the case of end, the previous op in thread is the previous op in task.
							// So no need to look separately in task stack.

							opdetails.taskID = task;
							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								cout << "While examining op " << opCount;
								cout << endl;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
								assert(previousOpInThread.taskID.compare(task) == 0);
								assert(existingEntry.taskID.compare(task) == 0);
#endif

								existingEntry.nextOpInTask = opCount;
								existingEntry.nextOpInThread = opCount;
								opdetails.prevOpInThread = previousOpInThread.opID;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							if (detector.taskIDMap.find(task) == detector.taskIDMap.end()) {
								cout << "ERROR: Cannot find entry for task " << task << " in taskIDMap\n";
								cout << "While examining op " << opCount << endl;
								return -1;
							} else {
								UAFDetector::taskDetails existingEntry = detector.taskIDMap[task];
								existingEntry.endOpID = opCount;
								existingEntry.lastOpInTaskID = opCount;
								if (!stackForNestingOrder.isEmpty(threadID)) {
									MultiStack::stackElementType topOfNestingStack = stackForNestingOrder.peek(threadID);
#ifdef PERMIT
									if (topOfNestingStack.opType.compare("revoke") == 0) {
#else
									if (topOfNestingStack.opType.compare("resume") == 0) {
#endif
#ifdef SANITYCHECK
										assert(topOfNestingStack.taskID.compare(task) == 0);
										assert(topOfNestingStack.threadID == threadID);
#endif
										existingEntry.lastResumeOpID = topOfNestingStack.opID;
									}
								}
								detector.taskIDMap.erase(detector.taskIDMap.find(task));
								detector.taskIDMap[task] = existingEntry;
							}


							stackElement.taskID = task;
							stackForThreadOrder.push(stackElement);
							stackForTaskOrder.stackClear(threadID, task);
							stackForNestingOrder.stackClear(threadID, task);

							if (!stackForGlobalLoop.isEmpty(threadID)) {
//								while (stackForGlobalLoop.peek(threadID).opType.compare("enterloop") != 0)
								while (stackForGlobalLoop.peek(threadID).opType.compare("enterloop") != 0 &&
										stackForGlobalLoop.peek(threadID).opType.compare("exitloop") != 0)
									stackForGlobalLoop.pop(threadID);
							}
						}
					} else if (match.compare("fork") == 0) {
						nodeCount++;
						opdetails.nodeID = nodeCount;
						stackElement.nodeID = nodeCount;

						if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
							UAFDetector::setOfOps set;
							set.opSet.insert(opCount);
							detector.nodeIDMap[nodeCount] = set;
						} else {
							cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
							cout << "ERROR: Existing entry:\n";
							detector.nodeIDMap[nodeCount].printDetails();
							return -1;
						}

						// Obtain 2nd argument (i.e., target thread) of fork
						IDType targetThread;
						for (unsigned j=threadPos+1; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							// find the next non-empty matches[i] after threadID of operation.
							if (!m1.empty() && m1.compare(" ") != 0) {
								targetThread = atoi(m1.c_str());
								break;
							}
						}

						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for fork " << opCount
								 << " on stackForThreadOrder\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread

							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							if (detector.threadIDMap.find(targetThread) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.forkOpID = opCount;
								detector.threadIDMap[targetThread] = threaddetails;
							} else {
								cout << "ERROR: Found duplicate entry for thread " << targetThread << " in threadIDMap\n";
								cout << "While examining op " << opCount << endl;
								cout << "Duplicate entry:\n";
								detector.threadIDMap[targetThread].printThreadDetails();
								cout << endl;
								return -1;
							}


							stackForThreadOrder.push(stackElement);
							stackForTaskOrder.push(stackElement);
							stackForGlobalLoop.push(stackElement);
						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadOrder.peek(threadID);
							// In the case of fork, the previous op in thread is not necessarily the previous op in task.
							MultiStack::stackElementType previousOpInTask = stackForTaskOrder.peek(threadID);

							// If the previous op in task stack is the same as the previous op in thread stack, then we are not
							// in the middle of a nesting loop.
							bool firstOpInsideNestingLoop;
							bool firstOpInsideGlobalLoop = false;
							if (previousOpInThread.opID == -1) {
								cout << "ERROR: ThreadOrder stack is not empty, but a peek operation returns invalid element\n";
								return -1;
							}
							if (!stackForTaskOrder.isEmpty(threadID)) {
								if (previousOpInTask.opID != previousOpInThread.opID)
									firstOpInsideNestingLoop = true;
								else {
									// If the previous op in task is a pause, then we are in a new block
#ifdef PERMIT
									if (detector.opIDMap[previousOpInTask.opID].opType.compare("permit") == 0)
#else
									if (detector.opIDMap[previousOpInTask.opID].opType.compare("pause") == 0)
#endif
										firstOpInsideNestingLoop = true;
									else
										firstOpInsideNestingLoop = false;
								}
							} else {
								// Task order stack is empty, this means we are not inside a nesting loop
								firstOpInsideNestingLoop = false;

								if (stackForNestingOrder.isEmpty(threadID) && !stackForGlobalLoop.isEmpty(threadID)) {
									// We are inside the global loop.
									MultiStack::stackElementType topOfGlobalLoopStack = stackForGlobalLoop.peek(threadID);
//									if (topOfGlobalLoopStack.opType.compare("enterloop") == 0)
									if (topOfGlobalLoopStack.opType.compare("enterloop") == 0
											|| topOfGlobalLoopStack.opType.compare("exitloop") == 0) {
										if (previousOpInThread.opType.compare("exitloop") == 0)
											firstOpInsideGlobalLoop = false;
										else
											firstOpInsideGlobalLoop = true;
									}
								} else if (!stackForNestingOrder.isEmpty(threadID)) {
									cout << "ERROR: TaskOrder stack is empty but NestingOrder stack is not: for thread " << threadID << "\n";
									cout << "ERROR: Top element in NestingOrder stack: " << stackForNestingOrder.peek(threadID).opID
										 << "\n";
									return -1;
								}
							}

							// previous-op-in-thread need not be the previous-op-in-task or the previous-op-in-block
							if (!firstOpInsideNestingLoop) {
								if (firstOpInsideGlobalLoop) {
								} else {
									opdetails.taskID = previousOpInThread.taskID;
								}
							} else {
								// If this is the first op inside the nesting loop, then we are in a new block
								opdetails.taskID = previousOpInTask.taskID;
							}

							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								cout << "While examining op " << opCount;
								cout << endl;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								if (!firstOpInsideNestingLoop) {
									if (previousOpInThread.taskID.compare("") != 0)
										existingEntry.nextOpInTask = opCount;
								}
								existingEntry.nextOpInThread = opCount;
								opdetails.prevOpInThread = previousOpInThread.opID;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							if (firstOpInsideNestingLoop) {
								if (detector.opIDMap.find(previousOpInTask.opID) == detector.opIDMap.end()) {
									cout << "ERROR: Cannot find entry for op " << previousOpInTask.opID << " in opIDMap\n";
									cout << "ERROR: While examining op " << opCount << "\n";
									return -1;
								} else {
									UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInTask.opID];
#ifdef SANITYCHECK
									// Sanity check: prev-op has same thread as current op in stack and opIDMap
									assert(previousOpInTask.threadID == threadID);
									assert(existingEntry.threadID == threadID);
#endif
									opdetails.taskID = previousOpInTask.taskID;
									existingEntry.nextOpInTask = opCount;
									detector.opIDMap.erase(detector.opIDMap.find(previousOpInTask.opID));
									detector.opIDMap[previousOpInTask.opID] = existingEntry;
								}


							}


							if (detector.threadIDMap.find(targetThread) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.forkOpID = opCount;
								detector.threadIDMap[targetThread] = threaddetails;
							} else {
								cout << "ERROR: Found duplicate entry for thread " << targetThread << " in threadIDMap\n";
								cout << "While examining op " << opCount << endl;
								cout << "Duplicate entry:\n";
								detector.threadIDMap[targetThread].printThreadDetails();
								cout << endl;
								return -1;
							}

							if (!firstOpInsideNestingLoop) {
								if (firstOpInsideGlobalLoop) {
								} else {
									stackElement.taskID = previousOpInThread.taskID;
								}
							} else {
								stackElement.taskID = previousOpInTask.taskID;
							}
							stackForThreadOrder.push(stackElement);
							if (!stackForTaskOrder.isEmpty(threadID))
								stackForTaskOrder.push(stackElement);
							else {
								if (stackForNestingOrder.isEmpty(threadID) && !stackForGlobalLoop.isEmpty(threadID)) {
									stackForGlobalLoop.push(stackElement);
								}
							}
						}
					} else if (match.compare("join") == 0) {
						nodeCount++;
						opdetails.nodeID = nodeCount;
						stackElement.nodeID = nodeCount;

						if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
							UAFDetector::setOfOps set;
							set.opSet.insert(opCount);
							detector.nodeIDMap[nodeCount] = set;
						} else {
							cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
							cout << "ERROR: Existing entry:\n";
							detector.nodeIDMap[nodeCount].printDetails();
							return -1;
						}

						// Obtain 2nd argument (i.e., target thread) of join
						IDType targetThread;
						for (unsigned j=threadPos+1; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							// find the next non-empty matches[i] after threadID of operation.
							if (!m1.empty() && m1.compare(" ") != 0) {
								targetThread = atoi(m1.c_str());
								break;
							}
						}

						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for join " << opCount
								 << " on stackForThreadOrder\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread

							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							if (detector.threadIDMap.find(targetThread) == detector.threadIDMap.end()) {
								cout << "ERROR: Cannot find entry for thread " << targetThread << " in threadIDMap\n";
								cout << "While examining op " << opCount << endl;
								return -1;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[targetThread];
#ifdef SANITYCHECK
								// Sanity check: the join op id should not be set till now
								assert(existingEntry.joinOpID == -1);
#endif
								existingEntry.joinOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(targetThread));
								detector.threadIDMap[targetThread] = existingEntry;
							}

							stackForThreadOrder.push(stackElement);
							stackForTaskOrder.push(stackElement);
							stackForGlobalLoop.push(stackElement);
						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadOrder.peek(threadID);
							// In the case of join, the previous op in thread is not necessarily the previous op in task.
							MultiStack::stackElementType previousOpInTask = stackForTaskOrder.peek(threadID);

							// If the previous op in task stack is the same as the previous op in thread stack, then we are not
							// in the middle of a nesting loop.
							bool firstOpInsideNestingLoop;
							bool firstOpInsideGlobalLoop = false;
							if (previousOpInThread.opID == -1) {
								cout << "ERROR: ThreadOrder stack is not empty, but a peek operation returns invalid element\n";
								return -1;
							}
							if (!stackForTaskOrder.isEmpty(threadID)) {
								if (previousOpInTask.opID != previousOpInThread.opID)
									firstOpInsideNestingLoop = true;
								else {
									// If the previous op in task is a pause, then we are in a new block
#ifdef PERMIT
									if (detector.opIDMap[previousOpInTask.opID].opType.compare("permit") == 0)
#else
									if (detector.opIDMap[previousOpInTask.opID].opType.compare("pause") == 0)
#endif
										firstOpInsideNestingLoop = true;
									else
										firstOpInsideNestingLoop = false;
								}
							} else {
								// Task order stack is empty, this means we are not inside a nesting loop
								firstOpInsideNestingLoop = false;

								if (stackForNestingOrder.isEmpty(threadID) && !stackForGlobalLoop.isEmpty(threadID)) {
									// We are inside the global loop.
									MultiStack::stackElementType topOfGlobalLoopStack = stackForGlobalLoop.peek(threadID);
//									if (topOfGlobalLoopStack.opType.compare("enterloop") == 0)
									if (topOfGlobalLoopStack.opType.compare("enterloop") == 0
											|| topOfGlobalLoopStack.opType.compare("exitloop") == 0) {
										if (previousOpInThread.opType.compare("exitloop") == 0)
											firstOpInsideGlobalLoop = false;
										else
											firstOpInsideGlobalLoop = true;
									}
								} else if (!stackForNestingOrder.isEmpty(threadID)) {
									cout << "ERROR: TaskOrder stack is empty but NestingOrder stack is not: for thread " << threadID << "\n";
									cout << "ERROR: Top element in NestingOrder stack: " << stackForNestingOrder.peek(threadID).opID
										 << "\n";
									return -1;
								}
							}

							// previous-op-in-thread need not be the previous-op-in-task or the previous-op-in-block
							if (!firstOpInsideNestingLoop) {
								if (firstOpInsideGlobalLoop) {
								} else {
									opdetails.taskID = previousOpInThread.taskID;
								}
							} else {
								// If this is the first op inside the nesting loop, then we are in a new block
								opdetails.taskID = previousOpInTask.taskID;
							}

							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								cout << "While examining op " << opCount;
								cout << endl;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								if (!firstOpInsideNestingLoop) {
									if (previousOpInThread.taskID.compare("") != 0)
										existingEntry.nextOpInTask = opCount;
								}
								existingEntry.nextOpInThread = opCount;
								opdetails.prevOpInThread = previousOpInThread.opID;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							if (firstOpInsideNestingLoop) {
								if (detector.opIDMap.find(previousOpInTask.opID) == detector.opIDMap.end()) {
									cout << "ERROR: Cannot find entry for op " << previousOpInTask.opID << " in opIDMap\n";
									cout << "ERROR: While examining op " << opCount << "\n";
									return -1;
								} else {
									UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInTask.opID];
#ifdef SANITYCHECK
									// Sanity check: prev-op has same thread as current op in stack and opIDMap
									assert(previousOpInTask.threadID == threadID);
									assert(existingEntry.threadID == threadID);
#endif
									opdetails.taskID = previousOpInTask.taskID;
									existingEntry.nextOpInTask = opCount;
									detector.opIDMap.erase(detector.opIDMap.find(previousOpInTask.opID));
									detector.opIDMap[previousOpInTask.opID] = existingEntry;
								}
							}

							if (detector.threadIDMap.find(targetThread) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.joinOpID = opCount;
								detector.threadIDMap[targetThread] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[targetThread];
#ifdef SANITYCHECK
								// Sanity check: the join op id should not be set till now
								assert(existingEntry.joinOpID == -1);
#endif
								existingEntry.joinOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(targetThread));
								detector.threadIDMap[targetThread] = existingEntry;
							}

							if (!firstOpInsideNestingLoop) {
								if (firstOpInsideGlobalLoop) {
								} else {
									stackElement.taskID = previousOpInThread.taskID;
								}
							} else {
								stackElement.taskID = previousOpInTask.taskID;
							}
							stackForThreadOrder.push(stackElement);
							if (!stackForTaskOrder.isEmpty(threadID))
								stackForTaskOrder.push(stackElement);
							else {
								if (stackForNestingOrder.isEmpty(threadID) && !stackForGlobalLoop.isEmpty(threadID)) {
									stackForGlobalLoop.push(stackElement);
								}
							}
						}
					} else if (match.compare("alloc") == 0) {
						// Obtain 2nd & 3rd argument (i.e., starting address & num of bytes) of alloc
						string baseAddress;
						IDType size;
						unsigned j;
						for (j=threadPos+1; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							// find the next non-empty matches[i] after threadID of operation.
							if (!m1.empty() && m1.compare(" ") != 0) {
								baseAddress = m1;
								break;
							}
						}
						for (j=j+1; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							// find the next non-empty matches[i] after threadID of operation.
							if (!m1.empty() && m1.compare(" ") != 0) {
								size = atoi(m1.c_str());
								break;
							}
						}

						UAFDetector::memoryOpDetails memdetails;
						memdetails.startingAddress = baseAddress;
						memdetails.range = size;
						if (detector.allocSet.find(opCount) == detector.allocSet.end()) {
							detector.allocSet[opCount] = memdetails;
						} else {
							cout << "ERROR: Found duplicate entry for op " << opCount << " in opIDMap\n";
								cout << endl;
							return -1;
						}
						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadOrder.isEmpty(threadID)){
							nodeCount++;
							opdetails.nodeID = nodeCount;
							stackElement.nodeID = nodeCount;

							if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
								UAFDetector::setOfOps set;
								set.opSet.insert(opCount);
								detector.nodeIDMap[nodeCount] = set;
							} else {
								cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
								cout << "ERROR: Existing entry:\n";
								detector.nodeIDMap[nodeCount].printDetails();
								return -1;
							}

							cout << "WARNING: No previous op found for alloc " << opCount
								 << " on stackForThreadOrder\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread

							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							stackForThreadOrder.push(stackElement);
							stackForTaskOrder.push(stackElement);
							stackForGlobalLoop.push(stackElement);
						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadOrder.peek(threadID);
							// In the case of alloc, the previous op in thread is not necessarily the previous op in task.
							MultiStack::stackElementType previousOpInTask = stackForTaskOrder.peek(threadID);

							// If the previous op in task stack is the same as the previous op in thread stack, then we are not
							// in the middle of a nesting loop.
							bool firstOpInsideNestingLoop;
							bool firstOpInsideGlobalLoop = false;
							if (previousOpInThread.opID == -1) {
								cout << "ERROR: ThreadOrder stack is not empty, but a peek operation returns invalid element\n";
								return -1;
							}
							if (!stackForTaskOrder.isEmpty(threadID)) {
								if (previousOpInTask.opID != previousOpInThread.opID)
									firstOpInsideNestingLoop = true;
								else {
									// If the previous op in task is a pause, then we are in a new block
#ifdef PERMIT
									if (detector.opIDMap[previousOpInTask.opID].opType.compare("permit") == 0)
#else
									if (detector.opIDMap[previousOpInTask.opID].opType.compare("pause") == 0)
#endif
										firstOpInsideNestingLoop = true;
									else
										firstOpInsideNestingLoop = false;
								}
							} else {
								// Task order stack is empty, this means we are not inside a nesting loop
								firstOpInsideNestingLoop = false;

								if (stackForNestingOrder.isEmpty(threadID) && !stackForGlobalLoop.isEmpty(threadID)) {
									// We are inside the global loop.
									MultiStack::stackElementType topOfGlobalLoopStack = stackForGlobalLoop.peek(threadID);
//									if (topOfGlobalLoopStack.opType.compare("enterloop") == 0)
									if (topOfGlobalLoopStack.opType.compare("enterloop") == 0
											|| topOfGlobalLoopStack.opType.compare("exitloop") == 0) {
										if (previousOpInThread.opType.compare("exitloop") == 0)
											firstOpInsideGlobalLoop = false;
										else
											firstOpInsideGlobalLoop = true;
									}
								} else if (!stackForNestingOrder.isEmpty(threadID)) {
									cout << "ERROR: TaskOrder stack is empty but NestingOrder stack is not: for thread " << threadID << "\n";
									cout << "ERROR: Top element in NestingOrder stack: " << stackForNestingOrder.peek(threadID).opID
										 << "\n";
									return -1;
								}
							}

							// previous-op-in-thread need not be the previous-op-in-task or the previous-op-in-block
							if (!firstOpInsideNestingLoop) {
								if (firstOpInsideGlobalLoop) {

									nodeCount++;
									opdetails.nodeID = nodeCount;
									stackElement.nodeID = nodeCount;

									if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
										UAFDetector::setOfOps set;
										set.opSet.insert(opCount);
										detector.nodeIDMap[nodeCount] = set;
									} else {
										cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
										cout << "ERROR: Existing entry:\n";
										detector.nodeIDMap[nodeCount].printDetails();
										return -1;
									}

								} else {
									opdetails.taskID = previousOpInThread.taskID;

									if (previousOpInThread.opType.compare("alloc") == 0 ||
											previousOpInThread.opType.compare("free") == 0 ||
											previousOpInThread.opType.compare("read") == 0 ||
											previousOpInThread.opType.compare("write") == 0) {
										opdetails.nodeID = previousOpInThread.nodeID;

										stackElement.nodeID = previousOpInThread.nodeID;

										if (detector.nodeIDMap.find(previousOpInThread.nodeID) == detector.nodeIDMap.end()) {
											cout << "ERROR: Cannot find entry for node " << previousOpInThread.nodeID << " in nodeIDMap\n";
											return -1;
										} else {
											UAFDetector::setOfOps existingEntry = detector.nodeIDMap[previousOpInThread.nodeID];
											existingEntry.opSet.insert(opCount);
											detector.nodeIDMap.erase(detector.nodeIDMap.find(previousOpInThread.nodeID));
											detector.nodeIDMap[previousOpInThread.nodeID] = existingEntry;
										}
									} else {
										nodeCount++;
										opdetails.nodeID = nodeCount;
										stackElement.nodeID = nodeCount;

										if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
											UAFDetector::setOfOps set;
											set.opSet.insert(opCount);
											detector.nodeIDMap[nodeCount] = set;
										} else {
											cout << "ERROR: Found duplicate entry for node " << nodeCount << " in nodeIDMap\n";
											return -1;
										}
									}
								}
							} else {
								// If this is the first op inside the nesting loop, then we are in a new block
								opdetails.taskID = previousOpInTask.taskID;

								nodeCount++;
								opdetails.nodeID = nodeCount;
								stackElement.nodeID = nodeCount;

								if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
									UAFDetector::setOfOps set;
									set.opSet.insert(opCount);
									detector.nodeIDMap[nodeCount] = set;
								} else {
									cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
									cout << "ERROR: Existing entry:\n";
									detector.nodeIDMap[nodeCount].printDetails();
									return -1;
								}

							}

							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								cout << "While examining op " << opCount;
								cout << endl;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								if (!firstOpInsideNestingLoop) {
									if (previousOpInThread.taskID.compare("") != 0)
										existingEntry.nextOpInTask = opCount;
								}
								existingEntry.nextOpInThread = opCount;
								opdetails.prevOpInThread = previousOpInThread.opID;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							if (firstOpInsideNestingLoop) {
								if (detector.opIDMap.find(previousOpInTask.opID) == detector.opIDMap.end()) {
									cout << "ERROR: Cannot find entry for op " << previousOpInTask.opID << " in opIDMap\n";
									cout << "ERROR: While examining op " << opCount << "\n";
									return -1;
								} else {
									UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInTask.opID];
#ifdef SANITYCHECK
									// Sanity check: prev-op has same thread as current op in stack and opIDMap
									assert(previousOpInTask.threadID == threadID);
									assert(existingEntry.threadID == threadID);
#endif
									opdetails.taskID = previousOpInTask.taskID;
									existingEntry.nextOpInTask = opCount;
									detector.opIDMap.erase(detector.opIDMap.find(previousOpInTask.opID));
									detector.opIDMap[previousOpInTask.opID] = existingEntry;
								}
							}


							if (!firstOpInsideNestingLoop) {
								if (firstOpInsideGlobalLoop) {
								} else {
									stackElement.taskID = previousOpInThread.taskID;
								}
							} else {
								stackElement.taskID = previousOpInTask.taskID;
							}
							stackForThreadOrder.push(stackElement);
							if (!stackForTaskOrder.isEmpty(threadID))
								stackForTaskOrder.push(stackElement);
							else {
								if (stackForNestingOrder.isEmpty(threadID) && !stackForGlobalLoop.isEmpty(threadID)) {
									stackForGlobalLoop.push(stackElement);
								}
							}
						}

					} else if (match.compare("free") == 0) {
						// Obtain 2nd & 3rd argument (i.e., starting address & num of bytes) of free
						string baseAddress;
						IDType size;
						unsigned j;
						for (j=threadPos+1; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							// find the next non-empty matches[i] after threadID of operation.
							if (!m1.empty() && m1.compare(" ") != 0) {
								baseAddress = m1;
								break;
							}
						}
						for (j=j+1; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							// find the next non-empty matches[i] after threadID of operation.
							if (!m1.empty() && m1.compare(" ") != 0) {
								size = atoi(m1.c_str());
								break;
							}
						}

						std::stringstream str1;
						str1 << baseAddress;
						IDType baseAddressIntFree, endAddressIntFree;
						str1 >> std::hex >> baseAddressIntFree;
						endAddressIntFree = baseAddressIntFree + size - 1;

						for (map<IDType, UAFDetector::memoryOpDetails>::iterator it = detector.allocSet.begin(); it != detector.allocSet.end(); it++) {
							std::stringstream str2;
							str2 << it->second.startingAddress;
							IDType baseAddressIntAlloc, endAddressIntAlloc;
							str2 >> std::hex >> baseAddressIntAlloc;
							endAddressIntAlloc = baseAddressIntAlloc + it->second.range - 1;

							if (baseAddressIntAlloc <= baseAddressIntFree && endAddressIntFree <= endAddressIntAlloc) {
								if (detector.allocIDMap.find(it->first) == detector.allocIDMap.end()) {
									UAFDetector::allocOpDetails allocdetails;
									allocdetails.freeOps.insert(opCount);
									detector.allocIDMap[it->first] = allocdetails;
								} else {
									UAFDetector::allocOpDetails existingEntry = detector.allocIDMap[it->first];
									existingEntry.freeOps.insert(opCount);
									detector.allocIDMap.erase(detector.allocIDMap.find(it->first));
									detector.allocIDMap[it->first] = existingEntry;
								}

								if (detector.freeIDMap.find(opCount) == detector.freeIDMap.end()) {
									UAFDetector::freeOpDetails freedetails;
									freedetails.allocOpID = it->first;
									detector.freeIDMap[opCount] = freedetails;
								} else {
									UAFDetector::freeOpDetails existingEntry = detector.freeIDMap[opCount];
									existingEntry.allocOpID = it->first;
									detector.freeIDMap.erase(detector.freeIDMap.find(opCount));
									detector.freeIDMap[opCount] = existingEntry;
								}
							}
						}

						for (map<IDType, UAFDetector::memoryOpDetails>::iterator it = detector.readSet.begin(); it != detector.readSet.end(); it++) {
							std::stringstream str2;
							str2 << it->second.startingAddress;
							IDType addressIntRead;
							str2 >> std::hex >> addressIntRead;

							IDType readNode = detector.opIDMap[it->first].nodeID;
							if (baseAddressIntFree <= addressIntRead && addressIntRead <= endAddressIntFree) {
								if (detector.freeIDMap.find(opCount) == detector.freeIDMap.end()) {
									UAFDetector::freeOpDetails freedetails;
									freedetails.readOps.insert(it->first);
									freedetails.nodes.insert(readNode);
									detector.freeIDMap[opCount] = freedetails;
								} else {
									UAFDetector::freeOpDetails existingEntry = detector.freeIDMap[opCount];
									existingEntry.readOps.insert(it->first);
									existingEntry.nodes.insert(readNode);
									detector.freeIDMap.erase(detector.freeIDMap.find(opCount));
									detector.freeIDMap[opCount] = existingEntry;
								}
							}
						}

						for (map<IDType, UAFDetector::memoryOpDetails>::iterator it = detector.writeSet.begin(); it != detector.writeSet.end(); it++) {
							std::stringstream str2;
							str2 << it->second.startingAddress;
							IDType addressIntWrite;
							str2 >> std::hex >> addressIntWrite;

							IDType writeNode = detector.opIDMap[it->first].nodeID;
							if (baseAddressIntFree <= addressIntWrite && addressIntWrite <= endAddressIntFree) {
								if (detector.freeIDMap.find(opCount) == detector.freeIDMap.end()) {
									UAFDetector::freeOpDetails freedetails;
									freedetails.writeOps.insert(it->first);
									freedetails.nodes.insert(writeNode);
									detector.freeIDMap[opCount] = freedetails;
								} else {
									UAFDetector::freeOpDetails existingEntry = detector.freeIDMap[opCount];
									existingEntry.writeOps.insert(it->first);
									existingEntry.nodes.insert(writeNode);
									detector.freeIDMap.erase(detector.freeIDMap.find(opCount));
									detector.freeIDMap[opCount] = existingEntry;
								}
							}
						}

						UAFDetector::memoryOpDetails memdetails;
						memdetails.startingAddress = baseAddress;
						memdetails.range = size;
						if (detector.freeSet.find(opCount) == detector.freeSet.end()) {
							detector.freeSet[opCount] = memdetails;
						} else {
							cout << "ERROR: Found duplicate entry for op " << opCount << " in opIDMap\n";
							return -1;
						}
						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadOrder.isEmpty(threadID)){

							nodeCount++;
							opdetails.nodeID = nodeCount;
							stackElement.nodeID = nodeCount;

							if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
								UAFDetector::setOfOps set;
								set.opSet.insert(opCount);
								detector.nodeIDMap[nodeCount] = set;
							} else {
								cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
								cout << "ERROR: Existing entry:\n";
								detector.nodeIDMap[nodeCount].printDetails();
								return -1;
							}

							cout << "WARNING: No previous op found for free " << opCount
								 << " on stackForThreadOrder\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread

							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							stackForThreadOrder.push(stackElement);
							stackForTaskOrder.push(stackElement);
							stackForGlobalLoop.push(stackElement);
						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadOrder.peek(threadID);
							// In the case of free, the previous op in thread is not necessarily the previous op in task.
							MultiStack::stackElementType previousOpInTask = stackForTaskOrder.peek(threadID);

							// If the previous op in task stack is the same as the previous op in thread stack, then we are not
							// in the middle of a nesting loop.
							bool firstOpInsideNestingLoop;
							bool firstOpInsideGlobalLoop = false;
							if (previousOpInThread.opID == -1) {
								cout << "ERROR: ThreadOrder stack is not empty, but a peek operation returns invalid element\n";
								return -1;
							}
							if (!stackForTaskOrder.isEmpty(threadID)) {
								if (previousOpInTask.opID != previousOpInThread.opID)
									firstOpInsideNestingLoop = true;
								else {
									// If the previous op in task is a pause, then we are in a new block
#ifdef PERMIT
									if (detector.opIDMap[previousOpInTask.opID].opType.compare("permit") == 0)
#else
									if (detector.opIDMap[previousOpInTask.opID].opType.compare("pause") == 0)
#endif
										firstOpInsideNestingLoop = true;
									else
										firstOpInsideNestingLoop = false;
								}
							} else {
								// Task order stack is empty, this means we are not inside a nesting loop
								firstOpInsideNestingLoop = false;

								if (stackForNestingOrder.isEmpty(threadID) && !stackForGlobalLoop.isEmpty(threadID)) {
									// We are inside the global loop.
									MultiStack::stackElementType topOfGlobalLoopStack = stackForGlobalLoop.peek(threadID);
//									if (topOfGlobalLoopStack.opType.compare("enterloop") == 0)
									if (topOfGlobalLoopStack.opType.compare("enterloop") == 0
											|| topOfGlobalLoopStack.opType.compare("exitloop") == 0) {
										if (previousOpInThread.opType.compare("exitloop") == 0)
											firstOpInsideGlobalLoop = false;
										else
											firstOpInsideGlobalLoop = true;
									}
								} else if (!stackForNestingOrder.isEmpty(threadID)) {
									cout << "ERROR: TaskOrder stack is empty but NestingOrder stack is not: for thread " << threadID << "\n";
									cout << "ERROR: Top element in NestingOrder stack: " << stackForNestingOrder.peek(threadID).opID
										 << "\n";
									return -1;
								}
							}

							// previous-op-in-thread need not be the previous-op-in-task or the previous-op-in-block
							if (!firstOpInsideNestingLoop) {
								if (firstOpInsideGlobalLoop) {

									nodeCount++;
									opdetails.nodeID = nodeCount;
									stackElement.nodeID = nodeCount;

									if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
										UAFDetector::setOfOps set;
										set.opSet.insert(opCount);
										detector.nodeIDMap[nodeCount] = set;
									} else {
										cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
										cout << "ERROR: Existing entry:\n";
										detector.nodeIDMap[nodeCount].printDetails();
										return -1;
									}
								} else {
									opdetails.taskID = previousOpInThread.taskID;
									if (previousOpInThread.opType.compare("alloc") == 0 ||
											previousOpInThread.opType.compare("free") == 0 ||
											previousOpInThread.opType.compare("read") == 0 ||
											previousOpInThread.opType.compare("write") == 0) {
										opdetails.nodeID = previousOpInThread.nodeID;

										stackElement.nodeID = previousOpInThread.nodeID;

										if (detector.nodeIDMap.find(previousOpInThread.nodeID) == detector.nodeIDMap.end()) {
											cout << "ERROR: Cannot find entry for node " << previousOpInThread.nodeID << " in nodeIDMap\n";
											return -1;
										} else {
											UAFDetector::setOfOps existingEntry = detector.nodeIDMap[previousOpInThread.nodeID];
											existingEntry.opSet.insert(opCount);
											detector.nodeIDMap.erase(detector.nodeIDMap.find(previousOpInThread.nodeID));
											detector.nodeIDMap[previousOpInThread.nodeID] = existingEntry;
										}
									} else {
										nodeCount++;
										opdetails.nodeID = nodeCount;
										stackElement.nodeID = nodeCount;

										if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
											UAFDetector::setOfOps set;
											set.opSet.insert(opCount);
											detector.nodeIDMap[nodeCount] = set;
										} else {
											cout << "ERROR: Found duplicate entry for node " << nodeCount << " in nodeIDMap\n";
											return -1;
										}
									}
								}
							} else {
								// If this is the first op inside the nesting loop, then we are in a new block
								opdetails.taskID = previousOpInTask.taskID;

								nodeCount++;
								opdetails.nodeID = nodeCount;
								stackElement.nodeID = nodeCount;

								if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
									UAFDetector::setOfOps set;
									set.opSet.insert(opCount);
									detector.nodeIDMap[nodeCount] = set;
								} else {
									cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
									cout << "ERROR: Existing entry:\n";
									detector.nodeIDMap[nodeCount].printDetails();
									return -1;
								}
							}

							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								cout << "While examining op " << opCount;
								cout << endl;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								if (!firstOpInsideNestingLoop) {
									if (previousOpInThread.taskID.compare("") != 0)
										existingEntry.nextOpInTask = opCount;
								}
								existingEntry.nextOpInThread = opCount;
								opdetails.prevOpInThread = previousOpInThread.opID;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							if (firstOpInsideNestingLoop) {
								if (detector.opIDMap.find(previousOpInTask.opID) == detector.opIDMap.end()) {
									cout << "ERROR: Cannot find entry for op " << previousOpInTask.opID << " in opIDMap\n";
									cout << "ERROR: While examining op " << opCount << "\n";
									return -1;
								} else {
									UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInTask.opID];
#ifdef SANITYCHECK
									// Sanity check: prev-op has same thread as current op in stack and opIDMap
									assert(previousOpInTask.threadID == threadID);
									assert(existingEntry.threadID == threadID);
#endif
									opdetails.taskID = previousOpInTask.taskID;
									existingEntry.nextOpInTask = opCount;
									detector.opIDMap.erase(detector.opIDMap.find(previousOpInTask.opID));
									detector.opIDMap[previousOpInTask.opID] = existingEntry;
								}
							}

							if (!firstOpInsideNestingLoop) {
								if (firstOpInsideGlobalLoop) {
								} else {
									stackElement.taskID = previousOpInThread.taskID;
								}
							} else {
								stackElement.taskID = previousOpInTask.taskID;
							}

							stackForThreadOrder.push(stackElement);
							if (!stackForTaskOrder.isEmpty(threadID))
								stackForTaskOrder.push(stackElement);
							else {
								if (stackForNestingOrder.isEmpty(threadID) && !stackForGlobalLoop.isEmpty(threadID)) {
									stackForGlobalLoop.push(stackElement);
								}
							}
						}

					} else if (match.compare("read") == 0) {
						// Obtain 2nd argument (i.e., address) of read
						string address;
						for (unsigned j=threadPos+1; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							// find the next non-empty matches[i] after threadID of operation.
							if (!m1.empty() && m1.compare(" ") != 0) {
								address = m1;
								break;
							}
						}

						std::stringstream str1;
						str1 << address;
						IDType addressIntRead;
						str1 >> std::hex >> addressIntRead;

						for (map<IDType, UAFDetector::memoryOpDetails>::iterator it = detector.allocSet.begin(); it != detector.allocSet.end(); it++) {
							std::stringstream str2;
							str2 << it->second.startingAddress;
							IDType baseAddressIntAlloc, endAddressIntAlloc;
							str2 >> std::hex >> baseAddressIntAlloc;
							endAddressIntAlloc = baseAddressIntAlloc + it->second.range - 1;

							if (baseAddressIntAlloc <= addressIntRead && addressIntRead <= endAddressIntAlloc) {
								if (detector.allocIDMap.find(it->first) == detector.allocIDMap.end()) {
									UAFDetector::allocOpDetails allocdetails;
									allocdetails.readOps.insert(opCount);
									detector.allocIDMap[it->first] = allocdetails;
								} else {
									UAFDetector::allocOpDetails existingEntry = detector.allocIDMap[it->first];
									existingEntry.readOps.insert(opCount);
									detector.allocIDMap.erase(detector.allocIDMap.find(it->first));
									detector.allocIDMap[it->first] = existingEntry;
								}
							}
						}

						for (map<IDType, UAFDetector::memoryOpDetails>::iterator it = detector.freeSet.begin(); it != detector.freeSet.end(); it++) {
							std::stringstream str2;
							str2 << it->second.startingAddress;
							IDType baseAddressIntFree, endAddressIntFree;
							str2 >> std::hex >> baseAddressIntFree;
							endAddressIntFree = baseAddressIntFree + it->second.range - 1;

							if (baseAddressIntFree <= addressIntRead && addressIntRead <= endAddressIntFree) {
								if (detector.freeIDMap.find(it->first) == detector.freeIDMap.end()) {
									UAFDetector::freeOpDetails freedetails;
									freedetails.readOps.insert(opCount);
									detector.freeIDMap[it->first] = freedetails;
								} else {
									UAFDetector::freeOpDetails existingEntry = detector.freeIDMap[it->first];
									existingEntry.readOps.insert(opCount);
									detector.freeIDMap.erase(detector.freeIDMap.find(it->first));
									detector.freeIDMap[it->first] = existingEntry;
								}
							}
						}

						UAFDetector::memoryOpDetails memdetails;
						memdetails.startingAddress = address;
						if (detector.readSet.find(opCount) == detector.readSet.end()) {
							detector.readSet[opCount] = memdetails;
						} else {
							cout << "ERROR: Found duplicate entry for op " << opCount << " in opIDMap\n";
							return -1;
						}
						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadOrder.isEmpty(threadID)){

							nodeCount++;
							opdetails.nodeID = nodeCount;
							stackElement.nodeID = nodeCount;

							for (map<IDType, UAFDetector::memoryOpDetails>::iterator it = detector.allocSet.begin(); it != detector.allocSet.end(); it++) {
								std::stringstream str2;
								str2 << it->second.startingAddress;
								IDType baseAddressIntAlloc, endAddressIntAlloc;
								str2 >> std::hex >> baseAddressIntAlloc;
								endAddressIntAlloc = baseAddressIntAlloc + it->second.range - 1;

								if (baseAddressIntAlloc <= addressIntRead && addressIntRead <= endAddressIntAlloc) {
									if (detector.allocIDMap.find(it->first) == detector.allocIDMap.end()) {
										UAFDetector::allocOpDetails allocdetails;
										allocdetails.nodes.insert(nodeCount);
										detector.allocIDMap[it->first] = allocdetails;
									} else {
										UAFDetector::allocOpDetails existingEntry = detector.allocIDMap[it->first];
										existingEntry.nodes.insert(nodeCount);
										detector.allocIDMap.erase(detector.allocIDMap.find(it->first));
										detector.allocIDMap[it->first] = existingEntry;
									}
								}
							}

							for (map<IDType, UAFDetector::memoryOpDetails>::iterator it = detector.freeSet.begin(); it != detector.freeSet.end(); it++) {
								std::stringstream str2;
								str2 << it->second.startingAddress;
								IDType baseAddressIntFree, endAddressIntFree;
								str2 >> std::hex >> baseAddressIntFree;
								endAddressIntFree = baseAddressIntFree + it->second.range - 1;

								if (baseAddressIntFree <= addressIntRead && addressIntRead <= endAddressIntFree) {
									if (detector.freeIDMap.find(it->first) == detector.freeIDMap.end()) {
										UAFDetector::freeOpDetails freedetails;
										freedetails.nodes.insert(nodeCount);
										detector.freeIDMap[it->first] = freedetails;
									} else {
										UAFDetector::freeOpDetails existingEntry = detector.freeIDMap[it->first];
										existingEntry.nodes.insert(nodeCount);
										detector.freeIDMap.erase(detector.freeIDMap.find(it->first));
										detector.freeIDMap[it->first] = existingEntry;
									}
								}
							}

							if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
								UAFDetector::setOfOps set;
								set.opSet.insert(opCount);
								detector.nodeIDMap[nodeCount] = set;
							} else {
								cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
								cout << "ERROR: Existing entry:\n";
								detector.nodeIDMap[nodeCount].printDetails();
								return -1;
							}

							cout << "WARNING: No previous op found for read " << opCount
								 << " on stackForThreadOrder\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread

							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}


							stackForThreadOrder.push(stackElement);
							stackForGlobalLoop.push(stackElement);
						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadOrder.peek(threadID);
							// In the case of read, the previous op in thread is not necessarily the previous op in task.
							MultiStack::stackElementType previousOpInTask = stackForTaskOrder.peek(threadID);

							// If the previous op in task stack is the same as the previous op in thread stack, then we are not
							// in the middle of a nesting loop.
							bool firstOpInsideNestingLoop;
							bool firstOpInsideGlobalLoop = false;
							if (previousOpInThread.opID == -1) {
								cout << "ERROR: ThreadOrder stack is not empty, but a peek operation returns invalid element\n";
								return -1;
							}
							if (!stackForTaskOrder.isEmpty(threadID)) {
								if (previousOpInTask.opID != previousOpInThread.opID)
									firstOpInsideNestingLoop = true;
								else {
									// If the previous op in task is a pause, then we are in a new block
#ifdef PERMIT
									if (detector.opIDMap[previousOpInTask.opID].opType.compare("permit") == 0)
#else
									if (detector.opIDMap[previousOpInTask.opID].opType.compare("pause") == 0)
#endif
										firstOpInsideNestingLoop = true;
									else
										firstOpInsideNestingLoop = false;
								}
							} else {
								// Task order stack is empty, this means we are not inside a nesting loop
								firstOpInsideNestingLoop = false;

								if (stackForNestingOrder.isEmpty(threadID) && !stackForGlobalLoop.isEmpty(threadID)) {
									// We are inside the global loop.
									MultiStack::stackElementType topOfGlobalLoopStack = stackForGlobalLoop.peek(threadID);
//									if (topOfGlobalLoopStack.opType.compare("enterloop") == 0)
									if (topOfGlobalLoopStack.opType.compare("enterloop") == 0
											|| topOfGlobalLoopStack.opType.compare("exitloop") == 0) {
										if (previousOpInThread.opType.compare("exitloop") == 0)
											firstOpInsideGlobalLoop = false;
										else
											firstOpInsideGlobalLoop = true;
									}
								} else if (!stackForNestingOrder.isEmpty(threadID)) {
									cout << "ERROR: TaskOrder stack is empty but NestingOrder stack is not: for thread " << threadID << "\n";
									cout << "ERROR: Top element in NestingOrder stack: " << stackForNestingOrder.peek(threadID).opID
										 << "\n";
									return -1;
								}
							}

							// previous-op-in-thread need not be the previous-op-in-task or the previous-op-in-block
							if (!firstOpInsideNestingLoop) {
								if (firstOpInsideGlobalLoop) {

									nodeCount++;
									opdetails.nodeID = nodeCount;
									stackElement.nodeID = nodeCount;

									if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
										UAFDetector::setOfOps set;
										set.opSet.insert(opCount);
										detector.nodeIDMap[nodeCount] = set;
									} else {
										cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
										cout << "ERROR: Existing entry:\n";
										detector.nodeIDMap[nodeCount].printDetails();
										return -1;
									}
								} else {
									opdetails.taskID = previousOpInThread.taskID;
									if (previousOpInThread.opType.compare("alloc") == 0 ||
											previousOpInThread.opType.compare("free") == 0 ||
											previousOpInThread.opType.compare("read") == 0 ||
											previousOpInThread.opType.compare("write") == 0) {
										opdetails.nodeID = previousOpInThread.nodeID;

										stackElement.nodeID = previousOpInThread.nodeID;

										if (detector.nodeIDMap.find(previousOpInThread.nodeID) == detector.nodeIDMap.end()) {
											cout << "ERROR: Cannot find entry for node " << previousOpInThread.nodeID << " in nodeIDMap\n";
											return -1;
										} else {
											UAFDetector::setOfOps existingEntry = detector.nodeIDMap[previousOpInThread.nodeID];
											existingEntry.opSet.insert(opCount);
											detector.nodeIDMap.erase(detector.nodeIDMap.find(previousOpInThread.nodeID));
											detector.nodeIDMap[previousOpInThread.nodeID] = existingEntry;
										}
									} else {
										nodeCount++;
										opdetails.nodeID = nodeCount;
										stackElement.nodeID = nodeCount;

										if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
											UAFDetector::setOfOps set;
											set.opSet.insert(opCount);
											detector.nodeIDMap[nodeCount] = set;
										} else {
											cout << "ERROR: Found duplicate entry for node " << nodeCount << " in nodeIDMap\n";
											return -1;
										}
									}
								}
							} else {
								// If this is the first op inside the nesting loop, then we are in a new block
								opdetails.taskID = previousOpInTask.taskID;

								nodeCount++;
								opdetails.nodeID = nodeCount;
								stackElement.nodeID = nodeCount;

								if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
									UAFDetector::setOfOps set;
									set.opSet.insert(opCount);
									detector.nodeIDMap[nodeCount] = set;
								} else {
									cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
									cout << "ERROR: Existing entry:\n";
									detector.nodeIDMap[nodeCount].printDetails();
									return -1;
								}
							}

							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								cout << "While examining op " << opCount;
								cout << endl;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								if (!firstOpInsideNestingLoop) {
									if (previousOpInThread.taskID.compare("") != 0 && previousOpInThread.opType.compare("end") != 0)
										existingEntry.nextOpInTask = opCount;
								}
								existingEntry.nextOpInThread = opCount;
								opdetails.prevOpInThread = previousOpInThread.opID;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							if (firstOpInsideNestingLoop) {
								if (detector.opIDMap.find(previousOpInTask.opID) == detector.opIDMap.end()) {
									cout << "ERROR: Cannot find entry for op " << previousOpInTask.opID << " in opIDMap\n";
									cout << "ERROR: While examining op " << opCount << "\n";
									return -1;
								} else {
									UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInTask.opID];
#ifdef SANITYCHECK
									// Sanity check: prev-op has same thread as current op in stack and opIDMap
									assert(previousOpInTask.threadID == threadID);
									assert(existingEntry.threadID == threadID);
#endif
									opdetails.taskID = previousOpInTask.taskID;
									existingEntry.nextOpInTask = opCount;
									detector.opIDMap.erase(detector.opIDMap.find(previousOpInTask.opID));
									detector.opIDMap[previousOpInTask.opID] = existingEntry;
								}
							}




							if (!firstOpInsideNestingLoop) {
								if (firstOpInsideGlobalLoop) {
								} else {
									stackElement.taskID = previousOpInThread.taskID;
								}
							} else {
								stackElement.taskID = previousOpInTask.taskID;
							}

							stackForThreadOrder.push(stackElement);
							if (!stackForTaskOrder.isEmpty(threadID))
								stackForTaskOrder.push(stackElement);
							else {
								if (stackForNestingOrder.isEmpty(threadID) && !stackForGlobalLoop.isEmpty(threadID)) {
									stackForGlobalLoop.push(stackElement);
								}
							}

							for (map<IDType, UAFDetector::memoryOpDetails>::iterator it = detector.allocSet.begin(); it != detector.allocSet.end(); it++) {
								std::stringstream str2;
								str2 << it->second.startingAddress;
								IDType baseAddressIntAlloc, endAddressIntAlloc;
								str2 >> std::hex >> baseAddressIntAlloc;
								endAddressIntAlloc = baseAddressIntAlloc + it->second.range - 1;

								if (baseAddressIntAlloc <= addressIntRead && addressIntRead <= endAddressIntAlloc) {
									if (detector.allocIDMap.find(it->first) == detector.allocIDMap.end()) {
										UAFDetector::allocOpDetails allocdetails;
										allocdetails.nodes.insert(opdetails.nodeID);
										detector.allocIDMap[it->first] = allocdetails;
									} else {
										UAFDetector::allocOpDetails existingEntry = detector.allocIDMap[it->first];
										existingEntry.nodes.insert(opdetails.nodeID);
										detector.allocIDMap.erase(detector.allocIDMap.find(it->first));
										detector.allocIDMap[it->first] = existingEntry;
									}
								}
							}

							for (map<IDType, UAFDetector::memoryOpDetails>::iterator it = detector.freeSet.begin(); it != detector.freeSet.end(); it++) {
								std::stringstream str2;
								str2 << it->second.startingAddress;
								IDType baseAddressIntFree, endAddressIntFree;
								str2 >> std::hex >> baseAddressIntFree;
								endAddressIntFree = baseAddressIntFree + it->second.range - 1;

								if (baseAddressIntFree <= addressIntRead && addressIntRead <= endAddressIntFree) {
									if (detector.freeIDMap.find(it->first) == detector.freeIDMap.end()) {
										UAFDetector::freeOpDetails freedetails;
										freedetails.nodes.insert(opdetails.nodeID);
										detector.freeIDMap[it->first] = freedetails;
									} else {
										UAFDetector::freeOpDetails existingEntry = detector.freeIDMap[it->first];
										existingEntry.nodes.insert(opdetails.nodeID);
										detector.freeIDMap.erase(detector.freeIDMap.find(it->first));
										detector.freeIDMap[it->first] = existingEntry;
									}
								}
							}
						}

					} else if (match.compare("write") == 0) {
						// Obtain 2nd argument (i.e., address) of write
						string address;
						for (unsigned j=threadPos+1; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							// find the next non-empty matches[i] after threadID of operation.
							if (!m1.empty() && m1.compare(" ") != 0) {
								address = m1;
								break;
							}
						}

						std::stringstream str1;
						str1 << address;
						IDType addressIntWrite;
						str1 >> std::hex >> addressIntWrite;

						for (map<IDType, UAFDetector::memoryOpDetails>::iterator it = detector.allocSet.begin(); it != detector.allocSet.end(); it++) {
							std::stringstream str2;
							str2 << it->second.startingAddress;
							IDType baseAddressIntAlloc, endAddressIntAlloc;
							str2 >> std::hex >> baseAddressIntAlloc;
							endAddressIntAlloc = baseAddressIntAlloc + it->second.range - 1;

							if (baseAddressIntAlloc <= addressIntWrite && addressIntWrite <= endAddressIntAlloc) {
								if (detector.allocIDMap.find(it->first) == detector.allocIDMap.end()) {
									UAFDetector::allocOpDetails allocdetails;
									allocdetails.writeOps.insert(opCount);
									detector.allocIDMap[it->first] = allocdetails;
								} else {
									UAFDetector::allocOpDetails existingEntry = detector.allocIDMap[it->first];
									existingEntry.writeOps.insert(opCount);
									detector.allocIDMap.erase(detector.allocIDMap.find(it->first));
									detector.allocIDMap[it->first] = existingEntry;
								}
							}
						}

						for (map<IDType, UAFDetector::memoryOpDetails>::iterator it = detector.freeSet.begin(); it != detector.freeSet.end(); it++) {
							std::stringstream str2;
							str2 << it->second.startingAddress;
							IDType baseAddressIntFree, endAddressIntFree;
							str2 >> std::hex >> baseAddressIntFree;
							endAddressIntFree = baseAddressIntFree + it->second.range - 1;

							if (baseAddressIntFree <= addressIntWrite && addressIntWrite <= endAddressIntFree) {
								if (detector.freeIDMap.find(it->first) == detector.freeIDMap.end()) {
									UAFDetector::freeOpDetails freedetails;
									freedetails.writeOps.insert(opCount);
									detector.freeIDMap[it->first] = freedetails;
								} else {
									UAFDetector::freeOpDetails existingEntry = detector.freeIDMap[it->first];
									existingEntry.writeOps.insert(opCount);
									detector.freeIDMap.erase(detector.freeIDMap.find(it->first));
									detector.freeIDMap[it->first] = existingEntry;
								}
							}
						}

						UAFDetector::memoryOpDetails memdetails;
						memdetails.startingAddress = address;
						if (detector.writeSet.find(opCount) == detector.writeSet.end()) {
							detector.writeSet[opCount] = memdetails;
						} else {
							cout << "ERROR: Found duplicate entry for op " << opCount << " in opIDMap\n";
							return -1;
						}

						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadOrder.isEmpty(threadID)){

							nodeCount++;
							opdetails.nodeID = nodeCount;
							stackElement.nodeID = nodeCount;

							for (map<IDType, UAFDetector::memoryOpDetails>::iterator it = detector.allocSet.begin(); it != detector.allocSet.end(); it++) {
								std::stringstream str2;
								str2 << it->second.startingAddress;
								IDType baseAddressIntAlloc, endAddressIntAlloc;
								str2 >> std::hex >> baseAddressIntAlloc;
								endAddressIntAlloc = baseAddressIntAlloc + it->second.range - 1;

								if (baseAddressIntAlloc <= addressIntWrite && addressIntWrite <= endAddressIntAlloc) {
									if (detector.allocIDMap.find(it->first) == detector.allocIDMap.end()) {
										UAFDetector::allocOpDetails allocdetails;
										allocdetails.nodes.insert(nodeCount);
										detector.allocIDMap[it->first] = allocdetails;
									} else {
										UAFDetector::allocOpDetails existingEntry = detector.allocIDMap[it->first];
										existingEntry.nodes.insert(nodeCount);
										detector.allocIDMap.erase(detector.allocIDMap.find(it->first));
										detector.allocIDMap[it->first] = existingEntry;
									}
								}
							}

							for (map<IDType, UAFDetector::memoryOpDetails>::iterator it = detector.freeSet.begin(); it != detector.freeSet.end(); it++) {
								std::stringstream str2;
								str2 << it->second.startingAddress;
								IDType baseAddressIntFree, endAddressIntFree;
								str2 >> std::hex >> baseAddressIntFree;
								endAddressIntFree = baseAddressIntFree + it->second.range - 1;

								if (baseAddressIntFree <= addressIntWrite && addressIntWrite <= endAddressIntFree) {
									if (detector.freeIDMap.find(it->first) == detector.freeIDMap.end()) {
										UAFDetector::freeOpDetails freedetails;
										freedetails.nodes.insert(nodeCount);
										detector.freeIDMap[it->first] = freedetails;
									} else {
										UAFDetector::freeOpDetails existingEntry = detector.freeIDMap[it->first];
										existingEntry.nodes.insert(nodeCount);
										detector.freeIDMap.erase(detector.freeIDMap.find(it->first));
										detector.freeIDMap[it->first] = existingEntry;
									}
								}
							}

							if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
								UAFDetector::setOfOps set;
								set.opSet.insert(opCount);
								detector.nodeIDMap[nodeCount] = set;
							} else {
								cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
								cout << "ERROR: Existing entry:\n";
								detector.nodeIDMap[nodeCount].printDetails();
								return -1;
							}

							cout << "WARNING: No previous op found for write " << opCount
								 << " on stackForThreadOrder\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
//							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread

							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							stackForThreadOrder.push(stackElement);
//							stackForTaskOrder.push(stackElement);
							stackForGlobalLoop.push(stackElement);
						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadOrder.peek(threadID);
							// In the case of write, the previous op in thread is not necessarily the previous op in task.
							MultiStack::stackElementType previousOpInTask = stackForTaskOrder.peek(threadID);

							// If the previous op in task stack is the same as the previous op in thread stack, then we are not
							// in the middle of a nesting loop.
							bool firstOpInsideNestingLoop;
							bool firstOpInsideGlobalLoop = false;
							if (previousOpInThread.opID == -1) {
								cout << "ERROR: ThreadOrder stack is not empty, but a peek operation returns invalid element\n";
								return -1;
							}
							if (!stackForTaskOrder.isEmpty(threadID)) {
								if (previousOpInTask.opID != previousOpInThread.opID)
									firstOpInsideNestingLoop = true;
								else {
									// If the previous op in task is a pause, then we are in a new block
#ifdef PERMIT
									if (detector.opIDMap[previousOpInTask.opID].opType.compare("permit") == 0)
#else
									if (detector.opIDMap[previousOpInTask.opID].opType.compare("pause") == 0)
#endif
										firstOpInsideNestingLoop = true;
									else
										firstOpInsideNestingLoop = false;
								}
							} else {
								// Task order stack is empty, this means we are not inside a nesting loop
								firstOpInsideNestingLoop = false;

								if (stackForNestingOrder.isEmpty(threadID) && !stackForGlobalLoop.isEmpty(threadID)) {
									// We are inside the global loop.
									MultiStack::stackElementType topOfGlobalLoopStack = stackForGlobalLoop.peek(threadID);
//									if (topOfGlobalLoopStack.opType.compare("enterloop") == 0)
									if (topOfGlobalLoopStack.opType.compare("enterloop") == 0
											|| topOfGlobalLoopStack.opType.compare("exitloop") == 0) {
										if (previousOpInThread.opType.compare("exitloop") == 0)
											firstOpInsideGlobalLoop = false;
										else
											firstOpInsideGlobalLoop = true;
									}
								} else if (!stackForNestingOrder.isEmpty(threadID)) {
									cout << "ERROR: TaskOrder stack is empty but NestingOrder stack is not: for thread " << threadID << "\n";
									cout << "ERROR: Top element in NestingOrder stack: " << stackForNestingOrder.peek(threadID).opID
										 << "\n";
									return -1;
								}
							}

							// previous-op-in-thread need not be the previous-op-in-task or the previous-op-in-block
							if (!firstOpInsideNestingLoop) {
								if (firstOpInsideGlobalLoop) {

									nodeCount++;
									opdetails.nodeID = nodeCount;
									stackElement.nodeID = nodeCount;

									if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
										UAFDetector::setOfOps set;
										set.opSet.insert(opCount);
										detector.nodeIDMap[nodeCount] = set;
									} else {
										cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
										cout << "ERROR: Existing entry:\n";
										detector.nodeIDMap[nodeCount].printDetails();
										return -1;
									}
								} else {
									opdetails.taskID = previousOpInThread.taskID;
									if (previousOpInThread.opType.compare("alloc") == 0 ||
											previousOpInThread.opType.compare("free") == 0 ||
											previousOpInThread.opType.compare("read") == 0 ||
											previousOpInThread.opType.compare("write") == 0) {
										opdetails.nodeID = previousOpInThread.nodeID;

										stackElement.nodeID = previousOpInThread.nodeID;

										if (detector.nodeIDMap.find(previousOpInThread.nodeID) == detector.nodeIDMap.end()) {
											cout << "ERROR: Cannot find entry for node " << previousOpInThread.nodeID << " in nodeIDMap\n";
											return -1;
										} else {
											UAFDetector::setOfOps existingEntry = detector.nodeIDMap[previousOpInThread.nodeID];
											existingEntry.opSet.insert(opCount);
											detector.nodeIDMap.erase(detector.nodeIDMap.find(previousOpInThread.nodeID));
											detector.nodeIDMap[previousOpInThread.nodeID] = existingEntry;
										}
									} else {
										nodeCount++;
										opdetails.nodeID = nodeCount;
										stackElement.nodeID = nodeCount;

										if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
											UAFDetector::setOfOps set;
											set.opSet.insert(opCount);
											detector.nodeIDMap[nodeCount] = set;
										} else {
											cout << "ERROR: Found duplicate entry for node " << nodeCount << " in nodeIDMap\n";
											return -1;
										}
									}
								}
							} else {
								// If this is the first op inside the nesting loop, then we are in a new block
								opdetails.taskID = previousOpInTask.taskID;

								nodeCount++;
								opdetails.nodeID = nodeCount;
								stackElement.nodeID = nodeCount;

								if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
									UAFDetector::setOfOps set;
									set.opSet.insert(opCount);
									detector.nodeIDMap[nodeCount] = set;
								} else {
									cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
									cout << "ERROR: Existing entry:\n";
									detector.nodeIDMap[nodeCount].printDetails();
									return -1;
								}
							}

							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								cout << "While examining op " << opCount;
								cout << endl;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								if (!firstOpInsideNestingLoop) {
									if (previousOpInThread.taskID.compare("") != 0)
										existingEntry.nextOpInTask = opCount;
								}
								existingEntry.nextOpInThread = opCount;
								opdetails.prevOpInThread = previousOpInThread.opID;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							if (firstOpInsideNestingLoop) {
								if (detector.opIDMap.find(previousOpInTask.opID) == detector.opIDMap.end()) {
									cout << "ERROR: Cannot find entry for op " << previousOpInTask.opID << " in opIDMap\n";
									cout << "ERROR: While examining op " << opCount << "\n";
									return -1;
								} else {
									UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInTask.opID];
#ifdef SANITYCHECK
									// Sanity check: prev-op has same thread as current op in stack and opIDMap
									assert(previousOpInTask.threadID == threadID);
									assert(existingEntry.threadID == threadID);
#endif
									opdetails.taskID = previousOpInTask.taskID;
									existingEntry.nextOpInTask = opCount;
									detector.opIDMap.erase(detector.opIDMap.find(previousOpInTask.opID));
									detector.opIDMap[previousOpInTask.opID] = existingEntry;
								}
							}


							if (!firstOpInsideNestingLoop) {
								if (firstOpInsideGlobalLoop) {
								} else {
									stackElement.taskID = previousOpInThread.taskID;
								}
							} else {
								stackElement.taskID = previousOpInTask.taskID;
							}

							stackForThreadOrder.push(stackElement);
							if (!stackForTaskOrder.isEmpty(threadID))
								stackForTaskOrder.push(stackElement);
							else {
								if (stackForNestingOrder.isEmpty(threadID) && !stackForGlobalLoop.isEmpty(threadID)) {
									stackForGlobalLoop.push(stackElement);
								}
							}
							for (map<IDType, UAFDetector::memoryOpDetails>::iterator it = detector.allocSet.begin(); it != detector.allocSet.end(); it++) {
								std::stringstream str2;
								str2 << it->second.startingAddress;
								IDType baseAddressIntAlloc, endAddressIntAlloc;
								str2 >> std::hex >> baseAddressIntAlloc;
								endAddressIntAlloc = baseAddressIntAlloc + it->second.range - 1;

								if (baseAddressIntAlloc <= addressIntWrite && addressIntWrite <= endAddressIntAlloc) {
									if (detector.allocIDMap.find(it->first) == detector.allocIDMap.end()) {
										UAFDetector::allocOpDetails allocdetails;
										allocdetails.nodes.insert(opdetails.nodeID);
										detector.allocIDMap[it->first] = allocdetails;
									} else {
										UAFDetector::allocOpDetails existingEntry = detector.allocIDMap[it->first];
										existingEntry.nodes.insert(opdetails.nodeID);
										detector.allocIDMap.erase(detector.allocIDMap.find(it->first));
										detector.allocIDMap[it->first] = existingEntry;
									}
								}
							}

							for (map<IDType, UAFDetector::memoryOpDetails>::iterator it = detector.freeSet.begin(); it != detector.freeSet.end(); it++) {
								std::stringstream str2;
								str2 << it->second.startingAddress;
								IDType baseAddressIntFree, endAddressIntFree;
								str2 >> std::hex >> baseAddressIntFree;
								endAddressIntFree = baseAddressIntFree + it->second.range - 1;

								if (baseAddressIntFree <= addressIntWrite && addressIntWrite <= endAddressIntFree) {
									if (detector.freeIDMap.find(it->first) == detector.freeIDMap.end()) {
										UAFDetector::freeOpDetails freedetails;
										freedetails.nodes.insert(opdetails.nodeID);
										detector.freeIDMap[it->first] = freedetails;
									} else {
										UAFDetector::freeOpDetails existingEntry = detector.freeIDMap[it->first];
										existingEntry.nodes.insert(opdetails.nodeID);
										detector.freeIDMap.erase(detector.freeIDMap.find(it->first));
										detector.freeIDMap[it->first] = existingEntry;
									}
								}
							}
						}

					}
#ifdef LOCKS
					else if (match.compare("wait") == 0) {

						nodeCount++;
						opdetails.nodeID = nodeCount;
						stackElement.nodeID = nodeCount;

						if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
							UAFDetector::setOfOps set;
							set.opSet.insert(opCount);
							detector.nodeIDMap[nodeCount] = set;
						} else {
							cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
							cout << "ERROR: Existing entry:\n";
							detector.nodeIDMap[nodeCount].printDetails();
							return -1;
						}

						// Obtain two arguments of wait
						IDType threadID;
						std::string lockID;
						unsigned j = typePos + 1;
						for (; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							if (!m1.empty() && m1.compare(" ") != 0){
								threadID = atoi(m1.c_str());
								break;
							}
						}
						for (j=j+1; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							if (!m1.empty() && m1.compare(" ") != 0){
								lockID = m1;
								break;
							}
						}

						if (detector.waitSet.find(opCount) == detector.waitSet.end()) {
							UAFDetector::lockOpDetails waitopdetails;
							waitopdetails.threadID = threadID;
							waitopdetails.lockID = lockID;
							detector.waitSet[opCount] = waitopdetails;
						} else {
							cout << "ERROR: Found duplicate entry for op " << opCount << "\n";
							cout << "ERROR: Existing entry:\n";
							detector.waitSet[opCount].printDetails();
							return -1;
						}

						bool notifyFlag = false, notifyAllFlag = false;
						IDType notifyOp, notifyAllOp;
						if (detector.lockToNotify.find(lockID) != detector.lockToNotify.end()) {
							UAFDetector::setOfOps notifySet = detector.lockToNotify[lockID];

							if (!notifySet.opSet.empty()) {
								notifyOp = *(notifySet.opSet.rbegin());
								if (detector.waitToNotify.find(opCount) == detector.waitToNotify.end()) {
									detector.waitToNotify[opCount] = notifyOp;
									notifyFlag = true;
								} else {
									if (detector.waitToNotify[opCount] < notifyOp) {
										detector.waitToNotify.erase(detector.waitToNotify.find(opCount));
										detector.waitToNotify[opCount] = notifyOp;
										notifyFlag = true;
									}
								}
							}
						}

						if (detector.lockToNotifyAll.find(lockID) != detector.lockToNotifyAll.end()) {
							UAFDetector::setOfOps notifyAllSet = detector.lockToNotifyAll[lockID];

							if (!notifyAllSet.opSet.empty()) {
								notifyAllOp = *(notifyAllSet.opSet.rbegin());
								if (detector.waitToNotify.find(opCount) == detector.waitToNotify.end()) {
									detector.waitToNotify[opCount] = notifyAllOp;
									notifyAllFlag = true;
								} else {
									if (detector.waitToNotify[opCount] < notifyAllOp) {
										detector.waitToNotify.erase(detector.waitToNotify.find(opCount));
										detector.waitToNotify[opCount] = notifyAllOp;
										notifyAllFlag = true;
									}
								}
							}
						}

						if (notifyFlag && !notifyAllFlag) {
							if (detector.notifyToWait.find(notifyOp) == detector.notifyToWait.end()) {
								detector.notifyToWait[notifyOp] = opCount;
							} else {
								cout << "WARNING: Wait op already set for notify op " << notifyOp << "\n";
								cout << "WARNING: Existing entry: " << detector.notifyToWait[notifyOp] << "\n";
								cout << "WARNING: Skipping this op\n";
								continue;
//								return -1;
							}
						} else if (!notifyFlag && notifyAllFlag) {
							if (detector.notifyAllToWaitSet.find(notifyAllOp) == detector.notifyAllToWaitSet.end()) {
								UAFDetector::setOfOps waitset;
								waitset.opSet.insert(opCount);
								detector.notifyAllToWaitSet[notifyAllOp] = waitset;
							} else {
								UAFDetector::setOfOps existingEntry = detector.notifyAllToWaitSet[notifyAllOp];
								existingEntry.opSet.insert(opCount);
								detector.notifyAllToWaitSet.erase(detector.notifyAllToWaitSet.find(notifyAllOp));
								detector.notifyAllToWaitSet[notifyAllOp] = existingEntry;
							}
						}

						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadOrder.isEmpty(threadID)) {
							cout << "WARNING: No previous op found for wait " << opCount
								 << " on stackForThreadOrder\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread

							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							stackForThreadOrder.push(stackElement);
							stackForGlobalLoop.push(stackElement);
						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadOrder.peek(threadID);
							// In the case of wait, the previous op in thread is not necessarily the previous op in task.
							MultiStack::stackElementType previousOpInTask = stackForTaskOrder.peek(threadID);

							// If the previous op in task stack is the same as the previous op in thread stack, then we are not
							// in the middle of a nesting loop.
							bool firstOpInsideNestingLoop;
							bool firstOpInsideGlobalLoop = false;
							if (previousOpInThread.opID == -1) {
								cout << "ERROR: ThreadOrder stack is not empty, but a peek operation returns invalid element\n";
								return -1;
							}
							if (!stackForTaskOrder.isEmpty(threadID)) {
								if (previousOpInTask.opID != previousOpInThread.opID)
									firstOpInsideNestingLoop = true;
								else {
									// If the previous op in task is a pause, then we are in a new block
#ifdef PERMIT
									if (detector.opIDMap[previousOpInTask.opID].opType.compare("permit") == 0)
#else
									if (detector.opIDMap[previousOpInTask.opID].opType.compare("pause") == 0)
#endif
										firstOpInsideNestingLoop = true;
									else
										firstOpInsideNestingLoop = false;
								}
							} else {
								// Task order stack is empty, this means we are not inside a nesting loop
								firstOpInsideNestingLoop = false;

								if (stackForNestingOrder.isEmpty(threadID) && !stackForGlobalLoop.isEmpty(threadID)) {
									// We are inside the global loop.
									MultiStack::stackElementType topOfGlobalLoopStack = stackForGlobalLoop.peek(threadID);
//									if (topOfGlobalLoopStack.opType.compare("enterloop") == 0)
									if (topOfGlobalLoopStack.opType.compare("enterloop") == 0
											|| topOfGlobalLoopStack.opType.compare("exitloop") == 0) {
										if (previousOpInThread.opType.compare("exitloop") == 0)
											firstOpInsideGlobalLoop = false;
										else
											firstOpInsideGlobalLoop = true;
									}
								} else if (!stackForNestingOrder.isEmpty(threadID)) {
									cout << "ERROR: TaskOrder stack is empty but NestingOrder stack is not: for thread " << threadID << "\n";
									cout << "ERROR: Top element in NestingOrder stack: " << stackForNestingOrder.peek(threadID).opID
										 << "\n";
									return -1;
								}
							}

							// previous-op-in-thread need not be the previous-op-in-task or the previous-op-in-block
							if (!firstOpInsideNestingLoop) {
								if (firstOpInsideGlobalLoop) {
								} else {
									opdetails.taskID = previousOpInThread.taskID;
								}
							} else {
								// If this is the first op inside the nesting loop, then we are in a new block
								opdetails.taskID = previousOpInTask.taskID;
							}

							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								cout << "While examining op " << opCount;
								cout << endl;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								if (!firstOpInsideNestingLoop) {
									if (previousOpInThread.taskID.compare("") != 0)
										existingEntry.nextOpInTask = opCount;
								}
								existingEntry.nextOpInThread = opCount;
								opdetails.prevOpInThread = previousOpInThread.opID;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							if (firstOpInsideNestingLoop) {
								if (detector.opIDMap.find(previousOpInTask.opID) == detector.opIDMap.end()) {
									cout << "ERROR: Cannot find entry for op " << previousOpInTask.opID << " in opIDMap\n";
									cout << "ERROR: While examining op " << opCount << "\n";
									return -1;
								} else {
									UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInTask.opID];
#ifdef SANITYCHECK
									// Sanity check: prev-op has same thread as current op in stack and opIDMap
									assert(previousOpInTask.threadID == threadID);
									assert(existingEntry.threadID == threadID);
#endif
									opdetails.taskID = previousOpInTask.taskID;
									existingEntry.nextOpInTask = opCount;
									detector.opIDMap.erase(detector.opIDMap.find(previousOpInTask.opID));
									detector.opIDMap[previousOpInTask.opID] = existingEntry;
								}
							}


							if (!firstOpInsideNestingLoop) {
								if (firstOpInsideGlobalLoop) {
								} else {
									stackElement.taskID = previousOpInThread.taskID;
								}
							} else {
								stackElement.taskID = previousOpInTask.taskID;
							}

							stackForThreadOrder.push(stackElement);
							if (!stackForTaskOrder.isEmpty(threadID))
								stackForTaskOrder.push(stackElement);
							else {
								if (stackForNestingOrder.isEmpty(threadID) && !stackForGlobalLoop.isEmpty(threadID)) {
									stackForGlobalLoop.push(stackElement);
								}
							}
						}
					} else if (match.compare("notify") == 0) {

						nodeCount++;
						opdetails.nodeID = nodeCount;
						stackElement.nodeID = nodeCount;

						if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
							UAFDetector::setOfOps set;
							set.opSet.insert(opCount);
							detector.nodeIDMap[nodeCount] = set;
						} else {
							cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
							cout << "ERROR: Existing entry:\n";
							detector.nodeIDMap[nodeCount].printDetails();
							return -1;
						}

						// Obtain two arguments of notify
						IDType threadID;
						std::string lockID;
						unsigned j = typePos + 1;
						for (; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							if (!m1.empty() && m1.compare(" ") != 0){
								threadID = atoi(m1.c_str());
								break;
							}
						}
						for (j=j+1; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							if (!m1.empty() && m1.compare(" ") != 0){
								lockID = m1;
								break;
							}
						}

						if (detector.notifySet.find(opCount) == detector.notifySet.end()) {
							UAFDetector::lockOpDetails notifyopdetails;
							notifyopdetails.threadID = threadID;
							notifyopdetails.lockID = lockID;
							detector.notifySet[opCount] = notifyopdetails;
						} else {
							cout << "ERROR: Found duplicate entry for op " << opCount << "\n";
							cout << "ERROR: Existing entry:\n";
							detector.notifySet[opCount].printDetails();
							return -1;
						}

						if (detector.lockToNotify.find(lockID) != detector.lockToNotify.end()) {
							UAFDetector::setOfOps notifyOps = detector.lockToNotify[lockID];
							notifyOps.opSet.insert(opCount);
							detector.lockToNotify.erase(detector.lockToNotify.find(lockID));
							detector.lockToNotify[lockID] = notifyOps;
						} else {
							UAFDetector::setOfOps notifyOps;
							notifyOps.opSet.insert(opCount);
							detector.lockToNotify[lockID] = notifyOps;
						}

						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadOrder.isEmpty(threadID)) {
							cout << "WARNING: No previous op found for wait " << opCount
								 << " on stackForThreadOrder\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread

							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}


							stackForThreadOrder.push(stackElement);
							stackForGlobalLoop.push(stackElement);
						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadOrder.peek(threadID);
							// In the case of wait, the previous op in thread is not necessarily the previous op in task.
							MultiStack::stackElementType previousOpInTask = stackForTaskOrder.peek(threadID);

							// If the previous op in task stack is the same as the previous op in thread stack, then we are not
							// in the middle of a nesting loop.
							bool firstOpInsideNestingLoop;
							bool firstOpInsideGlobalLoop = false;
							if (previousOpInThread.opID == -1) {
								cout << "ERROR: ThreadOrder stack is not empty, but a peek operation returns invalid element\n";
								return -1;
							}
							if (!stackForTaskOrder.isEmpty(threadID)) {
								if (previousOpInTask.opID != previousOpInThread.opID)
									firstOpInsideNestingLoop = true;
								else {
									// If the previous op in task is a pause, then we are in a new block
#ifdef PERMIT
									if (detector.opIDMap[previousOpInTask.opID].opType.compare("permit") == 0)
#else
									if (detector.opIDMap[previousOpInTask.opID].opType.compare("pause") == 0)
#endif
										firstOpInsideNestingLoop = true;
									else
										firstOpInsideNestingLoop = false;
								}
							} else {
								// Task order stack is empty, this means we are not inside a nesting loop
								firstOpInsideNestingLoop = false;

								if (stackForNestingOrder.isEmpty(threadID) && !stackForGlobalLoop.isEmpty(threadID)) {
									// We are inside the global loop.
									MultiStack::stackElementType topOfGlobalLoopStack = stackForGlobalLoop.peek(threadID);
//									if (topOfGlobalLoopStack.opType.compare("enterloop") == 0)
									if (topOfGlobalLoopStack.opType.compare("enterloop") == 0
											|| topOfGlobalLoopStack.opType.compare("exitloop") == 0) {
										if (previousOpInThread.opType.compare("exitloop") == 0)
											firstOpInsideGlobalLoop = false;
										else
											firstOpInsideGlobalLoop = true;
									}
								} else if (!stackForNestingOrder.isEmpty(threadID)) {
									cout << "ERROR: TaskOrder stack is empty but NestingOrder stack is not: for thread " << threadID << "\n";
									cout << "ERROR: Top element in NestingOrder stack: " << stackForNestingOrder.peek(threadID).opID
										 << "\n";
									return -1;
								}
							}

							// previous-op-in-thread need not be the previous-op-in-task or the previous-op-in-block
							if (!firstOpInsideNestingLoop) {
								if (firstOpInsideGlobalLoop) {
								} else {
									opdetails.taskID = previousOpInThread.taskID;
								}
							} else {
								// If this is the first op inside the nesting loop, then we are in a new block
								opdetails.taskID = previousOpInTask.taskID;
							}

							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								cout << "While examining op " << opCount;
								cout << endl;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								if (!firstOpInsideNestingLoop) {
									if (previousOpInThread.taskID.compare("") != 0)
										existingEntry.nextOpInTask = opCount;
								}
								existingEntry.nextOpInThread = opCount;
								opdetails.prevOpInThread = previousOpInThread.opID;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							if (firstOpInsideNestingLoop) {
								if (detector.opIDMap.find(previousOpInTask.opID) == detector.opIDMap.end()) {
									cout << "ERROR: Cannot find entry for op " << previousOpInTask.opID << " in opIDMap\n";
									cout << "ERROR: While examining op " << opCount << "\n";
									return -1;
								} else {
									UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInTask.opID];
#ifdef SANITYCHECK
									// Sanity check: prev-op has same thread as current op in stack and opIDMap
									assert(previousOpInTask.threadID == threadID);
									assert(existingEntry.threadID == threadID);
#endif
									opdetails.taskID = previousOpInTask.taskID;
									existingEntry.nextOpInTask = opCount;
									detector.opIDMap.erase(detector.opIDMap.find(previousOpInTask.opID));
									detector.opIDMap[previousOpInTask.opID] = existingEntry;
								}
							}

							if (!firstOpInsideNestingLoop) {
								if (firstOpInsideGlobalLoop) {
								} else {
									stackElement.taskID = previousOpInThread.taskID;
								}
							} else {
								stackElement.taskID = previousOpInTask.taskID;
							}

							stackForThreadOrder.push(stackElement);
							if (!stackForTaskOrder.isEmpty(threadID))
								stackForTaskOrder.push(stackElement);
							else {
								if (stackForNestingOrder.isEmpty(threadID) && !stackForGlobalLoop.isEmpty(threadID)) {
									stackForGlobalLoop.push(stackElement);
								}
							}
						}
					} else if (match.compare("notifyall") == 0) {

						nodeCount++;
						opdetails.nodeID = nodeCount;
						stackElement.nodeID = nodeCount;

						if (detector.nodeIDMap.find(nodeCount) == detector.nodeIDMap.end()) {
							UAFDetector::setOfOps set;
							set.opSet.insert(opCount);
							detector.nodeIDMap[nodeCount] = set;
						} else {
							cout << "ERROR: Found duplicate entry for node " << nodeCount << "\n";
							cout << "ERROR: Existing entry:\n";
							detector.nodeIDMap[nodeCount].printDetails();
							return -1;
						}

						// Obtain two arguments of notifyall
						IDType threadID;
						std::string lockID;
						unsigned j = typePos + 1;
						for (; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							if (!m1.empty() && m1.compare(" ") != 0){
								threadID = atoi(m1.c_str());
								break;
							}
						}
						for (j=j+1; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							if (!m1.empty() && m1.compare(" ") != 0){
								lockID = m1;
								break;
							}
						}

						if (detector.notifyAllSet.find(opCount) == detector.notifyAllSet.end()) {
							UAFDetector::lockOpDetails notifyallopdetails;
							notifyallopdetails.threadID = threadID;
							notifyallopdetails.lockID = lockID;
							detector.notifySet[opCount] = notifyallopdetails;
						} else {
							cout << "ERROR: Found duplicate entry for op " << opCount << "\n";
							cout << "ERROR: Existing entry:\n";
							detector.notifyAllSet[opCount].printDetails();
							return -1;
						}

						if (detector.lockToNotifyAll.find(lockID) != detector.lockToNotifyAll.end()) {
							UAFDetector::setOfOps notifyAllOps = detector.lockToNotifyAll[lockID];
							notifyAllOps.opSet.insert(opCount);
							detector.lockToNotifyAll.erase(detector.lockToNotifyAll.find(lockID));
							detector.lockToNotifyAll[lockID] = notifyAllOps;
						} else {
							UAFDetector::setOfOps notifyAllOps;
							notifyAllOps.opSet.insert(opCount);
							detector.lockToNotifyAll[lockID] = notifyAllOps;
						}

						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadOrder.isEmpty(threadID)) {
							cout << "WARNING: No previous op found for wait " << opCount
								 << " on stackForThreadOrder\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread

							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							stackForThreadOrder.push(stackElement);
							stackForGlobalLoop.push(stackElement);
						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadOrder.peek(threadID);
							// In the case of wait, the previous op in thread is not necessarily the previous op in task.
							MultiStack::stackElementType previousOpInTask = stackForTaskOrder.peek(threadID);

							// If the previous op in task stack is the same as the previous op in thread stack, then we are not
							// in the middle of a nesting loop.
							bool firstOpInsideNestingLoop;
							bool firstOpInsideGlobalLoop = false;
							if (previousOpInThread.opID == -1) {
								cout << "ERROR: ThreadOrder stack is not empty, but a peek operation returns invalid element\n";
								return -1;
							}
							if (!stackForTaskOrder.isEmpty(threadID)) {
								if (previousOpInTask.opID != previousOpInThread.opID)
									firstOpInsideNestingLoop = true;
								else {
									// If the previous op in task is a pause, then we are in a new block
#ifdef PERMIT
									if (detector.opIDMap[previousOpInTask.opID].opType.compare("permit") == 0)
#else
									if (detector.opIDMap[previousOpInTask.opID].opType.compare("pause") == 0)
#endif
										firstOpInsideNestingLoop = true;
									else
										firstOpInsideNestingLoop = false;
								}
							} else {
								// Task order stack is empty, this means we are not inside a nesting loop
								firstOpInsideNestingLoop = false;

								if (stackForNestingOrder.isEmpty(threadID) && !stackForGlobalLoop.isEmpty(threadID)) {
									// We are inside the global loop.
									MultiStack::stackElementType topOfGlobalLoopStack = stackForGlobalLoop.peek(threadID);
//									if (topOfGlobalLoopStack.opType.compare("enterloop") == 0)
									if (topOfGlobalLoopStack.opType.compare("enterloop") == 0
											|| topOfGlobalLoopStack.opType.compare("exitloop") == 0) {
										if (previousOpInThread.opType.compare("exitloop") == 0)
											firstOpInsideGlobalLoop = false;
										else
											firstOpInsideGlobalLoop = true;
									}
								} else if (!stackForNestingOrder.isEmpty(threadID)) {
									cout << "ERROR: TaskOrder stack is empty but NestingOrder stack is not: for thread " << threadID << "\n";
									cout << "ERROR: Top element in NestingOrder stack: " << stackForNestingOrder.peek(threadID).opID
										 << "\n";
									return -1;
								}
							}

							// previous-op-in-thread need not be the previous-op-in-task or the previous-op-in-block
							if (!firstOpInsideNestingLoop) {
								if (firstOpInsideGlobalLoop) {
								} else {
									opdetails.taskID = previousOpInThread.taskID;
								}
							} else {
								// If this is the first op inside the nesting loop, then we are in a new block
								opdetails.taskID = previousOpInTask.taskID;
							}

							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								cout << "While examining op " << opCount;
								cout << endl;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								if (!firstOpInsideNestingLoop) {
									if (previousOpInThread.taskID.compare("") != 0)
										existingEntry.nextOpInTask = opCount;
								}
								existingEntry.nextOpInThread = opCount;
								opdetails.prevOpInThread = previousOpInThread.opID;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							if (firstOpInsideNestingLoop) {
								if (detector.opIDMap.find(previousOpInTask.opID) == detector.opIDMap.end()) {
									cout << "ERROR: Cannot find entry for op " << previousOpInTask.opID << " in opIDMap\n";
									cout << "ERROR: While examining op " << opCount << "\n";
									return -1;
								} else {
									UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInTask.opID];
#ifdef SANITYCHECK
									// Sanity check: prev-op has same thread as current op in stack and opIDMap
									assert(previousOpInTask.threadID == threadID);
									assert(existingEntry.threadID == threadID);
#endif
									opdetails.taskID = previousOpInTask.taskID;
									existingEntry.nextOpInTask = opCount;
									detector.opIDMap.erase(detector.opIDMap.find(previousOpInTask.opID));
									detector.opIDMap[previousOpInTask.opID] = existingEntry;
								}
							}




							if (!firstOpInsideNestingLoop) {
								if (firstOpInsideGlobalLoop) {
								} else {
									stackElement.taskID = previousOpInThread.taskID;
								}
							} else {
								stackElement.taskID = previousOpInTask.taskID;
							}

							stackForThreadOrder.push(stackElement);
							if (!stackForTaskOrder.isEmpty(threadID))
								stackForTaskOrder.push(stackElement);
							else {
								if (stackForNestingOrder.isEmpty(threadID) && !stackForGlobalLoop.isEmpty(threadID)) {
									stackForGlobalLoop.push(stackElement);
								}
							}
						}
					}
#endif

					if (detector.opIDMap.find(opCount) == detector.opIDMap.end()) {
						detector.opIDMap[opCount] = opdetails;
					} else {
						cout << "ERROR: Found duplicate entry for " << opCount << " in opIDMap\n";
						cout << "While examining " << OpString << "\n";
						cout << "Duplicate entry:\n";
						detector.opIDMap[opCount].printOpDetails();
						cout << endl;
						return -1;
					}
					break;
				}
			}
		}
	}

	for (map<IDType, UAFDetector::threadDetails>::iterator it = detector.threadIDMap.begin(); it != detector.threadIDMap.end(); it++) {
		if (!stackForThreadOrder.isEmpty(it->first)) {
			MultiStack::stackElementType lastOp = stackForThreadOrder.peek(it->first);
			it->second.lastOpInThreadID = lastOp.opID;

			if (lastOp.taskID.compare("") == 0) continue;

			if (detector.taskIDMap.find(lastOp.taskID) == detector.taskIDMap.end()) {
				cout << "ERROR: Cannot find entry for task " << lastOp.taskID << " in taskIDMap\n";
				return -1;
			} else {
				UAFDetector::taskDetails existingEntry = detector.taskIDMap[lastOp.taskID];
				if (!stackForTaskOrder.isEmpty(it->first)) {
					MultiStack::stackElementType lastConcurrencyOpInTask = stackForTaskOrder.peek(it->first);
#ifdef PERMIT
					if (lastConcurrencyOpInTask.opType.compare("revoke") == 0) {
#else
					if (lastConcurrencyOpInTask.opType.compare("resume") == 0) {
#endif
						if (existingEntry.lastResumeOpID == -1)
							existingEntry.lastResumeOpID = lastConcurrencyOpInTask.opID;
					}
				}
				detector.taskIDMap.erase(detector.taskIDMap.find(lastOp.taskID));
				detector.taskIDMap[lastOp.taskID] = existingEntry;
			}
		}
	}

	for (std::map<std::string, UAFDetector::taskDetails>::iterator taskIt = detector.taskIDMap.begin();
			taskIt != detector.taskIDMap.end(); taskIt++) {
		if (taskIt->second.endOpID != -1) continue;
		if (taskIt->second.lastResumeOpID != -1) continue;

		if (taskIt->second.deqOpID == -1) continue;
		IDType threadID = detector.opIDMap[taskIt->second.deqOpID].threadID;
		MultiStack::stackElementType lastOpInTask = stackForNestingOrder.pop(threadID, taskIt->first);
		if (!stackForNestingOrder.isBottom(lastOpInTask)) {
#ifdef PERMIT
			if (lastOpInTask.opType.compare("revoke") == 0)
#else
			if (lastOpInTask.opType.compare("resume") == 0)
#endif
				taskIt->second.lastResumeOpID = lastOpInTask.opID;
		}

		lastOpInTask = stackForTaskOrder.pop(threadID, taskIt->first);
		taskIt->second.lastOpInTaskID = lastOpInTask.opID;
	}

	cout << "Finished parsing the file\n";

#ifdef TRACEDEBUG
	cout << "No of ops: " << opCount << "\n";
	cout << "No of tasks: " << detector.taskIDMap.size() << "\n";

	long long numOfAtomicTasks = 0;
	long long numOfTasksWithNonNullParent = 0;
	long long numOfTasksWithCascadingLoop = 0;
	int maxRecursiveDepth = 0; std::string taskWithMaxRecursiveDepth = "";
	int maxCascadingDepth = 0; std::string taskWithMaxCascadingDepth = "";
	IDType numOfNestingLoops = 0;

	std::set<IDType> setOfThreadsWithQueues;
	std::set<IDType> setOfThreadsWithNestingLoops;
	map<IDType, int> threadToMaxRecursiveDepth;
	map<IDType, int> threadToMaxCascadingDepth;

	for (map<string, UAFDetector::taskDetails>::iterator it = detector.taskIDMap.begin(); it != detector.taskIDMap.end(); it++) {
#ifdef SANITYCHECK
		if (it->second.atomic == true && (it->second.firstPauseOpID != -1 || it->second.lastResumeOpID != -1
				|| it->second.pauseResumeResetSequence.size() != 0 )) {
			cout << "ERROR: Atomic flag of task " << it->first << " is true but...\n";
			it->second.printTaskDetails();
		}
#endif
		if (it->second.parentTask.compare("") != 0)
			numOfTasksWithNonNullParent++;
		if (it->second.atomic == true)
			numOfAtomicTasks++;
		else {

			numOfNestingLoops += it->second.pauseResumeResetSequence.size();

			IDType currThreadID = detector.opIDMap[it->second.firstPauseOpID].threadID;
			setOfThreadsWithNestingLoops.insert(currThreadID);

			int currCascadingDepth = it->second.pauseResumeResetSequence.size();
			if (currCascadingDepth > 1)
				numOfTasksWithCascadingLoop++;
			if (maxCascadingDepth < currCascadingDepth) {
				maxCascadingDepth = currCascadingDepth;
				taskWithMaxCascadingDepth = it->first;
			}

			if (threadToMaxCascadingDepth.find(currThreadID) != threadToMaxCascadingDepth.end()) {
				IDType threadCascadingDepth = threadToMaxCascadingDepth.find(currThreadID)->second;
				if (threadCascadingDepth < currCascadingDepth) {
					threadToMaxCascadingDepth.erase(threadToMaxCascadingDepth.find(currThreadID));
					threadToMaxCascadingDepth[currThreadID] = currCascadingDepth;
				}
			} else {
				threadToMaxCascadingDepth[currThreadID] = currCascadingDepth;
			}

			int recursiveDepth = 0;
			std::string currTask = it->first;
			std::string prevTask = "";
			while (currTask.compare("") != 0) {
				recursiveDepth++;
				prevTask = currTask;
				currTask = detector.taskIDMap[currTask].parentTask;
			}

			if (maxRecursiveDepth < recursiveDepth) {
				maxRecursiveDepth = recursiveDepth;
				taskWithMaxRecursiveDepth = prevTask;
			}

			if (threadToMaxRecursiveDepth.find(currThreadID) != threadToMaxRecursiveDepth.end()) {
				IDType threadRecursiveDepth = threadToMaxRecursiveDepth.find(currThreadID)->second;
				if (threadRecursiveDepth < recursiveDepth) {
					threadToMaxRecursiveDepth.erase(threadToMaxRecursiveDepth.find(currThreadID));
					threadToMaxRecursiveDepth[currThreadID] = recursiveDepth;
				}
			} else {
				threadToMaxRecursiveDepth[currThreadID] = recursiveDepth;
			}
		}
	}
	cout << "No of atomic tasks: " << numOfAtomicTasks << "\n";
	cout << "No of non-atomic tasks: " << detector.taskIDMap.size() - numOfAtomicTasks << "\n";
	cout << "No of tasks with non-null parent: " << numOfTasksWithNonNullParent << "\n";
	cout << "No of shared variables guarding nesting loops: " << detector.nestingLoopMap.size() << "\n";
	cout << "No of nesting loops: " << numOfNestingLoops << "\n";
	if (maxRecursiveDepth != 0)
		cout << "Max recursive depth: " << maxRecursiveDepth << " (task "
			 << taskWithMaxRecursiveDepth << ")\n";
	if (maxCascadingDepth != 0)
		cout << "Max cascading depth: " << maxCascadingDepth << " (task "
			 << taskWithMaxCascadingDepth << ")\n";
	cout << "No of tasks with cascading depth > 1: " << numOfTasksWithCascadingLoop << "\n";
	cout << "No of threads: " << detector.threadIDMap.size() << "\n";
	cout << "No of threads with queues: " << setOfThreadsWithQueues.size() << "\n";
	cout << "No of threads with nesting loops: " << setOfThreadsWithNestingLoops.size() << "\n";
	cout << "No of alloc ops: " << detector.allocSet.size() << "\n";
	cout << "No of free ops: " << detector.freeSet.size() << "\n";
	cout << "No of read ops: " << detector.readSet.size() << "\n";
	cout << "No of write ops: " << detector.writeSet.size() << "\n";
	cout << "No of read + write ops: " << detector.readSet.size() + detector.writeSet.size() << "\n";
	cout << "No of nodes: " << detector.nodeIDMap.size() << "\n";
	cout << "Node Limit: " << NODELIMIT << "\n";

	cout << "threadToMaxRecursiveDepth:\n";
	for (map<IDType, IDType>::iterator it = threadToMaxRecursiveDepth.begin(); it != threadToMaxRecursiveDepth.end(); it++) {
		cout << "Thread " << it->first << ": " << it->second << "\n";
	}

	cout << "threadToMaxCascadingDepth:\n";
	for (map<IDType, IDType>::iterator it = threadToMaxCascadingDepth.begin(); it != threadToMaxCascadingDepth.end(); it++) {
		cout << "Thread " << it->first << ": " << it->second << "\n";
	}

#ifndef RUNOVERNODELIMIT
	if (detector.nodeIDMap.size() > NODELIMIT)
		return -2;
#endif

	Logger opTaskLogger;
	opTaskLogger.init(traceName + ".tasks");

	cout << "\nOps: \n";
	for (map<IDType, UAFDetector::opDetails>::iterator it = detector.opIDMap.begin(); it != detector.opIDMap.end(); it++) {
		cout << "\nOp: " << it->first << " - details: ";
		it->second.printOpDetails();
		cout << endl;

		opTaskLogger.streamObject << it->first << ":" << it->second.taskID << "\n";
		opTaskLogger.writeLog();
	}

	cout << "\nTasks:\n";
	for (map<string, UAFDetector::taskDetails>::iterator it = detector.taskIDMap.begin(); it != detector.taskIDMap.end(); it++) {
		cout << "Task ID: " << it->first << " - details: ";
		it->second.printTaskDetails();
		cout << endl;
	}
	cout << "\nThreads:\n";
	for (map<IDType, UAFDetector::threadDetails>::iterator it = detector.threadIDMap.begin(); it != detector.threadIDMap.end(); it++) {
		cout << "Thread ID: " << it->first << " - details:";
		it->second.printThreadDetails();
		cout << endl;
	}
	cout << "\nNesting Loops:\n";
	for (map<string, UAFDetector::nestingLoopDetails>::iterator it = detector.nestingLoopMap.begin(); it != detector.nestingLoopMap.end(); it++) {
		cout << "Shared variable: " << it->first << " - details: ";
		it->second.printNestingLoopDetails();
		cout << endl;
	}
	cout << "\nPause/Reset/Resume Ops:\n";
	for (map<IDType, string>::iterator it = detector.pauseResumeResetOps.begin(); it != detector.pauseResumeResetOps.end(); it++) {
		cout << "Op: " << it->first << " shared variable: " << it->second << endl;
	}
	cout << "\nSet - enq\n";
	for (map<IDType, UAFDetector::enqOpDetails>::iterator it = detector.enqToTaskEnqueued.begin(); it != detector.enqToTaskEnqueued.end(); it++) {
		cout << "Op: " << it->first << " - details: ";
		it->second.printEnqDetails();
		cout << endl;
	}
	cout << "\nSet - alloc\n";
	for (map<IDType, UAFDetector::memoryOpDetails>::iterator it = detector.allocSet.begin(); it != detector.allocSet.end(); it++) {
		cout << "Op: " << it->first << " - details :";
		it->second.printMemOpDetails();
		cout << endl;
	}
	cout << "\nSet - free\n";
	for (map<IDType, UAFDetector::memoryOpDetails>::iterator it = detector.freeSet.begin(); it != detector.freeSet.end(); it++) {
		cout << "Op: " << it->first << " - details :";
		it->second.printMemOpDetails();
		cout << endl;
	}
	cout << "\nSet - read\n";
	for (map<IDType, UAFDetector::memoryOpDetails>::iterator it = detector.readSet.begin(); it != detector.readSet.end(); it++) {
		cout << "Op: " << it->first << " - details :";
		it->second.printMemOpDetails();
		cout << endl;
	}
	cout << "\nSet - write\n";
	for (map<IDType, UAFDetector::memoryOpDetails>::iterator it = detector.writeSet.begin(); it != detector.writeSet.end(); it++) {
		cout << "Op: " << it->first << " - details :";
		it->second.printMemOpDetails();
		cout << endl;
	}
	cout << "\nMap - alloc\n";
	for (map<IDType, UAFDetector::allocOpDetails>::iterator it = detector.allocIDMap.begin(); it != detector.allocIDMap.end(); it++) {
		cout << "Op: " << it->first << endl;
		it->second.printDetails();
		cout << endl;
	}
	cout << "\nMap - free\n";
	for (map<IDType, UAFDetector::freeOpDetails>::iterator it = detector.freeIDMap.begin(); it != detector.freeIDMap.end(); it++) {
		cout << "Op: " << it->first << endl;
		it->second.printDetails();
		cout << endl;
	}
	cout << "\nMap - nodes\n";
	for (map<IDType, UAFDetector::setOfOps>::iterator it = detector.nodeIDMap.begin(); it != detector.nodeIDMap.end(); it++) {
		cout << "Node: " << it->first << "\n";
		it->second.printDetails();
		cout << "\n";
	}
#ifdef LOCKS
	cout << "\nMap - wait\n";
	for (map<IDType, UAFDetector::lockOpDetails>::iterator it = detector.waitSet.begin(); it != detector.waitSet.end(); it++) {
		cout << "Wait: " << it->first << "\n";
		it->second.printDetails();
		cout << "\n";
	}
	cout << "\nMap - notify\n";
	for (map<IDType, UAFDetector::lockOpDetails>::iterator it = detector.notifySet.begin(); it != detector.notifySet.end(); it++) {
		cout << "Notify: " << it->first << "\n";
		it->second.printDetails();
		cout << "\n";
	}
	cout << "\nMap - notifyAll\n";
	for (map<IDType, UAFDetector::lockOpDetails>::iterator it = detector.notifyAllSet.begin(); it != detector.notifyAllSet.end(); it++) {
		cout << "NotifyAll: " << it->first << "\n";
		it->second.printDetails();
		cout << "\n";
	}
	cout << "\nMap - lockToNotify\n";
	for (map<std::string, UAFDetector::setOfOps>::iterator it = detector.lockToNotify.begin(); it != detector.lockToNotify.end(); it++) {
		cout << "Lock ID: " << it->first << "\n";
		it->second.printDetails();
		cout << "\n";
	}
	cout << "\nMap - lockToNotifyAll\n";
	for (map<std::string, UAFDetector::setOfOps>::iterator it = detector.lockToNotifyAll.begin(); it != detector.lockToNotifyAll.end(); it++) {
		cout << "Lock ID: " << it->first << "\n";
		it->second.printDetails();
		cout << "\n";
	}
	cout << "\nMap - notifyToWait\n";
	for (map<IDType, IDType>::iterator it = detector.notifyToWait.begin(); it != detector.notifyToWait.end(); it++) {
		cout << "Notify " << it->first << " --> Wait " << it->second << "\n";
	}
	cout << "\nMap - notifyAllToWaitSet\n";
	for (map<IDType, UAFDetector::setOfOps>::iterator it = detector.notifyAllToWaitSet.begin(); it != detector.notifyAllToWaitSet.end(); it++) {
		cout << "NotifyAll " << it->first << "-->\n";
		it->second.printDetails();
		cout << "\n";
	}
	cout << "\nMap - waitToNotify\n";
	for (map<IDType, IDType>::iterator it = detector.waitToNotify.begin(); it != detector.waitToNotify.end(); it++) {
		cout << "Wait " << it->first << " --> Notify " << it->second << "\n";
	}
#endif
#endif

	// Initialize HB Graph
	detector.initGraph(nodeCount);

	return 0;
}
