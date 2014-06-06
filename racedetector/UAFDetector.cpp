/*
 * UAFDetector.cpp
 *
 *  Created on: 30-May-2014
 *      Author: shalini
 */

#include "UAFDetector.h"
#include <iostream>
#include <cassert>

#include <debugconfig.h>

using namespace std;

UAFDetector::UAFDetector() {
}

UAFDetector::~UAFDetector() {
}

void UAFDetector::initGraph(long long countOfNodes) {
	graph = HBGraph(countOfNodes);
}

int UAFDetector::addEdges() {
	assert (graph.totalNodes != 0);

	// LOOP-PO
	if (addLoopPOEdges() < 0) {
		cout << "ERROR: While adding LOOP-PO edges\n";
		return -1;
	}

	// TASK-PO
	if (addTaskPOEdges() < 0) {
		cout << "ERROR: While adding TASK-PO edges\n";
		return -1;
	}

	// ENQUEUE-ST
	if (addEnqueueSTorMTEdges() < 0) {
		cout << "ERROR: While adding ENQUEUE-ST/MT edges\n";
		return -1;
	}

	// FORK
	if (addForkEdges() < 0) {
		cout << "ERROR: While adding FORK edges\n";
		return -1;
	}

	// JOIN
	if (addJoinEdges() < 0) {
		cout << "ERROR: While adding JOIN edges\n";
		return -1;
	}

	// LOCK
	if (addLockEdges() < 0) {
		cout << "ERROR: While adding LOCK edges\n";
		return -1;
	}

	// CALLBACK-ST
	if (addCallbackSTEdges() < 0) {
		cout << "ERROR: While adding LOCK edges\n";
		return -1;
	}

#if 0
	bool edgeAdded = false;
	while (true) {
		int retValue;

		// FIFO-ATOMIC
		retValue = addFifoAtomicEdges();
		if (retValue == 1) edgeAdded = true;
		else if (retValue == 0) edgeAdded = false;
		else if (retValue == -1) {
			cout << "ERROR: While adding FIFO-ATOMIC edges\n";
			return -1;
		} else {
			cout << "ERROR: Unknown return value from addFifoAtomicEdges()\n";
			return -1;
		}

		// NO-PRE
		retValue = addNoPreEdges();
		if (retValue == 1) edgeAdded = true;
		else if (retValue == 0) edgeAdded = false;
		else if (retValue == -1) {
			cout << "ERROR: While adding NO-PRE edges\n";
			return -1;
		} else {
			cout << "ERROR: Unknown return value from addNoPreEdges()\n";
			return -1;
		}

		if (!edgeAdded) // If no edges were added in this iteration, stop.
			break;
	}
#endif
	return 0;
}

