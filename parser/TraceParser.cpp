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

/*
 * Constructor for TraceParser class.
 * Takes as argument name of the tracefile
 * Creates regex for a valid operation.
 */
TraceParser::TraceParser(char* traceFileName, Logger &logger) {
	traceFile.open(traceFileName, ios_base::in);
	if (!traceFile.is_open()) {
		cout << "Cannot open trace file\n";
		logger.streamObject << "Cannot open trace file "
							<< traceFileName << endl;
		logger.writeLog();
	}
	else {
		logger.streamObject << "Test string 1";
		logger.writeLog();
		logger.streamObject << "Test string 2";
		logger.writeLog();
	}

	opCount = 0;
	blockCount = 0;
	// The prefix regular expression
	prefixRegEx = " *";

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
			  	  	  	  	  	  	   + intRegEx + ") *\\) *" + "|" +
			  " *(deq) *\\( *(" 	   + intRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *" + "|" +
			  " *(end) *\\( *(" 	   + intRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *" + "|" +
			  " *(pause) *\\( *(" 	   + intRegEx + ") *, *(" 	   + hexRegEx + ") *, *("
			  	  	  	  	  	  	                               + hexRegEx + ") *\\) *" + "|" +
			  " *(resume) *\\( *(" 	   + intRegEx + ") *, *(" 	   + hexRegEx + ") *, *("
			  	  	  	  	  	  	                               + hexRegEx + ") *\\) *" + "|" +
			  " *(reset) *\\( *("	   + intRegEx + ") *, *("      + hexRegEx + ") *, *("
			  	  	  	  	  	  	                               + hexRegEx + ") *\\) *" + "|" +
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
		logger.streamObject << finalRegEx << " is not a valid regular expression: \""
							<< e.what() << "\"" << endl;
		logger.writeLog();
		return -1;
	}

	bool flag;
	long long typePos, threadPos;
	MultiStack stack1;	// keeps track of last deq, pause and resume operations - to obtain first pause and last resume.
	MultiStack stack2;	// keeps track of last op in each thread - to obtain next op ID in the same task.
	MultiStack stack3;  // keeps track of last op in each thread - to obtain next op ID for every operation in the thread.
	MultiStack stack4;  // keeps track of last deq, pause - to obtain task nesting.
	MultiStack stack5;  // get first op of each thread
	MultiStack stack6;	// keeps track of last op in each block

	MultiStack stackForThreadAndBlockOrder;
	MultiStack stackForTaskOrder;
//	MultiStack stackForBlockOrder;
	MultiStack stackForNestingOrder;

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
			UAFDetector::blockDetails blockdetails;
			UAFDetector::taskDetails taskdetails;
			UAFDetector::threadDetails threaddetails;

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

					// Obtain threadID for current block
					blockdetails.threadID = threadID;

					// Populate details for stack entry
					stackElement.opID = opCount;
					stackElement.opType = match;
					stackElement.threadID = threadID;

					if (match.compare("threadinit") == 0) {
						// threadinit is the beginning of a block
						blockCount++;
						opdetails.blockID = blockCount;

#ifdef SANITYCHECK
						// Sanity check: stack should be empty for threadID
						assert(stackForThreadAndBlockOrder.isEmpty(threadID));
						assert(stackForTaskOrder.isEmpty(threadID));
//						assert(stackForBlockOrder.isEmpty(threadID));
						assert(stackForNestingOrder.isEmpty(threadID));
#endif

						// Obtain details of current thread
						threaddetails.firstOpID = opCount;
						threaddetails.threadinitOpID = opCount;
						threaddetails.firstBlockID = blockCount;
						if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end())
							detector.threadIDMap[threadID] = threaddetails;
						else {
							cout << "ERROR: Found duplicate entry for thread " << threadID << " in threadIDMap\n";
							cout << "While examining op " << opCount << " " << OpString << ":\n";
							cout << "Duplicate entry\n";
							detector.threadIDMap[threadID].printThreadDetails();
						}

						// Obtain details of current block
						blockdetails.firstOpInBlock = opCount;
						if (detector.blockIDMap.find(blockCount) == detector.blockIDMap.end())
							detector.blockIDMap[blockCount] = blockdetails;
						else {
							cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
							cout << "While examining op " << opCount << " " << OpString << ":\n";
							cout << "Duplicate entry\n";
							detector.blockIDMap[blockCount].printBlockDetails();
						}

						// Populate blockID of stack entry
						stackElement.blockID = blockCount;
						stackForThreadAndBlockOrder.push(stackElement);
//						stackForBlockOrder.push(stackElement);

					} else if (match.compare("threadexit") == 0) {
						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadAndBlockOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for threadexit " << opCount
								 << " on stackForThreadOrder\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread (and the first block)
							blockCount++;
							opdetails.blockID = blockCount;

							// Update firstop, firstblock, lastblock and threadexitopID for the current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								threaddetails.threadexitOpID = opCount;
								threaddetails.firstOpID = opCount;
								threaddetails.firstBlockID = blockCount;
								threaddetails.lastBlockID = blockCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								cout << "ERROR: Found duplicate entry for thread " << threadID << " in threadIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.threadIDMap[threadID].printThreadDetails();
								return -1;
							}

							// Update
							if (detector.blockIDMap.find(blockCount) == detector.blockIDMap.end()) {
								blockdetails.firstOpInBlock = opCount;
								blockdetails.lastOpInBlock = opCount;
								detector.blockIDMap[blockCount] = blockdetails;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								return -1;
							}
							stackElement.blockID = blockCount;
//							stackForBlockOrder.push(stackElement);

							// Pop all operations on threadID. Not required since threadexit is the first op in thread
							//stackForThreadOrder.pop(threadID);

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadAndBlockOrder.peek(threadID);
							opdetails.blockID = previousOpInThread.blockID;
							opdetails.prevOpInBlock = previousOpInThread.opID;

							// Set current op as next-op for the stack top
							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for " << previousOpInThread.opID << " in opIDMap\n";
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID in stack and opIDMap
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								existingEntry.nextOpInBlock = opCount;
								existingEntry.nextOpInThread = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							// Update threadexit-opID and lastblock-ID for the current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								cout << "ERROR: Cannot find entry for thread " << threadID << " in threadIDMap\n";
								return -1;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
								existingEntry.threadexitOpID = opCount;
								existingEntry.lastBlockID = previousOpInThread.blockID;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							// Update lastOpInBlock for current block
							if (detector.blockIDMap.find(previousOpInThread.blockID) == detector.blockIDMap.end()) {
								cout << "ERROR: Cannot find entry for block " << previousOpInThread.blockID << " in blockIDMap\n";
								return -1;
							} else {
								UAFDetector::blockDetails existingEntry = detector.blockIDMap[previousOpInThread.blockID];

								// Sanity check: prev op has same threadID as current op in blockIDMap
								assert(existingEntry.threadID == threadID);

								existingEntry.lastOpInBlock = opCount;
								detector.blockIDMap.erase(detector.blockIDMap.find(previousOpInThread.blockID));
								detector.blockIDMap[previousOpInThread.blockID] = existingEntry;
							}

							stackElement.blockID = previousOpInThread.blockID;
						}
					} else if (match.compare("enterloop") == 0) {
						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadAndBlockOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for enterloop " << opCount
								 << " on stack\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;

							// Update firstop, firstblock and enterloopblock for current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								threaddetails.firstOpID = opCount;
								threaddetails.firstBlockID = blockCount;
								threaddetails.enterloopBlockID = blockCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								cout << "ERROR: Found duplicate entry for thread " << threadID << " in threadIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.threadIDMap[threadID].printThreadDetails();
								return -1;
							}

							if (detector.blockIDMap.find(blockCount) == detector.threadIDMap.end()) {
								blockdetails.firstOpInBlock = opCount;
								// enterloop is the end of a block
								blockdetails.lastOpInBlock = opCount;
								detector.blockIDMap[blockCount] = blockdetails;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								return -1;
							}
							stackElement.blockID = blockCount;

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadAndBlockOrder.peek(threadID);
							opdetails.blockID = previousOpInThread.blockID;
							opdetails.prevOpInBlock = previousOpInThread.opID;

							// Set current op as next-op for the stack top
							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for " << previousOpInThread.opID << " in opIDMap\n";
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev-op has same threadID as current op in stack and opIDMap
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								existingEntry.nextOpInBlock = opCount;
								existingEntry.nextOpInThread = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							// Set enterloopBlockID for the current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								cout << "ERROR: Cannot find entry for thread " << threadID << " in threadIDMap\n";
								return -1;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
								existingEntry.enterloopBlockID = previousOpInThread.blockID;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							if (detector.blockIDMap.find(previousOpInThread.blockID) == detector.threadIDMap.end()) {
								cout << "ERROR: Cannot find entry for block " << previousOpInThread.blockID << " in blockIDMap\n";
								return -1;
							} else {
								UAFDetector::blockDetails existingEntry = detector.blockIDMap[previousOpInThread.blockID];

#ifdef SANITYCHECK
								// Sanity check: block(top op) has same threadID as current op
								assert(existingEntry.threadID == threadID);
#endif

								// enterloop is the end of a block
								existingEntry.lastOpInBlock = opCount;
								detector.blockIDMap.erase(detector.blockIDMap.find(previousOpInThread.blockID));
								detector.blockIDMap[previousOpInThread.blockID] = existingEntry;
							}

							stackElement.blockID = previousOpInThread.blockID;
						}
					} else if (match.compare("exitloop") == 0) {
						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadAndBlockOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for exitloop " << opCount
								 << " on stack\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;

							// Update firstop, firstblock and exitloopblock for current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								threaddetails.firstOpID = opCount;
								threaddetails.firstBlockID = blockCount;
								threaddetails.exitloopBlockID = blockCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								cout << "ERROR: Found duplicate entry for thread " << threadID << " in threadIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.threadIDMap[threadID].printThreadDetails();
								return -1;
							}

							if (detector.blockIDMap.find(blockCount) == detector.threadIDMap.end()) {
								blockdetails.firstOpInBlock = opCount;
								detector.blockIDMap[blockCount] = blockdetails;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								return -1;
							}
							stackElement.blockID = blockCount;

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadAndBlockOrder.peek(threadID);

							// exitloop is the beginning of a block
							blockCount++;
							opdetails.blockID = blockCount;

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
								assert(existingEntry.blockID != blockCount);
								assert(previousOpInThread.blockID != blockCount);
