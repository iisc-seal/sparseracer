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

UAFDetector::UAFDetector()
	:
	  opIDMap(),
	  taskIDMap(),
	  nestingLoopMap(),
	  threadIDMap(),
	  blockIDMap(),
	  enqToTaskEnqueued(),
	  pauseResumeResetOps(),
	  allocSet(),
	  freeSet(),
	  readSet(),
	  writeSet(),
	  allocIDMap(),
	  freeIDMap()
{
	graph = NULL;
}

UAFDetector::~UAFDetector() {
}

void UAFDetector::initGraph(IDType countOfOps, IDType countOfNodes) {
	graph = new HBGraph(countOfOps, countOfNodes, opIDMap, blockIDMap);
	assert(graph != NULL);
}

int UAFDetector::addEdges(Logger &logger) {
	assert (graph->totalOps != 0);
	assert (graph->totalBlocks != 0);

	// LOOP-PO/FORK/JOIN
	if (add_LoopPO_Fork_Join_Edges() < 0) {
		cout << "ERROR: While adding LOOP-PO/Fork/Join edges\n";
		return -1;
	}

	// TASK-PO/ENQUEUE-ST/MT
	if (add_TaskPO_EnqueueSTOrMT_Edges() < 0) {
		cout << "ERROR: While adding TASK-PO edges\n";
		return -1;
	}

	bool edgeAdded = false;
	while (true) {
		int retValue;

#ifdef GRAPHDEBUGFULL
		graph->printGraph();
#endif

#if 0
#ifdef GRAPHDEBUG
		cout << "Adding Fifo-Atomic edges\n";
#endif
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

#ifdef GRAPHDEBUGFULL
		if (retValue == 1)
			graph->printGraph(false);
#endif

#ifdef GRAPHDEBUG
		cout << "Adding No-Pre edges\n";
#endif
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

#ifdef GRAPHDEBUGFULL
		if (retValue == 1)
			graph->printGraph(false);
#endif

#ifdef GRAPHDEBUG
		cout << "Adding Fifo-Callback edges\n";
#endif
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

#ifdef GRAPHDEBUGFULL
		if (retValue == 1)
			graph->printGraph(false);
#endif

#ifdef FIFOCALLBACK2
#ifdef GRAPHDEBUG
		cout << "Adding FIFO-Callback 2 edges\n";
#endif

		// FIFO-CALLBACK-2
		retValue = addFifoCallback2Edges();
		if (retValue == 1) edgeAdded = true;
		else if (retValue == 0) edgeAdded = edgeAdded || false; // Didn't add any new edge
		else if (retValue == -1) {
			cout << "ERROR: While adding FIFO-CALLBACK-2 edges\n";
			return -1;
		} else {
			cout << "ERROR: Unknown return value from addFifoCallbackEdges()\n";
			return -1;
		}
#endif

#ifdef GRAPHDEBUG
		cout << "Adding Fifo-Nested edges\n";
#endif
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

#ifdef GRAPHDEBUGFULL
		if (retValue == 1)
			graph->printGraph(false);
#endif

#ifdef GRAPHDEBUG
		cout << "Adding NoPre-Prefix edges\n";
#endif
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

#ifdef GRAPHDEBUGFULL
		if (retValue == 1)
			graph->printGraph(false);
#endif

#ifdef GRAPHDEBUG
		cout << "Adding NoPre-Suffix edges\n";
#endif
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

#ifdef GRAPHDEBUGFULL
		if (retValue == 1)
			graph->printGraph(false);
#endif
#endif

#ifdef GRAPHDEBUG
		cout << "Adding Trans-ST/MT edges\n";
#endif
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

		if (!edgeAdded) // If no edges were added in this iteration, stop.
			break;
		else
			edgeAdded = false;
	}

#ifdef GRAPHDEBUGFULL
	graph->printGraph();
#endif

	cout << "Total op edges = " << graph->numOfOpEdges << endl;
	cout << "Total block edges = " << graph->numOfBlockEdges << endl;
	return 0;
}