int UAFDetector::addLoopPOEdges() {
	// Adding LOOP-PO edges

	bool flag = false; // To keep track of whether edges were added.

	for (set<long long>::iterator it = threadinitSet.begin(); it != threadinitSet.end(); it++) {

		// Start from threadinit. For each op1 \in {threadinit, ... enterloop} add edge(op1, all-ops-after-op1).
		// First part of the rule - enterloop \notin {\alpha_1, ..., \alpha_i -1}

		long long alpha_i = *it;
		long long alpha_j;
		long long threadID = opIDMap[alpha_i].threadID;
		long long enterloopid = threadIDMap[threadID].enterloopOpID;

		// Until after enterloop is processed, i.e. until alpha_i reaches next of enterloop
		while (alpha_i != opToNextOpInThread[enterloopid]) {
			alpha_j = opToNextOpInThread[alpha_i];
			while (alpha_j != 0) {
				if (alpha_i < alpha_j) {
					int addEdgeRetValue = graph.addSingleEdge(alpha_i, alpha_j);
					if (addEdgeRetValue == 1) flag = true; // New edge added
					else if (addEdgeRetValue == 0) flag = false; // Edge already present.
					else if (addEdgeRetValue == -1) {
						cout << "ERROR: While adding Loop-PO edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					} else {
						cout << "ERROR: Unknown return value from graph.addSingleEdge(" << alpha_i << ", " << alpha_j << ")\n";
						return -1;
					}
				}
				alpha_j = opToNextOpInThread[alpha_j];
			}
			alpha_i = opToNextOpInThread[alpha_i];
		}
#if 0
		long long enterloopid; // temporary to store the id of enterloop operation.
		// Until we find enterloop in this thread
		while (opIDMap[alpha_i].opType.compare("enterloop") != 0) {
			alpha_j = opToNextOpInThread[alpha_i];
			while (alpha_j != 0) {
				if (alpha_i < alpha_j) {
					if (graph.addSingleEdge(alpha_i, alpha_j) < 0) {
						cout << "ERROR: While adding Loop-PO edge from " << alpha_i << " to " << alpha_j;
						return -1;
					} else
						flag = true;
					alpha_j = opToNextOpInThread[alpha_j];
				} else {
					cout << "ERROR: Found potential edge between " << alpha_i << " and " << alpha_j << endl;
					return -1;
				}
			}
			alpha_i = opToNextOpInThread[alpha_i];
		}

		// We found enterloop. We need to add edges from enterloop to all other ops as well.
		// May be bad programming - because replicating the entire loop body here.
		// alpha_i is enterloop.
		enterloopid = alpha_i;
		alpha_j = opToNextOpInThread[alpha_i];
		while (alpha_j != 0) {
			if (alpha_i < alpha_j) {
				if (graph.addSingleEdge(alpha_i, alpha_j) < 0) {
					cout << "ERROR: While adding Loop-PO edge from " << alpha_i << " to " << alpha_j;
					return -1;
				} else
						flag = true;
				alpha_j = opToNextOpInThread[alpha_j];
			} else {
				cout << "ERROR: Found potential edge between " << alpha_i << " and " << alpha_j << endl;
				return -1;
			}
		}
#endif

		// Start from exitloop. For each op2 \in {exitloop, ... threadexit}, add edge (any-op-before-op2, op2).
		// For ops \in {threadinit, ... , enterloop}, this would be redundant, so we skip those and start after enterloop.
		// Second part of the rule - exitloop \in {\alpha_1, ..., \alpha_j}

		// Obtain the exitloop corresponding to this thread.
		alpha_j = threadIDMap[threadID].exitloopOpID;
#if 0
		set<long long>::iterator exitIt;
		for (exitIt = exitloopSet.begin(); exitIt != exitloopSet.end(); exitIt++) {
			if (opIDMap[*exitIt].threadID == opIDMap[*it].threadID) {
				// Found the exitloop corresponding to the threadinit
				break;
			}
		}

		if (exitIt != exitloopSet.end()) {
			alpha_j = *exitIt;
#endif
#if 0
			// Until we find threadexit, add edges.
			//while (opIDMap[alpha_j].opType.compare("threadexit") != 0)
			// But there need not be threadexit even if we find an exit loop. So..
#endif
			if (alpha_j == -1)
				continue;
			while (alpha_j != 0) {
				alpha_i = opToNextOpInThread[enterloopid]; // alpha_i is initialized to next operation after enterloop.
				// Loop till you reach alpha_j
				while (alpha_i < alpha_j) {
					if (graph.addSingleEdge(alpha_i, alpha_j) < 0) {
						cout << "ERROR: While adding Loop-PO edge from " << alpha_i << " to " << alpha_j;
						return -1;
					} else
						flag = true;
					alpha_i = opToNextOpInThread[alpha_i];
				}
				alpha_j = opToNextOpInThread[alpha_j];
			}
//		}
#if 0
		// Currently assuming there need not be exitloop for all threads.
		else {
			cout << "ERROR: Cannot find exitloop for thread " <<  opIDMap[*it].threadID << endl;
			return -1;
		}
#endif
	}

	if (flag)
		return 1;
	else
		return 0;
}

int UAFDetector::addTaskPOEdges() {
	// Adding TASK-PO edges
	// Start at each deq, add edge to each op in the current task. This is done by
	// going over the next-op-in-task map.

	bool flag = false; // To keep track of whether edges were added.

	for (set<long long>::iterator it = deqSet.begin(); it != deqSet.end(); it++) {
		long long alpha_i = *it;
		long long alpha_j;

		while (alpha_i != 0) {
			alpha_j = opToNextOpInTask[alpha_i];
			while (alpha_j != 0) {
				int addEdgeRetValue = graph.addSingleEdge(alpha_i, alpha_j);
				if (addEdgeRetValue == 1) flag = true; // New edge added
				else if (addEdgeRetValue == 0) flag = false; // Edge already present
				else if (addEdgeRetValue == -1) {
					cout << "ERROR: While adding Task-PO edge from " << alpha_i << " to " << alpha_j << endl;
					return -1;
				} else {
					cout << "ERROR: Unknown return value when addSingleEdge(" << alpha_i << ", " << alpha_j << ")\n";
					return -1;
				}
				alpha_j = opToNextOpInTask[alpha_j];
			}
			alpha_i = opToNextOpInTask[alpha_i];
		}
	}

	if (flag)
		return 1;
	else
		return 0;
}

int UAFDetector::addEnqueueSTorMTEdges() {
	// Adding ENQUEUE-ST.
	// Iterate through enqSet and deqSet, find matching enq and deq.

	bool flag = false; // To keep track of whether edges were added.

#if 0
	for (map<long long, enqOpDetails>::iterator enqIt = enqSet.begin(); enqIt != enqSet.end(); enqIt++) {
		long long alpha_i = enqIt->first;
		long long alpha_j;

		string enqTask = enqIt->second.taskEnqueued;
		long long enqTargetThread = enqIt->second.targetThreadID;

		for (set<long long>::iterator deqit = deqSet.begin(); deqit != deqSet.end(); deqit++) {
			alpha_j = *deqit;
			if (opIDMap[alpha_j].taskID.compare(enqTask) == 0 && opIDMap[alpha_j].threadID == enqTargetThread) {
				if (graph.addSingleEdge(alpha_i, alpha_j) < 0) {
					cout << "ERROR: While adding Enqueue-ST/MT edge from " << alpha_i << " to " << alpha_j	<< endl;
					return -1;
				} else
					flag = true;
			}
		}
	}
#endif
	for (map<string, taskDetails>::iterator taskIt = taskIDMap.begin(); taskIt != taskIDMap.end(); taskIt++) {
		cout << "Task " << taskIt->first << endl;
		long long alpha_i = taskIt->second.enqOpID;
		long long alpha_j = taskIt->second.deqOpID;

		int addEdgeRetValue = graph.addSingleEdge(alpha_i, alpha_j);
		if (addEdgeRetValue == 1) flag = true; // New edge added
		else if (addEdgeRetValue == 0) flag = false; // Edge already present
		else if (addEdgeRetValue == -1) {
			cout << "ERROR: While adding Enqueue-ST/MT edge from " << alpha_i << " to " << alpha_j << endl;
			return -1;
		} else {
			cout << "ERROR: Unknown return value from addSingleEdge(" << alpha_i << ", " << alpha_j << ")\n";
			return -1;
		}
	}

	if (flag)
		return 1;
	else
		return 0;
}

int UAFDetector::addForkEdges() {
	// Adding FORK edges.
	// Iterate through forkSet and threadinitSet, obtain matching fork and threadinit.

	bool flag = false; // To keep track of whether edges were added.

#if 0
	for (map<long long, forkAndJoinOpDetails>::iterator forkIt = forkSet.begin(); forkIt != forkSet.end(); forkIt++) {
		long long alpha_i = forkIt->first;
		long long alpha_j;

		long long forkedThreadID = forkIt->second.targetThreadID;
		for (set<long long>::iterator threadinitIt = threadinitSet.begin(); threadinitIt != threadinitSet.end(); threadinitIt++) {
			alpha_j = *threadinitIt;
			if (opIDMap[alpha_j].threadID == forkedThreadID) {
				if (graph.addSingleEdge(alpha_i, alpha_j) < 0) {
					cout << "ERROR: While adding Fork edge from " << alpha_i << " to " << alpha_j	<< endl;
					return -1;
				} else
					flag = true;
			}
		}
	}
#endif
	for (map<long long, threadDetails>::iterator threadIt = threadIDMap.begin(); threadIt != threadIDMap.end(); threadIt++) {
		long long alpha_i = threadIt->second.forkOpID;
		long long alpha_j = threadIt->second.threadinitOpID;

		if (alpha_i == -1) {
			ERRLOG("Cannot find fork for thread " + threadIt->first);
			continue;
		}
		else if (alpha_j == -1) {
			ERRLOG("Cannot find threadinit for thread " + threadIt->first);
			continue;
		}
		int addEdgeRetValue = graph.addSingleEdge(alpha_i, alpha_j);
		if (addEdgeRetValue == 1) flag = true; // New edge added
		else if (addEdgeRetValue == 0) flag = false; // Edge already present
		else if (addEdgeRetValue == -1) {
			cout << "ERROR: While adding Fork edge from " << alpha_i << " to " << alpha_j << endl;
			return -1;
		} else {
			cout << "ERROR: Unknown return value from addSingleEdge(" << alpha_i << ", " << alpha_j << ")\n";
			return -1;
		}
	}

	if (flag)
		return 1;
	else
		return 0;
}

int UAFDetector::addJoinEdges() {
	// Adding JOIN edges.
	// Iterate through threadexitSet and joinSet, obtain matching threadexit and join.

	bool flag = false; // To keep track of whether edges were added.

#if 0
	for (set<long long>::iterator threadexitIt = threadexitSet.begin(); threadexitIt != threadexitSet.end(); threadexitIt++) {
		long long alpha_i = *threadexitIt;
		long long alpha_j;

		for (map<long long, forkAndJoinOpDetails>::iterator joinIt = joinSet.begin(); joinIt != joinSet.end(); joinIt++) {
			alpha_j = joinIt->first;
			if (joinIt->second.targetThreadID == opIDMap[alpha_i].threadID) {
				if (graph.addSingleEdge(alpha_i, alpha_j) < 0) {
					cout << "ERROR: While adding Join edge from " << alpha_i << " to " << alpha_j	<< endl;
					return -1;
				} else
					flag = true;
			}
		}
	}
#endif
	for (map<long long, threadDetails>::iterator threadIt = threadIDMap.begin(); threadIt != threadIDMap.end(); threadIt++) {
		long long alpha_i = threadIt->second.threadexitOpID;
		long long alpha_j = threadIt->second.joinOpID;

		if (alpha_i == -1) {
			ERRLOG("Cannot find threadexit for thread " + threadIt->first);
			continue;
		}
		else if (alpha_j == -1) {
			ERRLOG("Cannot find join for thread " + threadIt->first);
			continue;
		}
		int addEdgeRetValue = graph.addSingleEdge(alpha_i, alpha_j);
		if (addEdgeRetValue == 1) flag = true; // New edge added
		else if (addEdgeRetValue == 0) flag = false; // Edge already present
		else if (addEdgeRetValue == -1) {
			cout << "ERROR: While adding Join edge from " << alpha_i << " to " << alpha_j << endl;
			return -1;
		} else {
			cout << "ERROR: Unknown return value from addSingleEdge(" << alpha_i << ", " << alpha_j << ")\n";
			return -1;
		}
	}

	if (flag)
		return 1;
	else
		return 0;
}

int UAFDetector::addLockEdges() {
	// Adding LOCK edges.
	// Iterate through acquireSet and releaseSet, obtain matching acquire and release.

	bool flag = false; // To keep track of whether edges were added.

	for (map<long long, acquireAndReleaseOpDetails>::iterator releaseIt = releaseSet.begin(); releaseIt != releaseSet.end(); releaseIt++) {
		long long alpha_i = releaseIt->first;
		long long alpha_j;

		long long releaseThreadID = releaseIt->second.currThreadID;
		string releaseLockID = releaseIt->second.lockID;

		for (map<long long, acquireAndReleaseOpDetails>::iterator acquireIt = acquireSet.begin(); acquireIt != acquireSet.end(); acquireIt++) {
			alpha_j = acquireIt->first;

			long long acquireThreadID = acquireIt->second.currThreadID;
			string acquireLockID = acquireIt->second.lockID;
			if(releaseLockID.compare(acquireLockID) == 0 && releaseThreadID != acquireThreadID) {
				if (graph.addSingleEdge(alpha_i, alpha_j) < 0) {
					cout << "ERROR: While adding Lock edge from " << alpha_i << " to " << alpha_j << endl;
					return -1;
				} else
					flag = true;
			}
		}
	}

	if (flag)
		return 1;
	else
		return 0;
}

int UAFDetector::addCallbackSTEdges() {

	// Adding CALLBACK-ST edges.
	// Iterate through endSet. If next operation after end is a resume in the same task, then add edge.

	bool flag = false; // To keep track of whether edges were added.

	for (set<long long>::iterator endIt = endSet.begin(); endIt != endSet.end(); endIt++) {
		long long alpha_i  = *endIt;
		long long alpha_j;

		long long endThreadID = opIDMap[alpha_i].threadID;

		alpha_j = opToNextOpInThread[alpha_i];
		if (alpha_j != 0) {
			if (opIDMap[alpha_j].opType.compare("resume") == 0 && opIDMap[alpha_j].threadID == endThreadID) {
				if (graph.addSingleEdge(alpha_i, alpha_j) < 0) {
					cout << "ERROR: While adding Callback-ST edge from " << alpha_i << " to " << alpha_j << endl;
					return -1;
				} else
					flag = true;
			}
		}
	}

	if (flag)
		return 1;
	else
		return 0;
}

int UAFDetector::addFifoAtomicEdges() {

	// Adding FIFO-ATOMIC edges.
	// Iterate through enqSet. Find two enq's such that enq(-, p1, t, c) < enq(-, p2, t, c'). If noPause(p1), add edge between end(t,p1) and deq(t,p2)

	bool flag = false; // To keep track of whether edges were added.

	for (map<long long, enqOpDetails>::iterator enq1It = enqSet.begin(); enq1It != enqSet.end(); enq1It++) {
		long long enq_1 = enq1It->first;
		string taskEnqueued_1 = enq1It->second.taskEnqueued;
		long long targetThreadID_1 = enq1It->second.targetThreadID;

		// Check through all edges outgoing from enq_1. If there is an enq of the form we are looking for, continue.
		for (HBGraph::adjListNode* currNode = graph.adjList[enq_1].head; currNode != NULL; currNode = currNode->next) {
			if (opIDMap[currNode->destination].opType.compare("enq") == 0) {
				long long enq_2 = currNode->destination;
				string taskEnqueued_2 = enqSet[enq_2].taskEnqueued;
				long long targetThreadID_2 = enqSet[enq_2].targetThreadID;

				if (targetThreadID_1 == targetThreadID_2 && taskEnqueued_1 != taskEnqueued_2) { // If the enq posts to the same thread
					// Found enq2. Now, check if taskEnqueued1 is an atomic task.
					if (atomicTasks.find(taskEnqueued_1) != atomicTasks.end()) {
						// taskEnqueued_1 is an atomic task, i.e. noPause(p1) is true
						// Find end(targetThreadID_1, taskEnqueued_1) and deq(targetThreadID_2, taskEnqueued_2)

#if 0
						for (set<long long>::iterator endIt = endSet.begin(); endIt != endSet.end(); endIt++) {
							long long alpha_i = *endIt;
							if (opIDMap[alpha_i].threadID == targetThreadID_1 && opIDMap[alpha_i].taskID.compare(taskEnqueued_1) == 0) {
								// Found end(targetThreadID_1, taskEnqueued_1)
								for (set<long long>::iterator deqIt = deqSet.begin(); deqIt != deqSet.end(); deqIt++) {
									long long alpha_j = *deqIt;
									if (opIDMap[alpha_j].threadID == targetThreadID_2 && opIDMap[alpha_j].taskID.compare(taskEnqueued_2) == 0) {
										// Found deq(targetThreadID_2, taskEnqueued_2)
										int addEdgeRetValue = graph.addSingleEdge(alpha_i, alpha_j);
//										if (graph.addSingleEdge(alpha_i, alpha_j) < 0) {
										if (addEdgeRetValue == 1) flag = true; // New edge added.
										else if (addEdgeRetValue == 0) flag = false; // Edge already present.
										else if (addEdgeRetValue == -1) {
											cout << "ERROR: While adding Fifo-Atomic edge from " << alpha_i << " to " << alpha_j << endl;
											return -1;
										} else {
											cout << "ERROR: Unknown return value from graph.addSingleEdge() while adding Fifo-Atomic edge from " << alpha_i << " to " << alpha_j << endl;
											return -1;
										}
										break;
									}
								}
								break;
							}
						}
					}
#endif
						long long alpha_i = taskIDMap[taskEnqueued_1].endOpID;
						assert(taskIDMap[taskEnqueued_1].threadID == targetThreadID_1);

						long long alpha_j = taskIDMap[taskEnqueued_2].deqOpID;
						assert(taskIDMap[taskEnqueued_2].threadID == targetThreadID_2);

						int addEdgeRetValue = graph.addSingleEdge(alpha_i, alpha_j);
						if (addEdgeRetValue == 1) flag = true; // New edge added.
						else if (addEdgeRetValue == 0) flag = false; // Edge already present.
						else if (addEdgeRetValue == -1) {
							cout << "ERROR: While adding Fifo-Atomic edge from " << alpha_i << " to " << alpha_j << endl;
							return -1;
						} else {
							cout << "ERROR: Unknown return value from graph.addSingleEdge() while adding Fifo-Atomic edge from " << alpha_i << " to " << alpha_j << endl;
							return -1;
						}
					}
				} else if (taskEnqueued_1 == taskEnqueued_2) {
					// Suspicious because we found same task enqueued twice.
					cout << "ERROR: Found two instances of enq with same enqueued task " << enq_1 << " and " << enq_2
						 << endl;
					return -1;
				}
			}
		}
	}

	if (flag)
		return 1;
	else
		return 0;
}

