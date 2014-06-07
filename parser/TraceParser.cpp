/*
 * TraceParser.cpp
 *
 *  Created on: 29-May-2014
 *      Author: shalini
 */

#include "TraceParser.h"
#include <iostream>
#include <cstring>
#include <boost/regex.hpp>
#include "MultiStack.h"

#include <debugconfig.h>

/*
 * Constructor for TraceParser class.
 * Takes as argument name of the tracefile
 * Creates regex for a valid operation.
 */
TraceParser::TraceParser(char* traceFileName, Logger &logger) {
	traceFile.open(traceFileName, ios_base::in);
	if (!traceFile.is_open()) {
		cout << "Cannot open trace file\n";
		logger.writeLog("Cannot open trace-file");
	}

	opCount = 0;
	// The prefix regular expression
	prefixRegEx = "[0-9]+ *:";

	intRegEx = "[0-9]+";
	hexRegEx = "0[xX][0-9a-fA-F]+";

	// The operation regular expression.
	opRegEx = " *(threadinit) *\\( *(" + intRegEx + ") *\\) *" + "|" +
			  " *(threadexit) *\\( *(" + intRegEx + ") *\\) *" + "|" +
			  " *(fork) *\\( *("       + intRegEx + ") *, *(" 	   + intRegEx + ") *\\) *" + "|" +
			  " *(join) *\\( *("       + intRegEx + ") *, *(" 	   + intRegEx + ") *\\) *" + "|" +
			  " *(enterloop) *\\( *("  + intRegEx + ") *\\) *" + "|" +
			  " *(exitloop) *\\( *("   + intRegEx + ") *\\) *" + "|" +
			  " *(enq) *\\( *("        + intRegEx + ") *, *(" 	   + hexRegEx + ") *, *("
			  	  	  	  	  	  	   + intRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *" + "|" +
			  " *(deq) *\\( *(" 	   + intRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *" + "|" +
			  " *(end) *\\( *(" 	   + intRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *" + "|" +
			  " *(pause) *\\( *(" 	   + intRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *" + "|" +
			  " *(resume) *\\( *(" 	   + intRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *" + "|" +
			  " *(acquire) *\\( *("    + intRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *" + "|" +
			  " *(release) *\\( *("    + intRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *" + "|" +
			  " *(alloc) *\\( *(" 	   + intRegEx + ") *, *(" 	   + hexRegEx + ") *, *("
			  	  	  	  	  	  	  	  	  	    		  	   + intRegEx + ") *\\) *" + "|" +
			  " *(free) *\\( *(" 	   + intRegEx + ") *, *(" 	   + hexRegEx + ") *, *("
			  	  	  	  	  	  	  	  	  	   	   	   	  	   + intRegEx + ") *\\) *" + "|" +
			  " *(inc) *\\( *(" 	   + intRegEx + ") *, *\\( *(" + hexRegEx + ") *, *("
			  	  	  	  	  	  	  	  	  	  	   	   	  	   + intRegEx + ") *\\) *\\) *" + "|" +
			  " *(dec) *\\( *(" 	   + intRegEx + ") *, *\\( *(" + hexRegEx + ") *, *("
			  	  	  	  	  	  	  	  	  	  	   	   	   	   + intRegEx + ") *\\) *\\) *" + "|" +
			  " *(read) *\\( *(" 	   + intRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *" + "|" +
			  " *(write) *\\( *(" 	   + intRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *";

	// Regular expression for the entire line
	finalRegEx = "^" + prefixRegEx + "(" + opRegEx + ")" + "$";
}

TraceParser::~TraceParser() {
}

