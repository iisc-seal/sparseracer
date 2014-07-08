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
TraceParser::TraceParser(char* traceFileName, Logger &logger) {
	traceFile.open(traceFileName, ios_base::in);
	cout << traceFileName << endl;
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
	//prefixRegEx = " *";
	prefixRegEx = "\\s*";

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
			  " *(reset) *\\( *("	   + intRegEx + ") *, *("      + hexRegEx + ") *\\) *" + "|" +
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

	suffixRegEx = "\\s*";
	// Regular expression for the entire line
	finalRegEx = "^" + prefixRegEx + "(" + opRegEx + ")" + suffixRegEx + "$";
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

	long long typePos, threadPos;

	MultiStack stackForThreadAndBlockOrder;
	MultiStack stackForTaskOrder;
//	MultiStack stackForBlockOrder;
//	MultiStack stackForNestingOrder;

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
//						assert(stackForNestingOrder.isEmpty(threadID));
#endif

						// Obtain details of current thread
						if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
							UAFDetector::threadDetails threaddetails;
							threaddetails.firstOpID = opCount;
							threaddetails.threadinitOpID = opCount;
							threaddetails.firstBlockID = blockCount;
							detector.threadIDMap[threadID] = threaddetails;
						}
						else {
							UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
							// Sanity check: if there already exists an entry for thread, then this must
							// have been set when we saw a fork op for this thread. That is, the threadinit
							// op id has not been set.
							assert(existingEntry.threadinitOpID == -1);
#endif
							existingEntry.threadinitOpID = opCount;
							existingEntry.firstBlockID = blockCount;
							existingEntry.firstOpID = opCount;
							detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
							detector.threadIDMap[threadID] = existingEntry;
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
//							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread (and the first block)
							blockCount++;
							opdetails.blockID = blockCount;

							// Update firstop, firstblock, lastblock and threadexitopID for the current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.threadexitOpID = opCount;
								threaddetails.firstOpID = opCount;
								threaddetails.firstBlockID = blockCount;
								threaddetails.lastBlockID = blockCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.threadexitOpID == -1);
								assert(existingEntry.firstBlockID == -1);
								assert(existingEntry.firstOpID == -1);
								assert(existingEntry.lastBlockID == -1);
#endif
								existingEntry.threadexitOpID = opCount;
								existingEntry.firstOpID = opCount;
								existingEntry.firstBlockID = blockCount;
								existingEntry.lastBlockID = blockCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							if (detector.blockIDMap.find(blockCount) == detector.blockIDMap.end()) {
								blockdetails.firstOpInBlock = opCount;
								blockdetails.lastOpInBlock = opCount;
								detector.blockIDMap[blockCount] = blockdetails;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								cout << endl;
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

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID as current op in blockIDMap
								assert(existingEntry.threadID == threadID);
#endif

								existingEntry.lastOpInBlock = opCount;
								detector.blockIDMap.erase(detector.blockIDMap.find(previousOpInThread.blockID));
								detector.blockIDMap[previousOpInThread.blockID] = existingEntry;
							}

							stackElement.blockID = previousOpInThread.blockID;
							stackForThreadAndBlockOrder.stackClear(threadID);
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
//							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;

							// Update firstop, firstblock and enterloopblock for current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								threaddetails.firstBlockID = blockCount;
								threaddetails.enterloopBlockID = blockCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.enterloopBlockID == -1);
								assert(existingEntry.firstBlockID == -1);
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.enterloopBlockID = opCount;
								existingEntry.firstOpID = opCount;
								existingEntry.firstBlockID = blockCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							if (detector.blockIDMap.find(blockCount) == detector.blockIDMap.end()) {
								blockdetails.firstOpInBlock = opCount;
								// enterloop is the end of a block
								blockdetails.lastOpInBlock = opCount;
								detector.blockIDMap[blockCount] = blockdetails;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								cout << endl;
								return -1;
							}
							stackElement.blockID = blockCount;
							stackForThreadAndBlockOrder.push(stackElement);

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

							if (detector.blockIDMap.find(previousOpInThread.blockID) == detector.blockIDMap.end()) {
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
							stackForThreadAndBlockOrder.push(stackElement);
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
//							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;

							// Update firstop, firstblock and exitloopblock for current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								threaddetails.firstBlockID = blockCount;
								threaddetails.exitloopBlockID = blockCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.exitloopBlockID == -1);
								assert(existingEntry.firstBlockID == -1);
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.exitloopBlockID = opCount;
								existingEntry.firstOpID = opCount;
								existingEntry.firstBlockID = blockCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							if (detector.blockIDMap.find(blockCount) == detector.blockIDMap.end()) {
								blockdetails.firstOpInBlock = opCount;
								detector.blockIDMap[blockCount] = blockdetails;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								cout << endl;
								return -1;
							}
							stackElement.blockID = blockCount;
							stackForThreadAndBlockOrder.push(stackElement);

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
									assert(existingEntry.lastOpInBlock == -1 || existingEntry.lastOpInBlock == previousOpInThread.opID);
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
								cout << endl;
								return -1;
							}

							stackElement.blockID = blockCount;
							stackForThreadAndBlockOrder.push(stackElement);
						}
					} else if (match.compare("enq") == 0) {

						// Obtain 2nd & 3rd argument (i.e task enqueued & target threadID) of enq op
						string taskEnqueued;
						IDType targetThread;
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

						if (detector.taskIDMap.find(taskEnqueued) == detector.taskIDMap.end()) {
							UAFDetector::taskDetails taskdetails;
							taskdetails.enqOpID = opCount;
							detector.taskIDMap[taskEnqueued] = taskdetails;
						} else {
							cout << "ERROR: Found duplicate entry for task " << taskEnqueued << " in taskIDMap\n";
							cout << "While examining op " << opCount;
							cout << "\nDuplicate entry:\n";
							detector.taskIDMap[taskEnqueued].printTaskDetails();
							cout << endl;
							return -1;
						}

						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadAndBlockOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for enq " << opCount
								 << " on stack\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
//							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;

							// Update firstop, firstblock for current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								threaddetails.firstBlockID = blockCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstBlockID == -1);
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								existingEntry.firstBlockID = blockCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							if (detector.blockIDMap.find(blockCount) == detector.blockIDMap.end()) {
								blockdetails.firstOpInBlock = opCount;
								blockdetails.enqSet.insert(opCount);
								detector.blockIDMap[blockCount] = blockdetails;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								cout << endl;
								return -1;
							}

							stackElement.blockID = blockCount;
							stackForThreadAndBlockOrder.push(stackElement);
//							stackForTaskOrder.push(stackElement);

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
								if (previousOpInThread.taskID.compare("") != 0)
									existingEntry.nextOpInTask = opCount;
								existingEntry.nextOpInBlock = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}


							// Add enq to the enqSet for current block
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

							UAFDetector::enqOpDetails enqdetails;
							enqdetails.targetThread = targetThread;
							enqdetails.taskEnqueued = taskEnqueued;

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

							stackElement.blockID = previousOpInThread.blockID;
							stackElement.taskID = previousOpInThread.taskID;
							stackForThreadAndBlockOrder.push(stackElement);