#endif

								existingEntry.nextOpInThread = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							// Set exitloopblock for current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								cout << "ERROR: Cannot find entry for thread " << threadID << " in threadIDMap\n";
								return -1;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
								existingEntry.exitloopBlockID = blockCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							// Set first-op and prev-block for current block
							if (detector.blockIDMap.find(blockCount) == detector.blockIDMap.end()) {
								blockdetails.firstOpInBlock = opCount;

								// Get block entry for prev op (i.e. stack top)
								if (detector.blockIDMap.find(previousOpInThread.blockID) == detector.blockIDMap.end()) {
									cout << "ERROR: Cannot find entry for block " << previousOpInThread.blockID << " in blockIDMap\n";
									return -1;
								} else {
									UAFDetector::blockDetails existingEntry = detector.blockIDMap[previousOpInThread.blockID];

#ifdef SANITYCHECK
									// Sanity check: block(top-op) has same threadID as current op
									assert(existingEntry.threadID == threadID);
#endif

									existingEntry.lastOpInBlock = previousOpInThread.opID;
									existingEntry.nextBlockInThread = blockCount;
									blockdetails.prevBlockInThread = previousOpInThread.blockID;
									detector.blockIDMap.erase(detector.blockIDMap.find(previousOpInThread.blockID));
									detector.blockIDMap[previousOpInThread.blockID] = existingEntry;
								}
								detector.blockIDMap[blockCount] = blockdetails;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount << endl;
								cout << "Duplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								return -1;
							}

							stackElement.blockID = blockCount;
						}
					} else if (match.compare("enq") == 0) {
						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadAndBlockOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for enq " << opCount
								 << " on stack\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;

							// Update firstop, firstblock and exitloopblock for current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								threaddetails.firstOpID = opCount;
								threaddetails.firstBlockID = blockCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								cout << "ERROR: Found duplicate entry for thread " << threadID << " in threadIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.threadIDMap[threadID].printThreadDetails();
								return -1;
							}

							if (detector.blockIDMap.find(blockCount) == detector.threadIDMap.end()) {
								blockdetails.firstOpInBlock = opCount;
								blockdetails.enqSet.insert(opCount);
								detector.blockIDMap[blockCount] = blockdetails;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								return -1;
							}

							stackElement.blockID = blockCount;

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadAndBlockOrder.peek(threadID);
							// In the case of an enq operation, the previous op in the thread is in the same task as enq.
							// So no need to look separately in the task stack for this.

							opdetails.blockID = previousOpInThread.blockID;
							opdetails.taskID = previousOpInThread.taskID;
							opdetails.prevOpInBlock = previousOpInThread.opID;

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
								existingEntry.nextOpInTask = opCount;
								existingEntry.nextOpInBlock = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}


							// Add enq to the enqSet for current block
							if (detector.blockIDMap.find(blockCount) == detector.blockIDMap.end()) {
								cout << "ERROR: Cannot find entry for block " << previousOpInThread.blockID << " in blockIDMap\n";
								return -1;
							} else {
								// Get block entry for prev op (i.e. stack top)
								if (detector.blockIDMap.find(previousOpInThread.blockID) == detector.blockIDMap.end()) {
									cout << "ERROR: Cannot find entry for block " << previousOpInThread.blockID << " in blockIDMap\n";
									return -1;
								} else {
									UAFDetector::blockDetails existingEntry = detector.blockIDMap[previousOpInThread.blockID];

#ifdef SANITYCHECK
									// Sanity check: block(top-op) has same threadID as current op
									assert(existingEntry.threadID == threadID);
									assert(existingEntry.taskID.compare(previousOpInThread.taskID) == 0);
#endif

									existingEntry.enqSet.insert(opCount);
									detector.blockIDMap.erase(detector.blockIDMap.find(previousOpInThread.blockID));
									detector.blockIDMap[previousOpInThread.blockID] = existingEntry;
								}
								detector.blockIDMap[blockCount] = blockdetails;
							}

							UAFDetector::enqOpDetails enqdetails;

							// Obtain 2nd & 3rd argument (i.e task enqueued & target threadID) of enq op
							unsigned j;
							for (j=threadPos+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								// find the next non-empty matches[i] after threadID of operation.
								if (!m1.empty() && m1.compare(" ") != 0) {
									enqdetails.taskEnqueued = m1;
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									enqdetails.targetThread = atoi(m1.c_str());
									break;
								}
							}

							// Map enq op to its arguments;
							if (detector.enqToTaskEnqueued.find(opCount) == detector.enqToTaskEnqueued.end()) {
								detector.enqToTaskEnqueued[opCount] = enqdetails;
							} else {
								cout << "ERROR: Found duplicate entry for enq op " << opCount << " in enqToTaskEnqueued\n";
								cout << "Duplicate entry:\n";
								detector.enqToTaskEnqueued[opCount].printEnqDetails();
								return -1;
							}

							// Map task enqueued to the enq op in taskIDMap
							if (detector.taskIDMap.find(enqdetails.taskEnqueued) == detector.taskIDMap.end()) {
								UAFDetector::taskDetails taskdetails;
								taskdetails.enqOpID = opCount;
								detector.taskIDMap[enqdetails.taskEnqueued] = taskdetails;
							} else {
								cout << "ERROR: Found duplicate entry for task " << enqdetails.taskEnqueued << " in taskIDMap\n";
								cout << "While examining op: " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.taskIDMap[enqdetails.taskEnqueued].printTaskDetails();
								return -1;
							}

							stackElement.blockID = previousOpInThread.blockID;
							stackElement.taskID = previousOpInThread.taskID;

						}
					} else if (match.compare("deq") == 0) {
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
						if (stackForThreadAndBlockOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for deq " << opCount
								 << " on stack\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;
							opdetails.taskID = taskDequeued;

							// Update firstop, firstblock and exitloopblock for current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								threaddetails.firstOpID = opCount;
								threaddetails.firstBlockID = blockCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								cout << "ERROR: Found duplicate entry for thread " << threadID << " in threadIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.threadIDMap[threadID].printThreadDetails();
								return -1;
							}

							if (detector.blockIDMap.find(blockCount) == detector.threadIDMap.end()) {
								blockdetails.firstOpInBlock = opCount;
								blockdetails.taskID = taskDequeued;
								detector.blockIDMap[blockCount] = blockdetails;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								return -1;
							}

							if (detector.taskIDMap.find(taskDequeued) == detector.taskIDMap.end()) {
								cout << "ERROR: Cannot find entry for task " << taskDequeued << " in taskIDMap\n";
								return -1;
							} else {
								UAFDetector::taskDetails existingEntry = detector.taskIDMap[taskDequeued];
								existingEntry.deqOpID = opCount;
								detector.taskIDMap.erase(detector.taskIDMap.find(taskDequeued));
								detector.taskIDMap[taskDequeued] = existingEntry;
							}
							stackElement.blockID = blockCount;
							stackElement.taskID = taskDequeued;
							stackForNestedTasks.push(stackElement);

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadAndBlockOrder.peek(threadID);

							// deq is the beginning of a block
							blockCount++;
							opdetails.blockID = blockCount;
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
								assert(previousOpInThread.blockID != blockCount);
								assert(existingEntry.blockID != blockCount);
								assert(previousOpInThread.taskID.compare(taskDequeued) != 0);
								assert(existingEntry.taskID.compare(taskDequeued) != 0);
#endif

								existingEntry.nextOpInThread = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							// Update taskID, first-op-in-block, prev-block-in-thread for current block
							if (detector.blockIDMap.find(blockCount) == detector.blockIDMap.end()) {
								if (detector.blockIDMap.find(previousOpInThread.blockID) == detector.blockIDMap.end()) {
									cout << "ERROR: Cannot find entry for block " << previousOpInThread.blockID << " in blockIDMap\n";
									cout << "While examining op " << opCount << endl;
									return -1;
								} else {
									UAFDetector::blockDetails existingEntry = detector.blockIDMap[previousOpInThread.blockID];

#ifdef SANITYCHECK
									// Sanity check: prev-block has same thread as current block
									assert(existingEntry.threadID == threadID);
									assert(previousOpInThread.threadID == threadID);
#endif

									existingEntry.nextBlockInThread = blockCount;
									blockdetails.prevBlockInThread = previousOpInThread.blockID;
									blockdetails.taskID = taskDequeued;
									blockdetails.firstOpInBlock = opCount;
									detector.blockIDMap[blockCount] = blockdetails;
								}

							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								return -1;
							}

							// Map deqopID, parentTask, etc of current task
							if (detector.taskIDMap.find(taskDequeued) == detector.taskIDMap.end()) {
								cout << "ERROR: Cannot find entry for task " << taskDequeued << " in taskIDMap\n";
								return -1;
							} else {
								UAFDetector::taskDetails existingEntry = detector.taskIDMap[taskDequeued];
								existingEntry.deqOpID = opCount;
								existingEntry.firstBlockID = blockCount;

								// Find if there was a pause of some task when this task is dequeued.
								if (!stackForNestedTasks.isEmpty(threadID)) {
									MultiStack::stackElementType topOfTaskStack = stackForNestedTasks.peek(threadID);
									if (topOfTaskStack.opType.compare("pause") == 0) {
										//existingEntry.atomic = false;
										if (detector.taskIDMap.find(topOfTaskStack.taskID) == detector.taskIDMap.end()) {
											cout << "ERROR: Cannot find entry for block " << topOfTaskStack.taskID << " in taskIDMap\n";
											cout << "While examining op " << opCount << "\n";
											return -1;
										}
										existingEntry.parentTask = topOfTaskStack.taskID;
									}
								}

								detector.taskIDMap.erase(detector.taskIDMap.find(taskDequeued));
								detector.taskIDMap[taskDequeued] = existingEntry;
							}

							stackElement.blockID = previousOpInThread.blockID;
							stackElement.taskID = taskDequeued;
							stackForNestedTasks.push(stackElement);
						}
					} else if (match.compare("pause") == 0) {
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
							return -1;
						}

						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadAndBlockOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for pause " << opCount
								 << " on stack\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;
							opdetails.taskID = task;

							// Update firstop, firstblock and exitloopblock for current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								threaddetails.firstOpID = opCount;
								threaddetails.firstBlockID = blockCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								cout << "ERROR: Found duplicate entry for thread " << threadID << " in threadIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.threadIDMap[threadID].printThreadDetails();
								return -1;
							}

							if (detector.blockIDMap.find(blockCount) == detector.threadIDMap.end()) {
								blockdetails.firstOpInBlock = opCount;
								blockdetails.taskID = task;
								blockdetails.lastOpInBlock = opCount;
								detector.blockIDMap[blockCount] = blockdetails;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								return -1;
							}

							if (detector.taskIDMap.find(task) == detector.taskIDMap.end()) {
								UAFDetector::taskDetails taskdetails;
								taskdetails.firstPauseOpID = opCount;
								taskdetails.atomic = false;
								taskdetails.firstBlockID = blockCount;
								taskdetails.pauseSet.insert(opCount);
								detector.taskIDMap[task] = taskdetails;
							} else {
								cout << "ERROR: Found duplicate entry for task " << task << " in taskIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.taskIDMap[task].printTaskDetails();
								return -1;
							}
							stackElement.blockID = blockCount;
							stackElement.taskID = task;
							stackForNestedTasks.push(stackElement);

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadAndBlockOrder.peek(threadID);
							// In the case of pause, the previous op in thread is in the same task as pause.
							// So no need to look separately in the task stack.

							opdetails.taskID = task;
							opdetails.blockID = previousOpInThread.blockID;
							opdetails.prevOpInBlock = previousOpInThread.opID;
							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								cout << "While examining op " << opCount;
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

								existingEntry.nextOpInBlock = opCount;
								existingEntry.nextOpInTask = opCount;
								existingEntry.nextOpInThread = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							if (detector.blockIDMap.find(previousOpInThread.blockID) == detector.blockIDMap.end()) {
								cout << "ERROR: Cannot find entry for block " << previousOpInThread.blockID << " in blockIDMap\n";
								cout << "While examining op " << opCount;
								return -1;
							} else {
								UAFDetector::blockDetails existingEntry = detector.blockIDMap[previousOpInThread.blockID];
								existingEntry.lastOpInBlock = opCount;
								detector.blockIDMap.erase(detector.blockIDMap.find(previousOpInThread.blockID));
								detector.blockIDMap[previousOpInThread.blockID] = existingEntry;
							}

							if (detector.taskIDMap.find(task) == detector.taskIDMap.end()) {
								cout << "ERROR: Cannot find entry for task " << task << " in taskIDMap\n";
								cout << "While examining op " << opCount;
								return -1;
							} else {
								UAFDetector::taskDetails existingEntry = detector.taskIDMap[task];
								if (!stackForNestedTasks.isEmpty(threadID)) {
									MultiStack::stackElementType topOfTaskStack = stackForNestedTasks.peek(threadID);
									if (topOfTaskStack.opType.compare("deq") == 0) {
										existingEntry.firstPauseOpID = opCount;
										existingEntry.atomic = false;
									}
								}
								existingEntry.pauseSet.insert(opCount);
								detector.taskIDMap.erase(detector.taskIDMap.find(task));
								detector.taskIDMap[task] = existingEntry;
							}

							if (detector.pauseResumeResetOps.find(opCount) == detector.pauseResumeResetOps.end()) {
								detector.pauseResumeResetOps[opCount] = sharedVariable;
							} else {
								cout << "ERROR: Found duplicate entry for op " << opCount << " in pauseResumeResetOps\n";
								cout << "Duplicate entry: " << detector.pauseResumeResetOps[opCount];
								return -1;
							}

							stackElement.blockID = previousOpInThread.blockID;
							stackElement.taskID = task;
							stackForNestedTasks.push(stackElement);
						}
					} else if (match.compare("reset") == 0) {
						// Obtain 2nd & 3rd argument (i.e., current task & shared variable) of reset
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
							return -1;
						}

						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadAndBlockOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for reset " << opCount
								 << " on stack\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;
							opdetails.taskID = task;

							// Update firstop, firstblock and exitloopblock for current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								threaddetails.firstOpID = opCount;
								threaddetails.firstBlockID = blockCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								cout << "ERROR: Found duplicate entry for thread " << threadID << " in threadIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.threadIDMap[threadID].printThreadDetails();
								return -1;
							}

							if (detector.blockIDMap.find(blockCount) == detector.threadIDMap.end()) {
								blockdetails.firstOpInBlock = opCount;
								blockdetails.taskID = task;
								detector.blockIDMap[blockCount] = blockdetails;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								return -1;
							}

							if (detector.taskIDMap.find(task) == detector.taskIDMap.end()) {
								UAFDetector::taskDetails taskdetails;
								taskdetails.firstBlockID = blockCount;
								taskdetails.resetSet.insert(opCount);
								detector.taskIDMap[task] = taskdetails;
							} else {
								cout << "ERROR: Found duplicate entry for task " << task << " in taskIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.taskIDMap[task].printTaskDetails();
								return -1;
							}
							stackElement.blockID = blockCount;
							stackElement.taskID = task;
							stackForNestedTasks.push(stackElement);

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadAndBlockOrder.peek(threadID);
							// In the case of reset, the previous op in thread is the previous op in task.
							// So no need to look separately in task stack.

							opdetails.taskID = task;
							opdetails.blockID = previousOpInThread.blockID;
							opdetails.prevOpInBlock = previousOpInThread.opID;
							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								cout << "While examining op " << opCount;
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
								existingEntry.nextOpInBlock = opCount;
								existingEntry.nextOpInTask = opCount;
								existingEntry.nextOpInThread = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							if (detector.blockIDMap.find(previousOpInThread.blockID) == detector.blockIDMap.end()) {
								cout << "ERROR: Cannot find entry for block " << previousOpInThread.blockID << " in blockIDMap\n";
								cout << "While examining op " << opCount;
								return -1;
							} else {
								UAFDetector::blockDetails existingEntry = detector.blockIDMap[previousOpInThread.blockID];
								existingEntry.lastOpInBlock = opCount;

#ifdef SANITYCHECK
								// Sanity check: the current block has same task ID as current op
								assert(existingEntry.taskID.compare(task) == 0);
#endif

								detector.blockIDMap.erase(detector.blockIDMap.find(previousOpInThread.blockID));
								detector.blockIDMap[previousOpInThread.blockID] = existingEntry;
							}

							if (detector.taskIDMap.find(task) == detector.taskIDMap.end()) {
								cout << "ERROR: Cannot find entry for task " << task << " in taskIDMap\n";
								cout << "While examining op " << opCount;
								return -1;
							} else {
								UAFDetector::taskDetails existingEntry = detector.taskIDMap[task];
								existingEntry.resetSet.insert(opCount);
								detector.taskIDMap.erase(detector.taskIDMap.find(task));
								detector.taskIDMap[task] = existingEntry;
							}

							if (detector.pauseResumeResetOps.find(opCount) == detector.pauseResumeResetOps.end()) {
								detector.pauseResumeResetOps[opCount] = sharedVariable;
							} else {
								cout << "ERROR: Found duplicate entry for op " << opCount << " in pauseResumeResetOps\n";
								cout << "Duplicate entry: " << detector.pauseResumeResetOps[opCount];
								return -1;
							}

							stackElement.blockID = previousOpInThread.blockID;
							stackElement.taskID = task;
						}
					} else if (match.compare("resume") == 0) {
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
							return -1;
						}

						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadAndBlockOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for reset " << opCount
								 << " on stack\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;
							opdetails.taskID = task;

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadAndBlockOrder.peek(threadID);
							// In the case of resume, the previous op in thread is not the previous op in the task.
							// So, get top of task stack to obtain prev op in task.
							MultiStack::stackElementType previousOpInTask = stackForTaskOrder.peek(threadID);

							// resume is the beginning of a block
							blockCount++;
							opdetails.taskID = task;
							opdetails.blockID = blockCount;

							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								cout << "While examining op " << opCount;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
								assert(previousOpInThread.blockID != blockCount);
								assert(existingEntry.blockID != blockCount);
