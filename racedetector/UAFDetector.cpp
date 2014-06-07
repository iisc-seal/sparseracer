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

#define NILCallback "0x0"

UAFDetector::UAFDetector() {
}

UAFDetector::~UAFDetector() {
}

void UAFDetector::initGraph(long long countOfNodes) {
	graph = HBGraph(countOfNodes);
}

int UAFDetector::addEdges(Logger &logger) {
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

	// ENQUEUE-ST/MT
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
		cout << "ERROR: While adding CALLBACK-ST edges\n";
		return -1;
	}
#if 0
#endif

	bool edgeAdded = false;
	while (true) {
		int retValue;

#ifdef GRAPHDEBUG
		graph.printGraph(false);
#endif

		cout << "Adding Fifo-Atomic edges\n";
		// FIFO-ATOMIC
		retValue = addFifoAtomicEdges();
		if (retValue == 1) edgeAdded = true;
		else if (retValue == 0) edgeAdded = edgeAdded || false; // Didn't add any new edge
		else if (retValue == -1) {
			cout << "ERROR: While adding FIFO-ATOMIC edges\n";
			return -1;
		} else {
			cout << "ERROR: Unknown return value from addFifoAtomicEdges()\n";
			return -1;
		}

#ifdef GRAPHDEBUG
		if (retValue == 1)
			graph.printGraph(false);
#endif

		cout << "Adding No-Pre edges\n";
		// NO-PRE
		retValue = addNoPreEdges();
		if (retValue == 1) edgeAdded = true;
		else if (retValue == 0) edgeAdded = edgeAdded || false; // Didn't add any new edge
		else if (retValue == -1) {
			cout << "ERROR: While adding NO-PRE edges\n";
			return -1;
		} else {
			cout << "ERROR: Unknown return value from addNoPreEdges()\n";
			return -1;
		}

#ifdef GRAPHDEBUG
		if (retValue == 1)
			graph.printGraph(false);
#endif

		cout << "Adding Fifo-Callback edges\n";
		// FIFO-CALLBACK
		retValue = addFifoCallbackEdges();
		if (retValue == 1) edgeAdded = true;
		else if (retValue == 0) edgeAdded = edgeAdded || false; // Didn't add any new edge
		else if (retValue == -1) {
			cout << "ERROR: While adding FIFO-CALLBACK edges\n";
			return -1;
		} else {
			cout << "ERROR: Unknown return value from addFifoCallbackEdges()\n";
			return -1;
		}

#ifdef GRAPHDEBUG
		if (retValue == 1)
			graph.printGraph(false);
#endif

		cout << "Adding Fifo-Nested edges\n";
		// FIFO-NESTED
		retValue = addFifoNestedEdges();
		if (retValue == 1) edgeAdded = true;
		else if (retValue == 0) edgeAdded = edgeAdded || false; // Didn't add any new edge
		else if (retValue == -1) {
			cout << "ERROR: While adding FIFO-NESTED edges\n";
			return -1;
		} else {
			cout << "ERROR: Unknown return value from addFifoNestedEdges()\n";
			return -1;
		}

#ifdef GRAPHDEBUG
		if (retValue == 1)
			graph.printGraph(false);
#endif

		cout << "Adding NoPre-Prefix edges\n";
		// NOPRE-PREFIX
		retValue = addNoPrePrefixEdges();
		if (retValue == 1) edgeAdded = true;
		else if (retValue == 0) edgeAdded = edgeAdded || false; // Didn't add any new edge
		else if (retValue == -1) {
			cout << "ERROR: While adding NOPRE-PREFIX edges\n";
			return -1;
		} else {
			cout << "ERROR: Unknown return value from addNoPrePrefixEdges()\n";
			return -1;
		}

#ifdef GRAPHDEBUG
		if (retValue == 1)
			graph.printGraph(false);
#endif

		cout << "Adding NoPre-Suffix edges\n";
		// NOPRE-SUFFIX
		retValue = addNoPreSuffixEdges();
		if (retValue == 1) edgeAdded = true;
		else if (retValue == 0) edgeAdded = edgeAdded || false; // Didn't add any new edge
		else if (retValue == -1) {
			cout << "ERROR: While adding NOPRE-SUFFIX edges\n";
			return -1;
		} else {
			cout << "ERROR: Unknown return value from addNoPreSuffixEdges()\n";
			return -1;
		}

#ifdef GRAPHDEBUG
		if (retValue == 1)
			graph.printGraph(false);
#endif

		cout << "Adding Trans-ST/MT edges\n";
		// TRANS-ST/MT
		retValue = addTransSTOrMTEdges();
		if (retValue == 1) edgeAdded = true;
		else if (retValue == 0) edgeAdded = edgeAdded || false; // Didn't add any new edge
		else if (retValue == -1) {
			cout << "ERROR: While adding TRANS-ST/MT edges\n";
			return -1;
		} else {
			cout << "ERROR: Unknown return value from addTransSTOrMTEdges()\n";
			return -1;
		}
#if 0
#endif

		if (!edgeAdded) // If no edges were added in this iteration, stop.
			break;
		else
			edgeAdded = false;
	}