//							stackForTaskOrder.push(stackElement);

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
//							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;
							opdetails.taskID = taskDequeued;

							// Since deq is the first op of the thread, there is no enq op that
							// enqueues the task. Hence there would be no entry for this task in
							// taskIDMap
							if (detector.taskIDMap.find(taskDequeued) == detector.taskIDMap.end()) {
								UAFDetector::taskDetails taskdetails;
								taskdetails.deqOpID = opCount;
								detector.taskIDMap[taskDequeued] = taskdetails;
							} else {
								cout << "ERROR: Found duplicate entry for task " << taskDequeued << " in taskIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.taskIDMap[taskDequeued].printTaskDetails();
								cout << endl;
								return -1;
							}

							// Update firstop, firstblock and exitloopblock for current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								threaddetails.firstBlockID = blockCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstBlockID == -1);
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								existingEntry.firstBlockID = blockCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							if (detector.blockIDMap.find(blockCount) == detector.blockIDMap.end()) {
								blockdetails.firstOpInBlock = opCount;
								blockdetails.taskID = taskDequeued;
								detector.blockIDMap[blockCount] = blockdetails;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								cout << endl;
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
							stackForThreadAndBlockOrder.push(stackElement);
							stackForTaskOrder.push(stackElement);

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
									detector.blockIDMap.erase(detector.blockIDMap.find(previousOpInThread.blockID));
									detector.blockIDMap[previousOpInThread.blockID] = existingEntry;

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
								cout << endl;
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
								if (!stackForTaskOrder.isEmpty(threadID)) {
									MultiStack::stackElementType topOfNestingStack = stackForTaskOrder.peek(threadID);
									if (topOfNestingStack.opType.compare("pause") == 0) {
										if (detector.taskIDMap.find(topOfNestingStack.taskID) == detector.taskIDMap.end()) {
											cout << "ERROR: Cannot find entry for block " << topOfNestingStack.taskID << " in taskIDMap\n";
											cout << "While examining op " << opCount << "\n";
											return -1;
										}
										existingEntry.parentTask = topOfNestingStack.taskID;
									}
								}

								detector.taskIDMap.erase(detector.taskIDMap.find(taskDequeued));
								detector.taskIDMap[taskDequeued] = existingEntry;
							}

							stackElement.blockID = blockCount;
							stackElement.taskID = taskDequeued;
							stackForThreadAndBlockOrder.push(stackElement);
							stackForTaskOrder.push(stackElement);
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
						if (stackForThreadAndBlockOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for pause " << opCount
								 << " on stack\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
//							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;
							opdetails.taskID = task;

							// Update firstop, firstblock and exitloopblock for current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								threaddetails.firstBlockID = blockCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstBlockID == -1);
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								existingEntry.firstBlockID = blockCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							if (detector.blockIDMap.find(blockCount) == detector.blockIDMap.end()) {
								blockdetails.firstOpInBlock = opCount;
								blockdetails.taskID = task;
								blockdetails.lastOpInBlock = opCount;
								detector.blockIDMap[blockCount] = blockdetails;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								cout << endl;
								return -1;
							}

							if (detector.taskIDMap.find(task) == detector.taskIDMap.end()) {
								UAFDetector::taskDetails taskdetails;
								taskdetails.firstPauseOpID = opCount;
								taskdetails.atomic = false;
								taskdetails.firstBlockID = blockCount;
								UAFDetector::taskDetails::pauseResumePair pauseResumeDetails;
								pauseResumeDetails.pauseOp = opCount;
								taskdetails.pauseResumeSequence.push_back(pauseResumeDetails);
								detector.taskIDMap[task] = taskdetails;
							} else {
								cout << "ERROR: Found duplicate entry for task " << task << " in taskIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.taskIDMap[task].printTaskDetails();
								cout << endl;
								return -1;
							}

							if (detector.nestingLoopMap.find(sharedVariable) == detector.nestingLoopMap.end()) {
								UAFDetector::nestingLoopDetails loopdetails;
								loopdetails.pauseOpID = opCount;
								detector.nestingLoopMap[sharedVariable] = loopdetails;
							} else {
								cout << "ERROR: Found duplicate entry for shared variable " << sharedVariable << " in nestingLoopMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.nestingLoopMap[sharedVariable].printNestingLoopDetails();
								cout << endl;
								return -1;
							}
							stackElement.blockID = blockCount;
							stackElement.taskID = task;
							stackForThreadAndBlockOrder.push(stackElement);
							stackForTaskOrder.push(stackElement);

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

								existingEntry.nextOpInBlock = opCount;
								existingEntry.nextOpInTask = opCount;
								existingEntry.nextOpInThread = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							if (detector.blockIDMap.find(previousOpInThread.blockID) == detector.blockIDMap.end()) {
								cout << "ERROR: Cannot find entry for block " << previousOpInThread.blockID << " in blockIDMap\n";
								cout << "While examining op " << opCount;
								cout << endl;
								return -1;
							} else {
								UAFDetector::blockDetails existingEntry = detector.blockIDMap[previousOpInThread.blockID];
#ifdef SANITYCHECK
								// Sanity check: prev-op has the same thread as current op
								assert(existingEntry.threadID == threadID);
								assert(previousOpInThread.threadID == threadID);
#endif
								existingEntry.lastOpInBlock = opCount;
								detector.blockIDMap.erase(detector.blockIDMap.find(previousOpInThread.blockID));
								detector.blockIDMap[previousOpInThread.blockID] = existingEntry;
							}

							if (detector.taskIDMap.find(task) == detector.taskIDMap.end()) {
								cout << "ERROR: Cannot find entry for task " << task << " in taskIDMap\n";
								cout << "While examining op " << opCount;
								cout << endl;
								return -1;
							} else {
								UAFDetector::taskDetails existingEntry = detector.taskIDMap[task];
								if (!stackForTaskOrder.isEmpty(threadID)) {
									MultiStack::stackElementType topOfNestingStack = stackForTaskOrder.peek(threadID);
									if (topOfNestingStack.opType.compare("deq") == 0) {
										existingEntry.firstPauseOpID = opCount;
										existingEntry.atomic = false;
									}
								} else {
									cout << "ERROR: stackForNesting Order is empty\n";
									cout << "Expected to find the deq/previous-resume of task " << task << endl;
									cout << "While examining pause op " << opCount << endl;
									return -1;
								}

								UAFDetector::taskDetails::pauseResumePair pr;
								pr.pauseOp = opCount;
								existingEntry.pauseResumeSequence.push_back(pr);

								detector.taskIDMap.erase(detector.taskIDMap.find(task));
								detector.taskIDMap[task] = existingEntry;
							}

							if (detector.nestingLoopMap.find(sharedVariable) == detector.nestingLoopMap.end()) {
								UAFDetector::nestingLoopDetails loopdetails;
								loopdetails.pauseOpID = opCount;
								detector.nestingLoopMap[sharedVariable] = loopdetails;
							} else {
								cout << "ERROR: Found duplicate entry for shared variable " << sharedVariable << " in nestingLoopMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.nestingLoopMap[sharedVariable].printNestingLoopDetails();
								cout << endl;
								return -1;
							}

							stackElement.blockID = previousOpInThread.blockID;
							stackElement.taskID = task;
							stackForThreadAndBlockOrder.push(stackElement);
							stackForTaskOrder.push(stackElement);
						}
					} else if (match.compare("reset") == 0) {
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
						if (stackForThreadAndBlockOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for reset " << opCount
								 << " on stack\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
//							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;

							// Update firstop, firstblock and exitloopblock for current thread
							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstOpID = opCount;
								threaddetails.firstBlockID = blockCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstBlockID == -1);
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								existingEntry.firstBlockID = blockCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							if (detector.blockIDMap.find(blockCount) == detector.blockIDMap.end()) {
								blockdetails.firstOpInBlock = opCount;
								detector.blockIDMap[blockCount] = blockdetails;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								cout << endl;
								return -1;
							}

							if (detector.nestingLoopMap.find(sharedVariable) == detector.nestingLoopMap.end()) {
								UAFDetector::nestingLoopDetails loopdetails;
								loopdetails.resetSet.insert(opCount);
								detector.nestingLoopMap[sharedVariable] = loopdetails;
							} else {
								cout << "ERROR: Found duplicate entry for shared variable " << sharedVariable << " in nestingLoopMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.nestingLoopMap[sharedVariable].printNestingLoopDetails();
								cout << endl;
								return -1;
							}
							stackElement.blockID = blockCount;
							stackForThreadAndBlockOrder.push(stackElement);
//							stackForTaskOrder.push(stackElement);

						} else {
							MultiStack::stackElementType previousOpInThread = stackForThreadAndBlockOrder.peek(threadID);
							// In the case of reset, the previous op in thread is the previous op in task.
							// So no need to look separately in task stack.

							opdetails.blockID = previousOpInThread.blockID;
							opdetails.prevOpInBlock = previousOpInThread.opID;
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
								existingEntry.nextOpInBlock = opCount;
								existingEntry.nextOpInTask = opCount;
								existingEntry.nextOpInThread = opCount;
								opdetails.taskID = existingEntry.taskID;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							if (detector.nestingLoopMap.find(sharedVariable) == detector.nestingLoopMap.end()) {
								cout << "ERROR: Cannot find entry for shared variable " << sharedVariable << " in nestingLoopMap\n";
								cout << "While examining op " << opCount;
								cout << endl;
								return -1;
							} else {
								UAFDetector::nestingLoopDetails existingEntry = detector.nestingLoopMap[sharedVariable];
								existingEntry.resetSet.insert(opCount);
								detector.nestingLoopMap.erase(detector.nestingLoopMap.find(sharedVariable));
								detector.nestingLoopMap[sharedVariable] = existingEntry;
							}

							stackElement.blockID = previousOpInThread.blockID;
							stackElement.taskID = opdetails.taskID;
							stackForThreadAndBlockOrder.push(stackElement);
//							stackForTaskOrder.push(stackElement);
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
								cout << endl;
							return -1;
						}

						// Obtain the stack top to obtain the previous op in thread.
						if (stackForThreadAndBlockOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for resume " << opCount
								 << " on stack\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
//							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;
							opdetails.taskID = task;

							if (detector.taskIDMap.find(task) == detector.taskIDMap.end()) {
								UAFDetector::taskDetails taskdetails;
								taskdetails.firstBlockID = blockCount;
								taskdetails.atomic = false;

								UAFDetector::taskDetails::pauseResumePair pr;
								pr.resumeOp = opCount;
								taskdetails.pauseResumeSequence.push_back(pr);

								detector.taskIDMap[task] = taskdetails;
							} else {
								cout << "ERROR: Found duplicate entry for task " << task << " in taskIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.taskIDMap[task].printTaskDetails();
								cout << endl;
								return -1;
							}

							if (detector.nestingLoopMap.find(sharedVariable) == detector.nestingLoopMap.end()) {
								UAFDetector::nestingLoopDetails loopdetails;
								loopdetails.resumeOpID = opCount;
								detector.nestingLoopMap[sharedVariable] = loopdetails;
							} else {
								cout << "ERROR: Found duplicate entry for shared variable " << sharedVariable << " in nestingLoopMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.nestingLoopMap[sharedVariable].printNestingLoopDetails();
								cout << endl;
								return -1;
							}

							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstBlockID = blockCount;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstBlockID == -1);
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								existingEntry.firstBlockID = blockCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							if (detector.blockIDMap.find(blockCount) == detector.blockIDMap.end()) {
								blockdetails.taskID = task;
								blockdetails.firstOpInBlock = opCount;
								detector.blockIDMap[blockCount] = blockdetails;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount << endl;
								cout << "Duplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								cout << endl;
								return -1;
							}

							stackElement.blockID = blockCount;
							stackElement.taskID = task;
							stackForThreadAndBlockOrder.push(stackElement);
							stackForTaskOrder.push(stackElement);

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
								cout << endl;
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

							if (detector.taskIDMap.find(task) == detector.taskIDMap.end()) {
								cout << "ERROR: Cannot find entry for task " << task << " in taskIDMap\n";
								cout << "While examining op " << opCount;
							} else {
								UAFDetector::taskDetails existingEntry = detector.taskIDMap[task];
								for (vector<UAFDetector::taskDetails::pauseResumePair>::iterator prIt = existingEntry.pauseResumeSequence.begin();
										prIt != existingEntry.pauseResumeSequence.end(); prIt++) {
									if (prIt->resumeOp == -1) {
										IDType pauseOp = prIt->pauseOp;
										if (detector.pauseResumeResetOps.find(pauseOp) == detector.pauseResumeResetOps.end()) {
											cout << "ERROR: Cannot find shared variable of pause op " << pauseOp << " in pauseResumeResetOps\n";
											cout << "While examining op " << opCount << endl;
											return -1;
										} else {
											std::string pauseVariable = detector.pauseResumeResetOps[pauseOp];
											if (pauseVariable.compare(sharedVariable) == 0) {
												prIt->resumeOp = opCount;
											}
										}
									}
								}

								detector.taskIDMap.erase(detector.taskIDMap.find(task));
								detector.taskIDMap[task] = existingEntry;
							}

							if (detector.nestingLoopMap.find(sharedVariable) == detector.nestingLoopMap.end()) {
								cout << "ERROR: Cannot find entry for shared variable " << sharedVariable << " in nestingLoopMap\n";
								cout << "While examining op " << opCount;
							} else {
								UAFDetector::nestingLoopDetails existingEntry = detector.nestingLoopMap[sharedVariable];
								existingEntry.resumeOpID = opCount;
								detector.nestingLoopMap.erase(detector.nestingLoopMap.find(sharedVariable));
								detector.nestingLoopMap[sharedVariable] = existingEntry;
							}

							if (detector.blockIDMap.find(blockCount) == detector.blockIDMap.end()) {
								blockdetails.taskID = task;
								blockdetails.firstOpInBlock = opCount;
								if (detector.blockIDMap.find(previousOpInThread.blockID) == detector.blockIDMap.end()) {
									cout << "ERROR: Cannot find entry for block " << previousOpInThread.blockID << " in blockIDMap\n";
									cout << "While examining op " << opCount << endl;
									return -1;
								} else {
									UAFDetector::blockDetails existingEntry = detector.blockIDMap[previousOpInThread.blockID];
#ifdef SANITYCHECK
									// Sanity check: prev op has same thread as curr op
									assert(existingEntry.threadID == threadID);
									assert(previousOpInThread.threadID == threadID);
#endif
									existingEntry.nextBlockInThread = blockCount;
									detector.blockIDMap.erase(detector.blockIDMap.find(previousOpInThread.blockID));
									detector.blockIDMap[previousOpInThread.blockID] = existingEntry;

									blockdetails.prevBlockInThread = previousOpInThread.blockID;
								}

								if (detector.blockIDMap.find(previousOpInTask.blockID) == detector.blockIDMap.end()) {
									cout << "ERROR: Cannot find entry for block " << previousOpInTask.blockID << " in blockIDMap\n";
									cout << "While examining op " << opCount << endl;
									return -1;
								} else {
									UAFDetector::blockDetails existingEntry = detector.blockIDMap[previousOpInTask.blockID];
#ifdef SANITYCHECK
									// Sanity check: prev op has same thread as curr op
									assert(existingEntry.threadID == threadID);
									assert(previousOpInTask.threadID == threadID);
									assert(existingEntry.taskID.compare(task) == 0);
									assert(previousOpInTask.taskID.compare(task) == 0);
#endif
									existingEntry.nextBlockInTask = blockCount;
									detector.blockIDMap.erase(detector.blockIDMap.find(previousOpInTask.blockID));
									detector.blockIDMap[previousOpInTask.blockID] = existingEntry;
								}
								detector.blockIDMap[blockCount] = blockdetails;
							}

							stackElement.blockID = blockCount;
							stackElement.taskID = task;
							stackForThreadAndBlockOrder.push(stackElement);
							stackForTaskOrder.push(stackElement);

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
//							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;
							opdetails.taskID = task;

							if (detector.taskIDMap.find(task) == detector.taskIDMap.end()) {
								UAFDetector::taskDetails taskdetails;
								taskdetails.endOpID = opCount;
								taskdetails.firstBlockID = blockCount;
								taskdetails.lastBlockID = blockCount;
								detector.taskIDMap[task] = taskdetails;
							} else {
								cout << "ERROR: Found duplicate entry for task " << task << " in taskIDMap\n";
								cout << "While examining op " << opCount;
								cout << "\nDuplicate entry:\n";
								detector.taskIDMap[task].printTaskDetails();
								cout << endl;
								return -1;
							}

							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstBlockID = blockCount;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstBlockID == -1);
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								existingEntry.firstBlockID = blockCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							if (detector.blockIDMap.find(blockCount) == detector.blockIDMap.end()) {
								blockdetails.taskID = task;
								blockdetails.firstOpInBlock = opCount;
								blockdetails.lastOpInBlock = opCount;
								detector.blockIDMap[blockCount] = blockdetails;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount << endl;
								cout << "Duplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								cout << endl;
								return -1;
							}

							stackElement.blockID = blockCount;
							stackElement.taskID = task;
							stackForThreadAndBlockOrder.push(stackElement);

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

								existingEntry.nextOpInBlock = opCount;
								existingEntry.nextOpInTask = opCount;
								existingEntry.nextOpInThread = opCount;
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
								existingEntry.lastBlockID = previousOpInThread.blockID;
								if (!stackForTaskOrder.isEmpty(threadID)) {
									MultiStack::stackElementType topOfNestingStack = stackForTaskOrder.peek(threadID);
									if (topOfNestingStack.opType.compare("resume") == 0) {
#ifdef SANITYCHECK
										assert(topOfNestingStack.taskID.compare(task) == 0);
										assert(topOfNestingStack.blockID == previousOpInThread.blockID);
										assert(topOfNestingStack.threadID == threadID);
#endif
										existingEntry.lastResumeOpID = topOfNestingStack.opID;
									}
								} else {
									cout << "ERROR: stackForTaskOrder is empty\n";
									cout << "Expected to find deq/previous-resume of task " << task << endl;
									cout << "While examining end op " << opCount << endl;
									return -1;
								}
								detector.taskIDMap.erase(detector.taskIDMap.find(task));
								detector.taskIDMap[task] = existingEntry;
							}

							if (detector.blockIDMap.find(previousOpInThread.blockID) == detector.blockIDMap.end()) {
								cout << "ERROR: Cannot find entry for block " << previousOpInThread.blockID << " in blockIDMap\n";
								cout << "While examining op " << opCount << endl;
								detector.blockIDMap[previousOpInThread.blockID].printBlockDetails();
								cout << endl;
								return -1;
							} else {
								UAFDetector::blockDetails existingEntry = detector.blockIDMap[previousOpInThread.blockID];
#ifdef SANITYCHECK
								// Sanity check: prev op has same thread as curr op
								assert(existingEntry.threadID == threadID);
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.taskID.compare(task) == 0);
								assert(previousOpInThread.taskID.compare(task) == 0);