#endif

								existingEntry.nextOpInThread = opCount;
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
								assert(previousOpInTask.blockID != blockCount);
								assert(existingEntry.blockID != blockCount);
#endif

								existingEntry.nextOpInTask = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInTask.opID));
								detector.opIDMap[previousOpInTask.opID] = existingEntry;
							}

						}
					} else if (match.compare("end") == 0) {
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
						if (stackForThreadAndBlockOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for end " << opCount
								 << " on stackForThreadOrder\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;
							opdetails.taskID = task;

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadAndBlockOrder.peek(threadID);
							// In the case of end, the previous op in thread is the previous op in task.
							// So no need to look separately in task stack.

							opdetails.taskID = task;
							opdetails.blockID = previousOpInThread.blockID;
							opdetails.prevOpInBlock = previousOpInThread.opID;
							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								cout << "While examining op " << opCount;
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

								existingEntry.nextOpInBlock = opCount;
								existingEntry.nextOpInTask = opCount;
								existingEntry.nextOpInThread = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

						}
					} else if (match.compare("fork") == 0) {
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
						if (stackForThreadAndBlockOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for fork " << opCount
								 << " on stackForThreadOrder\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadAndBlockOrder.peek(threadID);
							// In the case of fork, the previous op in thread is the previous op in task.
							// So no need to look separately in task stack.

							opdetails.taskID = previousOpInThread.taskID;
							opdetails.blockID = previousOpInThread.blockID;
							opdetails.prevOpInBlock = previousOpInThread.opID;
							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								cout << "While examining op " << opCount;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								existingEntry.nextOpInBlock = opCount;
								existingEntry.nextOpInTask = opCount;
								existingEntry.nextOpInThread = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}
						}
					} else if (match.compare("join") == 0) {
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
						if (stackForThreadAndBlockOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for join " << opCount
								 << " on stackForThreadOrder\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadAndBlockOrder.peek(threadID);
							// In the case of join, the previous op in thread is the previous op in task.
							// So no need to look separately in task stack.

							opdetails.taskID = previousOpInThread.taskID;
							opdetails.blockID = previousOpInThread.blockID;
							opdetails.prevOpInBlock = previousOpInThread.opID;
							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								cout << "While examining op " << opCount;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								existingEntry.nextOpInBlock = opCount;
								existingEntry.nextOpInTask = opCount;
								existingEntry.nextOpInThread = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
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

						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadAndBlockOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for alloc " << opCount
								 << " on stackForThreadOrder\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadAndBlockOrder.peek(threadID);
							// In the case of alloc, the previous op in thread is the previous op in task.
							// So no need to look separately in task stack.

							opdetails.taskID = previousOpInThread.taskID;
							opdetails.blockID = previousOpInThread.blockID;
							opdetails.prevOpInBlock = previousOpInThread.opID;
							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								cout << "While examining op " << opCount;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								existingEntry.nextOpInBlock = opCount;
								existingEntry.nextOpInTask = opCount;
								existingEntry.nextOpInThread = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
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

						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadAndBlockOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for free " << opCount
								 << " on stackForThreadOrder\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadAndBlockOrder.peek(threadID);
							// In the case of free, the previous op in thread is the previous op in task.
							// So no need to look separately in task stack.

							opdetails.taskID = previousOpInThread.taskID;
							opdetails.blockID = previousOpInThread.blockID;
							opdetails.prevOpInBlock = previousOpInThread.opID;
							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								cout << "While examining op " << opCount;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								existingEntry.nextOpInBlock = opCount;
								existingEntry.nextOpInTask = opCount;
								existingEntry.nextOpInThread = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
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

						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadAndBlockOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for read " << opCount
								 << " on stackForThreadOrder\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadAndBlockOrder.peek(threadID);
							// In the case of read, the previous op in thread is the previous op in task.
							// So no need to look separately in task stack.

							opdetails.taskID = previousOpInThread.taskID;
							opdetails.blockID = previousOpInThread.blockID;
							opdetails.prevOpInBlock = previousOpInThread.opID;
							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								cout << "While examining op " << opCount;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								existingEntry.nextOpInBlock = opCount;
								existingEntry.nextOpInTask = opCount;
								existingEntry.nextOpInThread = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
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

						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadAndBlockOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for write " << opCount
								 << " on stackForThreadOrder\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadAndBlockOrder.peek(threadID);
							// In the case of fork, the previous op in thread is the previous op in task.
							// So no need to look separately in task stack.

							opdetails.taskID = previousOpInThread.taskID;
							opdetails.blockID = previousOpInThread.blockID;
							opdetails.prevOpInBlock = previousOpInThread.opID;
							if (detector.opIDMap.find(previousOpInThread.opID) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find entry for op " << previousOpInThread.opID << " in opIDMap\n";
								cout << "While examining op " << opCount;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								existingEntry.nextOpInBlock = opCount;
								existingEntry.nextOpInTask = opCount;
								existingEntry.nextOpInThread = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

						}

					}

					if (detector.opIDMap.find(opCount) == detector.opIDMap.end()) {
						detector.opIDMap[opCount] = opdetails;
					} else {
						cout << "ERROR: Found duplicate entry for " << opCount << " in opIDMap\n";
						cout << "While examining " << OpString << "\n";
						cout << "Duplicate entry:\n";
						detector.opIDMap[opCount].printOpDetails();
						return -1;
					}
					break;
				}
			}
		}
	}