#ifdef GRAPHDEBUG
	graph.printGraph(false);
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
					else if (addEdgeRetValue == 0) flag = flag || false; // No new edge added.
					else if (addEdgeRetValue == -1) {
						cout << "ERROR: While adding Loop-PO edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					} else {
						cout << "ERROR: Unknown return value from graph.addSingleEdge(" << alpha_i << ", " << alpha_j << ")\n";
						return -1;
					}
#ifdef GRAPHDEBUG
					if (addEdgeRetValue == 1)
						cout << "Loop-PO edge (" << alpha_i << "," << alpha_j << ")" << endl;
#endif
				}
				alpha_j = opToNextOpInThread[alpha_j];
			}
			alpha_i = opToNextOpInThread[alpha_i];
		}

		// Start from exitloop. For each op2 \in {exitloop, ... threadexit}, add edge (any-op-before-op2, op2).
		// For ops \in {threadinit, ... , enterloop}, this would be redundant, so we skip those and start after enterloop.
		// Second part of the rule - exitloop \in {\alpha_1, ..., \alpha_j}

		// Obtain the exitloop corresponding to this thread.
		alpha_j = threadIDMap[threadID].exitloopOpID;

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
					int addEdgeRetValue = graph.addSingleEdge(alpha_i, alpha_j);
					if (addEdgeRetValue == 1) flag = true;
					else if (addEdgeRetValue == 0) flag = flag || false; // No new edge added.
					else if (addEdgeRetValue == -1) {
						cout << "ERROR: While adding Loop-PO edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					} else {
						cout << "ERROR: Unknown return value from addSingleEdge() when adding Loop-PO edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					}
#ifdef GRAPHDEBUG
					if (addEdgeRetValue == 1)
						cout << "Loop-PO edge (" << alpha_i << "," << alpha_j << ")" << endl;
#endif
					alpha_i = opToNextOpInThread[alpha_i];
				}
				alpha_j = opToNextOpInThread[alpha_j];
			}
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
				else if (addEdgeRetValue == 0) flag = flag || false; // No new edge added.
				else if (addEdgeRetValue == -1) {
					cout << "ERROR: While adding Task-PO edge from " << alpha_i << " to " << alpha_j << endl;
					return -1;
				} else {
					cout << "ERROR: Unknown return value when addSingleEdge(" << alpha_i << ", " << alpha_j << ")\n";
					return -1;
				}
#ifdef GRAPHDEBUG
					if (addEdgeRetValue == 1)
						cout << "Task-PO edge (" << alpha_i << "," << alpha_j << ")" << endl;