#endif
								existingEntry.lastOpInBlock = opCount;
								detector.blockIDMap.erase(detector.blockIDMap.find(previousOpInThread.blockID));
								detector.blockIDMap[previousOpInThread.blockID] = existingEntry;
							}

							stackElement.blockID = previousOpInThread.blockID;
							stackElement.taskID = task;
							stackForThreadAndBlockOrder.push(stackElement);
							stackForTaskOrder.stackClear(threadID, task);

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
//							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;

							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstBlockID = blockCount;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstBlockID == -1);
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								existingEntry.firstBlockID = blockCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							if (detector.threadIDMap.find(targetThread) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstBlockID = blockCount;
								threaddetails.firstOpID = opCount;
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

							if (detector.blockIDMap.find(blockCount) == detector.blockIDMap.end()) {
								blockdetails.firstOpInBlock = opCount;
								detector.blockIDMap[blockCount] = blockdetails;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount << endl;
								cout << "Duplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								cout << endl;
								return -1;
							}

							stackElement.blockID = blockCount;
							stackForThreadAndBlockOrder.push(stackElement);
//							stackForTaskOrder.push(stackElement);
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
								cout << endl;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								existingEntry.nextOpInBlock = opCount;
								if (previousOpInThread.taskID.compare("") != 0)
									existingEntry.nextOpInTask = opCount;
								existingEntry.nextOpInThread = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
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

							stackElement.blockID = previousOpInThread.blockID;
							stackElement.taskID = previousOpInThread.taskID;
							stackForThreadAndBlockOrder.push(stackElement);
//							stackForTaskOrder.push(stackElement);
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
//							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;

							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstBlockID = blockCount;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstBlockID == -1);
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								existingEntry.firstBlockID = blockCount;
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

							if (detector.blockIDMap.find(blockCount) == detector.blockIDMap.end()) {
								blockdetails.firstOpInBlock = opCount;
								detector.blockIDMap[blockCount] = blockdetails;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount << endl;
								cout << "Duplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								cout << endl;
								return -1;
							}

							stackElement.blockID = blockCount;
							stackForThreadAndBlockOrder.push(stackElement);