#if 0

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

							MultiStack::stackElementType top = stack4.peek(opdetails.threadID);
							if (!stack4.isBottom(top)) {
								if (top.opType.compare("pause") == 0) {
									if (detector.taskIDMap.find(opdetails.taskID) == detector.taskIDMap.end()) {
										UAFDetector::taskDetails details;
										details.parentTask = top.taskID;
										detector.taskIDMap[opdetails.taskID] = details;
									} else {
										UAFDetector::taskDetails existingDetails = detector.taskIDMap.find(opdetails.taskID)->second;
										existingDetails.parentTask = top.taskID;
										detector.taskIDMap.erase(detector.taskIDMap.find(opdetails.taskID));
										detector.taskIDMap[opdetails.taskID] = existingDetails;
									}
								}
							} else {
								detector.taskIDMap[opdetails.taskID].parentTask = "";
								if (detector.taskIDMap.find(opdetails.taskID) == detector.taskIDMap.end()) {
									UAFDetector::taskDetails details;
									details.parentTask = "";
									detector.taskIDMap[opdetails.taskID] = details;
								} else {
									UAFDetector::taskDetails existingDetails = detector.taskIDMap.find(opdetails.taskID)->second;
									existingDetails.parentTask = "";
									detector.taskIDMap.erase(detector.taskIDMap.find(opdetails.taskID));
									detector.taskIDMap[opdetails.taskID] = existingDetails;
								}
							}
							stack4.push(element); // push deq to stack4.
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

							top = stack4.peek(opdetails.threadID);
							if (!stack4.isBottom(top)) {
								if (top.opType.compare("deq") == 0) {
									stack4.pop(opdetails.threadID);
								}
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
										detector.taskIDMap[opdetails.taskID] = oldDetails;
									} else {
										cout << "ERROR: Cannot find map entry for task " << opdetails.taskID << endl;
										return -1;
									}
								}

								MultiStack::stackElementType element;
								element.opID = opCount;
								element.opType = "pause";
								element.taskID = opdetails.taskID;
								element.threadID = opdetails.threadID;
								stack4.push(element);
							}

							if (match.compare("resume") == 0) {
								MultiStack::stackElementType top = stack4.peek(opdetails.threadID);
								if (top.opType.compare("pause") == 0) {
									stack4.pop(opdetails.threadID);
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
							if (stack3.isBottom(top)) {
								cout << "ERROR: Found stack bottom when computing next op in thread\n";
							}
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
						}

						MultiStack::stackElementType top = stack5.peek(opdetails.threadID);
						if (stack5.isBottom(top)) {
							MultiStack::stackElementType element;
							element.opID = opCount;
							element.opType = opdetails.opType;
							element.taskID = opdetails.taskID;
							element.threadID = opdetails.threadID;
							stack5.push(element);

							if (match.compare("threadinit") != 0) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								if (detector.threadIDMap.find(opdetails.threadID) == detector.threadIDMap.end())
									detector.threadIDMap[opdetails.threadID] = threaddetails;
								else {
									cout << "ERROR: thread " << opdetails.threadID << " already has an entry in threadIDMap\n";
								}
							}
						}

						// Obtaining sets of each kind of operations in the trace.
						if (match.compare("threadinit") == 0) {
							detector.threadinitSet.insert(opCount);

							long long threadID = opdetails.threadID;
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.threadinitOpID = opCount;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else if (detector.threadIDMap[threadID].threadinitOpID != -1 && detector.threadIDMap[threadID].threadinitOpID != opCount) {
								cout << "ERROR: threadinit already seen for thread " << threadID << " at op " << detector.threadIDMap[threadID].threadinitOpID << endl;
								return -1;
							} else {
								UAFDetector::threadDetails threaddetails = detector.threadIDMap.find(threadID)->second;
								threaddetails.threadinitOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = threaddetails;
							}
						} else if (match.compare("threadexit") == 0) {
							detector.threadexitSet.insert(opCount);

							long long threadID = opdetails.threadID;
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.threadexitOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else if (detector.threadIDMap[threadID].threadexitOpID != -1 && detector.threadIDMap[threadID].threadexitOpID != opCount) {
								cout << "ERROR: threadexit already seen for thread " << threadID << " at op " << detector.threadIDMap[threadID].threadexitOpID << endl;
								return -1;
							} else {
								UAFDetector::threadDetails threaddetails = detector.threadIDMap.find(threadID)->second;
								threaddetails.threadexitOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = threaddetails;
							}
						} else if (match.compare("enterloop") == 0) {
							detector.enterloopSet.insert(opCount);

							long long threadID = opdetails.threadID;
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.enterloopOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else if (detector.threadIDMap[threadID].enterloopOpID != -1 && detector.threadIDMap[threadID].enterloopOpID != opCount) {
								cout << "ERROR: enterloop already seen for thread " << threadID << " at op " << detector.threadIDMap[threadID].enterloopOpID << endl;
								return -1;
							} else {
								UAFDetector::threadDetails threaddetails = detector.threadIDMap.find(threadID)->second;
								threaddetails.enterloopOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = threaddetails;
							}
						} else if (match.compare("exitloop") == 0) {
							detector.exitloopSet.insert(opCount);

							long long threadID = opdetails.threadID;
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.exitloopOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else if (detector.threadIDMap[threadID].exitloopOpID != -1 && detector.threadIDMap[threadID].exitloopOpID != opCount) {
								cout << "ERROR: exitloop already seen for thread " << threadID << " at op " << detector.threadIDMap[threadID].exitloopOpID << endl;
								return -1;
							} else {
								UAFDetector::threadDetails threaddetails = detector.threadIDMap.find(threadID)->second;
								threaddetails.exitloopOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = threaddetails;
							}
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
								UAFDetector::threadDetails threaddetails;
								threaddetails.forkOpID = opCount;
								detector.threadIDMap[targetThread] = threaddetails;
							} else if (detector.threadIDMap[targetThread].forkOpID != -1 && detector.threadIDMap[targetThread].forkOpID != opCount) {
								cout << "ERROR: fork already seen for thread " << targetThread << " at op " << detector.threadIDMap[targetThread].forkOpID << endl;
								return -1;
							} else {
								UAFDetector::threadDetails threaddetails = detector.threadIDMap.find(targetThread)->second;
								threaddetails.forkOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(targetThread));
								detector.threadIDMap[targetThread] = threaddetails;
							}

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
								UAFDetector::threadDetails threaddetails;
								threaddetails.joinOpID = opCount;
								detector.threadIDMap[targetThread] = threaddetails;
							} else if (detector.threadIDMap[targetThread].joinOpID != -1 && detector.threadIDMap[targetThread].joinOpID != opCount) {
								cout << "ERROR: join already seen for thread " << targetThread << " at op " << detector.threadIDMap[targetThread].joinOpID << endl;
								return -1;
							} else {
								UAFDetector::threadDetails threaddetails = detector.threadIDMap.find(targetThread)->second;
								threaddetails.joinOpID = opCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(targetThread));
								detector.threadIDMap[targetThread] = threaddetails;
							}