int UAFDetector::addNoPreEdges() {
	// Adding NO-PRE edges.
	// For each atomic task p1 (executing in thread t), find enq(-, p2, t, c). For each op \in {deq(p1),...end(p1)},
	// if op < enq(-, p2, t, c), then add edge between end(p1) and deq(p2).

	bool flag = false; // To keep track of whether edges were added.

	for (set<string>::iterator atomicIt = atomicTasks.begin(); atomicIt != atomicTasks.end(); atomicIt++) {
		long long threadIDOfAtomicTask = taskIDMap[*atomicIt].threadID;
		string p1 = *atomicIt;
		long long alpha_i, alpha_j;

		// Obtain end(p1).
		for (set<long long>::iterator endIt = endSet.begin(); endIt != endSet.end(); endIt++) {
			if (opIDMap[*endIt].taskID.compare(p1) == 0) {
				alpha_i = *endIt;
				break;
			}
		}

		for (map<long long, enqOpDetails>::iterator enqIt = enqSet.begin(); enqIt != enqSet.end(); enqIt++) {
			if (enqIt->second.targetThreadID == threadIDOfAtomicTask && enqIt->second.taskEnqueued.compare(p1) != 0) {
				string p2 = enqIt->second.taskEnqueued;

				long long deqp1 = -1;
				alpha_j = -1;
				for (set<long long>::iterator deqIt = deqSet.begin(); deqIt != deqSet.end(); deqIt++) {
					if (opIDMap[*deqIt].taskID.compare(p2) == 0)
						alpha_j = *deqIt;
					if (opIDMap[*deqIt].taskID.compare(p1) == 0)
						deqp1 = *deqIt;
					if (deqp1 != -1 && alpha_j != -1)
						break;
				}

				if (alpha_j != -1) {
					// If no edge between alpha_i and alpha_j
					if (graph.edgeExists(alpha_i, alpha_j) == 0) {
						long long op = deqp1;
						while (op != alpha_i) {
							// Check if edge exists between op and alpha_j, If so ...
							if (graph.edgeExists(op, alpha_j) == 1) {
								int addEdgeRetValue = graph.addSingleEdge(alpha_i, alpha_j);
								if (addEdgeRetValue == 1) flag = true; // New edge added.
								else if (addEdgeRetValue == 0) flag = false; // Edge already present.
								else if (addEdgeRetValue == -1) {
									cout << "ERROR: While adding Nopre edge from " << alpha_i << " to " << alpha_j << endl;
									return -1;
								} else {
									cout << "ERROR: Unknown return value from graph.addSingleEdge() while adding Nopre edge from " << alpha_i << " to " << alpha_j << endl;
									return -1;
								}
								break;
							}
							op = opToNextOpInTask[op];
						}
					}
				}
			}
		}
	}

	if (flag)
		return 1;
	else
		return 0;
}

#ifdef GRAPHDEBUG
void UAFDetector::printEdges() {

	for (long long i=1; i <= graph.totalNodes; i++) {
		for (long long j=1; j <= graph.totalNodes; j++) {
			int retValue = graph.edgeExists(i, j);
			if (retValue == 1) {
				cout << "Edge: (" << i << ", " << opIDMap[i].opType << ") -- ("
								  << j << ", " << opIDMap[j].opType << ")\n";
			} else if (retValue == -1) {
				cout << "ERROR: With edge (" << i << ", " << opIDMap[i].opType << ") -- ("
											 << j << ", " << opIDMap[j].opType << ")\n";
			}
		}
	}
}
#endif