int UAFDetector::add_LoopPO_Fork_Join_Edges() {
	bool flag = false; // To keep track of whether edges were added.

	for (map<IDType, UAFDetector::threadDetails>::iterator it = threadIDMap.begin(); it != threadIDMap.end(); it++) {

		// Adding FORK edges

		IDType opI, opJ;
		opI = it->second.forkOpID;
		opJ = it->second.threadinitOpID;

		if (opI > 0 && opJ > 0) {
			int retOpValue = graph->opEdgeExists(opI, opJ);
			if (retOpValue == 0) {
				int addEdgeRetValue = graph->addOpEdge(opI, opJ);
				if (addEdgeRetValue == 1) {
					flag = true;
#ifdef GRAPHDEBUG
					cout << "FORK edge (" << opI << ", " << opJ << ") -- #op-edges "   << graph->numOfOpEdges
																<< " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
				} else if (addEdgeRetValue == -1) {
					cout << "ERROR: While adding FORK Op edge from " << opI << " to " << opJ << endl;
					return -1;
				}
			} else if (retOpValue == -1) {
				cout << "ERROR: While checking FORK Op edge from " << opI << " to " << opJ << endl;
				return -1;
			}
		} else {
			if (opI <= 0) {
				cout << "ERROR: Cannot find fork op of thread " << it->first << endl;
			}
			if (opJ <= 0) {
				cout << "ERROR: Cannot find threadinit of thread " << it->first << endl;
			}
		}

		// Adding JOIN edges

		opI = it->second.threadexitOpID;
		opJ = it->second.joinOpID;

		if (opI > 0 && opJ > 0) {
			int retOpValue = graph->opEdgeExists(opI, opJ);
			if (retOpValue == 0) {
				int addEdgeRetValue = graph->addOpEdge(opI, opJ);
				if (addEdgeRetValue == 1) {
					flag = true;
#ifdef GRAPHDEBUG
					cout << "JOIN edge (" << opI << ", " << opJ << ") -- #op-edges "   << graph->numOfOpEdges
																<< " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
				} else if (addEdgeRetValue == -1) {
					cout << "ERROR: While adding JOIN Op edge from " << opI << " to " << opJ << endl;
					return -1;
				}
			} else if (retOpValue == -1) {
				cout << "ERROR: While checking JOIN Op edge from " << opI << " to " << opJ << endl;
				return -1;
			}
		} else {
			if (opI <= 0) {
				cout << "ERROR: Cannot find threadexit op of thread " << it->first << endl;
			}
			if (opJ <= 0) {
				cout << "ERROR: Cannot find join of thread " << it->first << endl;
			}
		}


		// Adding LOOP-PO edges

		IDType blockI = it->second.firstBlockID;
		IDType enterloopBlock = it->second.enterloopBlockID;
		IDType lastBlockInThread = it->second.lastBlockID;

#ifdef SANITYCHECK
		if (blockI <= 0) {
			cout << "ERROR: Cannot find first block of thread " << it->first << endl;
			continue;
		}
		if (enterloopBlock <= 0) {
			cout << "ERROR: Cannot find enterloop block of thread " << it->first << endl;
			continue;
		}
		if (lastBlockInThread <= 0) {
			cout << "ERROR: Cannot find last block of thread " << it->first << endl;
			continue;
		}
#endif

		// Add edge from ops/blocks before enterloop to all ops/blocks subsequent to them

		for (IDType b1 = blockI; (b1 > 0 && b1 <= enterloopBlock); b1 = blockIDMap[b1].nextBlockInThread) {

#ifdef SANITYCHECK
			assert(b1 > 0);
#endif

			IDType blockJ = blockIDMap[b1].nextBlockInThread;

#ifdef SANITYCHECK
			if (blockJ <= 0) {
				cout << "ERROR: Cannot find next block of block " <<  b1 << " in thread " << it->first << endl;
				continue;
			}
#endif

			for (IDType b2 = blockJ; (b2 > 0 && b2 <= lastBlockInThread); b2 = blockIDMap[b2].nextBlockInThread) {

#ifdef SANITYCHECK
			assert(b2 > 0);
#endif
				opI = blockIDMap[b1].lastOpInBlock;
				opJ = blockIDMap[b2].firstOpInBlock;

#ifdef SANITYCHECK
				assert(opI > 0);
				assert(opJ > 0);
#endif
				int retOpValue = graph->opEdgeExists(opI, opJ);
				if (retOpValue == 0) {
					int addEdgeRetValue = graph->addOpEdge(opI, opJ);
					if (addEdgeRetValue == 1) {
						flag = true;
						cout << "LOOP-PO edge (" << opI << ", " << opJ << ") -- #op-edges "   << graph->numOfOpEdges
																	   << " -- #block-edges " << graph->numOfBlockEdges << endl;
					} else if (addEdgeRetValue == -1) {
						cout << "ERROR: While adding LOOP-PO Op edge from " << opI << " to " << opJ << endl;
						return -1;
					}
				} else if (retOpValue == -1) {
					cout << "ERROR: While checking LOOP-PO Op edge from " << opI << " to " << opJ << endl;
					return -1;
				}

			}
		}

		blockI = blockIDMap[enterloopBlock].nextBlockInThread;
		IDType exitloopBlock = it->second.exitloopBlockID;

#ifdef SANITYCHECK
		if (blockI <= 0) {
			cout << "ERROR: Cannot find next block of block " << enterloopBlock << " in thread " << it->first << endl;
			continue;
		}
		if (exitloopBlock <= 0) {
			cout << "ERROR: Cannot find exitloop block of thread " << it->first << endl;
			continue;
		}
#endif

		// Add edge from ops/blocks after enterloop to all ops/blocks after exitloop

		for (IDType b1 = exitloopBlock; (b1 > 0 && b1 <= lastBlockInThread); b1 = blockIDMap[b1].nextBlockInThread) {

#ifdef SANITYCHECK
			assert(b1 > 0);
#endif

			IDType blockJ = blockIDMap[b1].prevBlockInThread;

#ifdef SANITYCHECK
			if (blockJ <= 0) {
				cout << "ERROR: Cannot find previous block of block " << blockJ << " in thread " << it->first << endl;
				return -1;
			}
#endif

			for (IDType b2 = blockI; (b2 > 0 && b2 <= blockJ); b2 = blockIDMap[b2].nextBlockInThread) {

#ifdef SANITYCHECK
				assert(b2 > 0);
#endif

				opI = blockIDMap[b1].lastOpInBlock;
				opJ = blockIDMap[b2].firstOpInBlock;

#ifdef SANITYCHECK
				assert(opI > 0);
				assert(opJ > 0);
#endif
				int retOpValue = graph->opEdgeExists(opJ, opI);
				if (retOpValue == 0) {
					int addEdgeRetValue = graph->addOpEdge(opJ, opI);
					if (addEdgeRetValue == 1) {
						flag = true;
#ifdef GRAPHDEBUG
						cout << "LOOP-PO edge (" << opJ << ", " << opI << ") -- #op-edges "   << graph->numOfOpEdges
																	   << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
					} else if (addEdgeRetValue == -1) {
						cout << "ERROR: While adding LOOP-PO Op edge from " << opJ << " to " << opI << endl;
						return -1;
					}
				} else if (retOpValue == -1) {
					cout << "ERROR: While checking LOOP-PO Op edge from " << opJ << " to " << opI << endl;
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

int UAFDetector::add_TaskPO_EnqueueSTOrMT_Edges() {
	bool flag = false; // To keep track of whether edges were added.

	for (map<string, UAFDetector::taskDetails>::iterator it = taskIDMap.begin(); it != taskIDMap.end(); it++) {

		// ENQUEUE-ST/MT

		IDType enqOp = it->second.enqOpID;
		IDType deqOp = it->second.deqOpID;

		if (enqOp > 0 && deqOp > 0) {
			int retOpValue = graph->opEdgeExists(enqOp, deqOp);
			if (retOpValue == 0) {
				int addEdgeRetValue = graph->addOpEdge(enqOp, deqOp);
				if (addEdgeRetValue == 1) {
					flag = true;
#ifdef GRAPHDEBUG
					cout << "ENQUEUE-ST/MT edge (" << enqOp << ", " << deqOp << ") -- #op-edges "   << graph->numOfOpEdges
																<< " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
				} else if (addEdgeRetValue == -1) {
					cout << "ERROR: While adding ENQUEUE-ST/MT Op edge from " << enqOp << " to " << deqOp << endl;
					return -1;
				}
			} else if (retOpValue == -1) {
				cout << "ERROR: While checking ENQUEUE-ST/MT Op edge from " << enqOp << " to " << deqOp << endl;
				return -1;
			}
		} else {
			if (enqOp <= 0) {
				cout << "ERROR: Cannot find enq op of task " << it->first << endl;
			}
			if (deqOp <= 0) {
				cout << "ERROR: Cannot find deq op of task " << it->first << endl;
			}
		}

		// TASK-PO

		IDType firstBlockInTask = it->second.firstBlockID;
		IDType lastBlockInTask = it->second.lastBlockID;

#ifdef SANITYCHECK
		if (firstBlockInTask <= 0) {
			cout << "ERROR: Cannot find first block of task " << it->first << endl;
			return -1;
		}
		if (lastBlockInTask <= 0) {
			cout << "ERROR: Cannot find last block of task " << it->first << endl;
			return -1;
		}
#endif

		for (IDType b1 = firstBlockInTask; (b1 > 0 && b1 <= lastBlockInTask); b1 = blockIDMap[b1].nextBlockInTask) {
#ifdef SANITYCHECK
			assert(b1 > 0);
#endif
			IDType nextBlock = blockIDMap[b1].nextBlockInTask;
			for (IDType b2 = nextBlock; (b2 > 0 && b2 <= lastBlockInTask); b2 = blockIDMap[b2].nextBlockInTask) {
#ifdef SANITYCHECK
				assert(b2 > 0);
#endif

				IDType opI = blockIDMap[b1].lastOpInBlock;
				IDType opJ = blockIDMap[b2].firstOpInBlock;

#ifdef SANITYCHECK
				assert(opI > 0);
				assert(opJ > 0);
#endif
				int retOpValue = graph->opEdgeExists(opI, opJ);
				if (retOpValue == 0) {
					int addEdgeRetValue = graph->addOpEdge(opI, opJ);
					if (addEdgeRetValue == 1) {
						flag = true;
#ifdef GRAPHDEBUG
						cout << "TASK-PO edge (" << opI << ", " << opJ << ") -- #op-edges "   << graph->numOfOpEdges
																	   << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
					} else if (addEdgeRetValue == -1) {
						cout << "ERROR: While adding TASK-PO Op edge from " << opI << " to " << opJ << endl;
						return -1;
					}
				} else if (retOpValue == -1) {
					cout << "ERROR: While checking TASK-PO Op edge from " << opI << " to " << opJ << endl;
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

#if 0
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

		for (HBGraph::adjListNode* currNode = graph->adjList[enq_i].head; currNode != NULL; currNode = currNode->next) {
			if (opIDMap[currNode->destination].opType.compare("enq") == 0) {
				long long enq_2 = currNode->destination;
				string task2 = enqSet[enq_2].taskEnqueued;

				if (task1.compare(task2) != 0 && enqSet[enq_i].targetThreadID == enqSet[enq_2].targetThreadID) {
					long long alpha_j = taskIDMap[task2].deqOpID;

					if (alpha_j == -1) {
						cout << "ERROR: Cannot find deq of task " << task2	<< endl;
						continue;
					}

					int addEdgeRetValue = graph->addSingleEdge(alpha_i, alpha_j);
					if (addEdgeRetValue == 1) {
						flag = true; // New edge added.
						numOfEdges++;
					}
					else if (addEdgeRetValue == 0) flag = flag || false; // No new edge added.
					else if (addEdgeRetValue == -1) {
						cout << "ERROR: While adding Fifo-Atomic edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					} else {
						cout << "ERROR: Unknown return value from graph->addSingleEdge() while adding Fifo-Atomic edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					}
#ifdef GRAPHDEBUG
					if (addEdgeRetValue == 1)
						cout << "Fifo-Atomic edge (" << alpha_i << "," << alpha_j << ") -- " << numOfEdges << endl;
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

		long long deqOpIDOfp1 = taskIDMap[*atomicIt].deqOpID;
		if (deqOpIDOfp1 == -1) {
			cout << "ERROR: Cannot find deq of task " << p1 << endl;
			continue;
		}

		long long curr = deqOpIDOfp1;
		// Till we see end of p1 (alpha_i contains end of p1).
		while (curr > 0 && curr != alpha_i) {
			for (HBGraph::adjListNode* currNode = graph->adjList[curr].head; currNode != NULL; currNode = currNode->next) {
				if (currNode->destination <= 0) {
					cout << "ERROR: Found invalid edge from " << curr << endl;
					continue;
				}
				if (opIDMap[currNode->destination].opType.compare("enq") != 0) continue;

				string p2 = enqSet[currNode->destination].taskEnqueued;
				if (p2.compare("") == 0) {
					cout << "ERROR: Found enq of empty task at " << currNode->destination << endl;
					continue;
				}

				alpha_j = taskIDMap[p2].deqOpID;
				long long threadIDOfp2 = taskIDMap[p2].threadID;
				if (threadIDOfAtomicTask != threadIDOfp2) continue;
				if (alpha_j == -1) {
					cout << "ERROR: Cannot find deq of task " << p2 << endl;
					continue;
				}

				// If no edge between alpha_i and alpha_j.
				if (graph->edgeExists(alpha_i, alpha_j) == 0) {
					int addEdgeRetValue = graph->addSingleEdge(alpha_i, alpha_j);
					if (addEdgeRetValue == 1) {
						flag = true; // New edge added.
						numOfEdges++;
					}
					else if (addEdgeRetValue == 0) flag = flag || false; // No new edge added.
					else if (addEdgeRetValue == -1) {
						cout << "ERROR: While adding Nopre edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					} else {
						cout << "ERROR: Unknown return value from graph->addSingleEdge() while adding Nopre edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					}
#ifdef GRAPHDEBUG
					if (addEdgeRetValue == 1)
						cout << "Nopre edge (" << alpha_i << "," << alpha_j << ") -- " << numOfEdges << endl;
#endif
				}
			}

			curr = opToNextOpInTask[curr];
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

		for (HBGraph::adjListNode* currNode = graph->adjList[enq_1].head; currNode != NULL; currNode = currNode->next) {
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

				if (graph->edgeExists(enq_parent, enq_1) != 1) continue;

				int addEdgeRetValue = graph->addSingleEdge(alpha_i, alpha_j);
				if (addEdgeRetValue == 1) {
					flag = true; // New edge added.
					numOfEdges++;
				}
				else if (addEdgeRetValue == 0) flag = flag || false; // No new edge added.
				else if (addEdgeRetValue == -1) {
					cout << "ERROR: While adding Fifo-Callback edge from " << alpha_i << " to " << alpha_j << endl;
					return -1;
				} else {
					cout << "ERROR: Unknown return value from graph->addSingleEdge() while adding Fifo-Callback edge from " << alpha_i << " to " << alpha_j << endl;
					return -1;
				}
#ifdef GRAPHDEBUG
				if (addEdgeRetValue == 1)
					cout << "Fifo-Callback edge (" << alpha_i << "," << alpha_j << ") -- " << numOfEdges << endl;
#endif
			}
		}
	}

	if (flag)
		return 1;
	else
		return 0;
}

#ifdef FIFOCallback2
int UAFDetector::addFifoCallback2Edges() {
	bool flag = false;

	for(set<string>::iterator atomicTaskIt = atomicTasks.begin(); atomicTaskIt != atomicTasks.end(); atomicTaskIt++) {
		long long enqOpofTask = taskIDMap[*atomicTaskIt].enqOpID;
		string parentTask = taskIDMap[*atomicTaskIt].parentTask;

		if (parentTask.compare("") == 0) continue;
		long long parentTaskThreadID = taskIDMap[parentTask].threadID;
		long long lastResumeOfParent = taskIDMap[parentTask].lastResumeOpID;

		if (lastResumeOfParent == -1) continue;

		if (opToNextOpInThread[taskIDMap[*atomicTaskIt].endOpID] != lastResumeOfParent) continue;

		if (enqOpofTask == -1) {
			cout << "ERROR: Cannot find enq of task " << *atomicTaskIt << endl;
			continue;
		}

		for (HBGraph::adjListNode* currNode = graph->adjList[enqOpofTask].head; currNode != NULL; currNode = currNode->next) {
			if (opIDMap[currNode->destination].opType.compare("enq") != 0) continue;

			long long enqOpOfTask2 = currNode->destination;
			string task2 = enqSet[enqOpOfTask2].taskEnqueued;

			if (enqOpOfTask2 == enqOpofTask) {
				cout << "Something wrong " << enqOpOfTask2 << " " << enqOpofTask << endl;
			}
			if (task2.compare(*atomicTaskIt) == 0) continue;

			long long task2ThreadID = taskIDMap[task2].threadID;

			if (graph->edgeExists(currNode->destination, lastResumeOfParent) != 1) continue;

			if (parentTaskThreadID != task2ThreadID) continue;

			long long alpha_i = taskIDMap[parentTask].endOpID;
			long long alpha_j = taskIDMap[task2].deqOpID;
			int addEdgeRetValue = graph->addSingleEdge(alpha_i, alpha_j);
			if (addEdgeRetValue == 1) {
				flag = true; // New edge added.
				numOfEdges++;
			}
			else if (addEdgeRetValue == 0) flag = flag || false; // No new edge added.
			else if (addEdgeRetValue == -1) {
				cout << "ERROR: While adding Fifo-Callback-2 edge from " << alpha_i << " to " << alpha_j << endl;
				return -1;
			} else {
				cout << "ERROR: Unknown return value from graph->addSingleEdge() while adding Fifo-Callback-2 edge from " << alpha_i << " to " << alpha_j << endl;
				return -1;
			}
#ifdef GRAPHDEBUG
			if (addEdgeRetValue == 1)
				cout << "Fifo-Callback-2 edge (" << alpha_i << "," << alpha_j << ") -- " << numOfEdges << endl;
#endif
		}
	}

	if (flag)
		return 1;
	else
		return 0;
}
#endif

int UAFDetector::addFifoNestedEdges() {
	// Adding FIFO-NESTED
	// Find two enqs such that enq(-,p1,t,c) < enq(-,p2,t,c'). Then add edge between first-pause(p1) and deq(p2)

	bool flag = false; // To keep track of whether edges were added.

	for (map<long long, enqOpDetails>::iterator enq1It = enqSet.begin(); enq1It != enqSet.end(); enq1It++) {
		long long enq_1 = enq1It->first;
		string task1 = enq1It->second.taskEnqueued;
		if (atomicTasks.find(task1) != atomicTasks.end()) continue;

		for (HBGraph::adjListNode* currNode = graph->adjList[enq_1].head; currNode != NULL; currNode = currNode->next) {
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

					int addEdgeRetValue = graph->addSingleEdge(alpha_i, alpha_j);
					if (addEdgeRetValue == 1) {
						flag = true; // New edge added.
						numOfEdges++;
					}
					else if (addEdgeRetValue == 0) flag = flag || false; // No new edge added.
					else if (addEdgeRetValue == -1) {
						cout << "ERROR: While adding Fifo-Nested edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					} else {
						cout << "ERROR: Unknown return value from graph->addSingleEdge() while adding Fifo-Nested edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					}
#ifdef GRAPHDEBUG
					if (addEdgeRetValue == 1)
						cout << "Fifo-Nested edge (" << alpha_i << "," << alpha_j << ") -- " << numOfEdges << endl;
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
		//while (currOp != 0) {
		while (currOp > 0) {
			for (HBGraph::adjListNode* currNode = graph->adjList[currOp].head; currNode != NULL; currNode = currNode->next) {
				// See if there is an edge to enq from currOp
				long long dest = currNode->destination;
				if (opIDMap[dest].opType.compare("enq") != 0) continue;

				string enqTask = enqSet[dest].taskEnqueued;
				// Check if the enq's targetThread is same as that of the currOp
				if (enqSet[dest].targetThreadID != opIDMap[currOp].threadID) continue;

				if (graph->edgeExists(currOp, alpha_i) == 1) {
					// currOp < enq and currOp < first-pause
					long long alpha_j = taskIDMap[enqTask].deqOpID;
					if (alpha_j == -1) {
						cout << "ERROR: Cannot find deq of task " << enqTask << " enqueued at " << dest << endl;
						continue;
					}

					int addEdgeRetValue = graph->addSingleEdge(alpha_i, alpha_j);
					if (addEdgeRetValue == 1) {
						flag = true; // New edge added.
						numOfEdges++;
					}
					else if (addEdgeRetValue == 0) flag = flag || false; // No new edge added.
					else if (addEdgeRetValue == -1) {
						cout << "ERROR: While adding NoPre-Prefix edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					} else {
						cout << "ERROR: Unknown return value from graph->addSingleEdge() while adding NoPre-Prefix edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					}
#ifdef GRAPHDEBUG
					if (addEdgeRetValue == 1)
						cout << "NoPrePrefix edge (" << alpha_i << "," << alpha_j << ") -- " << numOfEdges << endl;
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

		for (HBGraph::adjListNode* currNode = graph->adjList[lastresume].head; currNode != NULL; currNode = currNode->next) {
			long long currOp = currNode->destination;
			for (HBGraph::adjListNode* enqNode = graph->adjList[currOp].head; enqNode != NULL; enqNode = enqNode->next) {
				if (opIDMap[enqNode->destination].opType.compare("enq") != 0) continue;
				long long enq = enqNode->destination;
				if (enqSet[enq].taskEnqueued.compare(task1) != 0 && enqSet[enq].targetThreadID == threadTask1) {
					string task2 = enqSet[enq].taskEnqueued;
					long long alpha_j = taskIDMap[task2].deqOpID;
					if (alpha_j == -1) {
						cout << "ERROR: Cannot find deq of task " << task2 << endl;
						continue;
					}

					int addEdgeRetValue = graph->addSingleEdge(alpha_i, alpha_j);
					if (addEdgeRetValue == 1) {
						flag = true; // New edge added.
						numOfEdges++;
					}
					else if (addEdgeRetValue == 0) flag = flag || false; // No new edge added.
					else if (addEdgeRetValue == -1) {
						cout << "ERROR: While adding NoPre-Prefix edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					} else {
						cout << "ERROR: Unknown return value from graph->addSingleEdge() while adding NoPre-Prefix edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					}
#ifdef GRAPHDEBUG
					if (addEdgeRetValue == 1)
						cout << "NoPreSuffix edge (" << alpha_i << "," << alpha_j << ") -- " << numOfEdges << endl;
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
#endif

int UAFDetector::addTransSTOrMTEdges() {
	bool flag = false;

	// Adding TRANS-ST/MT Edges

	for (map<IDType, UAFDetector::blockDetails>::iterator it = blockIDMap.begin(); it != blockIDMap.end(); it++) {
		IDType blockI = it->first;

#ifdef SANITYCHECK
		assert(blockI > 0);
#endif

		for (HBGraph::adjListNode* tempNode1 = graph->blockAdjList[blockI].head; tempNode1 != NULL; tempNode1 = tempNode1->next) {
			IDType blockK = tempNode1->destination;

#ifdef SANITYCHECK
			if (blockK <= 0) {
				cout << "ERROR: Invalid block edge from " << blockI << endl;
				return -1;
			}
#endif

			for (HBGraph::adjListNode* tempNode2 = graph->blockAdjList[blockK].head; tempNode2 != NULL; tempNode2 = tempNode2->next) {
				IDType blockJ = tempNode2->destination;

#ifdef SANITYCHECK
				if (blockJ <= 0) {
					cout << "ERROR: Invalid block edge from " << blockI << endl;
					return -1;
				}
#endif

				IDType threadI = it->second.threadID;
				IDType threadK = blockIDMap[blockK].threadID;
				IDType threadJ = blockIDMap[blockJ].threadID;

#ifdef SANITYCHECK
				if (threadI < 0) {
					cout << "ERROR: Cannot find thread ID of block " << blockI << endl;
					return -1;
				}
				if (threadJ < 0) {
					cout << "ERROR: Cannot find thread JD of block " << blockJ << endl;
					return -1;
				}
				if (threadK < 0) {
					cout << "ERROR: Cannot find thread KD of block " << blockK << endl;
					return -1;
				}
#endif
				if (!(((threadI == threadK) && (threadK == threadJ)) || (threadI != threadJ)))
					continue;

				IDType tempOp1, tempOp2;
				tempOp1 = blockIDMap[blockI].lastOpInBlock;
				tempOp2 = blockIDMap[blockJ].firstOpInBlock;

#ifdef SANITYCHECK
				if (tempOp1 <= 0) {
					cout << "ERROR: Cannot find last op of block " << blockI << endl;
					return -1;
				}
				if (tempOp2 <= 0) {
					cout << "ERROR: Cannot find first op of block " << blockJ << endl;
					return -1;
				}
#endif

				int retValue = graph->opEdgeExists(tempOp1, tempOp2);
				if (retValue == 1)
					continue;
				else if (retValue == -1) {
					cout << "ERROR: While checking op edge from " << tempOp1 << " to " << tempOp2 << endl;
					return -1;
				}

				IDType opI, opJ, firstOp;
				opI = blockIDMap[blockI].lastOpInBlock;
				firstOp = blockIDMap[blockI].firstOpInBlock;
#ifdef SANITYCHECK
				if (opI <= 0) {
					cout << "ERROR: Cannot find last op of block " << blockI << endl;
					continue;
				}
				if (firstOp <= 0) {
					cout << "ERROR: Cannot find first op of block " << blockI << endl;
					continue;
				}
#endif
				while (opI > 0 && opI <= firstOp) {
					opJ = -1;
					for (HBGraph::adjListNode* currNode1 = graph->opAdjList[opI].head; currNode1 != NULL; currNode1 = currNode1->next) {
						tempOp1 = currNode1->destination;
						if (tempOp1 <= 0) continue;

						for (HBGraph::adjListNode* currNode2 = graph->opAdjList[tempOp1].head; currNode2 != NULL; currNode2 = currNode2->next) {
							tempOp2 = currNode2->destination;
							if (tempOp2 <= 0) continue;

							if (opJ == -1 || tempOp2 < opJ)
								opJ = tempOp2;
						}
					}

					int retValue = graph->opEdgeExists(opI, opJ);
					if (retValue == 0) {
						int addEdgeRetValue = graph->addOpEdge(opI, opJ);
						if (addEdgeRetValue == 1) {
							flag = true;
#ifdef SANITYCHECK
							cout << "TRANS-ST/MT Edge (" << opI << ", " << opJ << ") -- #opEdges " << graph->numOfOpEdges
																			   << " -- #blockEdges " << graph->numOfBlockEdges << endl;
#endif
							if (opI == blockIDMap[blockI].lastOpInBlock && opJ == blockIDMap[blockJ].firstOpInBlock)
								break;
						}
					}

					opI = opIDMap[opI].prevOpInBlock;
				}
			}
		}
	}

#if 0
	long long alpha_i, alpha_k, alpha_j;

	for (alpha_i = 1; alpha_i <= graph->totalNodes; alpha_i++) {
		long long threadID_i = opIDMap[alpha_i].threadID;

		// adjList[i-1] is the node for op i.
		for (HBGraph::adjListNode* currNode = graph->adjList[alpha_i].head; currNode != NULL; currNode = currNode->next) {
			alpha_k = currNode->destination;

			for (HBGraph::adjListNode* nextNode = graph->adjList[alpha_k].head; nextNode != NULL; nextNode = nextNode->next) {
				alpha_j = nextNode->destination;
				if ((opIDMap[alpha_j].threadID == threadID_i && opIDMap[alpha_k].threadID == threadID_i) || (opIDMap[alpha_j].threadID != threadID_i)) {
					int addEdgeRetValue = graph->addSingleEdge(alpha_i, alpha_j);
					if (addEdgeRetValue == 1) {
						flag = true; // New edge added.
						numOfEdges++;
					}
					else if (addEdgeRetValue == 0) flag = flag || false; // No new edge added.
					else if (addEdgeRetValue == -1) {
						cout << "ERROR: While adding Trans-ST/MT edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					} else {
						cout << "ERROR: Unknown return value from graph->addSingleEdge() while adding Trans-ST/MT edge from " << alpha_i << " to " << alpha_j << endl;
						return -1;
					}
#ifdef GRAPHDEBUG
					if (addEdgeRetValue == 1)
						cout << "Trans-ST/MT edge (" << alpha_i << "," << alpha_j << ") -- " << numOfEdges << endl;
#endif
				}
			}
		}
	}
#endif

	if (flag)
		return 1;
	else
		return 0;
}

#if 0
int  UAFDetector::findUAFwithoutAlloc(Logger &logger){

	bool flag = false;

	// Loop through freeIDMap, for each free, find use that no HB edge

	for (map<long long, freeOpDetails>::iterator freeIt = freeIDMap.begin(); freeIt != freeIDMap.end(); freeIt++) {
		long long freeID = freeIt->first;
		long long allocID = freeIt->second.allocOpID;

		if (allocID == -1) {
			cout << "ERROR: Cannot find alloc for free op " << freeID << endl;
//			return -1;
			continue;
		}
#ifdef ACCESS
		for (set<long long>::iterator accessIt = freeIt->second.accessOps.begin(); accessIt != freeIt->second.accessOps.end(); accessIt++) {
			long long accessID = *accessIt;

			if (graph->edgeExists(freeID, accessID) == 1) {
				cout << "Definite UAF between access op " << accessID << " (access at address " << accessSet[accessID].startingAddress << ") "
					 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
					 << ")\n";
				cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
					 << allocSet[allocID].startingAddress << ")\n";
				flag = true;
				continue;
			}

			if (graph->edgeExists(accessID, freeID) == 0) {

#ifdef ADDITIONS
				// Even if there is no edge between access and free,
				// if there is an edge from alloc to access (alloc happens before access), and the alloc is in the same task as the access and the task is atomic, this is a false positive.
				// This is true only if free is in the same thread as alloc and access.
				if (graph->edgeExists(allocID, accessID) && opIDMap[allocID].taskID.compare(opIDMap[accessID].taskID) == 0
					&& atomicTasks.find(opIDMap[allocID].taskID) != atomicTasks.end() && opIDMap[freeID].threadID == opIDMap[accessID].threadID)
					continue;
#endif

				cout << "Potential UAF between read op " << accessID << " (access at address " << accessSet[accessID].startingAddress << ") "
					 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
					 << ")\n";
				cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
					 << allocSet[allocID].startingAddress << ")\n";
				flag = true;
				continue;
			}
		}

#else
		for (set<long long>::iterator readIt = freeIt->second.readOps.begin(); readIt != freeIt->second.readOps.end(); readIt++) {
			long long readID = *readIt;

			if (graph->edgeExists(freeID, readID) == 1) {
				cout << "Definite UAF between read op " << readID << " (read at address " << readSet[readID].startingAddress << ") "
					 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
					 << ")\n";
				cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
					 << allocSet[allocID].startingAddress << ")\n";
				flag = true;
				continue;
			}

			if (graph->edgeExists(readID, freeID) == 0) {

#ifdef ADDITIONS
				// Even if there is no edge between read and free,
				// if there is an edge from alloc to read (alloc happens before read), and the alloc is in the same task as the read and the task is atomic, this is a false positive.
				// This is true only if free is in the same thread as alloc and read.
				if (graph->edgeExists(allocID, readID) && opIDMap[allocID].taskID.compare(opIDMap[readID].taskID) == 0
					&& atomicTasks.find(opIDMap[allocID].taskID) != atomicTasks.end() && opIDMap[freeID].threadID == opIDMap[readID].threadID)
					continue;
#endif
				cout << "Potential UAF between read op " << readID << " (read at address " << readSet[readID].startingAddress << ") "
					 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
					 << ")\n";
				cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
					 << allocSet[allocID].startingAddress << ")\n";
				flag = true;
				continue;
			}
		}

		for (set<long long>::iterator writeIt = freeIt->second.writeOps.begin(); writeIt != freeIt->second.writeOps.end(); writeIt++) {
			long long writeID = *writeIt;

			if (graph->edgeExists(freeID, writeID) == 1) {
				cout << "Definite UAF between write op " << writeID << " (write at address " << writeSet[writeID].startingAddress << ") "
					 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
					 << ")\n";
				cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
					 << allocSet[allocID].startingAddress << ")\n";
				flag = true;
				continue;
			}

			if (graph->edgeExists(writeID, freeID) == 0) {

#ifdef ADDITIONS
				// Even if there is no edge between write and free,
				// if there is an edge from alloc to write (alloc happens before write), and the alloc is in the same task as the write and the task is atomic, this is a false positive.
				// This is true only if free is in the same thwrite as alloc and write.
				if (graph->edgeExists(allocID, writeID) && opIDMap[allocID].taskID.compare(opIDMap[writeID].taskID) == 0
					&& atomicTasks.find(opIDMap[allocID].taskID) != atomicTasks.end() && opIDMap[freeID].threadID == opIDMap[writeID].threadID)
					continue;
#endif

				cout << "Potential UAF between write op " << writeID << " (write at address " << writeSet[writeID].startingAddress << ") "
					 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
					 << ")\n";
				cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
					 << allocSet[allocID].startingAddress << ")\n";
				flag = true;
				continue;
			}
		}
#endif
	}

	if (flag)
		return 1;
	else
		return 0;
}

#ifndef ACCESS
int UAFDetector::findDataRaces(Logger &logger){

	bool flag = false;

	for (map<long long, allocOpDetails>::iterator allocIt = allocIDMap.begin(); allocIt != allocIDMap.end(); allocIt++) {
		for (set<long long>::iterator writeIt = allocIt->second.writeOps.begin(); writeIt != allocIt->second.writeOps.end(); writeIt++) {

			string writeAddress1 = writeSet[*writeIt].startingAddress;

			// write-write races
			for (set<long long>::iterator write2It = allocIt->second.writeOps.begin(); write2It != allocIt->second.writeOps.end(); write2It++) {
				if (*write2It == *writeIt) continue;

				string writeAddress2 = writeSet[*write2It].startingAddress;
				if (writeAddress1.compare(writeAddress2) != 0) continue;

				if (graph->edgeExists(*writeIt, *write2It) == 0 && graph->edgeExists(*write2It, *writeIt) == 0) {
					cout << "Potential data race between write ops " << *writeIt << " and " << *write2It << " on address "
						 << writeAddress1 << endl;
					flag = true;
					continue;
				}
			}

			// write-read / read-write races
			for (set<long long>::iterator readIt = allocIt->second.readOps.begin(); readIt != allocIt->second.readOps.end(); readIt++) {
				if (*readIt == *writeIt) continue;

				string readAddress = readSet[*readIt].startingAddress;
				if (writeAddress1.compare(readAddress) != 0) continue;

				if (graph->edgeExists(*writeIt, *readIt) == 0 && graph->edgeExists(*readIt, *writeIt) == 0) {
					cout << "Potential data race between read op " << *readIt << " and write op " << *writeIt << " on address "
						 << readAddress << endl;
					flag = true;
					continue;
				}
			}
		}
	}

	if (flag)
		return 1;
	else
		return 0;
}
#endif

#ifdef GRAPHDEBUG
void UAFDetector::printEdges() {

	for (long long i=1; i <= graph->totalNodes; i++) {
		for (long long j=1; j <= graph->totalNodes; j++) {
			int retValue = graph->edgeExists(i, j);
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
#endif