#ifdef LOCKS
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

							if (detector.lockIDMap.find(acquireopdetails.lockID) == detector.lockIDMap.end()) {
								UAFDetector::lockDetails details;
								details.acquireOps.insert(opCount);
								detector.lockIDMap[acquireopdetails.lockID] = details;
							} else {
								UAFDetector::lockDetails existingDetails;
								existingDetails = detector.lockIDMap.find(acquireopdetails.lockID)->second;
								existingDetails.acquireOps.insert(opCount);
								detector.lockIDMap.erase(detector.lockIDMap.find(acquireopdetails.lockID));
								detector.lockIDMap[acquireopdetails.lockID] = existingDetails;
							}
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

							// Add this release op entry to the set.
							detector.releaseSet[opCount] = releaseopdetails;

							if (detector.lockIDMap.find(releaseopdetails.lockID) == detector.lockIDMap.end()) {
								UAFDetector::lockDetails details;
								details.releaseOps.insert(opCount);
								detector.lockIDMap[releaseopdetails.lockID] = details;
							} else {
								UAFDetector::lockDetails existingDetails;
								existingDetails = detector.lockIDMap.find(releaseopdetails.lockID)->second;
								existingDetails.releaseOps.insert(opCount);
								detector.lockIDMap.erase(detector.lockIDMap.find(releaseopdetails.lockID));
								detector.lockIDMap[releaseopdetails.lockID] = existingDetails;
							}
						} else if (match.compare("entermonitor") == 0) {
							// Obtain two arguments of acquire.
							UAFDetector::acquireAndReleaseOpDetails entermonitoropdetails;
							unsigned j = typePos + 1;
							for (; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0){
									entermonitoropdetails.currThreadID = atoi(m1.c_str());
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0){
									entermonitoropdetails.lockID = m1;
									break;
								}
							}

							// Add this entermonitor op entry to the set.
							detector.entermonitorSet[opCount] = entermonitoropdetails;

							if (detector.lockIDMap.find(entermonitoropdetails.lockID) == detector.lockIDMap.end()) {
								UAFDetector::lockDetails details;
								details.entermonitorOps.insert(opCount);
								detector.lockIDMap[entermonitoropdetails.lockID] = details;
							} else {
								UAFDetector::lockDetails existingDetails;
								existingDetails = detector.lockIDMap.find(entermonitoropdetails.lockID)->second;
								existingDetails.entermonitorOps.insert(opCount);
								detector.lockIDMap.erase(detector.lockIDMap.find(entermonitoropdetails.lockID));
								detector.lockIDMap[entermonitoropdetails.lockID] = existingDetails;
							}
						} else if (match.compare("exitmonitor") == 0) {
							// Obtain two arguments of exitmonitor.
							UAFDetector::acquireAndReleaseOpDetails exitmonitoropdetails;
							unsigned j = typePos + 1;
							for (; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0){
									exitmonitoropdetails.currThreadID = atoi(m1.c_str());
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0){
									exitmonitoropdetails.lockID = m1;
									break;
								}
							}

							// Add this exitmonitor op entry to the set.
							detector.exitmonitorSet[opCount] = exitmonitoropdetails;

							if (detector.lockIDMap.find(exitmonitoropdetails.lockID) == detector.lockIDMap.end()) {
								UAFDetector::lockDetails details;
								details.exitmonitorOps.insert(opCount);
								detector.lockIDMap[exitmonitoropdetails.lockID] = details;
							} else {
								UAFDetector::lockDetails existingDetails;
								existingDetails = detector.lockIDMap.find(exitmonitoropdetails.lockID)->second;
								existingDetails.exitmonitorOps.insert(opCount);
								detector.lockIDMap.erase(detector.lockIDMap.find(exitmonitoropdetails.lockID));
								detector.lockIDMap[exitmonitoropdetails.lockID] = existingDetails;
							}
						} else if (match.compare("wait") == 0) {
							// Obtain two arguments of acquire.
							UAFDetector::acquireAndReleaseOpDetails waitopdetails;
							unsigned j = typePos + 1;
							for (; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0){
									waitopdetails.currThreadID = atoi(m1.c_str());
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0){
									waitopdetails.lockID = m1;
									break;
								}
							}

							// Add this wait op entry to the set.
							detector.waitSet[opCount] = waitopdetails;

							if (detector.lockIDMap.find(waitopdetails.lockID) == detector.lockIDMap.end()) {
								UAFDetector::lockDetails details;
								details.waitOps.insert(opCount);
								detector.lockIDMap[waitopdetails.lockID] = details;
							} else {
								UAFDetector::lockDetails existingDetails;
								existingDetails = detector.lockIDMap.find(waitopdetails.lockID)->second;
								existingDetails.waitOps.insert(opCount);
								detector.lockIDMap.erase(detector.lockIDMap.find(waitopdetails.lockID));
								detector.lockIDMap[waitopdetails.lockID] = existingDetails;
							}
						} else if (match.compare("notify") == 0) {
							// Obtain two arguments of notify.
							UAFDetector::acquireAndReleaseOpDetails notifyopdetails;
							unsigned j = typePos + 1;
							for (; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0){
									notifyopdetails.currThreadID = atoi(m1.c_str());
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0){
									notifyopdetails.lockID = m1;
									break;
								}
							}

							// Add this notify op entry to the set.
							detector.notifySet[opCount] = notifyopdetails;

							if (detector.lockIDMap.find(notifyopdetails.lockID) == detector.lockIDMap.end()) {
								UAFDetector::lockDetails details;
								details.notifyOps.insert(opCount);
								detector.lockIDMap[notifyopdetails.lockID] = details;
							} else {
								UAFDetector::lockDetails existingDetails;
								existingDetails = detector.lockIDMap.find(notifyopdetails.lockID)->second;
								existingDetails.notifyOps.insert(opCount);
								detector.lockIDMap.erase(detector.lockIDMap.find(notifyopdetails.lockID));
								detector.lockIDMap[notifyopdetails.lockID] = existingDetails;
							}
						} else if (match.compare("notifyall") == 0) {
							// Obtain two arguments of notifyAll.
							UAFDetector::acquireAndReleaseOpDetails notifyAllopdetails;
							unsigned j = typePos + 1;
							for (; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0){
									notifyAllopdetails.currThreadID = atoi(m1.c_str());
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0){
									notifyAllopdetails.lockID = m1;
									break;
								}
							}

							// Add this notifyAll op entry to the set.
							detector.notifyAllSet[opCount] = notifyAllopdetails;

							if (detector.lockIDMap.find(notifyAllopdetails.lockID) == detector.lockIDMap.end()) {
								UAFDetector::lockDetails details;
								details.notifyAllOps.insert(opCount);
								detector.lockIDMap[notifyAllopdetails.lockID] = details;
							} else {
								UAFDetector::lockDetails existingDetails;
								existingDetails = detector.lockIDMap.find(notifyAllopdetails.lockID)->second;
								existingDetails.notifyAllOps.insert(opCount);
								detector.lockIDMap.erase(detector.lockIDMap.find(notifyAllopdetails.lockID));
								detector.lockIDMap[notifyAllopdetails.lockID] = existingDetails;
							}