#endif
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

	for (map<string, taskDetails>::iterator taskIt = taskIDMap.begin(); taskIt != taskIDMap.end(); taskIt++) {
		long long alpha_i = taskIt->second.enqOpID;
		long long alpha_j = taskIt->second.deqOpID;

		int addEdgeRetValue = graph.addSingleEdge(alpha_i, alpha_j);
		if (addEdgeRetValue == 1) flag = true; // New edge added
		else if (addEdgeRetValue == 0) flag = flag || false; // No new edge added.
		else if (addEdgeRetValue == -1) {
			cout << "ERROR: While adding Enqueue-ST/MT edge from " << alpha_i << " to " << alpha_j << endl;
			return -1;
		} else {
			cout << "ERROR: Unknown return value from addSingleEdge(" << alpha_i << ", " << alpha_j << ")\n";
			return -1;
		}
#ifdef GRAPHDEBUG
		if (addEdgeRetValue == 1)
			cout << "Enqueue-ST/MT edge (" << alpha_i << "," << alpha_j << ")" << endl;
#endif
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

	for (map<long long, threadDetails>::iterator threadIt = threadIDMap.begin(); threadIt != threadIDMap.end(); threadIt++) {
		long long alpha_i = threadIt->second.forkOpID;
		long long alpha_j = threadIt->second.threadinitOpID;

		if (alpha_i == -1) {
			cout << "Cannot find fork for thread " << threadIt->first << endl;
			continue;
		}
		else if (alpha_j == -1) {
			cout << "Cannot find threadinit for thread " << threadIt->first << endl;
			continue;
		}
		int addEdgeRetValue = graph.addSingleEdge(alpha_i, alpha_j);
		if (addEdgeRetValue == 1) flag = true; // New edge added
		else if (addEdgeRetValue == 0) flag = flag || false; // No new edge added.
		else if (addEdgeRetValue == -1) {
			cout << "ERROR: While adding Fork edge from " << alpha_i << " to " << alpha_j << endl;
			return -1;
		} else {
			cout << "ERROR: Unknown return value from addSingleEdge(" << alpha_i << ", " << alpha_j << ")\n";
			return -1;
		}
#ifdef GRAPHDEBUG
		if (addEdgeRetValue == 1)
			cout << "Fork edge (" << alpha_i << "," << alpha_j << ")" << endl;
#endif
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

	for (map<long long, threadDetails>::iterator threadIt = threadIDMap.begin(); threadIt != threadIDMap.end(); threadIt++) {
		long long alpha_i = threadIt->second.threadexitOpID;
		long long alpha_j = threadIt->second.joinOpID;

		if (alpha_i == -1) {
			cout << "Cannot find threadexit for thread " << threadIt->first << endl;
			continue;
		}
		else if (alpha_j == -1) {
			cout << "Cannot find join for thread " << threadIt->first << endl;
			continue;
		}
		int addEdgeRetValue = graph.addSingleEdge(alpha_i, alpha_j);
		if (addEdgeRetValue == 1) flag = true; // New edge added
		else if (addEdgeRetValue == 0) flag = flag || false; // No new edge added.
		else if (addEdgeRetValue == -1) {
			cout << "ERROR: While adding Join edge from " << alpha_i << " to " << alpha_j << endl;
			return -1;
		} else {
			cout << "ERROR: Unknown return value from addSingleEdge(" << alpha_i << ", " << alpha_j << ")\n";
			return -1;
		}
#ifdef GRAPHDEBUG
		if (addEdgeRetValue == 1)
			cout << "Join edge (" << alpha_i << "," << alpha_j << ")" << endl;
#endif
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
				int addEdgeRetValue = graph.addSingleEdge(alpha_i, alpha_j);
				if (addEdgeRetValue == 1) flag = true;
				else if (addEdgeRetValue == 0) flag = flag || false; // No new edge added.
				else if (addEdgeRetValue == -1) {
					cout << "ERROR: While adding Lock edge from " << alpha_i << " to " << alpha_j << endl;
					return -1;
				} else {
					cout << "ERROR: Unknown return value from addSingleEdge() while adding Lock edge from " << alpha_i << " to " << alpha_j << endl;
					return -1;
				}
#ifdef GRAPHDEBUG
				if (addEdgeRetValue == 1)
					cout << "Lock edge (" << alpha_i << "," << alpha_j << ")" << endl;
#endif
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
				int addEdgeRetValue = graph.addSingleEdge(alpha_i, alpha_j);
				if (addEdgeRetValue == 1) flag = true;
				else if (addEdgeRetValue == 0) flag = flag || false; // No new edge added.
				else if (addEdgeRetValue == -1) {
					cout << "ERROR: While adding Callback-ST edge from " << alpha_i << " to " << alpha_j << endl;
					return -1;
				} else {
					cout << "ERROR: Unknown return value from addSingleEdge() while adding Callback-ST edge from " << alpha_i << " to " << alpha_j << endl;
					return -1;
				}
#ifdef GRAPHDEBUG
				if (addEdgeRetValue == 1)
					cout << "Callback-ST edge (" << alpha_i << "," << alpha_j << ")" << endl;
#endif
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

	for (set<string>::iterator atomicIt = atomicTasks.begin(); atomicIt != atomicTasks.end(); atomicIt++) {
		long long alpha_i = taskIDMap[*atomicIt].endOpID;
		long long enq_i = taskIDMap[*atomicIt].enqOpID;
		string task1 = *atomicIt;

		if (alpha_i == -1) {
			cout << "ERROR: Cannot find end of task " << *atomicIt << endl;
			continue;
		}
		if (enq_i == -1) {
			cout << "ERROR: Cannot find enq of task " << *atomicIt << endl;
			continue;
		}

		for (HBGraph::adjListNode* currNode = graph.adjList[enq_i].head; currNode != NULL; currNode = currNode->next) {
			if (opIDMap[currNode->destination].opType.compare("enq") == 0) {
				long long enq_2 = currNode->destination;
				string task2 = enqSet[enq_2].taskEnqueued;

				if (task1.compare(task2) != 0 && enqSet[enq_i].targetThreadID == enqSet[enq_2].targetThreadID) {
					long long alpha_j = taskIDMap[task2].deqOpID;

					if (alpha_j == -1) {
						cout << "ERROR: Cannot find deq of task " << task2	<< endl;
						continue;
					}

					int addEdgeRetValue = graph.addSingleEdge(alpha_i, alpha_j);
					if (addEdgeRetValue == 1) flag = true; // New edge added.
					else if (addEdgeRetValue == 0) flag = flag || false; // No new edge added.
					else if (addEdgeRetValue == -1) {
						cout << "ERROR: While adding Fifo-Atomic edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					} else {
						cout << "ERROR: Unknown return value from graph.addSingleEdge() while adding Fifo-Atomic edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					}
#ifdef GRAPHDEBUG
					if (addEdgeRetValue == 1)
						cout << "Fifo-Atomic edge (" << alpha_i << "," << alpha_j << ")" << endl;
#endif
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
		alpha_i = taskIDMap[*atomicIt].endOpID;
		if (alpha_i == -1) {
			cout << "ERROR: Cannot find end of atomic task " << *atomicIt << endl;
			continue;
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
								else if (addEdgeRetValue == 0) flag = flag || false; // No new edge added.
								else if (addEdgeRetValue == -1) {
									cout << "ERROR: While adding Nopre edge from " << alpha_i << " to " << alpha_j << endl;
									return -1;
								} else {
									cout << "ERROR: Unknown return value from graph.addSingleEdge() while adding Nopre edge from " << alpha_i << " to " << alpha_j << endl;
									return -1;
								}
#ifdef GRAPHDEBUG
								if (addEdgeRetValue == 1)
									cout << "Nopre edge (" << alpha_i << "," << alpha_j << ")" << endl;
#endif
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

int UAFDetector::addFifoCallbackEdges() {
	// Adding FIFO-CALLBACK
	// Find two enqs such that enq(-,p1,t,c') < enq(-,c,t,nil). Then add edge between end(p1) and deq(c)

	bool flag = false; // To keep track of whether edges were added.

	for (map<long long, enqOpDetails>::iterator enq1It = enqSet.begin(); enq1It != enqSet.end(); enq1It++) {
		long long enq_1 = enq1It->first;

		for (HBGraph::adjListNode* currNode = graph.adjList[enq_1].head; currNode != NULL; currNode = currNode->next) {
			long long destination = currNode->destination;
			if (opIDMap[destination].opType.compare("enq") != 0) continue;

			if ((enqSet[enq_1].targetThreadID == enqSet[destination].targetThreadID) && (enqSet[enq_1].taskEnqueued.compare(enqSet[destination].taskEnqueued) != 0) && (enqSet[destination].callback.compare(NILCallback) == 0)) {
				string task1 = enq1It->second.taskEnqueued;
				long long alpha_i = taskIDMap[task1].endOpID;

				string callback2 = enqSet[destination].taskEnqueued;
				long long alpha_j = taskIDMap[callback2].deqOpID;

				if (alpha_j == -1) {
					cout << "ERROR: Found enq of task " << callback2 << " at " << destination << ", but cannot find deq\n";
					continue;
				}
				if (alpha_i == -1) {
					cout << "ERROR: Found enq of task " << task1 << " at " << enq_1 << ", but cannot find end\n";
					break;
				}

				string parentTask = taskIDMap[task1].parentTask;
				long long enq_parent = taskIDMap[parentTask].enqOpID;
				if (enq_parent == -1 && parentTask.compare("") != 0) {
					cout << "ERROR: Cannot find enq of task " << parentTask << ", the parent task of " << task1 << endl;
					continue;
				}

				if (enq_parent == -1) {
					cout << "ERROR: No parent for task " << task1 << endl;
					continue;
				}

				if (graph.edgeExists(enq_parent, enq_1) != 1) continue;

				int addEdgeRetValue = graph.addSingleEdge(alpha_i, alpha_j);
				if (addEdgeRetValue == 1) flag = true; // New edge added.
				else if (addEdgeRetValue == 0) flag = flag || false; // No new edge added.
				else if (addEdgeRetValue == -1) {
					cout << "ERROR: While adding Fifo-Callback edge from " << alpha_i << " to " << alpha_j << endl;
					return -1;
				} else {
					cout << "ERROR: Unknown return value from graph.addSingleEdge() while adding Fifo-Callback edge from " << alpha_i << " to " << alpha_j << endl;
					return -1;
				}
#ifdef GRAPHDEBUG
				if (addEdgeRetValue == 1)
					cout << "Fifo-Callback edge (" << alpha_i << "," << alpha_j << ")" << endl;
#endif
			}
		}
	}

	if (flag)
		return 1;
	else
		return 0;
}

int UAFDetector::addFifoNestedEdges() {
	// Adding FIFO-NESTED
	// Find two enqs such that enq(-,p1,t,c) < enq(-,p2,t,c'). Then add edge between first-pause(p1) and deq(p2)

	bool flag = false; // To keep track of whether edges were added.

	for (map<long long, enqOpDetails>::iterator enq1It = enqSet.begin(); enq1It != enqSet.end(); enq1It++) {
		long long enq_1 = enq1It->first;
		string task1 = enq1It->second.taskEnqueued;
		if (atomicTasks.find(task1) != atomicTasks.end()) continue;

		for (HBGraph::adjListNode* currNode = graph.adjList[enq_1].head; currNode != NULL; currNode = currNode->next) {
			long long destination = currNode->destination;
			if (opIDMap[destination].opType.compare("enq") == 0) {
				if ((enqSet[enq_1].targetThreadID == enqSet[destination].targetThreadID) && (enqSet[enq_1].taskEnqueued.compare(enqSet[destination].taskEnqueued) != 0)) {
					long long alpha_i = taskIDMap[task1].firstPauseOpID;

					string task2 = enqSet[destination].taskEnqueued;
					long long alpha_j = taskIDMap[task2].deqOpID;

					if (alpha_j == -1) {
//						ERRLOG("ERROR: Found enq of task " + task2 + " at " + destination + ", but cannot find deq\n");
						cout << "ERROR: Found enq of task " << task2 << " at " << destination << ", but cannot find deq\n";
						continue;
					}
					if (alpha_i == -1) {
//						ERRLOG("ERROR: Found enq of task " + task1 + " at " + enq_1 + ", but cannot find first-pause\n");
						cout << "ERROR: Found enq of task " << task1 << " at " << enq_1 << ", but cannot find first-pause\n";
						break;
					}

					int addEdgeRetValue = graph.addSingleEdge(alpha_i, alpha_j);
					if (addEdgeRetValue == 1) flag = true; // New edge added.
					else if (addEdgeRetValue == 0) flag = flag || false; // No new edge added.
					else if (addEdgeRetValue == -1) {
						cout << "ERROR: While adding Fifo-Nested edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					} else {
						cout << "ERROR: Unknown return value from graph.addSingleEdge() while adding Fifo-Nested edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					}
#ifdef GRAPHDEBUG
					if (addEdgeRetValue == 1)
						cout << "Fifo-Nested edge (" << alpha_i << "," << alpha_j << ")" << endl;
#endif
				}
			}
		}
	}

	if (flag)
		return 1;
	else
		return 0;
}

int UAFDetector::addNoPrePrefixEdges() {
	// Adding NOPRE-PREFIX edges

	bool flag = false;

	for (map<string, taskDetails>::iterator taskIt = taskIDMap.begin(); taskIt != taskIDMap.end(); taskIt++) {
		// Find task with first pause.
		long long alpha_i = taskIt->second.firstPauseOpID;
		if (alpha_i == -1) continue;

		long long deqTask1 = taskIt->second.deqOpID;
		if (deqTask1 == -1) {
			cout << "ERROR: Cannot find deq of task " << taskIt->first << endl;
			continue;
		}

		long long currOp = deqTask1;
		while (currOp != 0) {
			for (HBGraph::adjListNode* currNode = graph.adjList[currOp].head; currNode != NULL; currNode = currNode->next) {
				// See if there is an edge to enq from currOp
				long long dest = currNode->destination;
				if (opIDMap[dest].opType.compare("enq") != 0) continue;

				string enqTask = enqSet[dest].taskEnqueued;
				// Check if the enq's targetThread is same as that of the currOp
				if (enqSet[dest].targetThreadID != opIDMap[currOp].threadID) continue;

				if (graph.edgeExists(currOp, alpha_i) == 1) {
					// currOp < enq and currOp < first-pause
					long long alpha_j = taskIDMap[enqTask].deqOpID;
					if (alpha_j == -1) {
						cout << "ERROR: Cannot find deq of task " << enqTask << " enqueued at " << dest << endl;
						continue;
					}

					int addEdgeRetValue = graph.addSingleEdge(alpha_i, alpha_j);
					if (addEdgeRetValue == 1) flag = true; // New edge added.
					else if (addEdgeRetValue == 0) flag = flag || false; // No new edge added.
					else if (addEdgeRetValue == -1) {
						cout << "ERROR: While adding NoPre-Prefix edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					} else {
						cout << "ERROR: Unknown return value from graph.addSingleEdge() while adding NoPre-Prefix edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					}
#ifdef GRAPHDEBUG
					if (addEdgeRetValue == 1)
						cout << "NoPrePrefix edge (" << alpha_i << "," << alpha_j << ")" << endl;
#endif
				}
			}
			currOp = opToNextOpInTask[currOp];
		}
	}

	if (flag)
		return 1;
	else
		return 0;
}

int UAFDetector::addNoPreSuffixEdges() {
	// Adding NOPRE-SUFFIX edges

	bool flag = false;

	for (map<string, taskDetails>::iterator taskIt = taskIDMap.begin(); taskIt != taskIDMap.end(); taskIt++) {
		long long lastresume = taskIt->second.lastResumeOpID;
		long long alpha_i = taskIt->second.endOpID;
		string task1 = taskIt->first;
		long long threadTask1 = taskIt->second.threadID;
		if (lastresume == -1) continue;
		if (alpha_i == -1) {
			cout << "ERROR: Cannot find end of task " << taskIt->first << endl;
			continue;
		}

		for (HBGraph::adjListNode* currNode = graph.adjList[lastresume].head; currNode != NULL; currNode = currNode->next) {
			long long currOp = currNode->destination;
			for (HBGraph::adjListNode* enqNode = graph.adjList[currOp].head; enqNode != NULL; enqNode = enqNode->next) {
				if (opIDMap[enqNode->destination].opType.compare("enq") != 0) continue;
				long long enq = enqNode->destination;
				if (enqSet[enq].taskEnqueued.compare(task1) != 0 && enqSet[enq].targetThreadID == threadTask1) {
					string task2 = enqSet[enq].taskEnqueued;
					long long alpha_j = taskIDMap[task2].deqOpID;
					if (alpha_j == -1) {
						cout << "ERROR: Cannot find deq of task " << task2 << endl;
						continue;
					}

					int addEdgeRetValue = graph.addSingleEdge(alpha_i, alpha_j);
					if (addEdgeRetValue == 1) flag = true; // New edge added.
					else if (addEdgeRetValue == 0) flag = flag || false; // No new edge added.
					else if (addEdgeRetValue == -1) {
						cout << "ERROR: While adding NoPre-Prefix edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					} else {
						cout << "ERROR: Unknown return value from graph.addSingleEdge() while adding NoPre-Prefix edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					}
#ifdef GRAPHDEBUG
					if (addEdgeRetValue == 1)
						cout << "NoPreSuffix edge (" << alpha_i << "," << alpha_j << ")" << endl;
#endif
				}
			}
		}
	}

	if (flag)
		return 1;
	else
		return 0;
}

int UAFDetector::addTransSTOrMTEdges() {
	// Adding TRANS-ST/MT Edges

	bool flag = false;

	long long alpha_i, alpha_k, alpha_j;

	for (alpha_i = 1; alpha_i <= graph.totalNodes; alpha_i++) {
		long long threadID_i = opIDMap[alpha_i].threadID;

		for (HBGraph::adjListNode* currNode = graph.adjList[alpha_i].head; currNode != NULL; currNode = currNode->next) {
			alpha_k = currNode->destination;

			for (HBGraph::adjListNode* nextNode = graph.adjList[alpha_k].head; nextNode != NULL; nextNode = nextNode->next) {
				alpha_j = nextNode->destination;
				if ((opIDMap[alpha_j].threadID == threadID_i && opIDMap[alpha_k].threadID == threadID_i) || (opIDMap[alpha_j].threadID != threadID_i)) {
					int addEdgeRetValue = graph.addSingleEdge(alpha_i, alpha_j);
					if (addEdgeRetValue == 1) flag = true; // New edge added.
					else if (addEdgeRetValue == 0) flag = flag || false; // No new edge added.
					else if (addEdgeRetValue == -1) {
						cout << "ERROR: While adding Trans-ST/MT edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					} else {
						cout << "ERROR: Unknown return value from graph.addSingleEdge() while adding Trans-ST/MT edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					}
#ifdef GRAPHDEBUG
					if (addEdgeRetValue == 1)
						cout << "Trans-ST/MT edge (" << alpha_i << "," << alpha_j << ")" << endl;
#endif
				}
			}
		}
	}

	if (flag)
		return 1;
	else
		return 0;
}

void UAFDetector::findUAF(Logger &logger) {

	// Loop through free set, loop through
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