int TraceParser::parse(UAFDetector &detector, Logger &logger) {
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

	bool flag;
	long long typePos, threadPos;
	MultiStack stack1;	// keeps track of last deq, pause and resume operations - to obtain first pause and last resume.
	MultiStack stack2;	// keeps track of last op in each thread - to obtain next op ID in the same task
	MultiStack stack3;  // keeps track of last op in each thread - to obtain next op ID for every operation in the thread.

	while (traceFile >> line) {
		// Check whether the line is a valid line according to finalRegEx
		if (!boost::regex_match(line.c_str(), matches, reg)) {
			cout << "ERROR: Line in trace file is not valid\n";
			cout << line << endl;
		}
		else {
			opCount++;
			flag = false;

			// temp variable to store operation details.
			// Declared here so as to get a fresh variable for each operation.
			UAFDetector::opDetails opdetails;

			// matches[] contains the substrings that matched subexpressions of the regex.
			// matches[1] is the complete operation (without prefix or suffix), for e.g., threadinit(0)
			// Map operation string to opID.
			string OpString(matches[1].first, matches[1].second);
			detector.opStringToOpIDMap[OpString] = opCount;

			for (unsigned i=2; i < matches.size(); i++) {
				string match(matches[i].first, matches[i].second);
				// find the next non-empty matches[i] after matches[1] - this would be the type of operation.
				if (!match.empty() && match.compare(" ") != 0) {
					if (!flag) {
						flag = true;
						typePos = i;

						opdetails.opType = match;
						for (unsigned j=typePos+1; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							// find the next non-empty matches[i] after type of operation.
							if (!m1.empty() && m1.compare(" ") != 0) {
								opdetails.threadID = atoi(m1.c_str());
								threadPos = j;
								break;
							}
						}

						// if operation is deq, end, pause or resume, get the task ID too.
						if ((match.compare("deq") == 0)   || (match.compare("end") == 0) ||
							(match.compare("pause") == 0) || (match.compare("resume") == 0)) {
							for (unsigned j=threadPos+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								// find the next non-empty matches[i] after threadID of operation.
								if (!m1.empty() && m1.compare(" ") != 0) {
									opdetails.taskID = m1;
									break;
								}
							}
						}

						// For operations other than deq, end, pause or resume, get taskID from last op on stack 1
						if ((match.compare("deq") != 0) && (match.compare("end") != 0) &&
							(match.compare("pause") != 0) && (match.compare("resume") != 0)) {
							MultiStack::stackElementType top = stack1.peek(opdetails.threadID);
							if (!stack1.isBottom(top))
								opdetails.taskID = top.taskID;
						}

						// add entry for current operation in the opIDMap, if it does not already exist.
						if (detector.opIDMap.find(opCount) == detector.opIDMap.end())
							detector.opIDMap[opCount] = opdetails;
						else {
							cout << "ERROR: opIDMap entry already exists for " << OpString << endl;
							return -1;
						}

						// Push deq to stack2
						if (match.compare("deq") == 0) {
							MultiStack::stackElementType element;
							element.opID = opCount;
							element.opType = opdetails.opType;
							element.taskID = opdetails.taskID;
							element.threadID = opdetails.threadID;
							stack2.push(element);
						} else if (match.compare("end") == 0) {
							// If the current operation is end, pop the top element from stack 2.
							// Don't push the end.
							MultiStack::stackElementType top = stack2.peek(opdetails.threadID);
							if (!stack2.isBottom(top)) {
								if (top.threadID == opdetails.threadID && top.taskID == opdetails.taskID) {
									if (detector.opToNextOpInTask.find(top.opID) == detector.opToNextOpInTask.end()) {
										detector.opToNextOpInTask[top.opID] = opCount;
										stack2.pop(opdetails.threadID);
									} else {
										cout << "ERROR: Next op in task already exists for op " << top.opID << " " << top.opType
											 << "(" << top.taskID << ", " << top.threadID << ")\n";
										return -1;
									}
								} else {
									cout << "ERROR: Prev op in stack does not have same task ID and thread ID as end op" << endl;
									cout << "Prev op: " << top.opID << " " << top.opType << "(" << top.threadID << ", " << top.taskID << ")\n";
									cout << "end op: " << opCount << " " << opdetails.opType << "(" << opdetails.threadID << ", " << opdetails.taskID << ")\n";
									return -1;
								}
							} else {
								cout << "ERROR: Found stack bottom when analyzing end for next op in task\n";
								return -1;
							}
						} else {
							// Obtain the top element in stack2, make curr op the next op ID of the top.
							MultiStack::stackElementType top = stack2.peek(opdetails.threadID);
							if (!stack2.isBottom(top)) {
								if (top.threadID == opdetails.threadID && top.taskID == opdetails.taskID) {
									if (detector.opToNextOpInTask.find(top.opID) == detector.opToNextOpInTask.end()) {
										detector.opToNextOpInTask[top.opID] = opCount;
										stack2.pop(opdetails.threadID);

										MultiStack::stackElementType element;
										element.opID = opCount;
										element.opType = opdetails.opType;
										element.taskID = opdetails.taskID;
										element.threadID = opdetails.threadID;
										stack2.push(element);
									} else {
										cout << "ERROR: Next operation already exists for op ID: " << top.opID
											 << " Next op set to :" << detector.opToNextOpInTask.find(top.opID)->second << endl;
										return -1;
									}
								} else {
									cout << "ERROR: Top op does not have same threadID and taskID: " << top.opType << " - " << top.threadID << ", " << top.taskID;
									cout << endl << "Current op: " << opdetails.opType << " - " << opdetails.threadID << ", " << opdetails.taskID << endl;
									return -1;
								}
							}
						}

						// Push deq, pause and resume to stack 1 and obtain first pause of a task.
						if ((match.compare("deq") == 0) || (match.compare("pause") == 0) || (match.compare("resume") == 0)) {

							UAFDetector::taskDetails taskdetails;
							taskdetails.threadID = opdetails.threadID;

							// Add taskDetails (with only the threadID) to map on seeing deq
							if (match.compare("deq") == 0) {
								if (detector.taskIDMap.find(opdetails.taskID) == detector.taskIDMap.end())
									detector.taskIDMap[opdetails.taskID] = taskdetails;
								else {
									UAFDetector::taskDetails oldDetails = detector.taskIDMap.find(opdetails.taskID)->second;
									oldDetails.threadID = taskdetails.threadID;
									detector.taskIDMap.erase(detector.taskIDMap.find(opdetails.taskID));
									detector.taskIDMap[opdetails.taskID] = oldDetails;
								}
#ifdef TRACEDEBUG
								cout << "At a deq Task " << opdetails.taskID;
								detector.taskIDMap[opdetails.taskID].printTaskDetails();
								cout << endl;
#endif
							}

							// Find out if this is the first pause
							if (match.compare("pause") == 0) {
								MultiStack::stackElementType top = stack1.peek(opdetails.threadID);
								// If the top operation on stack 1 is a deq (i.e no pauses yet)
								if (top.opType.compare("deq") == 0) {
									taskdetails.firstPauseOpID = opCount;
									// We add taskDetails to map on seeing deq, retrieve that and update with first pause op.
									if (detector.taskIDMap.find(opdetails.taskID) != detector.taskIDMap.end()) {
										UAFDetector::taskDetails oldDetails = detector.taskIDMap.find(opdetails.taskID)->second;
										detector.taskIDMap.erase(detector.taskIDMap.find(opdetails.taskID));
										oldDetails.firstPauseOpID = taskdetails.firstPauseOpID;
//										detector.taskIDMap[opdetails.taskID] = taskdetails;
										detector.taskIDMap[opdetails.taskID] = oldDetails;
#ifdef TRACEDEBUG
										cout << "At a pause Task " << opdetails.taskID;
										detector.taskIDMap[opdetails.taskID].printTaskDetails();
										cout << endl;
#endif
									} else {
										cout << "ERROR: Cannot find map entry for task " << opdetails.taskID << endl;
										return -1;
									}
								}
							}

							// Push the operation on to stack 1
							MultiStack::stackElementType element;
							element.opID = opCount;
							element.opType = opdetails.opType;
							element.taskID = opdetails.taskID;
							element.threadID = opdetails.threadID;
							stack1.push(element);

						}

						// On seeing end, pop ops of this task from stack 1
						if (match.compare("end") == 0) {

							UAFDetector::taskDetails taskdetails;
							taskdetails.threadID = opdetails.threadID;

							MultiStack::stackElementType top = stack1.peek(opdetails.threadID);
							// Obtain last resume.
							if (top.opType.compare("resume") == 0) {
								taskdetails.lastResumeOpID = top.opID;
								// Find the entry corresponding to the current task on the map and update with last resume
								if (detector.taskIDMap.find(opdetails.taskID) != detector.taskIDMap.end()) {
									UAFDetector::taskDetails existingTaskDetails;
									existingTaskDetails = detector.taskIDMap.find(opdetails.taskID)->second;
//									taskdetails.firstPauseOpID = existingTaskDetails.firstPauseOpID;
									existingTaskDetails.lastResumeOpID = taskdetails.lastResumeOpID;
									detector.taskIDMap.erase(detector.taskIDMap.find(opdetails.taskID));
//									detector.taskIDMap[opdetails.taskID] = taskdetails;
									detector.taskIDMap[opdetails.taskID] = existingTaskDetails;
#ifdef TRACEDEBUG
										cout << "At an end Task " << opdetails.taskID;
										detector.taskIDMap[opdetails.taskID].printTaskDetails();
										cout << endl;
#endif
								} else {
									cout << "ERROR: Cannot find map entry for task " << opdetails.taskID << endl;
									return -1;
								}
							}

							// Pop ops until the deq
							top = stack1.peek(opdetails.threadID);
							while (top.opType.compare("deq") != 0) {
								stack1.pop(opdetails.threadID);
								top = stack1.peek(opdetails.threadID);
							}

							// Pop deq
							stack1.pop(opdetails.threadID);
						}

						// To obtain the next op in the same thread.
						if (match.compare("threadinit") == 0) {
							MultiStack::stackElementType element;
							element.opID = opCount;
							element.opType = opdetails.opType;
							element.taskID = opdetails.taskID;
							element.threadID = opdetails.threadID;

							stack3.push(element);
						} else if (match.compare("threadexit") == 0) {
							MultiStack::stackElementType top = stack3.peek(opdetails.threadID);
							if (!stack3.isBottom(top)) {
								if (detector.opToNextOpInThread.find(top.opID) == detector.opToNextOpInThread.end()) {
									detector.opToNextOpInThread[top.opID] = opCount;
									stack3.pop(opdetails.threadID);
								} else {
									cout << "ERROR: Next op already present in the map for op " << top.opID << " " << top.opType
										 << "(" << top.threadID << ", " << top.taskID << ")\n";
									return -1;
								}
							} else {
								cout << "ERROR: Found stack bottom when examining threadexit to compute next op in thread\n";
								return -1;
							}
						} else {
							MultiStack::stackElementType top = stack3.peek(opdetails.threadID);
							if (!stack3.isBottom(top)) {
								if (detector.opToNextOpInThread.find(top.opID) == detector.opToNextOpInThread.end()) {
									detector.opToNextOpInThread[top.opID] = opCount;
									stack3.pop(opdetails.threadID);

									MultiStack::stackElementType element;
									element.opID = opCount;
									element.opType = opdetails.opType;
									element.taskID = opdetails.taskID;
									element.threadID = opdetails.threadID;
									stack3.push(element);
								} else {
									cout << "ERROR: Next op already present in the map for op " << top.opID << " " << top.opType
										 << "(" << top.threadID << ", " << top.taskID << ")\n";
								}
							} else {
								cout << "ERROR: Found stack bottom when examining threadexit to compute next op in thread\n";
								return -1;
							}
						}

						// Obtaining sets of each kind of operations in the trace.
						if (match.compare("threadinit") == 0) {
							detector.threadinitSet.insert(opCount);

							long long threadID = opdetails.threadID;
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								detector.threadIDMap[threadID].threadinitOpID = opCount;
							} else if (detector.threadIDMap[threadID].threadinitOpID != -1 && detector.threadIDMap[threadID].threadinitOpID != opCount) {
								cout << "ERROR: threadinit already seen for thread " << threadID << " at op " << detector.threadIDMap[threadID].threadinitOpID << endl;
								return -1;
							} else
								detector.threadIDMap[threadID].threadinitOpID = opCount;
						} else if (match.compare("threadexit") == 0) {
							detector.threadexitSet.insert(opCount);

							long long threadID = opdetails.threadID;
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								detector.threadIDMap[threadID].threadexitOpID = opCount;
							} else if (detector.threadIDMap[threadID].threadexitOpID != -1 && detector.threadIDMap[threadID].threadexitOpID != opCount) {
								cout << "ERROR: threadexit already seen for thread " << threadID << " at op " << detector.threadIDMap[threadID].threadexitOpID << endl;
								return -1;
							} else
								detector.threadIDMap[threadID].threadexitOpID = opCount;
						} else if (match.compare("enterloop") == 0) {
							detector.enterloopSet.insert(opCount);

							long long threadID = opdetails.threadID;
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								detector.threadIDMap[threadID].enterloopOpID = opCount;
							} else if (detector.threadIDMap[threadID].enterloopOpID != -1 && detector.threadIDMap[threadID].enterloopOpID != opCount) {
								cout << "ERROR: enterloop already seen for thread " << threadID << " at op " << detector.threadIDMap[threadID].enterloopOpID << endl;
								return -1;
							} else
								detector.threadIDMap[threadID].enterloopOpID = opCount;
						} else if (match.compare("exitloop") == 0) {
							detector.exitloopSet.insert(opCount);

							long long threadID = opdetails.threadID;
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								detector.threadIDMap[threadID].exitloopOpID = opCount;
							} else if (detector.threadIDMap[threadID].exitloopOpID != -1 && detector.threadIDMap[threadID].exitloopOpID != opCount) {
								cout << "ERROR: exitloop already seen for thread " << threadID << " at op " << detector.threadIDMap[threadID].exitloopOpID << endl;
								return -1;
							} else
								detector.threadIDMap[threadID].exitloopOpID = opCount;
						} else if (match.compare("pause") == 0) {
							detector.pauseSet.insert(opCount);
						} else if (match.compare("resume") == 0) {
							detector.resumeSet.insert(opCount);
						} else if (match.compare("deq") == 0) {
							detector.deqSet.insert(opCount);

							string taskID = opdetails.taskID;
							if (detector.taskIDMap.find(taskID) == detector.taskIDMap.end()){
								UAFDetector::taskDetails details;
								details.deqOpID = opCount;
								detector.taskIDMap[taskID] = details;
							} else if (detector.taskIDMap[taskID].deqOpID != -1 && detector.taskIDMap[taskID].deqOpID != opCount) {
								cout << "ERROR: deq already seen for task " << taskID << " at op " << detector.taskIDMap[taskID].deqOpID << endl;
								return -1;
							} else {
								UAFDetector::taskDetails existingDetails = detector.taskIDMap.find(taskID)->second;
								existingDetails.deqOpID = opCount;
								detector.taskIDMap.erase(detector.taskIDMap.find(taskID));
								detector.taskIDMap[taskID] = existingDetails;
							}
#ifdef TRACEDEBUG
							cout << "Task " << taskID << " at deq: ";
							detector.taskIDMap[taskID].printTaskDetails();
							cout << endl;
#endif
						} else if (match.compare("end") == 0) {
							detector.endSet.insert(opCount);

							string taskID = opdetails.taskID;
							if (detector.taskIDMap.find(taskID) == detector.taskIDMap.end()){
								detector.taskIDMap[taskID].endOpID = opCount;
							} else if (detector.taskIDMap[taskID].endOpID != -1 && detector.taskIDMap[taskID].endOpID != opCount) {
								cout << "ERROR: end already seen for task " << taskID << " at op " << detector.taskIDMap[taskID].endOpID << endl;
								return -1;
							} else {
								UAFDetector::taskDetails existingDetails = detector.taskIDMap.find(taskID)->second;
								existingDetails.endOpID = opCount;
								detector.taskIDMap.erase(detector.taskIDMap.find(taskID));
								detector.taskIDMap[taskID] = existingDetails;
//								detector.taskIDMap[taskID].endOpID = opCount;
							}
						} else if (match.compare("enq") == 0) {
							// Obtain the four arguments to enq.
							UAFDetector::enqOpDetails enqopdetails;
							unsigned j;
							for (j=typePos+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									enqopdetails.currThreadID = atoi(m1.c_str());
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									enqopdetails.taskEnqueued = m1;
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									enqopdetails.targetThreadID = atoi(m1.c_str());
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									enqopdetails.callback = m1;
									break;
								}
							}

							// Add this enq op entry to the set.
							detector.enqSet[opCount] = enqopdetails;

							string taskID = enqopdetails.taskEnqueued;
							string callback = enqopdetails.callback;
							if (detector.taskIDMap.find(taskID) == detector.taskIDMap.end()){
								UAFDetector::taskDetails details;
								details.enqOpID = opCount;
								details.callback = callback;
								detector.taskIDMap[taskID] = details;
							} else if (detector.taskIDMap[taskID].enqOpID != -1 && detector.taskIDMap[taskID].enqOpID != opCount) {
								cout << "ERROR: enq already seen for task " << taskID << " at op " << detector.taskIDMap[taskID].enqOpID << endl;
								return -1;
							} else {
								UAFDetector::taskDetails existingDetails = detector.taskIDMap.find(taskID)->second;
								existingDetails.enqOpID = opCount;
								existingDetails.callback = callback;
								detector.taskIDMap.erase(detector.taskIDMap.find(taskID));
								detector.taskIDMap[taskID] = existingDetails;
							}
#ifdef TRACEDEBUG
							cout << "Task " << taskID << " at enq ";
							detector.taskIDMap[taskID].printTaskDetails();
							cout << endl;
#endif
						} else if (match.compare("fork") == 0) {
							// Obtain two arguments of fork.
							UAFDetector::forkAndJoinOpDetails forkopdetails;
							unsigned j = typePos + 1;
							for (; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									forkopdetails.currThreadID = atoi(m1.c_str());
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									forkopdetails.targetThreadID = atoi(m1.c_str());
									break;
								}
							}

							// Add this fork op entry to the set.
							detector.forkSet[opCount] = forkopdetails;

							long long targetThread = forkopdetails.targetThreadID;
							if (detector.threadIDMap.find(targetThread) == detector.threadIDMap.end()) {
								detector.threadIDMap[targetThread].forkOpID = opCount;
							} else if (detector.threadIDMap[targetThread].forkOpID != -1 && detector.threadIDMap[targetThread].forkOpID != opCount) {
								cout << "ERROR: fork already seen for thread " << targetThread << " at op " << detector.threadIDMap[targetThread].forkOpID << endl;
								return -1;
							} else
								detector.threadIDMap[targetThread].forkOpID = opCount;

						} else if (match.compare("join") == 0) {
							// Obtain two arguments of join.
							UAFDetector::forkAndJoinOpDetails joinopdetails;
							unsigned j = typePos + 1;
							for (; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									joinopdetails.currThreadID = atoi(m1.c_str());
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									joinopdetails.targetThreadID = atoi(m1.c_str());
									break;
								}
							}

							// Add this fork op entry to the set.
							detector.joinSet[opCount] = joinopdetails;

							long long targetThread = joinopdetails.targetThreadID;
							if (detector.threadIDMap.find(targetThread) == detector.threadIDMap.end()) {
								detector.threadIDMap[targetThread].joinOpID = opCount;
							} else if (detector.threadIDMap[targetThread].joinOpID != -1 && detector.threadIDMap[targetThread].joinOpID != opCount) {
								cout << "ERROR: join already seen for thread " << targetThread << " at op " << detector.threadIDMap[targetThread].joinOpID << endl;
								return -1;
							} else
								detector.threadIDMap[targetThread].joinOpID = opCount;
						} else if (match.compare("acquire") == 0) {
							// Obtain two arguments of acquire.
							UAFDetector::acquireAndReleaseOpDetails acquireopdetails;
							unsigned j = typePos + 1;
							for (; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0){
									acquireopdetails.currThreadID = atoi(m1.c_str());
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0){
									acquireopdetails.lockID = m1;
									break;
								}
							}

							// Add this acquire op entry to the set.
							detector.acquireSet[opCount] = acquireopdetails;
						} else if (match.compare("release") == 0) {
							// Obtain two arguments of release.
							UAFDetector::acquireAndReleaseOpDetails releaseopdetails;
							unsigned j = typePos + 1;
							for (; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0){
									releaseopdetails.currThreadID = atoi(m1.c_str());
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0){
									releaseopdetails.lockID = m1;
									break;
								}
							}

							// Add this acquire op entry to the set.
							detector.releaseSet[opCount] = releaseopdetails;
						} else if (match.compare("alloc") == 0) {
							UAFDetector::memoryOpDetails allocopdetails;
							unsigned j = typePos + 1;
							for (; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									allocopdetails.threadID = atoi(m1.c_str());
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									allocopdetails.startingAddress = m1;
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									allocopdetails.range = atoi(m1.c_str());
									break;
								}
							}

							detector.allocSet[opCount] = allocopdetails;
						} else if (match.compare("free") == 0) {
							UAFDetector::memoryOpDetails freeopdetails;
							unsigned j = typePos + 1;
							for (; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									freeopdetails.threadID = atoi(m1.c_str());
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									freeopdetails.startingAddress = m1;
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									freeopdetails.range = atoi(m1.c_str());
									break;
								}
							}
							detector.freeSet[opCount] = freeopdetails;
						} else if (match.compare("inc") == 0) {
							UAFDetector::memoryOpDetails incopdetails;
							unsigned j = typePos + 1;
							for (; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									incopdetails.threadID = atoi(m1.c_str());
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									incopdetails.startingAddress = m1;
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									incopdetails.range = atoi(m1.c_str());
									break;
								}
							}
							detector.incSet[opCount] = incopdetails;
						} else if (match.compare("dec") == 0) {
							UAFDetector::memoryOpDetails decopdetails;
							unsigned j = typePos + 1;
							for (; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									decopdetails.threadID = atoi(m1.c_str());
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									decopdetails.startingAddress = m1;
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									decopdetails.range = atoi(m1.c_str());
									break;
								}
							}
							detector.decSet[opCount] = decopdetails;
						} else if (match.compare("read") == 0) {
							UAFDetector::memoryOpDetails readopdetails;
							unsigned j = typePos + 1;
							for (; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									readopdetails.threadID = atoi(m1.c_str());
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									readopdetails.startingAddress = m1;
									break;
								}
							}
							detector.readSet[opCount] = readopdetails;
						} else if (match.compare("write") == 0) {
							UAFDetector::memoryOpDetails writeopdetails;
							unsigned j = typePos + 1;
							for (; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									writeopdetails.threadID = atoi(m1.c_str());
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									writeopdetails.startingAddress = m1;
									break;
								}
							}
							detector.writeSet[opCount] = writeopdetails;
						}

						break;
					}
				}
			}
		}
	}

	// If the first pause and last resume ops are not set for a task, add it to set of atomic tasks.
	for (map<string, UAFDetector::taskDetails>::iterator it = detector.taskIDMap.begin(); it != detector.taskIDMap.end(); it++) {
		if (it->second.firstPauseOpID == -1 && it->second.lastResumeOpID == -1) {
			detector.atomicTasks.insert(it->first);
		} else if (it->second.firstPauseOpID == -1) {
			cout << "ERROR: First pause not set, but last resume set: Task " << it->first
				 << " Last resume " << it->second.lastResumeOpID << endl;
		} else if (it->second.lastResumeOpID == -1) {
			cout << "ERROR: First pause set, but last resume not set: Task " << it->first
				 << " First pause " << it->second.firstPauseOpID << endl;
		}
	}

	// Initialize HB Graph
	detector.initGraph(opCount);