#endif
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

							if (detector.allocIDMap.find(opCount) == detector.allocIDMap.end()) {
								UAFDetector::allocOpDetails dummyDetails;
								detector.allocIDMap[opCount] = dummyDetails;
							} else {
								cout << "ERROR: Found duplicate entry for alloc " << opCount << " in allocIDMap\n";
								return -1;
							}
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

							std::stringstream str;
							str << freeopdetails.startingAddress;
							long long baseAddressFree;
							str >> std::hex >> baseAddressFree;
							long long endAddressFree = baseAddressFree + freeopdetails.range;

							for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.allocSet.begin(); it != detector.allocSet.end(); it++) {
								string startAddress = detector.allocSet[it->first].startingAddress;
								std::stringstream str1;
								str1 << startAddress;
								long long baseAddress;
								str1 >> std::hex >> baseAddress;
								long long endAddress = baseAddress + detector.allocSet[it->first].range;

								if (baseAddress <= baseAddressFree && endAddressFree <= endAddress) {
									// Free block within alloc block
									if (detector.allocIDMap.find(it->first) == detector.allocIDMap.end()){
										cout << "ERROR: Cannot find alloc " << it->first << " in allocIDMap while adding free " << opCount << endl;
										return -1;
									} else {
										UAFDetector::allocOpDetails allocdetails;
										allocdetails = detector.allocIDMap.find(it->first)->second;
										allocdetails.freeOps.insert(opCount);
										detector.allocIDMap.erase(detector.allocIDMap.find(it->first));
										detector.allocIDMap[it->first] = allocdetails;

										if (detector.freeIDMap.find(opCount) == detector.freeIDMap.end()) {
											UAFDetector::freeOpDetails freedetails;
											freedetails.allocOpID = it->first;
#ifdef ACCESS
											for (set<long long>::iterator accessIt = allocdetails.accessOps.begin(); accessIt != allocdetails.accessOps.end(); accessIt++) {
												string accessAddress = detector.accessSet[*accessIt].startingAddress;
												std::stringstream accessStr;
												accessStr << accessAddress;
												long long accessStartAddress;
												accessStr >> std::hex >> accessStartAddress;

												if (baseAddressFree <= accessStartAddress && accessStartAddress <= endAddressFree) {
													freedetails.accessOps.insert(*accessIt);
												}
											}
#else
											for (set<long long>::iterator readIt = allocdetails.readOps.begin(); readIt != allocdetails.readOps.end(); readIt++) {
												string readAddress = detector.readSet[*readIt].startingAddress;
												std::stringstream readStr;
												readStr << readAddress;
												long long readStartAddress;
												readStr >> std::hex >> readStartAddress;

												if (baseAddressFree <= readStartAddress && readStartAddress <= endAddressFree) {
													freedetails.readOps.insert(*readIt);
												}
											}

											for (set<long long>::iterator writeIt = allocdetails.writeOps.begin(); writeIt != allocdetails.writeOps.end(); writeIt++) {
												string writeAddress = detector.writeSet[*writeIt].startingAddress;
												std::stringstream writeStr;
												writeStr << writeAddress;
												long long writeStartAddress;
												writeStr >> std::hex >> writeStartAddress;

												if (baseAddressFree <= writeStartAddress && writeStartAddress <= endAddressFree) {
													freedetails.writeOps.insert(*writeIt);
												}
											}
#endif

											for (set<long long>::iterator incIt = allocdetails.incOps.begin(); incIt != allocdetails.incOps.end(); incIt++) {
												string incAddress = detector.incSet[*incIt].startingAddress;
												std::stringstream incStr;
												incStr << incAddress;
												long long incStartAddress;
												incStr >> std::hex >> incStartAddress;

												if (baseAddressFree <= incStartAddress && incStartAddress <= endAddressFree) {
													freedetails.incOps.insert(*incIt);
												}
											}

											for (set<long long>::iterator decIt = allocdetails.decOps.begin(); decIt != allocdetails.decOps.end(); decIt++) {
												string decAddress = detector.decSet[*decIt].startingAddress;
												std::stringstream decStr;
												decStr << decAddress;
												long long decStartAddress;
												decStr >> std::hex >> decStartAddress;

												if (baseAddressFree <= decStartAddress && decStartAddress <= endAddressFree) {
													freedetails.decOps.insert(*decIt);
												}
											}
#ifdef DEBUGLINES
											cout << "Assigning to freeIDMap " << opCount << endl;
#endif
											detector.freeIDMap[opCount] = freedetails;
										} else {
											cout << "ERROR: Found duplicate entry for free " << opCount << " in freeIDMap\n";
											return -1;
										}
									}
								}
							}
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

							std::stringstream str;
							str << incopdetails.startingAddress;
							long long baseAddress;
							str >> std::hex >> baseAddress;
							long long endAddress = baseAddress + incopdetails.range;

							for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.allocSet.begin(); it != detector.allocSet.end(); it++) {
								std::stringstream str1;
								str1 << detector.allocSet[it->first].startingAddress;
								long long allocBaseAddress;
								str1 >> std::hex >> allocBaseAddress;
								long long allocEndAddress = allocBaseAddress + detector.allocSet[it->first].range;

								if (allocBaseAddress <= baseAddress && endAddress <= allocEndAddress) {
									UAFDetector::allocOpDetails existingDetails;
									if (detector.allocIDMap.find(it->first) == detector.allocIDMap.end()) {
										cout << "ERROR: Cannot find alloc " << it->first << " in the allocIDMap\n";
									} else {
										existingDetails = detector.allocIDMap.find(it->first)->second;
										existingDetails.incOps.insert(opCount);
										detector.allocIDMap.erase(detector.allocIDMap.find(it->first));
										detector.allocIDMap[it->first] = existingDetails;
									}
								}
							}

							for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.freeSet.begin(); it != detector.freeSet.end(); it++) {
								std::stringstream str1;
								str1 << detector.freeSet[it->first].startingAddress;
								long long freeBaseAddress;
								str1 >> std::hex >> freeBaseAddress;
								long long freeEndAddress = freeBaseAddress + detector.freeSet[it->first].range;

								if (freeBaseAddress <= baseAddress && endAddress <= freeEndAddress) {
									UAFDetector::freeOpDetails existingDetails;
									if (detector.freeIDMap.find(it->first) == detector.freeIDMap.end()) {
										cout << "ERROR: Cannot find free " << it->first << " in the freeIDMap\n";
									} else {
										existingDetails = detector.freeIDMap.find(it->first)->second;
										existingDetails.incOps.insert(opCount);
										detector.freeIDMap.erase(detector.freeIDMap.find(it->first));
#ifdef DEBUGLINES
											cout << "Assigning to freeIDMap " << it->first << endl;
#endif
										detector.freeIDMap[it->first] = existingDetails;
									}
								}
							}
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

							std::stringstream str;
							str << decopdetails.startingAddress;
							long long baseAddress;
							str >> std::hex >> baseAddress;
							long long endAddress = baseAddress + decopdetails.range;

							for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.allocSet.begin(); it != detector.allocSet.end(); it++) {
								std::stringstream str1;
								str1 << it->second.startingAddress;
								long long allocBaseAddress;
								str1 >> std::hex >> allocBaseAddress;
								long long allocEndAddress = allocBaseAddress + it->second.range;

								if (allocBaseAddress <= baseAddress && endAddress <= allocEndAddress) {
									UAFDetector::allocOpDetails existingDetails;
									if (detector.allocIDMap.find(it->first) == detector.allocIDMap.end()) {
										cout << "ERROR: Cannot find alloc " << it->first << " in the allocIDMap\n";
									} else {
										existingDetails = detector.allocIDMap.find(it->first)->second;
										existingDetails.decOps.insert(opCount);
										detector.allocIDMap.erase(detector.allocIDMap.find(it->first));
										detector.allocIDMap[it->first] = existingDetails;
									}
								}
							}

							for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.freeSet.begin(); it != detector.freeSet.end(); it++) {
								std::stringstream str1;
								str1 << detector.freeSet[it->first].startingAddress;
								long long freeBaseAddress;
								str1 >> std::hex >> freeBaseAddress;
								long long freeEndAddress = freeBaseAddress + detector.freeSet[it->first].range;

								if (freeBaseAddress <= baseAddress && endAddress <= freeEndAddress) {
									UAFDetector::freeOpDetails existingDetails;
									if (detector.freeIDMap.find(it->first) == detector.freeIDMap.end()) {
										cout << "ERROR: Cannot find free " << it->first << " in the freeIDMap\n";
									} else {
										existingDetails = detector.freeIDMap.find(it->first)->second;
										existingDetails.decOps.insert(opCount);
										detector.freeIDMap.erase(detector.freeIDMap.find(it->first));
#ifdef DEBUGLINES
											cout << "Assigning to freeIDMap " << it->first << endl;
#endif
										detector.freeIDMap[it->first] = existingDetails;
									}
								}
							}

#ifdef ACCESS
						} else if (match.compare("access") == 0) {
							UAFDetector::memoryOpDetails accessopdetails;
							unsigned j = typePos + 1;
							for (; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									accessopdetails.threadID = atoi(m1.c_str());
									break;
								}
							}
							for (j=j+1; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									accessopdetails.startingAddress = m1;
									break;
								}
							}
							detector.accessSet[opCount] = accessopdetails;

							std::stringstream str;
							str << accessopdetails.startingAddress;
							long long baseAddress;
							str >> std::hex >> baseAddress;

							bool allocFound = false;
							for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.allocSet.begin(); it != detector.allocSet.end(); it++) {
								std::stringstream str1;
								str1 << it->second.startingAddress;
								long long allocBaseAddress;
								str1 >> std::hex >> allocBaseAddress;
								long long allocEndAddress = allocBaseAddress + it->second.range;

								if (allocBaseAddress <= baseAddress && baseAddress <= allocEndAddress) {
									UAFDetector::allocOpDetails existingDetails;
									if (detector.allocIDMap.find(it->first) == detector.allocIDMap.end()) {
										cout << "ERROR: Cannot find alloc " << it->first << " in the allocIDMap\n";
									} else {
										existingDetails = detector.allocIDMap.find(it->first)->second;
										existingDetails.accessOps.insert(opCount);
										detector.allocIDMap.erase(detector.allocIDMap.find(it->first));
										detector.allocIDMap[it->first] = existingDetails;
										allocFound = true;
									}
								}
							}
							if (!allocFound) {
								cout << "ERROR: Cannot find alloc for access op " << opCount << endl;
								return -1;
							}

							for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.freeSet.begin(); it != detector.freeSet.end(); it++) {
								std::stringstream str1;
								str1 << it->second.startingAddress;
								long long freeBaseAddress;
								str1 >> std::hex >> freeBaseAddress;
								long long freeEndAddress = freeBaseAddress + it->second.range;

								if (freeBaseAddress <= baseAddress && baseAddress <= freeEndAddress) {
									UAFDetector::freeOpDetails existingDetails;
									if (detector.freeIDMap.find(it->first) == detector.freeIDMap.end()) {
										cout << "ERROR: Cannot find free " << it->first << " in the freeIDMap\n";
									} else {
										existingDetails = detector.freeIDMap.find(it->first)->second;
										existingDetails.accessOps.insert(opCount);
										detector.freeIDMap.erase(detector.freeIDMap.find(it->first));
#ifdef DEBUGLINES
											cout << "Assigning to freeIDMap " << it->first << endl;
#endif
										detector.freeIDMap[it->first] = existingDetails;
									}
								}
							}