//							stackForTaskOrder.push(stackElement);
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
								cout << endl;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								existingEntry.nextOpInBlock = opCount;
								if (previousOpInThread.taskID.compare("") != 0)
									existingEntry.nextOpInTask = opCount;
								existingEntry.nextOpInThread = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
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

							stackElement.blockID = previousOpInThread.blockID;
							stackElement.taskID = previousOpInThread.taskID;
							stackForThreadAndBlockOrder.push(stackElement);
//							stackForTaskOrder.push(stackElement);
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
						if (stackForThreadAndBlockOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for alloc " << opCount
								 << " on stackForThreadOrder\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
//							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;

							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstBlockID = blockCount;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstBlockID == -1);
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								existingEntry.firstBlockID = blockCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							if (detector.blockIDMap.find(blockCount) == detector.blockIDMap.end()) {
								blockdetails.firstOpInBlock = opCount;
								detector.blockIDMap[blockCount] = blockdetails;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount << endl;
								cout << "Duplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								cout << endl;
								return -1;
							}

							stackElement.blockID = blockCount;
							stackForThreadAndBlockOrder.push(stackElement);
//							stackForTaskOrder.push(stackElement);
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
								cout << endl;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								existingEntry.nextOpInBlock = opCount;
								if (previousOpInThread.taskID.compare("") != 0)
									existingEntry.nextOpInTask = opCount;
								existingEntry.nextOpInThread = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							stackElement.blockID = previousOpInThread.blockID;
							stackElement.taskID = previousOpInThread.taskID;
							stackForThreadAndBlockOrder.push(stackElement);