#ifdef TRACEDEBUG
	cout << "No of ops: " << opCount << endl;
	for (map<long long, UAFDetector::opDetails>::iterator it = detector.opIDMap.begin(); it != detector.opIDMap.end(); it++) {
		cout << "Key: " << it->first << " - details: ";
//		cout << "Optype: " << it->second.opType
//			 << " TaskID: " << it->second.taskID
//			 << " ThreadID: " << it->second.threadID << endl;
		it->second.printOpDetails();
		cout << endl;
	}
	for (map<string, UAFDetector::taskDetails>::iterator it = detector.taskIDMap.begin(); it != detector.taskIDMap.end(); it++) {
		cout << "Task ID: " << it->first << " - details: ";
//		cout << "Thread ID: " << it->second.threadID
//			 << "First pause: " << it->second.firstPauseOpID
//			 << "Last resume: " << it->second.lastResumeOpID << endl;
		it->second.printTaskDetails();
		cout << endl;
	}
	for (map<long long, UAFDetector::threadDetails>::iterator it = detector.threadIDMap.begin(); it != detector.threadIDMap.end(); it++) {
		cout << "Thread ID: " << it->first << " - details: ";
		it->second.printThreadDetails();
		cout << endl;
	}
	for (set<string>::iterator it = detector.atomicTasks.begin(); it != detector.atomicTasks.end(); it++) {
		cout << "Atomic Task: " << *it << endl;
	}
	for (int i=1; i <= opCount; i++) {
		cout << "Op " << i << "-> " << detector.opToNextOpInTask[i] << endl;
	}
	for (int i=1; i <= opCount; i++) {
		cout << "Op " << i << "-> " << detector.opToNextOpInThread[i] << endl;
	}
	for (set<long long>::iterator it = detector.threadinitSet.begin(); it != detector.threadinitSet.end(); it++) {
		cout << "Threadinit: " << *it << endl;
	}
	for (set<long long>::iterator it = detector.threadexitSet.begin(); it != detector.threadexitSet.end(); it++) {
		cout << "Threadexit: " << *it << endl;
	}
	for (set<long long>::iterator it = detector.enterloopSet.begin(); it != detector.enterloopSet.end(); it++) {
		cout << "Enterloop: " << *it << endl;
	}
	for (set<long long>::iterator it = detector.exitloopSet.begin(); it != detector.exitloopSet.end(); it++) {
		cout << "Exitloop: " << *it << endl;
	}
	for (set<long long>::iterator it = detector.deqSet.begin(); it != detector.deqSet.end(); it++) {
		cout << "Deq: " << *it << endl;
	}
	for (set<long long>::iterator it = detector.endSet.begin(); it != detector.endSet.end(); it++) {
		cout << "End: " << *it << endl;
	}
	for (set<long long>::iterator it = detector.pauseSet.begin(); it != detector.pauseSet.end(); it++) {
		cout << "Pause: " << *it << endl;
	}
	for (set<long long>::iterator it = detector.resumeSet.begin(); it != detector.resumeSet.end(); it++) {
		cout << "Resume: " << *it << endl;
	}
	for (map<long long, UAFDetector::enqOpDetails>::iterator it = detector.enqSet.begin(); it != detector.enqSet.end(); it++) {
		cout << "Enq: " << it->first << "(" << it->second.currThreadID << ", " << it->second.taskEnqueued
			 << ", " << it->second.targetThreadID << ", " << it->second.callback << ")" << endl;
	}
	for (map<long long, UAFDetector::forkAndJoinOpDetails>::iterator it = detector.forkSet.begin(); it != detector.forkSet.end(); it++) {
		cout << "Fork: " << it->first << "(" << it->second.currThreadID << ", " << it->second.targetThreadID << ")\n";
	}
	for (map<long long, UAFDetector::forkAndJoinOpDetails>::iterator it = detector.joinSet.begin(); it != detector.joinSet.end(); it++) {
		cout << "Join: " << it->first << "(" << it->second.currThreadID << ", " << it->second.targetThreadID << ")\n";
	}
	for (map<long long, UAFDetector::acquireAndReleaseOpDetails>::iterator it = detector.acquireSet.begin(); it != detector.acquireSet.end(); it++) {
		cout << "Acquire: " << it->first << "(" << it->second.currThreadID << ", " << it->second.lockID << ")\n";
	}
	for (map<long long, UAFDetector::acquireAndReleaseOpDetails>::iterator it = detector.releaseSet.begin(); it != detector.releaseSet.end(); it++) {
		cout << "Release: " << it->first << "(" << it->second.currThreadID << ", " << it->second.lockID << ")\n";
	}
	for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.allocSet.begin(); it != detector.allocSet.end(); it++) {
		cout << "Alloc: " << it->first << "(" << it->second.threadID << ", " << it->second.startingAddress << ", " << it->second.range << ")\n";
	}
	for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.freeSet.begin(); it != detector.freeSet.end(); it++) {
		cout << "Free: " << it->first << "(" << it->second.threadID << ", " << it->second.startingAddress << ", " << it->second.range << ")\n";
	}
	for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.incSet.begin(); it != detector.incSet.end(); it++) {
		cout << "Inc: " << it->first << "(" << it->second.threadID << ", " << it->second.startingAddress << ", " << it->second.range << ")\n";
	}
	for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.decSet.begin(); it != detector.decSet.end(); it++) {
		cout << "Dec: " << it->first << "(" << it->second.threadID << ", " << it->second.startingAddress << ", " << it->second.range << ")\n";
	}
	for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.readSet.begin(); it != detector.readSet.end(); it++) {
		cout << "Read: " << it->first << "(" << it->second.threadID << ", " << it->second.startingAddress << ", " << it->second.range << ")\n";
	}
	for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.writeSet.begin(); it != detector.writeSet.end(); it++) {
		cout << "Write: " << it->first << "(" << it->second.threadID << ", " << it->second.startingAddress << ", " << it->second.range << ")\n";
	}
#endif

	return 0;
}