#else
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

							std::stringstream str;
							str << readopdetails.startingAddress;
							long long baseAddress;
							str >> std::hex >> baseAddress;

							bool allocFound = false;
							for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.allocSet.begin(); it != detector.allocSet.end(); it++) {
								std::stringstream str1;
								str1 << it->second.startingAddress;
								long long allocBaseAddress;
								str1 >> std::hex >> allocBaseAddress;
								long long allocEndAddress = allocBaseAddress + it->second.range;

								if (allocBaseAddress <= baseAddress && baseAddress <= allocEndAddress) {
									UAFDetector::allocOpDetails existingDetails;
									if (detector.allocIDMap.find(it->first) == detector.allocIDMap.end()) {
										cout << "ERROR: Cannot find alloc " << it->first << " in the allocIDMap\n";
									} else {
										existingDetails = detector.allocIDMap.find(it->first)->second;
										existingDetails.readOps.insert(opCount);
										detector.allocIDMap.erase(detector.allocIDMap.find(it->first));
										detector.allocIDMap[it->first] = existingDetails;
										allocFound = true;
									}
								}
							}
							if (!allocFound) {
								cout << "ERROR: Cannot find alloc for read op " << opCount << endl;
								return -1;
							}

							for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.freeSet.begin(); it != detector.freeSet.end(); it++) {
								std::stringstream str1;
								str1 << it->second.startingAddress;
								long long freeBaseAddress;
								str1 >> std::hex >> freeBaseAddress;
								long long freeEndAddress = freeBaseAddress + it->second.range;

								if (freeBaseAddress <= baseAddress && baseAddress <= freeEndAddress) {
									UAFDetector::freeOpDetails existingDetails;
									if (detector.freeIDMap.find(it->first) == detector.freeIDMap.end()) {
										cout << "ERROR: Cannot find free " << it->first << " in the freeIDMap\n";
									} else {
										existingDetails = detector.freeIDMap.find(it->first)->second;
										existingDetails.readOps.insert(opCount);
										detector.freeIDMap.erase(detector.freeIDMap.find(it->first));
#ifdef DEBUGLINES
											cout << "Assigning to freeIDMap " << it->first << endl;
#endif
										detector.freeIDMap[it->first] = existingDetails;
									}
								}
							}
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

							std::stringstream str;
							str << writeopdetails.startingAddress;
							long long baseAddress;
							str >> std::hex >> baseAddress;

							for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.allocSet.begin(); it != detector.allocSet.end(); it++) {
								std::stringstream str1;
								str1 << it->second.startingAddress;
								long long allocBaseAddress;
								str1 >> std::hex >> allocBaseAddress;
								long long allocEndAddress = allocBaseAddress + it->second.range;

								if (allocBaseAddress <= baseAddress && baseAddress <= allocEndAddress) {
									UAFDetector::allocOpDetails existingDetails;
									if (detector.allocIDMap.find(it->first) == detector.allocIDMap.end()) {
										cout << "ERROR: Cannot find alloc " << it->first << " in the allocIDMap\n";
									} else {
										existingDetails = detector.allocIDMap.find(it->first)->second;
										existingDetails.writeOps.insert(opCount);
										detector.allocIDMap.erase(detector.allocIDMap.find(it->first));
										detector.allocIDMap[it->first] = existingDetails;
									}
								}
							}

							for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.freeSet.begin(); it != detector.freeSet.end(); it++) {
								std::stringstream str1;
								str1 << it->second.startingAddress;
								long long freeBaseAddress;
								str1 >> std::hex >> freeBaseAddress;
								long long freeEndAddress = freeBaseAddress + it->second.range;

								if (freeBaseAddress <= baseAddress && baseAddress <= freeEndAddress) {
									UAFDetector::freeOpDetails existingDetails;
									if (detector.freeIDMap.find(it->first) == detector.freeIDMap.end()) {
										cout << "ERROR: Cannot find free " << it->first << " in the freeIDMap\n";
									} else {
										existingDetails = detector.freeIDMap.find(it->first)->second;
										existingDetails.writeOps.insert(opCount);
										detector.freeIDMap.erase(detector.freeIDMap.find(it->first));
#ifdef DEBUGLINES
											cout << "Assigning to freeIDMap " << it->first << endl;
#endif
										detector.freeIDMap[it->first] = existingDetails;
									}
								}
							}
#endif
						}

						if (match.compare("threadinit") == 0 || match.compare("deq") == 0 ||
								match.compare("resume") == 0 || match.compare("exitloop") == 0) {
							blockCount++;

							if (detector.opIDMap.find(opCount) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find opIDMap entry for op " << opCount << endl;
								return -1;
							} else {
								UAFDetector::opDetails existingDetails = detector.opIDMap.find(opCount)->second;
								existingDetails.blockID = blockCount;
								detector.opIDMap.erase(detector.opIDMap.find(opCount));
								detector.opIDMap[opCount] = existingDetails;
							}

							MultiStack::stackElementType element;
							element.opID = blockCount; // opID really stores the block ID in this case.
							element.threadID = opdetails.threadID;
							if (stack6.isBottom(stack6.peek(opdetails.threadID))) {
								stack6.push(element);
							} else {
								MultiStack::stackElementType top = stack6.peek(opdetails.threadID);
								long long prevBlockID = top.opID;
								while (!stack6.isBottom(top)) {
									top = stack6.pop(opdetails.threadID);
								}
								stack6.push(element);
								if (detector.blockToNextBlockInThread.find(prevBlockID) == detector.blockToNextBlockInThread.end()) {
									detector.blockToNextBlockInThread[prevBlockID] = blockCount;
								} else {
									cout << "ERROR: Next block already exists for block " << prevBlockID << endl;
									cout << "Existing entry " << detector.blockToNextBlockInThread[prevBlockID] << endl;
									return -1;
								}
							}

							std::vector<long long> ops(1, opCount);
							if (detector.blockIDMap.find(blockCount) == detector.blockIDMap.end()) {
								detector.blockIDMap[blockCount] = ops;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								return -1;
							}
						} else if (match.compare("enterloop") == 0 || match.compare("end") == 0 ||
								   match.compare("pause") == 0 || match.compare("threadexit") == 0) {
							MultiStack::stackElementType top = stack6.peek(opdetails.threadID);
							if (stack6.isBottom(top)) {
								cout << "ERROR: No operations before op " << opCount << " in the current block\n";
							}
							if (detector.opIDMap.find(opCount) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find opIDMap entry for op " << opCount << endl;
								return -1;
							} else {
								UAFDetector::opDetails existingDetails = detector.opIDMap.find(opCount)->second;
								existingDetails.blockID = top.opID;
								detector.opIDMap.erase(detector.opIDMap.find(opCount));
								detector.opIDMap[opCount] = existingDetails;
							}

							MultiStack::stackElementType element;
							element.opID = top.opID; // opID really stores the block ID in this case.
							element.threadID = opdetails.threadID;
							stack6.push(element);

							if (detector.blockIDMap.find(top.opID) == detector.blockIDMap.end()) {
								cout << "ERROR: Cannot find entry for block " << top.opID << " in blockIDMap\n";
								return -1;
							} else {
								std::vector<long long> existingOps = detector.blockIDMap.find(top.opID)->second;
								existingOps.push_back(opCount);
								detector.blockIDMap.erase(detector.blockIDMap.find(top.opID));
								detector.blockIDMap[top.opID] = existingOps;
							}
						} else {
							bool newBlock = false;
							long long currBlock;
							if (stack6.isBottom(stack6.peek(opdetails.threadID))) {
								if (blockCount == 0) {
									newBlock = true;
									blockCount++;
									currBlock = blockCount;
								} else {
									cout << "ERROR: blockCount not zero with bottom of stack while obtaining block information\n";
									return -1;
								}
							} else {
								MultiStack::stackElementType top = stack6.peek(opdetails.threadID);
								currBlock = top.opID;
							}
							assert (currBlock > 0);
							if (detector.opIDMap.find(opCount) == detector.opIDMap.end()) {
								cout << "ERROR: Cannot find opIDMap entry for op " << opCount << endl;
								return -1;
							} else {
								UAFDetector::opDetails existingDetails = detector.opIDMap.find(opCount)->second;
								existingDetails.blockID = currBlock;
								detector.opIDMap.erase(detector.opIDMap.find(opCount));
								detector.opIDMap[opCount] = existingDetails;
							}


							MultiStack::stackElementType element;
							element.opID = currBlock; // opID really stores the block ID in this case.
							element.threadID = opdetails.threadID;
							stack6.push(element);

							if (detector.blockIDMap.find(currBlock) == detector.blockIDMap.end()) {
								cout << "ERROR: Cannot find entry for block " << currBlock << " in blockIDMap\n";
								//return -1;
								std::vector<long long> ops(1, opCount);
								detector.blockIDMap[currBlock] = ops;
							} else {
								std::vector<long long> existingOps = detector.blockIDMap.find(currBlock)->second;
								existingOps.push_back(opCount);
								detector.blockIDMap.erase(detector.blockIDMap.find(currBlock));
								detector.blockIDMap[currBlock] = existingOps;
							}
						}
#endif


#if 0
	cout << "Finished parsing the file\n";

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
	detector.initGraph(opCount, blockCount);