//							stackForTaskOrder.push(stackElement);

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
						endAddressIntFree = baseAddressIntFree + size;

						for (map<IDType, UAFDetector::memoryOpDetails>::iterator it = detector.allocSet.begin(); it != detector.allocSet.end(); it++) {
							std::stringstream str2;
							str2 << it->second.startingAddress;
							IDType baseAddressIntAlloc, endAddressIntAlloc;
							str2 >> std::hex >> baseAddressIntAlloc;
							endAddressIntAlloc = baseAddressIntAlloc + it->second.range;

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

							if (baseAddressIntFree <= addressIntRead && addressIntRead <= endAddressIntFree) {
								if (detector.freeIDMap.find(opCount) == detector.freeIDMap.end()) {
									UAFDetector::freeOpDetails freedetails;
									freedetails.readOps.insert(it->first);
									detector.freeIDMap[opCount] = freedetails;
								} else {
									UAFDetector::freeOpDetails existingEntry = detector.freeIDMap[opCount];
									existingEntry.readOps.insert(it->first);
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

							if (baseAddressIntFree <= addressIntWrite && addressIntWrite <= endAddressIntFree) {
								if (detector.freeIDMap.find(opCount) == detector.freeIDMap.end()) {
									UAFDetector::freeOpDetails freedetails;
									freedetails.writeOps.insert(it->first);
									detector.freeIDMap[opCount] = freedetails;
								} else {
									UAFDetector::freeOpDetails existingEntry = detector.freeIDMap[opCount];
									existingEntry.writeOps.insert(it->first);
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
						if (stackForThreadAndBlockOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for free " << opCount
								 << " on stackForThreadOrder\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
//							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;

							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstBlockID = blockCount;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstBlockID == -1);
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								existingEntry.firstBlockID = blockCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							if (detector.blockIDMap.find(blockCount) == detector.blockIDMap.end()) {
								blockdetails.firstOpInBlock = opCount;
								detector.blockIDMap[blockCount] = blockdetails;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount << endl;
								cout << "Duplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								cout << endl;
								return -1;
							}

							stackElement.blockID = blockCount;
							stackForThreadAndBlockOrder.push(stackElement);
//							stackForTaskOrder.push(stackElement);
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
								cout << endl;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								existingEntry.nextOpInBlock = opCount;
								if (previousOpInThread.taskID.compare("") != 0)
									existingEntry.nextOpInTask = opCount;
								existingEntry.nextOpInThread = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							stackElement.blockID = previousOpInThread.blockID;
							stackElement.taskID = previousOpInThread.taskID;
							stackForThreadAndBlockOrder.push(stackElement);
//							stackForTaskOrder.push(stackElement);
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
							endAddressIntAlloc = baseAddressIntAlloc + it->second.range;

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
							endAddressIntFree = baseAddressIntFree + it->second.range;

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
						if (stackForThreadAndBlockOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for read " << opCount
								 << " on stackForThreadOrder\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
//							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;

							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstBlockID = blockCount;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstBlockID == -1);
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								existingEntry.firstBlockID = blockCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							if (detector.blockIDMap.find(blockCount) == detector.blockIDMap.end()) {
								blockdetails.firstOpInBlock = opCount;
								detector.blockIDMap[blockCount] = blockdetails;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount << endl;
								cout << "Duplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								cout << endl;
								return -1;
							}

							stackElement.blockID = blockCount;
							stackForThreadAndBlockOrder.push(stackElement);
//							stackForTaskOrder.push(stackElement);
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
								cout << endl;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								existingEntry.nextOpInBlock = opCount;
								if (previousOpInThread.taskID.compare("") != 0)
									existingEntry.nextOpInTask = opCount;
								existingEntry.nextOpInThread = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							stackElement.blockID = previousOpInThread.blockID;
							stackElement.taskID = previousOpInThread.taskID;
							stackForThreadAndBlockOrder.push(stackElement);
//							stackForTaskOrder.push(stackElement);

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
							endAddressIntAlloc = baseAddressIntAlloc + it->second.range;

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
							endAddressIntFree = baseAddressIntFree + it->second.range;

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
						if (stackForThreadAndBlockOrder.isEmpty(threadID)){
							cout << "WARNING: No previous op found for write " << opCount
								 << " on stackForThreadOrder\n";

#ifdef SANITYCHECK
							// Sanity check: all stacks need to be empty if this is the first op
							assert(stackForTaskOrder.isEmpty(threadID));
//							assert(stackForBlockOrder.isEmpty(threadID));
//							assert(stackForNestingOrder.isEmpty(threadID));
#endif

							// This means this is the first op in the thread
							blockCount++;
							opdetails.blockID = blockCount;

							if (detector.threadIDMap.find(threadID) == detector.threadIDMap.end()) {
								UAFDetector::threadDetails threaddetails;
								threaddetails.firstBlockID = blockCount;
								threaddetails.firstOpID = opCount;
								detector.threadIDMap[threadID] = threaddetails;
							} else {
								UAFDetector::threadDetails existingEntry = detector.threadIDMap[threadID];
#ifdef SANITYCHECK
								assert(existingEntry.firstBlockID == -1);
								assert(existingEntry.firstOpID == -1);
#endif
								existingEntry.firstOpID = opCount;
								existingEntry.firstBlockID = blockCount;
								detector.threadIDMap.erase(detector.threadIDMap.find(threadID));
								detector.threadIDMap[threadID] = existingEntry;
							}

							if (detector.blockIDMap.find(blockCount) == detector.blockIDMap.end()) {
								blockdetails.firstOpInBlock = opCount;
								detector.blockIDMap[blockCount] = blockdetails;
							} else {
								cout << "ERROR: Found duplicate entry for block " << blockCount << " in blockIDMap\n";
								cout << "While examining op " << opCount << endl;
								cout << "Duplicate entry:\n";
								detector.blockIDMap[blockCount].printBlockDetails();
								cout << endl;
								return -1;
							}

							stackElement.blockID = blockCount;
							stackForThreadAndBlockOrder.push(stackElement);
//							stackForTaskOrder.push(stackElement);
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
								cout << endl;
								return -1;
							} else {
								UAFDetector::opDetails existingEntry = detector.opIDMap[previousOpInThread.opID];

#ifdef SANITYCHECK
								// Sanity check: prev op has same threadID and taskID as current op
								assert(previousOpInThread.threadID == threadID);
								assert(existingEntry.threadID == threadID);
#endif

								existingEntry.nextOpInBlock = opCount;
								if (previousOpInThread.taskID.compare("") != 0)
									existingEntry.nextOpInTask = opCount;
								existingEntry.nextOpInThread = opCount;
								detector.opIDMap.erase(detector.opIDMap.find(previousOpInThread.opID));
								detector.opIDMap[previousOpInThread.opID] = existingEntry;
							}

							stackElement.blockID = previousOpInThread.blockID;
							stackElement.taskID = previousOpInThread.taskID;
							stackForThreadAndBlockOrder.push(stackElement);
//							stackForTaskOrder.push(stackElement);
						}

					}

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
		if (!stackForThreadAndBlockOrder.isEmpty(it->first)) {
			MultiStack::stackElementType lastOp = stackForThreadAndBlockOrder.peek(it->first);
			if (it->second.lastBlockID == -1) {
				UAFDetector::threadDetails existingEntry = it->second;
				existingEntry.lastBlockID = lastOp.blockID;
				detector.threadIDMap.erase(detector.threadIDMap.find(it->first));
				detector.threadIDMap[it->first] = existingEntry;
			}
			if (detector.blockIDMap.find(lastOp.blockID) == detector.blockIDMap.end()) {
				cout << "ERROR: Cannot find entry for block " << lastOp.blockID << " in blockIDMap\n";
				return -1;
			} else {
				UAFDetector::blockDetails existingEntry = detector.blockIDMap[lastOp.blockID];
				if (existingEntry.lastOpInBlock == -1) {
					existingEntry.lastOpInBlock = lastOp.opID;
					detector.blockIDMap.erase(detector.blockIDMap.find(lastOp.blockID));
					detector.blockIDMap[lastOp.blockID] = existingEntry;
				}
			}

			if (lastOp.taskID.compare("") == 0) continue;

			if (detector.taskIDMap.find(lastOp.taskID) == detector.taskIDMap.end()) {
				cout << "ERROR: Cannot find entry for task " << lastOp.taskID << " in taskIDMap\n";
				return -1;
			} else {
				UAFDetector::taskDetails existingEntry = detector.taskIDMap[lastOp.taskID];
				if (existingEntry.lastBlockID == -1)
					existingEntry.lastBlockID = lastOp.blockID;
				if (!stackForTaskOrder.isEmpty(it->first)) {
					MultiStack::stackElementType lastConcurrencyOpInTask = stackForTaskOrder.peek(it->first);
					if (lastConcurrencyOpInTask.opType.compare("resume") == 0) {
						if (existingEntry.lastResumeOpID == -1)
							existingEntry.lastResumeOpID = lastConcurrencyOpInTask.opID;
					}
				}
				detector.taskIDMap.erase(detector.taskIDMap.find(lastOp.taskID));
				detector.taskIDMap[lastOp.taskID] = existingEntry;
			}
		}
	}

	cout << "Finished parsing the file\n";

	// Initialize HB Graph
	detector.initGraph(opCount, blockCount);

#ifdef TRACEDEBUG
	cout << "No of ops: " << opCount << endl;
	cout << "No of blocks: " << detector.blockIDMap.size() << endl;
	cout << "No of tasks: " << detector.taskIDMap.size() << endl;
	long long numOfAtomicTasks = 0;
	for (map<string, UAFDetector::taskDetails>::iterator it = detector.taskIDMap.begin(); it != detector.taskIDMap.end(); it++) {
		if (it->second.atomic == true)
			numOfAtomicTasks++;
	}
	cout << "No of atomic tasks: " << numOfAtomicTasks << endl;
	cout << "No of nesting loops: " << detector.nestingLoopMap.size() << endl;
	cout << "No of threads: " << detector.threadIDMap.size() << endl;

	cout << "\nOps: \n";
	for (map<IDType, UAFDetector::opDetails>::iterator it = detector.opIDMap.begin(); it != detector.opIDMap.end(); it++) {
		cout << "\nOp: " << it->first << " - details: ";
		it->second.printOpDetails();
		cout << endl;
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
	cout << "\nBlocks:\n";
	for (map<IDType, UAFDetector::blockDetails>::iterator it = detector.blockIDMap.begin(); it != detector.blockIDMap.end(); it++) {
		cout << "Block ID: " << it->first << " - details: ";
		it->second.printBlockDetails();
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
#endif

	return 0;
}