#ifdef TRACEDEBUG
	cout << "No of ops: " << opCount << endl;
	cout << "No of blocks: " << detector.blockIDMap.size() << endl;
	cout << "No of tasks: " << detector.taskIDMap.size() << endl;
	cout << "No of atomic tasks: " << detector.atomicTasks.size() << endl;
	cout << "No of threads: " << detector.threadIDMap.size() << endl;
	for (map<long long, UAFDetector::opDetails>::iterator it = detector.opIDMap.begin(); it != detector.opIDMap.end(); it++) {
		cout << "Op: " << it->first << " - details: ";
		it->second.printOpDetails();
		cout << endl;
	}
	for (map<string, UAFDetector::taskDetails>::iterator it = detector.taskIDMap.begin(); it != detector.taskIDMap.end(); it++) {
		cout << "Task ID: " << it->first << " - details: ";
		it->second.printTaskDetails();
		cout << endl;
	}
	for (map<long long, UAFDetector::threadDetails>::iterator it = detector.threadIDMap.begin(); it != detector.threadIDMap.end(); it++) {
		cout << "Thread ID: " << it->first << " - details:";
		it->second.printThreadDetails();
		cout << endl;
	}
	for (map<long long, vector<long long> >::iterator it = detector.blockIDMap.begin(); it != detector.blockIDMap.end(); it++) {
		cout << "Block ID: " << it->first << " - details\n";
		for (vector<long long>::iterator itt = it->second.begin(); itt != it->second.end(); itt++) {
			cout << *itt << " \n";
		}
	}
	for (set<string>::iterator it = detector.atomicTasks.begin(); it != detector.atomicTasks.end(); it++) {
		cout << "Atomic Task: " << *it << endl;
	}
	cout << "Next-op-in-task map" << endl;
	for (int i=1; i <= opCount; i++) {
		cout << "Op " << i << "-> " << detector.opToNextOpInTask[i] << endl;
	}
	cout << "Next-op-in-thread map" << endl;
	for (int i=1; i <= opCount; i++) {
		cout << "Op " << i << "-> " << detector.opToNextOpInThread[i] << endl;
	}
	cout << "Set - threadinit\n";
	for (set<long long>::iterator it = detector.threadinitSet.begin(); it != detector.threadinitSet.end(); it++) {
		cout << *it << " ";
	}
	cout << endl;
	cout << "Set - threadexit\n";
	for (set<long long>::iterator it = detector.threadexitSet.begin(); it != detector.threadexitSet.end(); it++) {
		cout << *it << " ";
	}
	cout << endl;
	cout << "Set - enterloop\n";
	for (set<long long>::iterator it = detector.enterloopSet.begin(); it != detector.enterloopSet.end(); it++) {
		cout << *it << " ";
	}
	cout << endl;
	cout << "Set - exitloop\n";
	for (set<long long>::iterator it = detector.exitloopSet.begin(); it != detector.exitloopSet.end(); it++) {
		cout << *it << " ";
	}
	cout << endl;
	cout << "Set - deq\n";
	for (set<long long>::iterator it = detector.deqSet.begin(); it != detector.deqSet.end(); it++) {
		cout << *it << " ";
	}
	cout << endl;
	cout << "Set - end\n";
	for (set<long long>::iterator it = detector.endSet.begin(); it != detector.endSet.end(); it++) {
		cout << *it << " ";
	}
	cout << endl;
	cout << "Set - pause\n";
	for (set<long long>::iterator it = detector.pauseSet.begin(); it != detector.pauseSet.end(); it++) {
		cout << *it << " ";
	}
	cout << endl;
	cout << "Set - resume\n";
	for (set<long long>::iterator it = detector.resumeSet.begin(); it != detector.resumeSet.end(); it++) {
		cout << *it << " ";
	}
	cout << endl;
	cout << "Set - enq\n";
	for (map<long long, UAFDetector::enqOpDetails>::iterator it = detector.enqSet.begin(); it != detector.enqSet.end(); it++) {
		cout << "Op- " << it->first << ":\n(" << it->second.currThreadID << ", " << it->second.taskEnqueued
			 << ", " << it->second.targetThreadID << ", " << it->second.callback << ")" << endl;
	}
	cout << "Set - fork\n";
	for (map<long long, UAFDetector::forkAndJoinOpDetails>::iterator it = detector.forkSet.begin(); it != detector.forkSet.end(); it++) {
		cout << "Op- " << it->first << ":\n(" << it->second.currThreadID << ", " << it->second.targetThreadID << ")\n";
	}
	cout << "Set - join\n";
	for (map<long long, UAFDetector::forkAndJoinOpDetails>::iterator it = detector.joinSet.begin(); it != detector.joinSet.end(); it++) {
		cout << "Op- " << it->first << ":\n(" << it->second.currThreadID << ", " << it->second.targetThreadID << ")\n";
	}
#ifdef LOCKS
	cout << "Set - acquire\n";
	for (map<long long, UAFDetector::acquireAndReleaseOpDetails>::iterator it = detector.acquireSet.begin(); it != detector.acquireSet.end(); it++) {
		cout << "Op- " << it->first << ":\n(" << it->second.currThreadID << ", " << it->second.lockID << ")\n";
	}
	cout << "Set - release\n";
	for (map<long long, UAFDetector::acquireAndReleaseOpDetails>::iterator it = detector.releaseSet.begin(); it != detector.releaseSet.end(); it++) {
		cout << "Op- " << it->first << ":\n(" << it->second.currThreadID << ", " << it->second.lockID << ")\n";
	}
	cout << "Set - entermonitor\n";
	for (map<long long, UAFDetector::acquireAndReleaseOpDetails>::iterator it = detector.entermonitorSet.begin(); it != detector.entermonitorSet.end(); it++) {
		cout << "Op- " << it->first << ":\n(" << it->second.currThreadID << ", " << it->second.lockID << ")\n";
	}
	cout << "Set - exitmonitor\n";
	for (map<long long, UAFDetector::acquireAndReleaseOpDetails>::iterator it = detector.exitmonitorSet.begin(); it != detector.exitmonitorSet.end(); it++) {
		cout << "Op- " << it->first << ":\n(" << it->second.currThreadID << ", " << it->second.lockID << ")\n";
	}
	cout << "Set - wait\n";
	for (map<long long, UAFDetector::acquireAndReleaseOpDetails>::iterator it = detector.waitSet.begin(); it != detector.waitSet.end(); it++) {
		cout << "Op- " << it->first << ":\n(" << it->second.currThreadID << ", " << it->second.lockID << ")\n";
	}
	cout << "Set - notify\n";
	for (map<long long, UAFDetector::acquireAndReleaseOpDetails>::iterator it = detector.notifySet.begin(); it != detector.notifySet.end(); it++) {
		cout << "Op- " << it->first << ":\n(" << it->second.currThreadID << ", " << it->second.lockID << ")\n";
	}
	cout << "Set - notifyAll\n";
	for (map<long long, UAFDetector::acquireAndReleaseOpDetails>::iterator it = detector.notifyAllSet.begin(); it != detector.notifyAllSet.end(); it++) {
		cout << "Op- " << it->first << ":\n(" << it->second.currThreadID << ", " << it->second.lockID << ")\n";
	}

#endif
	cout << "Set - alloc\n";
	for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.allocSet.begin(); it != detector.allocSet.end(); it++) {
		cout << "Op- " << it->first << ":\n(" << it->second.threadID << ", " << it->second.startingAddress << ", " << it->second.range << ")\n";
	}
	cout << "Set - free\n";
	for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.freeSet.begin(); it != detector.freeSet.end(); it++) {
		cout << "Op- " << it->first << ":\n(" << it->second.threadID << ", " << it->second.startingAddress << ", " << it->second.range << ")\n";
	}
	cout << "Set - inc\n";
	for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.incSet.begin(); it != detector.incSet.end(); it++) {
		cout << "Op- " << it->first << ":\n(" << it->second.threadID << ", " << it->second.startingAddress << ", " << it->second.range << ")\n";
	}
	cout << "Set - dec\n";
	for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.decSet.begin(); it != detector.decSet.end(); it++) {
		cout << "Op- " << it->first << ":\n(" << it->second.threadID << ", " << it->second.startingAddress << ", " << it->second.range << ")\n";
	}
#ifdef ACCESS
	cout << "Set - access\n";
	for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.accessSet.begin(); it != detector.accessSet.end(); it++) {
		cout << "Op- " << it->first << ":\n(" << it->second.threadID << ", " << it->second.startingAddress << ", " << it->second.range << ")\n";
	}
#else
	cout << "Set - read\n";
	for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.readSet.begin(); it != detector.readSet.end(); it++) {
		cout << "Op- " << it->first << ":\n(" << it->second.threadID << ", " << it->second.startingAddress << ", " << it->second.range << ")\n";
	}
	cout << "Set - write\n";
	for (map<long long, UAFDetector::memoryOpDetails>::iterator it = detector.writeSet.begin(); it != detector.writeSet.end(); it++) {
		cout << "Op- " << it->first << ":\n(" << it->second.threadID << ", " << it->second.startingAddress << ", " << it->second.range << ")\n";
	}
#endif
#ifdef LOCKS
	cout << "Set - lock\n";
	for (map<string, UAFDetector::lockDetails>::iterator it = detector.lockIDMap.begin(); it != detector.lockIDMap.end(); it++) {
		cout << it->first;
		it->second.printDetails();
		cout << endl;
	}
#endif
	cout << "Set - alloc\n";
	for (map<long long, UAFDetector::allocOpDetails>::iterator it = detector.allocIDMap.begin(); it != detector.allocIDMap.end(); it++) {
		cout << "Op- " << it->first << endl;
		it->second.printDetails();
		cout << endl;
	}
	cout << "Set - free\n";
	for (map<long long, UAFDetector::freeOpDetails>::iterator it = detector.freeIDMap.begin(); it != detector.freeIDMap.end(); it++) {
		cout << "Op- " << it->first << endl;
		it->second.printDetails();
		cout << endl;
	}
#endif
#endif

	return 0;
}
