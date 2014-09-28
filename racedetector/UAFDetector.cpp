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
typedef std::multiset<HBGraph::adjListNode>::iterator nodeIterator;

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
	raceCount = 0;
	uafCount = 0;
	raceType = UNKNOWN;
}

UAFDetector::~UAFDetector() {
}

void UAFDetector::initGraph(IDType countOfNodes, IDType countOfBlocks) {
	graph = new HBGraph(countOfNodes, countOfBlocks, opIDMap, blockIDMap, nodeIDMap);
	assert(graph != NULL);
}

//int UAFDetector::addEdges(Logger &logger) {
int UAFDetector::addEdges() {
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

	// PAUSE-ST/MT OR RESUME-ST/MT
	if (add_PauseSTMT_ResumeSTMT_Edges() < 0) {
		cout << "ERROR: While adding PAUSE-ST/MT or RESUME-ST/MT edges\n";
		return -1;
	}

	// WAIT-NOTIFY
	if (add_WaitNotify_Edges() < 0) {
		cout << "ERROR: While adding WAIT-NOTIFY edges\n";
		return -1;
	}

	bool edgeAdded = false;
	while (true) {
		int retValue;

#ifdef PRINTGRAPH
		graph->printGraph();
#endif

		// FIFO-ATOMIC/NO-PRE
#ifdef GRAPHDEBUG
		cout << "Adding Fifo-Atomic/No-Pre edges\n";
#endif
		retValue = add_FifoAtomic_NoPre_Edges();
		if (retValue == 1) edgeAdded = true;
		else if (retValue == -1) {
			cout << "ERROR: While adding FIFO-ATOMIC/NOPRE edges\n";
			return -1;
		} else if (retValue != 0) {
			cout << "ERROR: Unknown return value from add_FifoAtomic_NoPre_Edges(): "
				 << retValue << endl;
			return -1;
		}

#ifdef PRINTGRAPH
		if (retValue == 1)
			graph->printGraph();
#endif

		// FIFO-NESTED-1/2/GEN Or ENQRESET-ST-1
#ifdef GRAPHDEBUG
		cout << "Adding Fifo-Nested-1/2/GEN or EnqResetST-1 edges\n";
#endif

		retValue = add_FifoNested_1_2_Gen_EnqResetST_1_Edges();
		if (retValue == 1) edgeAdded = true;
		else if (retValue == -1) {
			cout << "ERROR: While adding FIFO-NESTED-1/2/GEN or ENQRESET-ST-1 edges\n";
			return -1;
		} else if (retValue != 0) {
			cout << "ERROR: Unknown return value from add_FifoNested_1_2_Gen_EnqResetST_1_Edges()\n";
			return -1;
		}

#ifdef PRINTGRAPH
		if (retValue == 1)
			graph->printGraph();
#endif

		// ENQRESET-ST-2/3
#ifdef GRAPHDEBUG
		cout << "Adding EnqReset-ST-2/3 edges\n";
#endif

		retValue = add_EnqReset_ST_2_3_Edges();
		if (retValue == 1) edgeAdded = true;
		else if (retValue == -1) {
			cout << "ERROR: While adding ENQRESET-ST-2/3 edges\n";
			return -1;
		} else if (retValue != 0) {
			cout << "ERROR: Unknown return value from add_EnqReset_ST_2_3_Edges()\n";
			return -1;
		}

#ifdef PRINTGRAPH
		if (retValue == 1)
			graph->printGraph();
#endif

		// TRANS-ST/MT
#ifdef GRAPHDEBUG
		cout << "Adding Trans-ST/MT edges\n";
#endif
		retValue = addTransSTOrMTEdges();
		if (retValue == 1) edgeAdded = true;
		else if (retValue == -1) {
			cout << "ERROR: While adding TRANS-ST/MT edges\n";
			return -1;
		} else if (retValue != 0) {
			cout << "ERROR: Unknown return value from addTransSTOrMTEdges()\n";
			return -1;
		}

		if (!edgeAdded) // If no edges were added in this iteration, stop.
			break;
		else
			edgeAdded = false;
	}

#ifdef PRINTGRAPH
	graph->printGraph();
#endif

	cout << "Total op edges = " << graph->numOfOpEdges << "\n";
	cout << "Total block edges = " << graph->numOfBlockEdges << "\n";
	cout << "Total op edges removed = " << graph->numOfOpEdgesRemoved << "\n";
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
			IDType nodeI = opIDMap[opI].nodeID;
			IDType nodeJ = opIDMap[opJ].nodeID;
			if (nodeI <= 0) {
				cout << "ERROR: Invalid node ID for op " << opI << "\n";
				return -1;
			} else if (nodeJ <= 0) {
				cout << "ERROR: Invalid node ID for op " << opJ << "\n";
				return -1;
			} else {
				int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ);
				if (addEdgeRetValue == 1) {
					flag = true;
#ifdef GRAPHDEBUG
					cout << "FORK edge (" << nodeI << ", " << nodeJ << ") -- #op-edges "   << graph->numOfOpEdges
						 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
#ifdef GRAPHDEBUGFULL
				} else if (addEdgeRetValue == 0) {
					cout << "DEBUG: Edge (" << nodeI << ", " << nodeJ << ") already implied in the graph\n";
#endif
				} else if (addEdgeRetValue == -1) {
					cout << "ERROR: While adding FORK Op edge from " << nodeI << " to " << nodeJ << endl;
					return -1;
				}
			}
#ifdef GRAPHDEBUGFULL
		} else {
			if (opI <= 0) {
				cout << "DEBUG: Cannot find fork op of thread " << it->first << endl;
				cout << "DEBUG: Skipping FORK-edge for thread " << it->first << endl;
			} else if (opJ <= 0) {
				cout << "DEBUG: Cannot find threadinit of thread " << it->first << endl;
				cout << "DEBUG: Skipping FORK-edge for thread " << it->first << endl;
			}
#endif
		}

		// Adding JOIN edges

		opI = it->second.threadexitOpID;
		opJ = it->second.joinOpID;

		if (opI > 0 && opJ > 0) {
			IDType nodeI = opIDMap[opI].nodeID;
			IDType nodeJ = opIDMap[opJ].nodeID;
			if (nodeI <= 0) {
				cout << "ERROR: Invalid node ID for op " << opI << "\n";
				return -1;
			} else if (nodeJ <= 0) {
				cout << "ERROR: Invalid node ID for op " << opJ << "\n";
				return -1;
			} else {
				int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ);
				if (addEdgeRetValue == 1) {
					flag = true;
#ifdef GRAPHDEBUG
					cout << "JOIN edge (" << nodeI << ", " << nodeJ << ") -- #op-edges "   << graph->numOfOpEdges
						 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
#ifdef GRAPHDEBUGFULL
				} else if (addEdgeRetValue == 0) {
					cout << "DEBUG: Edge (" << nodeI << ", " << nodeJ << ") already implied in the graph\n";
#endif
				} else if (addEdgeRetValue == -1) {
					cout << "ERROR: While adding JOIN Op edge from " << nodeI << " to " << nodeJ << endl;
					return -1;
				}
			}
		} else {
#ifdef GRAPHDEBUGFULL
			if (opI <= 0) {
#ifdef GRAPHDEBUG
				cout << "DEBUG: Cannot find threadexit op of thread " << it->first << endl;
				cout << "DEBUG: Skipping JOIN-edge for thread " << it->first << endl;
#endif
			} else if (opJ <= 0) {
#ifdef GRAPHDEBUG
				cout << "DEBUG: Cannot find join op of thread " << it->first << endl;
				cout << "DEBUG: Skipping JOIN-edge for thread " << it->first << endl;
#endif
			}
#endif
		}


		// Adding LOOP-PO edges

		IDType blockI = it->second.firstBlockID;
		IDType enterloopBlock = it->second.enterloopBlockID;
		IDType lastBlockInThread = it->second.lastBlockID;

#ifdef SANITYCHECK
		if (blockI <= 0) {
#ifdef GRAPHDEBUGFULL
			cout << "DEBUG: Cannot find first block of thread " << it->first << endl;
			cout << "DEBUG: Skipping LOOP-PO edges for this thread\n";
#endif
			continue;
//			return -1;
		}
		if (enterloopBlock <= 0) {
#ifdef GRAPHDEBUGFULL
			cout << "DEBUG: Cannot find enterloop block of thread " << it->first << endl;
			cout << "DEBUG: Skipping LOOP-PO edges for thread " << it->first << endl;
#endif
			continue;
		}
		if (lastBlockInThread <= 0) {
#ifdef GRAPHDEBUGFULL
			cout << "DEBUG: Cannot find last block of thread " << it->first << endl;
			cout << "DEBUG: Skipping LOOP-PO edges for this thread\n";
#endif
			continue;
//			return -1;
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
#ifdef GRAPHDEBUGFULL
				cout << "DEBUG: Cannot find next block of block " <<  b1 << " in thread " << it->first << endl;
				cout << "DEBUG: Block " << b1 << ": ops(" << blockIDMap[b1].firstOpInBlock
					 << " - " << blockIDMap[b1].lastOpInBlock << "\n";
				cout << "DEBUG: Skipping LOOP-PO edge for this block\n";
#endif
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
				IDType nodeI = opIDMap[opI].nodeID;
				IDType nodeJ = opIDMap[opJ].nodeID;
				if (nodeI <= 0) {
					cout << "ERROR: Invalid node ID for op " << opI << "\n";
					return -1;
				} else if (nodeJ <= 0) {
					cout << "ERROR: Invalid node ID for op " << opJ << "\n";
					return -1;
				} else {
					int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ);
					if (addEdgeRetValue == 1) {
						flag = true;
#ifdef GRAPHDEBUG
						cout << "LOOP-PO edge (" << nodeI << ", " << nodeJ << ") -- #op-edges "   << graph->numOfOpEdges
							 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
#ifdef GRAPHDEBUGFULL
					} else if (addEdgeRetValue == 0) {
						cout << "DEBUG: Edge (" << nodeI << ", " << nodeJ << ") already implied in the graph\n";
#endif
					} else if (addEdgeRetValue == -1) {
						cout << "ERROR: While adding LOOP-PO Op edge from " << nodeI << " to " << nodeJ << endl;
						return -1;
					}
				}
			}
		}

		blockI = blockIDMap[enterloopBlock].nextBlockInThread;
		IDType exitloopBlock = it->second.exitloopBlockID;

#ifdef SANITYCHECK
		if (blockI <= 0) {
#ifdef GRAPHDEBUGFULL
			cout << "DEBUG: Cannot find next block of block " << enterloopBlock << " in thread " << it->first << endl;
			cout << "DEBUG: Block " << enterloopBlock << ": ops (" << blockIDMap[enterloopBlock].firstOpInBlock
				 << " - " << blockIDMap[enterloopBlock].lastOpInBlock << ")\n";
			cout << "DEBUG: Skipping LOOP-PO edge for this block\n";
#endif
			continue;
		}
		if (exitloopBlock <= 0) {
#ifdef GRAPHDEBUGFULL
			cout << "DEBUG: Cannot find exitloop block of thread " << it->first << endl;
			cout << "DEBUG: Skipping LOOP-PO edges for this thread\n";
#endif
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
#ifdef GRAPHDEBUGFULL
				cout << "DEBUG: Cannot find previous block of block " << blockJ << " in thread " << it->first << endl;
				cout << "DEBUG: Block " << blockJ << ": ops(" << blockIDMap[blockJ].firstOpInBlock
					 << " - " << blockIDMap[blockJ].lastOpInBlock << "\n";
				cout << "DEBUG: Skipping LOOP-PO edges for this block\n";
#endif
//				return -1;
				continue;
			}
#endif

			for (IDType b2 = blockI; (b2 > 0 && b2 <= blockJ); b2 = blockIDMap[b2].nextBlockInThread) {

#ifdef SANITYCHECK
				assert(b2 > 0);
#endif

				opI = blockIDMap[b1].firstOpInBlock;
				opJ = blockIDMap[b2].lastOpInBlock;

				if (opI <= 0) {
					cout << "ERROR: Cannot find first op of block " << b1 << "\n";
					return -1;
				}
				if (opJ <= 0) {
					cout << "ERROR: Cannot find last op of block " << b2 << "\n";
					return -1;
				}
#ifdef SANITYCHECK
				assert(opI > 0);
				assert(opJ > 0);
#endif
				IDType nodeI = opIDMap[opI].nodeID;
				IDType nodeJ = opIDMap[opJ].nodeID;
				if (nodeI <= 0) {
					cout << "ERROR: Invalid node ID for op " << opI << "\n";
					return -1;
				} else if (nodeJ <= 0) {
					cout << "ERROR: Invalid node ID for op " << opJ << "\n";
					return -1;
				} else {
					int addEdgeRetValue = graph->addOpEdge(nodeJ, nodeI);
					if (addEdgeRetValue == 1) {
						flag = true;
#ifdef GRAPHDEBUG
						cout << "LOOP-PO edge (" << nodeJ << ", " << nodeI << ") -- #op-edges "   << graph->numOfOpEdges
							 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
#ifdef GRAPHDEBUGFULL
					} else if (addEdgeRetValue == 0) {
						cout << "DEBUG: Edge (" << nodeJ << ", " << nodeI << ") already implied in the graph\n";
#endif
					} else if (addEdgeRetValue == -1) {
						cout << "ERROR: While adding LOOP-PO Op edge from " << nodeJ << " to " << nodeI << endl;
						return -1;
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

int UAFDetector::add_TaskPO_EnqueueSTOrMT_Edges() {
	bool flag = false; // To keep track of whether edges were added.

	for (map<string, UAFDetector::taskDetails>::iterator it = taskIDMap.begin(); it != taskIDMap.end(); it++) {

		// ENQUEUE-ST/MT

		IDType enqOp = it->second.enqOpID;
		IDType deqOp = it->second.deqOpID;

		if (enqOp > 0 && deqOp > 0) {
			IDType nodeEnq = opIDMap[enqOp].nodeID;
			IDType nodeDeq = opIDMap[deqOp].nodeID;
			if (nodeEnq <= 0) {
				cout << "ERROR: Invalid node ID for op " << enqOp << "\n";
				return -1;
			} else if (nodeDeq <= 0) {
				cout << "ERROR: Invalid node ID for op " << deqOp << "\n";
				return -1;
			} else {
				int addEdgeRetValue = graph->addOpEdge(nodeEnq, nodeDeq);
				if (addEdgeRetValue == 1) {
					flag = true;
#ifdef GRAPHDEBUG
					cout << "ENQUEUE-ST/MT edge (" << nodeEnq << ", " << nodeDeq << ") -- #op-edges "   << graph->numOfOpEdges
						 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
#ifdef GRAPHDEBUGFULL
				} else if (addEdgeRetValue == 0) {
					cout << "DEBUG: Edge (" << nodeEnq << ", " << nodeDeq << ") already implied in the graph\n";
#endif
				} else if (addEdgeRetValue == -1) {
					cout << "ERROR: While adding ENQUEUE-ST/MT Op edge from " << nodeEnq << " to " << nodeDeq << endl;
					return -1;
				}
			}
		} else {
			if (enqOp <= 0) {
#ifdef GRAPHDEBUGFULL
				cout << "DEBUG: Cannot find enq op of task " << it->first << endl;
				cout << "DEBUG: Skipping ENQUEUE-ST/MT edge for this task\n";
#endif
			}
			if (deqOp <= 0) {
#ifdef GRAPHDEBUGFULL
				cout << "DEBUG: Cannot find deq op of task " << it->first << endl;
				cout << "DEBUG: Skipping ENQUEUE-ST/MT edge for this task\n";
#endif
			}
		}

		// TASK-PO

		IDType firstBlockInTask = it->second.firstBlockID;
		IDType lastBlockInTask = it->second.lastBlockID;

#ifdef SANITYCHECK
		if (firstBlockInTask <= 0) {
#ifdef GRAPHDEBUGFULL
			cout << "DEBUG: Cannot find first block of task " << it->first << endl;
			cout << "DEBUG: Skipping TASK-PO edges for this task\n";
#endif
//			return -1;
			continue;
		}
		if (lastBlockInTask <= 0) {
#ifdef GRAPHDEBUGFULL
			cout << "DEBUG: Cannot find last block of task " << it->first << endl;
			cout << "DEBUG: Skipping TASK-PO edges for this task\n";
#endif
//			return -1;
			continue;
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
				IDType nodeI = opIDMap[opI].nodeID;
				IDType nodeJ = opIDMap[opJ].nodeID;
				if (nodeI <= 0) {
					cout << "ERROR: Invalid node ID for op " << opI << "\n";
					return -1;
				} else if (nodeJ <= 0) {
					cout << "ERROR: Invalid node ID for op " << opJ << "\n";
					return -1;
				} else {
					int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ);
					if (addEdgeRetValue == 1) {
						flag = true;
#ifdef GRAPHDEBUG
						cout << "TASK-PO edge (" << nodeI << ", " << nodeJ << ") -- #op-edges "   << graph->numOfOpEdges
							 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
#ifdef GRAPHDEBUGFULL
					} else if (addEdgeRetValue == 0) {
						cout << "DEBUG: Edge (" << nodeI << ", " << nodeJ << ") already implied in the graph\n";
#endif
					} else if (addEdgeRetValue == -1) {
						cout << "ERROR: While adding TASK-PO Op edge from " << nodeI << " to " << nodeJ << endl;
						return -1;
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

int UAFDetector::add_FifoAtomic_NoPre_Edges() {

	bool flag = false; // To keep track of whether edges were added.


	for (map<string, taskDetails>::iterator it = taskIDMap.begin(); it != taskIDMap.end(); it++) {

		// If the task is not atomic, the rule does not apply
		if (it->second.atomic == false) {
#ifdef GRAPHDEBUGFULL
			cout << "DEBUG: Task " << it->first << " is not atomic\n";
			cout << "DEBUG: Skipping FIFO-ATOMIC/NO-PRE edges for this task\n";
#endif
			continue;
		}

		if (it->second.enqOpID > 0 && it->second.endOpID <= 0 ) {
			// Saw an enq of the task, but no end op
			// FIFO-ATOMIC/NO-PRE edges are from end of a task to deq of another task
			// Formerly I checked if there was a deq of the task, and if not, I assumed there
			// was no end op. However..
			// What if there was no deq but there is an end. Maybe our weird slicing criterion
			// creates this situation.
			// So we check if there is an end op for the task
#ifdef GRAPHDEBUGFULL
			cout << "DEBUG: Saw enq of task " << it->first << " at " << it->second.enqOpID << ", but no end op\n";
			cout << "DEBUG: Skipping FIFO-ATOMIC/NO-PRE edges for this task\n";
#endif
			continue;
		}

		// Adding FIFO-ATOMIC edges.
		IDType enqOp = it->second.enqOpID;
		if (enqOp <= 0) {
#ifdef GRAPHDEBUGFULL
			cout << "DEBUG: Cannot find enq of task " << it->first << endl;
			cout << "DEBUG: Skipping FIFO-ATOMIC edge for this task\n";
#endif
		} else {
			IDType enqOpBlock = opIDMap[enqOp].blockID;
			if (enqOpBlock <= 0) {
#ifdef GRAPHDEBUGFULL
				cout << "DEBUG: Cannot find block of enq op " << enqOp << endl;
				cout << "DEBUG: Skipping FIFO-ATOMIC edge for task " << it->first << "\n";
#endif
			} else {
				IDType opI = it->second.endOpID;
				if (opI <= 0) {
#ifdef GRAPHDEBUGFULL
					cout << "DEBUG: Cannot find end of task " << opI << endl;
					cout << "DEBUG: Skipping FIFO-ATOMIC edge for task " << it->first << "\n";
#endif
				} else {
					for (std::multiset<HBGraph::adjListNode>::iterator blockIt = graph->blockAdjList[enqOpBlock].begin();
							blockIt != graph->blockAdjList[enqOpBlock].end(); blockIt++) {
						IDType tempBlock = blockIt->blockID;
						if (tempBlock <= 0) {
#ifdef GRAPHDEBUGFULL
							cout << "DEBUG: Found invalid block edge from " << enqOpBlock << endl;
#endif
							continue;
						}

						for (set<IDType>::iterator enqIt = blockIDMap[tempBlock].enqSet.begin();
								enqIt != blockIDMap[tempBlock].enqSet.end(); enqIt++) {
							IDType tempenqOp = *enqIt;

							// If we are looking at enqs within the same block,
							// make sure we do not have the same enq as both
							// source and destination for the HB edge we check.
							if (enqOpBlock == tempBlock && enqOp == tempenqOp) continue;

							string taskName = enqToTaskEnqueued[tempenqOp].taskEnqueued;
							if (taskName.compare("") == 0) {
#ifdef GRAPHDEBUGFULL
								cout << "DEBUG: Cannot find task enqueued in op " << tempenqOp << endl;
#endif
								continue;
							}

							// FIFO-ATOMIC does not apply if the two tasks are not posted to the same thread
							if (enqToTaskEnqueued[enqOp].targetThread != enqToTaskEnqueued[tempenqOp].targetThread) continue;

							IDType nodeEnq = opIDMap[enqOp].nodeID;
							IDType nodeTempEnq = opIDMap[tempenqOp].nodeID;
							if (nodeEnq <= 0) {
								cout << "ERROR: Invalid node ID for op " << enqOp << "\n";
								return -1;
							} else if (nodeTempEnq <= 0) {
								cout << "ERROR: Invalid node ID for op " << tempenqOp << "\n";
								return -1;
							} else {
								int retOpValue1 = graph->opEdgeExists(nodeEnq, nodeTempEnq);
								if (retOpValue1 == 1) {
									IDType opJ = taskIDMap[taskName].deqOpID;
									if (opJ <= 0) {
#ifdef GRAPHDEBUGFULL
										cout << "DEBUG: Cannot find deq op of task " << taskName << endl;
#endif
										continue;
									}

									IDType nodeI = opIDMap[opI].nodeID;
									IDType nodeJ = opIDMap[opJ].nodeID;
									if (nodeI <= 0) {
										cout << "ERROR: Invalid node ID for op " << opI << "\n";
										return -1;
									} else if (nodeJ <= 0) {
										cout << "ERROR: Invalid node ID for op " << opJ << "\n";
										return -1;
									} else {
										int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ);
										if (addEdgeRetValue == 1) {
											flag = true;
#ifdef GRAPHDEBUG
											cout << "FIFO-ATOMIC edge (" << nodeI << ", " << nodeJ << ") -- #op-edges "   << graph->numOfOpEdges
												 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
#ifdef GRAPHDEBUGFULL
										} else if (addEdgeRetValue == 0) {
											cout << "DEBUG: Edge (" << nodeI << ", " << nodeJ << ") already implied in the graph\n";
#endif
										} else if (addEdgeRetValue == -1) {
											cout << "ERROR: While adding FIFO-ATOMIC edge from " << nodeI << " to " << nodeJ << endl;
											return -1;
										}
									}
								} else if (retOpValue1 == -1) {
									cout << "ERROR: While checking for op-edge (" << nodeEnq << ", " << nodeTempEnq << ")\n";
									return -1;
								}
							}
						}
					}
				}
			}
		}

		// NO-PRE edges

		IDType i = it->second.deqOpID;
		IDType opI = it->second.endOpID;

#ifdef SANITYCHECK
		if (i <= 0) {
#ifdef GRAPHDEBUGFULL
			cout << "DEBUG: Cannot find deq op of task " << it->first << endl;
			cout << "DEBUG: If there is no deq op, find the first op of task\n";
#endif
			IDType firstBlockOfTask = taskIDMap[it->first].firstBlockID;
			if (firstBlockOfTask <= 0) {
#ifdef GRAPHDEBUGFULL
				cout << "DEBUG: Cannot find first block of task " << it->first << "\n";
				cout << "DEBUG: This means there are no ops in the task\n";
				cout << "DEBUG: Skipping NOPRE edges for this task\n";
#endif
			} else {
				i = blockIDMap[firstBlockOfTask].firstOpInBlock;
#ifdef GRAPHDEBUGFULL
				cout << "DEBUG: Using op " << i << " in place of deq op, since it is the "
					 << "first op in the task " << it->first << "\n";
#endif
			}
		}
		if (opI <= 0) {
#ifdef GRAPHDEBUGFULL
			cout << "DEBUG: Cannot find end op of task " << it->first << endl;
			cout << "DEBUG: Skipping NOPRE edges for this task\n";
#endif
		}
#endif

		if (i > 0 && opI > 0) {
			for(; (i > 0 && i <= opI); i = opIDMap[i].nextOpInTask) {
				IDType blockI = opIDMap[i].blockID;
#ifdef SANITYCHECK
				if (blockI <= 0) {
					cout << "ERROR: Cannot find block of op " << i << endl;
					return -1;
				}
#endif
				if (blockI > 0) {
					for (std::multiset<HBGraph::adjListNode>::iterator blockIt = graph->blockAdjList[blockI].begin();
							blockIt != graph->blockAdjList[blockI].end(); blockIt++) {
#ifdef SANITYCHECK
						assert(blockIt->blockID);
#endif

						for (set<IDType>::iterator enqIt = blockIDMap[blockIt->blockID].enqSet.begin();
								enqIt != blockIDMap[blockIt->blockID].enqSet.end(); enqIt++) {
#ifdef SANITYCHECK
							assert(*enqIt > 0);

#endif
							IDType tempenqOp = *enqIt;

							// If we are looking at enqs within the same block,
							// make sure we do not have the same enq as both
							// source and destination for the HB edge we check.
							if (blockI == blockIt->blockID && i == tempenqOp) continue;

							string taskName = enqToTaskEnqueued[tempenqOp].taskEnqueued;
							if (taskName.compare("") == 0) {
#ifdef GRAPHDEBUGFULL
								cout << "DEBUG: Cannot find task enqueued in op " << tempenqOp << endl;
#endif
//								continue;
								return -1;
							}

							// NO-PRE does not apply if the two tasks are in different threads
							if (opIDMap[i].threadID != enqToTaskEnqueued[tempenqOp].targetThread) continue;

							IDType nodei = opIDMap[i].nodeID;
							IDType nodeTempEnq = opIDMap[tempenqOp].nodeID;
							if (nodei <= 0) {
								cout << "ERROR: Invalid node ID for op " << i << "\n";
								return -1;
							} else if (nodeTempEnq <= 0) {
								cout << "ERROR: Invalid node ID for op " << nodeTempEnq << "\n";
								return -1;
							} else {
								int retOpValue1 = graph->opEdgeExists(nodei, nodeTempEnq);
								if (retOpValue1 == 1) {
									IDType opJ = taskIDMap[taskName].deqOpID;
									if (opJ <= 0) {
#ifdef GRAPHDEBUGFULL
										cout << "DEBUG: Cannot find deq op of task " << taskName << endl;
#endif
#ifdef EXTRARULES
										cout << "DEBUG: Instead adding the edge to the first op of task "
											 << taskName << "\n";
										IDType firstBlockOfTask = taskIDMap[taskName].firstBlockID;
										if (firstBlockOfTask <= 0) {
#ifdef GRAPHDEBUG
											cout << "DEBUG: Cannot find first block of task " << taskName << "\n";
											cout << "DEBUG: Skipping NOPRE edge from end op " << opI
												 << " of task " << it->first << " to first op of task " << taskName
												 << "\n";
											continue;
#endif
										} else {
											IDType firstOpOfBlock = blockIDMap[firstBlockOfTask].firstOpInBlock;
											if (firstOpOfBlock <= 0) {
#ifdef GRAPHDEBUG
												cout << "DEBUG: Cannot find first op of block " << firstBlockOfTask
													 << " - this is the first block of task " << taskName << "\n";
												cout << "DEBUG: Skipping NOPRE edge from end op " << opI
													 << " of task " << it->first << " to first op of task "
													 << taskName << "\n";
												continue;
#endif
											} else {
#ifdef GRAPHDEBUG
												cout << "DEBUG: Adding NOPRE edge from end op " << opI
													 << " of task " << it->first << " to first op " << firstOpOfBlock
													 << " of task " << taskName << "\n";
#endif
												opJ = firstOpOfBlock;
											}
										}
#endif
									}

									if (opJ <= 0) {
#ifdef GRAPHDEBUGFULL
										cout << "DEBUG: Skipping NOPRE edge from end op " << opI
											 << " of task " << it->first << " to deq op of task " << taskName << "\n";
#endif
										continue;
									}

									IDType nodeI = opIDMap[opI].nodeID;
									IDType nodeJ = opIDMap[opJ].nodeID;
									if (nodeI <= 0 ) {
										cout << "ERROR: Invalid node ID for op " << opI << "\n";
										return -1;
									} else if (nodeJ <= 0) {
										cout << "ERROR: Invalid node ID for op " << opJ << "\n";
										return -1;
									} else {
										int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ);
										if (addEdgeRetValue == 1) {
											flag = true;
#ifdef GRAPHDEBUG
											cout << "NO-PRE edge (" << nodeI << ", " << nodeJ << ") -- #op-edges "   << graph->numOfOpEdges
												 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
#ifdef GRAPHDEBUGFULL
										} else if (addEdgeRetValue == 0) {
											cout << "DEBUG: Edge (" << nodeI << ", " << nodeJ << ") already implied in the graph\n";
#endif
										} else if (addEdgeRetValue == -1) {
											cout << "ERROR: While adding NO-PRE edge from " << nodeI << " to " << nodeJ << endl;
											return -1;
										}
									}
								} else if (retOpValue1 == -1) {
									cout << "ERROR: While checking for op-edge (" << nodei << ", " << nodeTempEnq << ")\n";
									return -1;
								}
							}
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

int UAFDetector::add_PauseSTMT_ResumeSTMT_Edges() {
	bool flag = false;

	for (map<std::string, UAFDetector::nestingLoopDetails>::iterator it = nestingLoopMap.begin(); it != nestingLoopMap.end(); it++) {
		IDType opI, opJ;

		for (vector<UAFDetector::pauseResumeResetTuple>::iterator prrIt = it->second.pauseResumeResetSet.begin();
				prrIt != it->second.pauseResumeResetSet.end(); prrIt++) {

			// PAUSE-ST/MT
			IDType pauseOp = prrIt->pauseOp;
			IDType resetOp = prrIt->resetOp;
			IDType resumeOp = prrIt->resumeOp;

			opI = pauseOp;

			if (opI > 0 && resetOp > 0) {
				IDType threadOfPauseOp = opIDMap[opI].threadID;
				if (threadOfPauseOp < 0) {
					cout << "ERROR: Cannot find thread ID of pause op " << opI << "\n";
					return -1;
				}
				IDType threadOfResetOp = opIDMap[resetOp].threadID;
				if (threadOfResetOp < 0) {
					cout << "ERROR: Cannot find thread ID of reset op " << resetOp << "\n";
					return -1;
				}

				if (threadOfPauseOp != threadOfResetOp) {
					opJ = resetOp;
					IDType nodeI = opIDMap[opI].nodeID;
					IDType nodeJ = opIDMap[opJ].nodeID;
					if (nodeI <= 0) {
						cout << "ERROR: Invalid node ID for op " << opI << "\n";
						return -1;
					} else if (nodeJ <= 0) {
						cout << "ERROR: Invalid node ID for op " << opJ << "\n";
						return -1;
					} else {
						int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ);
						if (addEdgeRetValue == 1) {
							flag = true;
#ifdef GRAPHDEBUG
							cout << "PAUSE-MT edge (" << nodeI << ", " << nodeJ << ") -- #op-edges "   << graph->numOfOpEdges
								 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
#ifdef GRAPHDEBUGFULL
						} else if (addEdgeRetValue == 0) {
							cout << "DEBUG: Edge (" << nodeI << ", " << nodeJ << ") already implied in the graph\n";
#endif
						} else if (addEdgeRetValue == -1) {
							cout << "ERROR: While adding PAUSE-MT edge from " << nodeI << " to " << nodeJ << endl;
							return -1;
						}
					}
				} else {
					std::string taskOfResetOp = opIDMap[resetOp].taskID;
					std::string taskOfPauseOp = opIDMap[pauseOp].taskID;
					if (taskOfResetOp.compare("") != 0 && taskOfPauseOp.compare("") != 0 &&
							taskOfResetOp.compare(taskOfPauseOp) != 0) {
						opJ = taskIDMap[taskOfResetOp].deqOpID;
						if (opJ <= 0) {
#ifdef GRAPHDEBUGFULL
							cout << "DEBUG: Cannot find deq of task " << taskOfResetOp << "\n";
							cout << "DEBUG: Skipping PAUSE-ST/MT for this task\n";
#endif
						} else {
							IDType nodeI = opIDMap[opI].nodeID;
							IDType nodeJ = opIDMap[opJ].nodeID;
							if (nodeI <= 0) {
								cout << "ERROR: Invalid node ID for op " << opI << "\n";
								return -1;
							} else if (nodeJ <= 0) {
								cout << "ERROR: Invalid node ID for op " << opJ << "\n";
								return -1;
							} else {
								int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ);
								if (addEdgeRetValue == 1) {
									flag = true;
#ifdef GRAPHDEBUG
									cout << "PAUSE-ST edge (" << nodeI << ", " << nodeJ << ") -- #op-edges "   << graph->numOfOpEdges
										 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
#ifdef GRAPHDEBUGFULL
								} else if (addEdgeRetValue == 0) {
									cout << "DEBUG: Edge (" << nodeI << ", " << nodeJ << ") already implied in the graph\n";
#endif
								} else if (addEdgeRetValue == -1) {
									cout << "ERROR: While adding PAUSE-ST edge from " << nodeI << " to " << nodeJ << endl;
									return -1;
								}
							}
						}
					} else if (taskOfResetOp.compare("") == 0) {
#ifdef GRAPHDEBUGFULL
						cout << "DEBUG: Cannot find task of reset op " << resetOp << "\n";
						cout << "DEBUG: Skipping PAUSE-ST/MT for this task\n";
#endif
					} else if (taskOfPauseOp.compare("") == 0) {
						cout << "ERROR: Cannot find task of pause op " << pauseOp << "\n";
						return -1;
					} else if (taskOfResetOp.compare(taskOfPauseOp) == 0) {
#ifdef GRAPHDEBUGFULL
						cout << "DEBUG: Pause op " << pauseOp << " and reset op " << resetOp
							 << " are in the same task " << taskOfResetOp << "\n";
#ifdef EXTRARULES
						cout << "DEBUG: Adding edge from pause op to reset op\n";

						opJ = resetOp;
						IDType nodeI = opIDMap[opI].nodeID;
						IDType nodeJ = opIDMap[opJ].nodeID;
						if (nodeI <= 0) {
							cout << "ERROR: Invalid node ID for op " << opI << "\n";
							return -1;
						} else if (nodeJ <= 0) {
							cout << "ERROR: Invalid node ID for op " << opJ << "\n";
							return -1;
						} else {
							int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ);
							if (addEdgeRetValue == 1) {
								flag = true;
#ifdef GRAPHDEBUG
								cout << "PAUSE-ST edge (" << nodeI << ", " << nodeJ << ") -- #op-edges "   << graph->numOfOpEdges
									 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
							} else if (addEdgeRetValue == 0) {
								cout << "DEBUG: Edge (" << nodeI << ", " << nodeJ << ") already implied in the graph\n";
							} else if (addEdgeRetValue == -1) {
								cout << "ERROR: While adding PAUSE-ST edge from " << nodeI << " to " << nodeJ << endl;
								return -1;
							}
						}
#else
						cout << "DEBUG: Skipping PAUSE-ST edge for this task\n";
#endif
#endif
					}
				}

			} else if (opI <= 0) {
#ifdef GRAPHDEBUGFULL
				cout << "DEBUG: Cannot find pause for shared variable " << it->first << "\n";
				cout << "DEBUG: Skipping PAUSE-ST/MT\n";
#endif
			} else if (resetOp <= 0) {
#ifdef GRAPHDEBUGFULL
				cout << "DEBUG: Cannot find reset for shared variable " << it->first << "\n";
				cout << "DEBUG: Skipping PAUSE-ST/MT\n";
#endif
			}

			// RESUME-ST/MT
			if (resetOp > 0 && resumeOp > 0) {

				IDType threadOfResumeOp = opIDMap[resumeOp].threadID;
				if (threadOfResumeOp < 0) {
					cout << "ERROR: Cannot find thread ID of resume op " << resumeOp << "\n";
					return -1;
				}
				IDType threadOfResetOp = opIDMap[resetOp].threadID;
				if (threadOfResetOp < 0) {
					cout << "ERROR: Cannot find thread ID of reset op " << resetOp << "\n";
					return -1;
				}

				opI = resetOp;
				opJ = resumeOp;

				if (threadOfResetOp != threadOfResumeOp) {
					IDType nodeI = opIDMap[opI].nodeID;
					IDType nodeJ = opIDMap[opJ].nodeID;
					if (nodeI <= 0) {
						cout << "ERROR: Invalid node ID for op " << opI << "\n";
						return -1;
					} else if (nodeJ <= 0) {
						cout << "ERROR: Invalid node ID for op " << opJ << "\n";
						return -1;
					} else {
						int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ);
						if (addEdgeRetValue == 1) {
							flag = true;
#ifdef GRAPHDEBUG
							cout << "RESUME-MT edge (" << nodeI << ", " << nodeJ << ") -- #op-edges "   << graph->numOfOpEdges
								 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
#ifdef GRAPHDEBUGFULL
						} else if (addEdgeRetValue == 0) {
							cout << "DEBUG: Edge (" << nodeI << ", " << nodeJ << ") already implied in the graph\n";
#endif
						} else if (addEdgeRetValue == -1) {
							cout << "ERROR: While adding RESUME-MT edge from " << nodeI << " to " << nodeJ << endl;
							return -1;
						}
					}
				} else {
					std::string taskOfResetOp = opIDMap[resetOp].taskID;
					std::string taskOfResumeOp = opIDMap[resumeOp].taskID;
					if (taskOfResetOp.compare("") != 0 && taskOfResumeOp.compare("") != 0 &&
							taskOfResetOp.compare(taskOfResumeOp) != 0) {
						opI = taskIDMap[taskOfResetOp].endOpID;
						if (opI <= 0) {
#ifdef GRAPHDEBUGFULL
							cout << "DEBUG: Cannot find end of task " << taskOfResetOp << "\n";
							cout << "DEBUG: Skipping RESUME-ST/MT for this task\n";
#endif
						} else {
							IDType nodeI = opIDMap[opI].nodeID;
							IDType nodeJ = opIDMap[opJ].nodeID;
							if (nodeI <= 0) {
								cout << "ERROR: Invalid node ID for op " << opI << "\n";
								return -1;
							} else if (nodeJ <= 0) {
								cout << "ERROR: Invalid node ID for op " << opJ << "\n";
								return -1;
							} else {
								int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ);
								if (addEdgeRetValue == 1) {
									flag = true;
#ifdef GRAPHDEBUG
									cout << "RESUME-ST edge (" << nodeI << ", " << nodeJ << ") -- #op-edges "   << graph->numOfOpEdges
										 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
#ifdef GRAPHDEBUGFULL
								} else if (addEdgeRetValue == 0) {
									cout << "DEBUG: Edge (" << nodeI << ", " << nodeJ << ") already implied in the graph\n";
#endif
								} else if (addEdgeRetValue == -1) {
									cout << "ERROR: While adding RESUME-ST edge from " << nodeI << " to " << nodeJ << endl;
									return -1;
								}
							}
						}
					} else if (taskOfResetOp.compare("") == 0) {
#ifdef GRAPHDEBUGFULL
						cout << "DEBUG: Cannot find task of reset op " << resetOp << "\n";
						cout << "DEBUG: Skipping PAUSE-ST/MT for this task\n";
#endif
					} else if (taskOfResumeOp.compare("") == 0) {
						cout << "ERROR: Cannot find task of resume op " << resumeOp << "\n";
						return -1;
					} else if (taskOfResetOp.compare(taskOfResumeOp) == 0) {
#ifdef GRAPHDEBUGFULL
						cout << "DEBUG: Resume op " << resumeOp << " and reset op " << resetOp
							 << " are in the same task " << taskOfResetOp << "\n";
#ifdef EXTRARULES
						cout << "DEBUG: Adding edge from reset op to resume op\n";

						opI = resetOp;
						opJ = resumeOp;
						IDType nodeI = opIDMap[opI].nodeID;
						IDType nodeJ = opIDMap[opJ].nodeID;
						if (nodeI <= 0) {
							cout << "ERROR: Invalid node ID for op " << opI << "\n";
							return -1;
						} else if (nodeJ <= 0) {
							cout << "ERROR: Invalid node ID for op " << opJ << "\n";
							return -1;
						} else {
							int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ);
							if (addEdgeRetValue == 1) {
								flag = true;
#ifdef GRAPHDEBUG
								cout << "RESUME-ST edge (" << nodeI << ", " << nodeJ << ") -- #op-edges "   << graph->numOfOpEdges
									 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
							} else if (addEdgeRetValue == 0) {
								cout << "DEBUG: Edge (" << nodeI << ", " << nodeJ << ") already implied in the graph\n";
							} else if (addEdgeRetValue == -1) {
								cout << "ERROR: While adding RESUME-ST edge from " << nodeI << " to " << nodeJ << endl;
								return -1;
							}
						}
#else
						cout << "DEBUG: Skipping RESUME-ST edge for this task\n";
#endif
#endif
					}
				}
			} else if (resetOp <= 0) {
#ifdef GRAPHDEBUGFULL
				cout << "DEBUG: Cannot find reset for shared variable " << it->first << "\n";
				cout << "DEBUG: Skipping RESUME-ST/MT\n";
#endif
			} else if (resumeOp <= 0) {
#ifdef GRAPHDEBUGFULL
				cout << "DEBUG: Cannot find resume for shared variable " << it->first << "\n";
				cout << "DEBUG: Skipping RESUME-ST/MT\n";
#endif
			}
		}
	}

	if (flag)
		return 1;
	else
		return 0;
}

int UAFDetector::add_WaitNotify_Edges() {
	bool flag = false;

	for (map<IDType, IDType>::iterator it = notifyToWait.begin(); it != notifyToWait.end(); it++) {
		// Adding NOTIFY-WAIT edges

		IDType opI = it->first;
		IDType opJ = it->second;

		if (opI > 0 && opJ > 0) {
			IDType nodeI = opIDMap[opI].nodeID;
			IDType nodeJ = opIDMap[opJ].nodeID;
			if (nodeI <= 0) {
				cout << "ERROR: Invalid node ID for op " << opI << "\n";
				return -1;
			} else if (nodeJ <= 0) {
				cout << "ERROR: Invalid node ID for op" << opJ << "\n";
				return -1;
			} else {
				int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ);
				if (addEdgeRetValue == 1) {
					flag = true;
#ifdef GRAPHDEBUG
					cout << "NOTIFY-WAIT edge (" << nodeI << ", " << nodeJ << ") -- #op-edges "   << graph->numOfOpEdges
						 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
#ifdef GRAPHDEBUGFULL
				} else if (addEdgeRetValue == 0) {
					cout << "DEBUG: Edge (" << nodeI << ", " << nodeJ << ") already implied in the graph\n";
#endif
				} else if (addEdgeRetValue == -1) {
					cout << "ERROR: While adding NOTIFY-WAIT edge from " << nodeI << " to " << nodeJ << endl;
					return -1;
				}
			}
		} else {
            if (opI <= 0) {
            	cout << "ERROR: Found invalid notify op in notifyToWait\n";
            	return -1;
            }
            if (opJ <= 0) {
            	cout << "ERROR: Found invalid wait op for notify " << opI << " in notifyToWait\n";
            	return -1;
            }
		}
	}

	for (map<IDType, UAFDetector::setOfOps>::iterator it = notifyAllToWaitSet.begin();
			it != notifyAllToWaitSet.end(); it++) {
		// Adding NOTIFYALL-WAIT edges

		IDType opI = it->first;
		for (set<IDType>::iterator waitIt = it->second.opSet.begin(); waitIt != it->second.opSet.end(); waitIt++) {
			IDType opJ = *waitIt;

			if (opI > 0 && opJ > 0) {
				IDType nodeI = opIDMap[opI].nodeID;
				IDType nodeJ = opIDMap[opJ].nodeID;
				if (nodeI <= 0) {
					cout << "ERROR: Invalid node ID for op " << opI << "\n";
					return -1;
				} else if (nodeJ <= 0) {
					cout << "ERROR: Invalid node ID for op " << opJ << "\n";
					return -1;
				} else {
					int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ);
					if (addEdgeRetValue == 1) {
						flag = true;
#ifdef GRAPHDEBUG
						cout << "NOTIFYALL-WAIT edge (" << nodeI << ", " << nodeJ << ") -- #op-edges "   << graph->numOfOpEdges
							 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
#ifdef GRAPHDEBUGFULL
					} else if (addEdgeRetValue == 0) {
						cout << "DEBUG: Edge (" << nodeI << ", " << nodeJ << ") already implied in the graph\n";
#endif
					} else if (addEdgeRetValue == -1) {
						cout << "ERROR: While adding NOTIFYALL-WAIT edge from " << nodeI << " to " << nodeJ << endl;
						return -1;
					}
				}
			} else {
				if (opI <= 0) {
					cout << "ERROR: Found invalid notifyall op in notifyAllToWaitSet\n";
					return -1;
				}
				if (opJ <= 0) {
					cout << "ERROR: Found invalid wait op for notifyall " << opI << " in notifyAllToWaitSet\n";
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

int UAFDetector::add_FifoNested_1_2_Gen_EnqResetST_1_Edges() {
	bool flag = false;

	for (map<std::string, UAFDetector::taskDetails>::iterator it = taskIDMap.begin(); it != taskIDMap.end(); it++) {

#if 0
		if (it->second.enqOpID > 0 && it->second.deqOpID <= 0) {
			// Saw an enq of task, but no deq
			continue;
		}
#endif

		IDType opI, opJ;
		IDType threadI, threadJ;

		// If the task is atomic, FIFO-NESTED rule does not apply.
		if (it->second.atomic == true) continue;

		// FIFO-NESTED-1
		IDType enqI, blockI;
		opI = it->second.firstPauseOpID;
		enqI = it->second.enqOpID;

		if (opI > 0 && enqI > 0) {
			threadI = opIDMap[opI].threadID;
			blockI = opIDMap[enqI].blockID;
			if (threadI < 0) {
				cout << "ERROR: Cannot find thread ID of op " << opI << "\n";
				return -1;
			}
			if (blockI <= 0) {
				cout << "ERROR: Cannot find block ID of op " << enqI << "\n";
				return -1;
			}
//			for (HBGraph::adjListNode* currNode = graph->blockAdjList[blockI].head; currNode != NULL; currNode = currNode->next) {
			for (std::multiset<HBGraph::adjListNode>::iterator blockIt = graph->blockAdjList[blockI].begin();
					blockIt != graph->blockAdjList[blockI].end(); blockIt++) {
#ifdef SANITYCHECK
//				assert(currNode->destination > 0);
				assert(blockIt->blockID > 0);
#endif

//				IDType blockJ = currNode->destination;
				IDType blockJ = blockIt->blockID;

				for (set<IDType>::iterator enqIt = blockIDMap[blockJ].enqSet.begin(); enqIt != blockIDMap[blockJ].enqSet.end(); enqIt++) {
#ifdef SANITYCHECK
					assert(*enqIt > 0);
#endif

					IDType enqJ = *enqIt;
					threadJ = enqToTaskEnqueued[enqJ].targetThread;
#ifdef SANITYCHECK
					if (threadJ < 0) {
						cout << "ERROR: Cannot find target thread of enq " << enqJ << endl;
						return -1;
					}
#endif

					// Rule applies only if enqJ posts to the same thread as first pause opI
					if (threadJ != threadI) continue;

					// If enqI and enqJ are the same, do not proceed
					if (blockI == blockJ && enqI == enqJ) continue;

					IDType nodeEnqI = opIDMap[enqI].nodeID;
					IDType nodeEnqJ = opIDMap[enqJ].nodeID;
					if (nodeEnqI <= 0) {
						cout << "ERROR: Invalid node ID for op " << enqI << "\n";
						return -1;
					} else if (nodeEnqJ <= 0) {
						cout << "ERROR: Invalid node ID for op " << enqJ << "\n";
						return -1;
					} else {
						int retValue = graph->opEdgeExists(nodeEnqI, nodeEnqJ);
						if (retValue == 1) {
							std::string taskJ = enqToTaskEnqueued[enqJ].taskEnqueued;
#ifdef SANITYCHECK
							assert(taskJ.compare("") != 0);
#endif
							opJ = taskIDMap[taskJ].deqOpID;
							if (opJ <= 0) {
#ifdef GRAPHDEBUGFULL
								cout << "DEBUG: Cannot find deq of task " << taskJ << endl;
								cout << "DEBUG: Skipping FIFO-NESTED-1 edge from pause op " << opI << "\n";
#endif
								continue;
							}
							IDType nodeI = opIDMap[opI].nodeID;
							IDType nodeJ = opIDMap[opJ].nodeID;
							if (nodeI <= 0) {
								cout << "ERROR: Invalid node ID for op " << opI << "\n";
								return -1;
							} else if (nodeJ <= 0) {
								cout << "ERROR: Invalid node ID for op " << opJ << "\n";
								return -1;
							} else {
								int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ);
								if (addEdgeRetValue == 1) {
									flag = true;
#ifdef GRAPHDEBUG
									cout << "FIFO-NESTED-1 edge (" << nodeI << ", " << nodeJ << ") -- #op-edges "   << graph->numOfOpEdges
										 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
#ifdef GRAPHDEBUGFULL
								} else if (addEdgeRetValue == 0) {
									cout << "DEBUG: Edge (" << nodeI << ", " << nodeJ << ") already implied in the graph\n";
#endif
								} else if (addEdgeRetValue == -1) {
									cout << "ERROR: While adding FIFO-NESTED-1 edge from " << nodeI << " to " << nodeJ << endl;
									return -1;
								}
							}
						} else if (retValue == -1) {
							cout << "ERROR: While checking edge from " << nodeEnqI << " to " << nodeEnqJ << endl;
							return -1;
						}
					}
				}
			}
		} else if (opI <= 0) {
#ifdef GRAPHDEBUGFULL
			cout << "DEBUG: Cannot find first pause of task " << it->first << "\n";
			cout << "DEBUG: Skipping FIFO-NESTED-1 edge for this task\n";
#endif
		} else if (enqI <= 0) {
#ifdef GRAPHDEBUGFULL
			cout << "DEBUG: Cannot find enq of task " << it->first << "\n";
			cout << "DEBUG: Skipping FIFO-NESTED-1 edge for this task\n";
#endif
		}


		// FIFO-NESTED-2
		opI = it->second.endOpID;
		IDType opL = it->second.lastResumeOpID;

		if (opL > 0) {
			if (opI <= 0) {
#ifdef GRAPHDEBUGFULL
				cout << "DEBUG: Cannot find end op of task " << it->first << "\n";
				cout << "DEBUG: Skipping FIFO-NESTED-2 edge for this task\n";
#endif
#ifdef EXTRARULES
				IDType lastBlockOfTask = taskIDMap[it->first].lastBlockID;
				if (lastBlockOfTask <= 0) {
					cout << "DEBUG: Cannot find last block of task " << it->first << "\n";
					cout << "DEBUG: Skipping FIFO-NESTED-2 edge for this task\n";
				} else {
					IDType lastOpOfTask = blockIDMap[lastBlockOfTask].lastOpInBlock;
					if (lastOpOfTask <= 0) {
						cout << "DEBUG: Cannot find last op of block " << lastBlockOfTask
							 << " - this block is the last block of task " << it->first << "\n";
						cout << "DEBUG: Skipping FIFO-NESTED-2 edge for this task\n";
					} else {
						opI = lastOpOfTask;
						cout << "DEBUG: Adding FIFO-NESTED-2 edge from last op " << opI
							 << " of task " << it->first << "\n";
					}
				}
#endif
			}
			if (opI > 0) {
				IDType blockL = opIDMap[opL].blockID;
				threadI = opIDMap[opI].threadID;
				if (blockL > 0 && threadI > 0) {
//					for (HBGraph::adjListNode* currNode = graph->blockAdjList[blockL].head; currNode != NULL; currNode = currNode->next) {
					for (std::multiset<HBGraph::adjListNode>::iterator blockIt = graph->blockAdjList[blockL].begin();
							blockIt != graph->blockAdjList[blockL].end(); blockIt++) {
#ifdef SANITYCHECK
//						assert(currNode->destination > 0);
						assert(blockIt->blockID > 0);
#endif

//						IDType blockJ = currNode->destination;
						IDType blockJ = blockIt->blockID;
						for (set<IDType>::iterator enqIt = blockIDMap[blockJ].enqSet.begin(); enqIt != blockIDMap[blockJ].enqSet.end(); enqIt++) {
#ifdef SANITYCHECK
							assert(*enqIt > 0);
#endif

							IDType enqJ = *enqIt;
							threadJ = enqToTaskEnqueued[enqJ].targetThread;
#ifdef SANITYCHECK
							if (threadJ < 0) {
								cout << "ERROR: Cannot find target thread of enq op " << enqJ << endl;
//								continue;
								return -1;
							}
#endif

							// Rule applies only if enqJ posts to the same thread as end op opI
							if (threadI != threadJ) continue;

							IDType nodeL = opIDMap[opL].nodeID;
							IDType nodeEnqJ = opIDMap[enqJ].nodeID;
							if (nodeL <= 0) {
								cout << "ERROR: Invalid node ID for op " << opL << "\n";
								return -1;
							} else if (nodeEnqJ <= 0) {
								cout << "ERROR: Invalid node ID for op " << enqJ << "\n";
								return -1;
							} else {
								int retValue = graph->opEdgeExists(nodeL, nodeEnqJ);
								if (retValue == 1) {
									std::string taskJ = enqToTaskEnqueued[enqJ].taskEnqueued;
#ifdef SANITYCHECK
									assert(taskJ.compare("") != 0);
#endif
									opJ = taskIDMap[taskJ].deqOpID;
									if (opJ <= 0) {
#ifdef GRAPHDEBUGFULL
										cout << "DEBUG: Cannot find deq of task " << taskJ << endl;
#ifdef EXTRARULES
										IDType firstBlockOfTask = taskIDMap[taskJ].firstBlockID;
										if (firstBlockOfTask <= 0) {
											cout << "DEBUG: Cannot find first block of task " << taskJ << "\n";
											cout << "DEBUG: Skipping FIFO-NESTED-2 edge from " << opI
												 << " to first op of task " << taskJ << "\n";
										} else {
											IDType firstOpOfTask = blockIDMap[firstBlockOfTask].firstOpInBlock;
											if (firstOpOfTask <= 0) {
												cout << "DEBUG: Cannot find first op of task " << taskJ << "\n";
												cout << "DEBUG: Skipping FIFO-NESTED-2 edge from " << opI
													 << " to first op of task " << taskJ << "\n";
											} else {
												opJ = firstOpOfTask;
												cout << "DEBUG: Adding FIFO-NESTED-2 edge from " << opI
													 << " to first op " << opJ << " of task " << taskJ << "\n";
											}
										}
#endif
#endif
									}
									if (opJ > 0) {
										IDType nodeI = opIDMap[opI].nodeID;
										IDType nodeJ = opIDMap[opJ].nodeID;
										if (nodeI <= 0) {
											cout << "ERROR: Invalid node ID for op " << opI << "\n";
											return -1;
										} else if (nodeJ <= 0) {
											cout << "ERROR: Invalid node ID for op " << opJ << "\n";
											return -1;
										} else {
											int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ);
											if (addEdgeRetValue == 1) {
												flag = true;
#ifdef GRAPHDEBUG
												cout << "FIFO-NESTED-2 edge (" << nodeI << ", " << nodeJ << ") -- #op-edges "   << graph->numOfOpEdges
													 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
#ifdef GRAPHDEBUGFULL
											} else if (addEdgeRetValue == 0) {
												cout << "DEBUG: Edge (" << nodeI << ", " << nodeJ << ") already implied in the graph\n";
#endif
											} else if (addEdgeRetValue == -1) {
												cout << "ERROR: While adding FIFO-NESTED-2 edge from " << nodeI << " to " << nodeJ << endl;
												return -1;
											}
										}
									} else {
#ifdef GRAPHDEBUGFULL
										cout << "DEBUG: Cannot find deq op (or first op) of task " << taskJ << "\n";
										cout << "DEBUG: Skipping FIFO-NESTED-2 edge from op " << opI << " to task " << taskJ << "\n";
#endif
									}
								} else if (retValue == -1) {
									cout << "ERROR: While checking edge from " << nodeL << " to " << nodeEnqJ << endl;
									return -1;
								}
							}
						}
					}
				} else if (blockL <= 0) {
					cout << "ERROR: Cannot find block of resume op " << opL << " of task " << it->first << "\n";
					return -1;
				} else if (threadI < 0) {
					cout << "ERROR: Cannot find thread of op " << opI << "\n";
					return -1;
				}
			} else {
#ifdef GRAPHDEBUGFULL
				cout << "DEBUG: Cannot find end op (or last op) of task " << it->first << "\n";
				cout << "DEBUG: Skipping FIFO-NESTED-2 edge for this task\n";
#endif
			}
		} else {
#ifdef GRAPHDEBUGFULL
			cout << "DEBUG: Cannot find last resume of task " << it->first << "\n";
			cout << "DEBUG: Skipping FIFO-NESTED-2 edge for this task\n";
#endif
		}

		// FIFO-NESTED-GEN / ENQRESET-ST-1
		for (vector<UAFDetector::pauseResumeResetTuple>::iterator prIt = taskIDMap[it->first].pauseResumeResetSequence.begin();
				prIt != taskIDMap[it->first].pauseResumeResetSequence.end(); prIt++) {

			IDType resumeOp = prIt->resumeOp;
			if (resumeOp == -1) continue;

			// FIFO-NESTED-GEN
			IDType blockOfResumeOp = opIDMap[resumeOp].blockID;
			threadI = opIDMap[resumeOp].threadID;

#ifdef SANITYCHECK
			if (blockOfResumeOp <= 0) {
				cout << "ERROR: Cannot find block of op " << resumeOp << endl;
				continue;
			}
			if (threadI < 0) {
				cout << "ERROR: Cannot find thread ID of op " << resumeOp << endl;
				continue;
			}
#endif

			if (blockOfResumeOp > 0 && threadI >= 0) {
//				for (HBGraph::adjListNode* currNode = graph->blockAdjList[blockOfResumeOp].head; currNode != NULL; currNode = currNode->next) {
				for (std::multiset<HBGraph::adjListNode>::iterator blockIt = graph->blockAdjList[blockOfResumeOp].begin();
						blockIt != graph->blockAdjList[blockOfResumeOp].end(); blockIt++) {
#ifdef SANITYCHECK
//					assert(currNode->destination > 0);
					assert(blockIt->blockID > 0);
#endif

//					IDType blockJ = currNode->destination;
					IDType blockJ = blockIt->blockID;
					for (set<IDType>::iterator enqIt = blockIDMap[blockJ].enqSet.begin(); enqIt != blockIDMap[blockJ].enqSet.end(); enqIt++) {
#ifdef SANITYCHECK
						assert(*enqIt > 0);
#endif
						IDType enqJ = *enqIt;
						threadJ = enqToTaskEnqueued[enqJ].targetThread;
#ifdef SANITYCHECK
						if (threadJ < 0) {
							cout << "ERROR: Cannot find target thread of enq op " << enqJ << endl;
//							continue;
							return -1;
						}
#endif

						// Rule applies only if enqJ posts to the same thread as resumeOp
						if (threadI != threadJ) continue;

						IDType nodeResumeOp = opIDMap[resumeOp].nodeID;
						IDType nodeEnqJ = opIDMap[enqJ].nodeID;
						if (nodeResumeOp <= 0) {
							cout << "ERROR: Invalid node ID for op " << resumeOp << "\n";
							return -1;
						} else if (nodeEnqJ <= 0) {
							cout << "ERROR: Invalid node ID for op " << enqJ << "\n";
							return -1;
						} else {
							int retValue = graph->opEdgeExists(nodeResumeOp, nodeEnqJ);
							if (retValue == 1) {

								if (prIt +1 == taskIDMap[it->first].pauseResumeResetSequence.end())
									continue;

								opI = (prIt+1)->pauseOp;
#ifdef GRAPHDEBUGFULL
								if (opI <= 0) {
									cout << "DEBUG: Cannot find pause op after resume op " << resumeOp << endl;
									cout << "DEBUG: Skipping FIFO-NESTED-GEN edge from resume op " << resumeOp << "\n";
									continue;
								}
#endif

								std::string taskEnqueued = enqToTaskEnqueued[enqJ].taskEnqueued;
#ifdef SANITYCHECK
								if (taskEnqueued.compare("") == 0) {
									cout << "ERROR: Cannot find task enqueued in enq op " << enqJ << endl;
									return -1;
								}
#endif
								opJ = taskIDMap[taskEnqueued].deqOpID;
								if (opJ <= 0) {
#ifdef GRAPHDEBUGFULL
									cout << "DEBUG: Cannot find deq op of task " << taskEnqueued << endl;
#ifdef EXTRARULES
									IDType firstBlockOfTask = taskIDMap[taskEnqueued].firstBlockID;
									if (firstBlockOfTask <= 0) {
										cout << "DEBUG: Cannot find first block of task " << taskEnqueued << "\n";
										cout << "DEBUG: Skipping FIFO-NESTED-GEN from op " << opI << " to task "
											 << taskEnqueued << "\n";
									} else {
										IDType firstOpOfTask = blockIDMap[firstBlockOfTask].firstOpInBlock;
										if (firstOpOfTask <= 0) {
											cout << "DEBUG: Cannot find first op of block " << firstBlockOfTask
												 << " of task " << taskEnqueued << "\n";
											cout << "DEBUG: Skipping FIFO-NESTED-GEN from op " << opI << " to task "
												 << taskEnqueued << "\n";
										} else {
											opJ = firstOpOfTask;
											cout << "DEBUG: Adding FIFO-NESTED-GEN edge from " << opI << " to "
												 << opJ << "\n";
										}
									}
#endif
#endif
								}
								if (opJ > 0) {
									IDType nodeI = opIDMap[opI].nodeID;
									IDType nodeJ = opIDMap[opJ].nodeID;
									if (nodeI <= 0) {
										cout << "ERROR: Invalid node ID for op " << opI << "\n";
										return -1;
									} else if (nodeJ <= 0) {
										cout << "ERROR: Invalid node ID for op " << opJ << "\n";
										return -1;
									} else {
										int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ);
										if (addEdgeRetValue == 1) {
											flag = true;
#ifdef GRAPHDEBUG
											cout << "FIFO-NESTED-GEN edge (" << nodeI << ", " << nodeJ << ") -- #op-edges "   << graph->numOfOpEdges
												 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
#ifdef GRAPHDEBUGFULL
										} else if (addEdgeRetValue == 0) {
											cout << "DEBUG: Edge (" << nodeI << ", " << nodeJ << ") already implied in the graph\n";
#endif
										} else if (addEdgeRetValue == -1) {
											cout << "ERROR: While adding FIFO-NESTED-GEN edge from " << nodeI << " to " << nodeJ << endl;
											return -1;
										}
									}
								} else {
#ifdef GRAPHDEBUGFULL
									cout << "DEBUG: Cannot find deq op (or first op) of task " << taskEnqueued << "\n";
									cout << "DEBUG: Skipping FIFO-NESTED-GEN edge from op " << opI << "\n";
#endif
								}
							} else if (retValue == -1) {
								cout << "ERROR: While checking edge from " << resumeOp << " to " << enqJ << endl;
								return -1;
							}
						}
					}
				}
			} else if (blockOfResumeOp <= 0) {
				cout << "ERROR: Cannot find block of resume op " << resumeOp << "\n";
				return -1;
			} else if (threadI < 0) {
				cout << "ERROR: Cannot find thread of resume op " << resumeOp << "\n";
				return -1;
			}

			// ENQRESET-ST-1
			IDType enqK = it->second.enqOpID;
			IDType resetOp = prIt->resetOp;

			if (enqK > 0 && resetOp > 0) {
				IDType blockK = opIDMap[enqK].blockID;

				std::string taskOfResetOp = opIDMap[resetOp].taskID;
				if (taskOfResetOp.compare("") == 0) {
#ifdef GRAPHDEBUGFULL
					cout << "DEBUG: Cannot find task of op " << resetOp << endl;
					cout << "DEBUG: Skipping ENQRESET-ST-1 edge for resume op " << resumeOp << "\n";
#endif
					continue;
				}

				IDType enqN = taskIDMap[taskOfResetOp].enqOpID;
				if (enqN <= 0) {
#ifdef GRAPHDEBUGFULL
					cout << "DEBUG: Cannot find enq of reset task " << taskOfResetOp << endl;
					cout << "DEBUG: Skipping ENQRESET-ST-1 edge for resume op " << resumeOp << "\n";
#endif
					continue;
				}

				if (enqK == enqN) {
#ifdef GRAPHDEBUGFULL
					cout << "DEBUG: Same task found to reset and resume a nesting loop\n";
					cout << "DEBUG: Skipping ENQRESET-ST-1 edge for resume op " << resumeOp << "\n";
#endif
//					return -1;
					continue;
				}

				IDType threadK = enqToTaskEnqueued[enqK].targetThread;
				IDType threadN = enqToTaskEnqueued[enqN].targetThread;

				if (threadK < 0) {
					cout << "ERROR: Cannot find target thread of enq op " << enqK << "\n";
					return -1;
				}
				if (threadN < 0) {
					cout << "ERROR: Cannot find target thread of enq op " << enqN << "\n";
					return -1;
				}

				// Rule applies only if both enqs post to the same thread
				if (threadK != threadN)
					continue;

//				for (HBGraph::adjListNode *currNode = graph->blockAdjList[blockK].head; currNode != NULL; currNode = currNode->next) {
				for (std::multiset<HBGraph::adjListNode>::iterator blockKIt = graph->blockAdjList[blockK].begin();
						blockKIt != graph->blockAdjList[blockK].end(); blockKIt++) {
#ifdef SANITYCHECK
//					assert(currNode->destination > 0);
					assert(blockKIt->blockID > 0);
#endif

//					IDType blockJ = currNode->destination;
					IDType blockJ = blockKIt->blockID;
					for (set<IDType>::iterator enqIt = blockIDMap[blockJ].enqSet.begin();
							enqIt != blockIDMap[blockJ].enqSet.end(); enqIt++) {
#ifdef SANITYCHECK
						assert(*enqIt > 0);
#endif

						IDType enqL = *enqIt;
						// We need to check if enqK < enqL < enqN.
						// If any of these are the same op, we do not proceed
						if (enqK == enqL || enqL == enqN)
							continue;

						IDType threadL = enqToTaskEnqueued[enqL].targetThread;
						if (threadL < 0) {
							cout << "ERROR: Cannot find target thread of enq op " << enqL << "\n";
							return -1;
						}

						if (threadK != threadL)
							continue;

						IDType nodeEnqK = opIDMap[enqK].nodeID;
						IDType nodeEnqL = opIDMap[enqL].nodeID;
						IDType nodeEnqN = opIDMap[enqN].nodeID;
						if (nodeEnqK <= 0) {
							cout << "ERROR: Invalid node ID for op " << enqK << "\n";
							return -1;
						} else if (nodeEnqL <= 0) {
							cout << "ERROR: Invalid node ID for op " << enqL << "\n";
							return -1;
						} else if (nodeEnqN <= 0) {
							cout << "ERROR: Invalid node ID for op " << enqN << "\n";
							return -1;
						} else {
							int retValue1 = graph->opEdgeExists(nodeEnqK, nodeEnqL);
							if (retValue1 == 1) {
								int retValue2 = graph->opEdgeExists(nodeEnqL, nodeEnqN);
								if (retValue2 == 1) {
									std::string taskEnqueuedInL = enqToTaskEnqueued[enqL].taskEnqueued;
#ifdef SANITYCHECK
									assert(taskEnqueuedInL.compare("") != 0);
#endif
									opI = taskIDMap[taskEnqueuedInL].endOpID;

									if (opI <= 0) {
#ifdef GRAPHDEBUGFULL
										cout << "DEBUG: Cannot find end op of task " << taskEnqueuedInL << "\n";
#ifdef EXTRARULES
										IDType lastBlockOfTask = taskIDMap[taskEnqueuedInL].lastBlockID;
										if (lastBlockOfTask <= 0) {
											cout << "DEBUG: Cannot find last block of task " << taskEnqueuedInL << "\n";
											cout << "DEBUG: Skipping ENQRESET-ST-1 edge for this task\n";
										} else {
											IDType lastOpOfTask = blockIDMap[lastBlockOfTask].lastOpInBlock;
											if (lastOpOfTask <= 0) {
												cout << "DEBUG: Cannot find last op of task " << taskEnqueuedInL << "\n";
												cout << "DEBUG: Skipping ENQRESET-ST-1 edge for this task\n";
											} else {
												opI = lastOpOfTask;
												cout << "DEBUG: Adding ENQRESET-ST-1 edge from " << opI << " to " << opJ << "\n";
											}
										}
#endif
#endif
									}

									if (opI > 0) {
										opJ = resumeOp;

										IDType nodeI = opIDMap[opI].nodeID;
										IDType nodeJ = opIDMap[opJ].nodeID;
										if (nodeI <= 0) {
											cout << "ERROR: Invalid node ID for op " << opI << "\n";
											return -1;
										} else if (nodeJ <= 0) {
											cout << "ERROR: Invalid node ID for op " << opJ << "\n";
											return -1;
										} else {
											int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ);
											if (addEdgeRetValue == 1) {
												flag = true;
#ifdef GRAPHDEBUG
												cout << "ENQRESET-ST-1 edge (" << nodeI << ", " << nodeJ << ") -- #op-edges "   << graph->numOfOpEdges
													 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
#ifdef GRAPHDEBUGFULL
											} else if (addEdgeRetValue == 0) {
												cout << "DEBUG: Edge (" << nodeI << ", " << nodeJ << ") already implied in the graph\n";
#endif
											} else if (addEdgeRetValue == -1) {
												cout << "ERROR: While adding ENQRESET-ST-1 edge from " << nodeI << " to " << nodeJ << endl;
												return -1;
											}
										}
									} else {
#ifdef GRAPHDEBUGFULL
										cout << "DEBUG: Cannot find end op (or last op) of task " << taskEnqueuedInL << "\n";
										cout << "DEBUG: Skipping ENQRESET-ST-1 edge from end of task " << taskEnqueuedInL << "\n";
#endif
									}
								} else if (retValue2 == -1) {
									cout << "ERROR: While checking edge from " << nodeEnqL << " to " << nodeEnqN << endl;
									return -1;
								}
							} else if (retValue1 == -1) {
								cout << "ERROR: While checking edge from " << nodeEnqK << " to " << nodeEnqL << endl;
								return -1;
							}
						}
					}
				}
			} else if (enqK <= 0) {
#ifdef GRAPHDEBUGFULL
				cout << "DEBUG: Cannot find enq op of task " << it->first << "\n";
				cout << "DEBUG: Skipping ENQRESET-ST-1 edge for this task\n";
#endif
			} else if (resetOp <= 0) {
#ifdef GRAPHDEBUGFULL
				cout << "DEBUG: Cannot find reset op for loop with pause " << prIt->pauseOp << " and resume " << prIt->resumeOp << "\n";
				cout << "DEBUG: Skipping ENQRESET-ST-1 edge\n";
#endif
			}
		}
	}

	if (flag)
		return 1;
	else
		return 0;
}

int UAFDetector::add_EnqReset_ST_2_3_Edges() {
	bool flag = false;

	IDType opI, opJ;

	for (map<std::string, UAFDetector::nestingLoopDetails>::iterator it = nestingLoopMap.begin();
			it != nestingLoopMap.end(); it++) {
		for (vector<UAFDetector::pauseResumeResetTuple>::iterator prrIt = it->second.pauseResumeResetSet.begin();
				prrIt != it->second.pauseResumeResetSet.end(); prrIt++) {

			IDType opK = prrIt->resetOp;
			if (opK <= 0) {
#ifdef GRAPHDEBUGFULL
				cout << "DEBUG: Cannot find reset op of nesting loop of shared variable " << it->first
					 << " with pause " << prrIt->pauseOp << " and resume " << prrIt->resumeOp << "\n";
				cout << "DEBUG: Skipping ENQRESET-ST-2/3 edges for this loop\n";
#endif
				continue;
			}

			IDType opM = prrIt->resumeOp;

			if (opM <= 0) {
#ifdef GRAPHDEBUGFULL
				cout << "DEBUG: Cannot find resume op of nesting loop of shared variable " << it->first
					 << " with pause " << prrIt->pauseOp << " and reset " << prrIt->resetOp << "\n";
				cout << "DEBUG: Skipping ENQRESET-ST-2/3 edges for this loop\n";
#endif
				continue;
			}

			IDType threadK = opIDMap[opK].threadID;
			IDType threadM = opIDMap[opM].threadID;

			if (threadK < 0) {
				cout << "ERROR: Cannot find thread of reset op " << opK << "\n";
				return -1;
			}
			if (threadM < 0) {
				cout << "ERROR: Cannot find thread of resume op " << opM << "\n";
				return -1;
			}

			// reset (opK) and resume(opM) needs to be in the same thread
			if (threadK != threadM)
				continue;

			std::string taskK = opIDMap[opK].taskID;
			std::string taskM = opIDMap[opM].taskID;
#ifdef SANITYCHECK
			assert(taskK.compare("") != 0);
			assert(taskM.compare("") != 0);
#endif
			if (taskK.compare("") == 0) {
#ifdef GRAPHDEBUGFULL
				cout << "DEBUG: Reset op " << opK << " is not contained in a task\n";
				cout << "DEBUG: Skipping ENQRESET-ST-2/3 edges concerning this reset op\n";
#endif
				continue;
			}

			if (taskM.compare("") == 0) {
				cout << "ERROR: Cannot find task of resume op " << opM << "\n";
				return -1;
			}

			// Task containing reset needs to be atomic
			if (taskIDMap[taskK].atomic == false) continue;

			IDType lastResumeOftaskM = taskIDMap[taskM].lastResumeOpID;
			if (lastResumeOftaskM <= 0) {
				cout << "ERROR: Cannot find last resume of task " << taskM << " but we saw a resume op " << opM << "\n";
				return -1;
			}
			// opM needs to be the last resume of taskM
			if (opM != lastResumeOftaskM)
				continue;

			// taskM needs to be the parent of taskK
			if (taskM.compare(taskIDMap[taskK].parentTask) != 0)
				continue;

			IDType blockK = opIDMap[opK].blockID;
#ifdef SANITYCHECK
			if (blockK <= 0) {
				cout << "ERROR: Cannot find block of op " << opK << "\n";
				return -1;
			}
#endif

//			for (HBGraph::adjListNode *currNode = graph->blockAdjList[blockK].head;
//					currNode != NULL; currNode = currNode->next) {
			for (std::multiset<HBGraph::adjListNode>::iterator blockKIt = graph->blockAdjList[blockK].begin();
					blockKIt != graph->blockAdjList[blockK].end(); blockKIt++) {
#ifdef SANITYCHECK
//				assert(currNode->destination > 0);
				assert(blockKIt->blockID > 0);
#endif

//				IDType blockJ = currNode->destination;
				IDType blockJ = blockKIt->blockID;
				for (set<IDType>::iterator enqIt = blockIDMap[blockJ].enqSet.begin(); enqIt != blockIDMap[blockJ].enqSet.end();
						enqIt++) {
#ifdef SANITYCHECK
					assert(*enqIt > 0);
#endif
					IDType opL = *enqIt;
					IDType threadL = enqToTaskEnqueued[opL].targetThread;
//					IDType threadK = enqToTaskEnqueued[opK].targetThread;
					IDType threadK = opIDMap[opK].threadID;

					if (threadL < 0) {
						cout << "ERROR: Cannot find thread of op " << opL << "\n";
						return -1;
					}
					if (threadK < 0) {
						cout << "ERROR: Cannot find thread of op " << opK << "\n";
						return -1;
					}
					// opL needs to post to the same thread as reset (opK)
					if (threadL != threadK)
						continue;

#ifdef GRAPHDEBUGFULL
					cout << "DEBUG: checking op edge (" << opK << ", " << opL << ")\n";
#endif
					IDType nodeK = opIDMap[opK].nodeID;
					IDType nodeL = opIDMap[opL].nodeID;
					if (nodeK <= 0) {
						cout << "ERROR: Invalid node ID for op " << opK << "\n";
						return -1;
					} else if (nodeL <= 0) {
						cout << "ERROR: Invalid node ID for op " << opL << "\n";
						return -1;
					} else {
						int retValue = graph->opEdgeExists(nodeK, nodeL);
						if (retValue == 1) {
							std::string taskL = enqToTaskEnqueued[opL].taskEnqueued;
#ifdef SANITYCHECK
							if (taskL.compare("") == 0) {
								cout << "ERROR: Cannot find task enqueued in enq op " << opL << "\n";
								return -1;
							}
#endif
							opJ = taskIDMap[taskL].deqOpID;

							if (opJ <= 0) {
#ifdef GRAPHDEBUGFULL
								cout << "DEBUG: Cannot find deq of task " << taskL << "\n";
#ifdef EXTRARULES
								IDType firstBlockOfTask = taskIDMap[taskL].firstBlockID;
								if (firstBlockOfTask <= 0) {
									cout << "DEBUG: Cannot find first block of task " << taskL <<"\n";
									cout << "DEBUG: Skipping ENQRESET-ST-2 edge to deq of task " << taskL << "\n";
								} else {
									IDType firstOpOfTask = blockIDMap[firstBlockOfTask].firstOpInBlock;
									if (firstOpOfTask <= 0) {
										cout << "DEBUG: Cannot find first op of block " << firstBlockOfTask
											 << " of task " << taskL << "\n";
										cout << "DEBUG: Skipping ENQRESET-ST-2 edge to deq of task " << taskL << "\n";
									} else {
										opJ = firstOpOfTask;
										cout << "DEBUG: Adding ENQRESET-ST-2 edge to first op " << opJ
											 << " of task " << taskL << "\n";
									}
								}
#endif
#endif
							}

							// ENQRESET-ST-2
							opI = taskIDMap[taskM].endOpID;

							if (opI <= 0) {
#ifdef GRAPHDEBUGFULL
								cout << "DEBUG: Cannot find end of task " << taskM << "\n";
#ifdef EXTRARULES
								IDType lastBlockOfTask = taskIDMap[taskM].lastBlockID;
								if (lastBlockOfTask <= 0) {
									cout << "DEBUG: Cannot find last block of task " << taskM << "\n";
									cout << "DEBUG: Skipping ENQRESET-ST-2 edge from end of task " << taskM << "\n";
								} else {
									IDType lastOpOfTask = blockIDMap[lastBlockOfTask].lastOpInBlock;
									if (lastOpOfTask <= 0) {
										cout << "DEBUG: Cannot find last op of task " << taskM << "\n";
										cout << "DEBUG: Skipping ENQRESET-ST-2 edge from end of task " << taskM << "\n";
									} else {
										opI = lastOpOfTask;
										cout << "DEBUG: Adding ENQRESET-ST-2 edge from last op " << opI << " of task " << taskM << "\n";
									}
								}
#endif
#endif
							}

							if (opI > 0 && opJ > 0) {
#ifdef GRAPHDEBUGFULL
								cout << "DEBUG: checking op edge (" << opI << ", " << opJ << ")\n";
#endif
								IDType nodeI = opIDMap[opI].nodeID;
								IDType nodeJ = opIDMap[opJ].nodeID;
								if (nodeI <= 0) {
									cout << "ERROR: Invalid node ID for op " << opI << "\n";
									return -1;
								} else if (nodeJ <= 0) {
									cout << "ERROR: Invalid node ID for op " << opJ << "\n";
									return -1;
								} else {
									int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ);
									if (addEdgeRetValue == 1) {
										flag = true;
#ifdef GRAPHDEBUG
										cout << "ENQRESET-ST-2 edge (" << nodeI << ", " << nodeJ << ") -- #op-edges "   << graph->numOfOpEdges
											 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
#ifdef GRAPHDEBUGFULL
									} else if (addEdgeRetValue == 0) {
										cout << "DEBUG: Edge (" << nodeI << ", " << nodeJ << ") already implied in the graph\n";
#endif
									} else if (addEdgeRetValue == -1) {
										cout << "ERROR: While adding ENQRESET-ST-2 edge from " << nodeI << " to " << nodeJ << endl;
										return -1;
									}
								}
							} else if (opI <= 0) {
#ifdef GRAPHDEBUGFULL
								cout << "DEBUG: Cannot find end of task " << taskM << "\n";
								cout << "DEBUG: Skipping ENQRESET-ST-2 edge from end of task\n";
#endif
							} else if (opJ <= 0) {
#ifdef GRAPHDEBUGFULL
								cout << "DEBUG: Cannot find deq of task " << taskL << "\n";
								cout << "DEBUG: Skipping ENQRESET-ST-2 edge from deq of task\n";
#endif
							}

							// ENQRESET-ST-3
							std::string sharedVariable = it->first;
							bool foundFlag = false;
							for (vector<UAFDetector::pauseResumeResetTuple>::iterator prIt = taskIDMap[taskM].pauseResumeResetSequence.begin();
								prIt != taskIDMap[taskM].pauseResumeResetSequence.end(); prIt++) {
								if (prIt->resumeOp == opM) {
									if (prIt+1 == taskIDMap[taskM].pauseResumeResetSequence.end())
										continue;
									IDType nextPause = (prIt+1)->pauseOp;

									if (nextPause <= 0) {
#ifdef GRAPHDEBUGFULL
										cout << "DEBUG: Cannot find next pause after the nesting loop of shared variable " << sharedVariable
											 << " with pause " << prIt->pauseOp << ", resume " << prIt->resumeOp << ", reset " << prIt->resetOp
											 << "\n";
#endif
//										continue;
										break;
									}

									opI = nextPause;
									foundFlag = true;
									break;
								}
							}

							if (foundFlag && opJ > 0) {
#ifdef GRAPHDEBUGFULL
								cout << "DEBUG: checking op edge (" << opI << ", " << opJ << ")\n";
#endif
								IDType nodeI = opIDMap[opI].nodeID;
								IDType nodeJ = opIDMap[opJ].nodeID;
								if (nodeI <= 0) {
									cout << "ERROR: Invalid node ID for op " << opI << "\n";
									return -1;
								} else if (nodeJ <= 0) {
									cout << "ERROR: Invalid node ID for op " << opJ << "\n";
									return -1;
								} else {
									int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ);
									if (addEdgeRetValue == 1) {
										flag = true;
	#ifdef GRAPHDEBUG
										cout << "ENQRESET-ST-3 edge (" << nodeI << ", " << nodeJ << ") -- #op-edges "   << graph->numOfOpEdges
											 << " -- #block-edges " << graph->numOfBlockEdges << endl;
	#endif
#ifdef GRAPHDEBUGFULL
									} else if (addEdgeRetValue == 0) {
										cout << "DEBUG: Edge (" << nodeI << ", " << nodeJ << ") already implied in the graph\n";
#endif
									} else if (addEdgeRetValue == -1) {
										cout << "ERROR: While adding ENQRESET-ST-3 edge from " << nodeI << " to " << nodeJ << endl;
										return -1;
									}
								}
							} else if (opJ <= 0) {
#ifdef GRAPHDEBUGFULL
								cout << "DEBUG: Cannot find deq op (or first op) of task " << taskL << "\n";
								cout << "DEBUG: Skipping ENQRESET-ST-3 edge from op " << opI << " to this task\n";
#endif
							}
						} else if (retValue == -1) {
							cout << "ERROR: While checking edge from " << nodeK << " to " << nodeL << endl;
							return -1;
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

int UAFDetector::addTransSTOrMTEdges() {
	bool flag = false;

	// Adding TRANS-ST/MT Edges

	for (map<IDType, UAFDetector::blockDetails>::iterator it = blockIDMap.begin(); it != blockIDMap.end(); it++) {
		IDType blockI = it->first;

#ifdef SANITYCHECK
		assert(blockI > 0);
#endif

//		for (HBGraph::adjListNode* tempNode1 = graph->blockAdjList[blockI].head; tempNode1 != NULL; tempNode1 = tempNode1->next) {
		for (std::multiset<HBGraph::adjListNode>::iterator blockIIt = graph->blockAdjList[blockI].begin();
				blockIIt != graph->blockAdjList[blockI].end(); blockIIt++) {
//			IDType blockK = tempNode1->destination;
			IDType blockK = blockIIt->blockID;

#ifdef SANITYCHECK
			if (blockK <= 0) {
				cout << "ERROR: Invalid block edge from " << blockI << endl;
				return -1;
			}
#endif

			if (blockI == blockK) {
#ifdef GRAPHDEBUGFULL
				cout << "DEBUG: TRANS-edge: blockI: " << blockI << " blockK: " << blockK << "\n";
				cout << "DEBUG: TRANS-edge: Inferring edge to same block\n";
#endif
				continue;
			}

			for (std::multiset<HBGraph::adjListNode>::iterator blockKIt = graph->blockAdjList[blockK].begin();
					blockKIt != graph->blockAdjList[blockK].end(); blockKIt++) {
				IDType blockJ = blockKIt->blockID;

#ifdef SANITYCHECK
				if (blockJ <= 0) {
					cout << "ERROR: Invalid block edge from " << blockK << endl;
					return -1;
				}
#endif

				// If blockK == blockJ, we are trying to infer the same edges from blockI to blockK/blockJ
				// Redundant
				// Similarly, other cases
				if (blockI == blockK || blockK == blockJ || blockI == blockJ) {
#ifdef GRAPHDEBUGFULL
					cout << "DEBUG: TRANS-edge: blockI: " << blockI << " blockK: " << blockK
						 << " blockJ: " << blockJ << "\n";
					cout << "DEBUG: TRANS-edge: Inferring edge to same block\n";
#endif
					continue;
				}

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
				if (!(((threadI == threadK) && (threadK == threadJ)) || (threadI != threadJ))) {
#ifdef GRAPHDEBUGFULL
					cout << "DEBUG: TRANS-edge: threadI: " << threadI << " threadK: " << threadK
						 << " threadJ: " << threadJ << "\n";
					cout << "DEBUG: TRANS-edge: Violates thread criterion\n";
#endif
					continue;
				}

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

				IDType nodeTempOp1 = opIDMap[tempOp1].nodeID;
				IDType nodeTempOp2 = opIDMap[tempOp2].nodeID;
				if (nodeTempOp1 <= 0) {
					cout << "ERROR: Invalid node ID for op " << tempOp1 << "\n";
					return -1;
				}
				if (nodeTempOp2 <= 0) {
					cout << "ERROR: Invalid node ID for op " << tempOp2 << "\n";
					return -1;
				}
				int retValue = graph->opEdgeExists(nodeTempOp1, nodeTempOp2, blockI, blockJ);
				if (retValue == 1) {
#ifdef GRAPHDEBUGFULL
					cout << "DEBUG: TRANS-edge: Edge already exists from blockI: " << blockI
						 << " (node " << nodeTempOp1 << ") to blockJ: " << blockJ << " (node "
						 << nodeTempOp2 << ")\n";
#endif
					continue;
				}
				else if (retValue == -1) {
					cout << "ERROR: While checking node edge from " << nodeTempOp1 << " to " << nodeTempOp2 << endl;
					return -1;
				}

				IDType opI, opJ, firstOp;
				int addEdgeRetValue;
				opI = blockIDMap[blockI].lastOpInBlock;
				firstOp = blockIDMap[blockI].firstOpInBlock;
#ifdef SANITYCHECK
				if (opI <= 0) {
					cout << "ERROR: Cannot find last op of block " << blockI << endl;
//					continue;
					return -1;
				}
				if (firstOp <= 0) {
					cout << "ERROR: Cannot find first op of block " << blockI << endl;
//					continue;
					return -1;
				}
#endif
				IDType nodeI = 0;
				IDType prevNodeI = 0;

				while (opI > 0 && opI >= firstOp) {
					// Find the earliest op in blockK such that there exists edge (opI, op)
					IDType minOpInK = -1, minNodeInK = -1;
					opJ = -1;

					nodeI = opIDMap[opI].nodeID;
					if (nodeI <= 0) {
						cout << "ERROR: Invalid nodeID for op " << opI << "\n";
						return -1;
					}

					if (prevNodeI != 0) {
						if (prevNodeI == nodeI) {
							opI = opIDMap[opI].prevOpInBlock;
							continue;
						}
					}
					prevNodeI = nodeI;

					// Loop through adjacency list of opI
					HBGraph::adjListNode destNode(blockK);
					std::pair<std::multiset<HBGraph::adjListNode>::iterator, std::multiset<HBGraph::adjListNode>::iterator> ret =
							graph->opAdjList[nodeI].equal_range(destNode);

					if (ret.first != ret.second) {
						IDType count = 0;
						for (std::multiset<HBGraph::adjListNode>::iterator retIt = ret.first;
								retIt != ret.second; retIt++) {
							count++;
							IDType opDest = *(nodeIDMap[retIt->nodeID].opSet.begin());
#ifdef EXTRADEBUGINFO
							cout << "DEBUG: opDest = " << opDest << "\n";
#endif
							if (minOpInK == -1 || minOpInK > opDest) {
								minOpInK = opDest;
								minNodeInK = retIt->nodeID;
							}
						}

						if (count > 1) {
							graph->removeOpEdgesToBlock(ret.first, ret.second, nodeI, blockK);

							if (minOpInK > 0) {
								if (minNodeInK <= 0) {
									cout << "ERROR: Invalid node ID for op " << minOpInK << "\n";
									return -1;
								}
								// Restore the edge from opI to the earliest op in blockK
								addEdgeRetValue = graph->addOpEdge(nodeI, minNodeInK);
								if (addEdgeRetValue == 1) {
#ifdef GRAPHDEBUGFULL
									cout << "Restored edge from " << nodeI
										 << " to " << minNodeInK << "\n";
								} else if (addEdgeRetValue == 0) {
									cout << "Did not add restoration edge from "
										 << nodeI << " to " << minNodeInK << "\n";
#endif
								} else {
									cout << "ERROR: While adding restoration edge from "
										 << nodeI << " to " << minNodeInK << "\n";
									return -1;
								}
							}
#ifdef PRINTGRAPH
							graph->printGraph();
#endif
						} else if (count < 1) {
							cout << "ERROR: While finding edges from op " << opI
								 << " to ops in block " << blockK << "\n";
							cout << "ERROR: equal_range() gives a non-empty range, "
								 << " distance() gives count < 1\n";
							return -1;
						}
					} else {
						minOpInK = -1;
					}

					if (minOpInK <= 0) {
#ifdef GRAPHDEBUGFULL
						cout << "DEBUG: Did not find any edge from op " << opI
							 << " (node " << nodeI << ")"
							 << " to any ops in block " << blockK << "\n";
#endif
						opI = opIDMap[opI].prevOpInBlock;
						continue;
					}

					IDType nodeTempOp = 0;
					IDType prevNodeTempOp = 0;

					for (IDType tempOp = minOpInK;
							tempOp > 0 && tempOp <= blockIDMap[blockK].lastOpInBlock;
							tempOp = opIDMap[tempOp].nextOpInBlock) {
						// Find the earliest op in blockJ such that there exists edge (tempOp, op)
						IDType minOpInJ = -1, minNodeInJ = -1;

						nodeTempOp = opIDMap[tempOp].nodeID;
						if (nodeTempOp <= 0) {
							cout << "ERROR: Invalid node ID for op " << tempOp << "\n";
							return -1;
						}

						// The loop steps through the nextOp map of the block. Now that we have ops merged into nodes,
						// the loop should really be stepping through nextNode map. To take care of that, I record the
						// previous node analyzed.
						if (prevNodeTempOp != 0) {
							if (prevNodeTempOp == nodeTempOp)
								continue;
						}
						prevNodeTempOp = nodeTempOp;

						// Loop through adjacency list of tempOp
						HBGraph::adjListNode destNode(blockJ);
						std::pair<std::multiset<HBGraph::adjListNode>::iterator, std::multiset<HBGraph::adjListNode>::iterator> ret =
								graph->opAdjList[nodeTempOp].equal_range(destNode);
						if (ret.first != ret.second) {
							IDType count = 0;
							for (std::multiset<HBGraph::adjListNode>::iterator retIt = ret.first;
									retIt != ret.second; retIt++) {
								count++;
								IDType opDest = *(nodeIDMap[retIt->nodeID].opSet.begin());
#ifdef EXTRADEBUGINFO
								cout << "DEBUG: opDest = " << opDest << "\n";
#endif
								if (minOpInJ == -1 || minOpInJ > opDest) {
									minOpInJ = opDest;
									minNodeInJ = retIt->nodeID;
								}
							}

							if (count > 1) {
								graph->removeOpEdgesToBlock(ret.first, ret.second, nodeTempOp, blockJ);
#ifdef PRINTGRAPH
								graph->printGraph();
#endif

								if (minOpInJ > 0) {
									if (minNodeInJ <= 0) {
										cout << "ERROR: Invalid node ID for op " << minOpInJ << "\n";
										return -1;
									}
									// Restore the edge from tempOp to the earliest op in blockJ
									addEdgeRetValue = graph->addOpEdge(nodeTempOp, minNodeInJ);
									if (addEdgeRetValue == 1) {
#ifdef GRAPHDEBUGFULL
										cout << "Restored edge from " << nodeTempOp << " to " << minNodeInJ << endl;
									} else if (addEdgeRetValue == 0) {
										cout << "Did not add restoration edge from " << nodeTempOp << " to " << minNodeInJ << endl;
#endif
									} else {
										cout << "ERROR: While adding restoration edge from " << nodeTempOp << " to " << minNodeInJ << endl;
										return -1;
									}
								}
							} else if (count < 1) {
								cout << "ERROR: While finding edges from op " << tempOp
									 << " to ops in block " << blockJ << "\n";
								cout << "ERROR: equal_range() gives a non-empty range, "
									 << " distance() gives count < 1\n";
								return -1;
							}
						} else
							minOpInJ = -1;

						if (minOpInJ > 0) {
							if (opJ == -1 || opJ > minOpInJ)
								opJ = minOpInJ;
						}
					}

					if (opJ > 0) {
						IDType nodeJ = opIDMap[opJ].nodeID;
						if (nodeJ <= 0) {
							cout << "ERROR: Invalid node ID for op " << opJ << "\n";
							return -1;
						}
						addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ);
						if (addEdgeRetValue == 1) {
							flag = true;
#ifdef GRAPHDEBUG
							cout << "TRANS-ST/MT Edge (" << nodeI << ", " << nodeJ << ") -- #opEdges " << graph->numOfOpEdges
								 << " -- #blockEdges " << graph->numOfBlockEdges << endl;
#endif
							if (opI == blockIDMap[blockI].lastOpInBlock && opJ == blockIDMap[blockJ].firstOpInBlock)
								break;
#ifdef GRAPHDEBUGFULL
						} else if (addEdgeRetValue == 0) {
							cout << "DEBUG: Edge (" << nodeI << ", " << nodeJ << ") already implied in the graph\n";
#endif
						} else if (addEdgeRetValue == -1) {
							cout << "ERROR: While adding TRANS-ST/MT edge " << nodeI << " to " << nodeJ << endl;
							return -1;
						}
					}

					opI = opIDMap[opI].prevOpInBlock;
				}
			}
		}
	}

	if (flag)
		return 1;
	else
		return 0;
}

IDType  UAFDetector::findUAF() {

	bool flag = false;

	// Loop through freeIDMap, for each free, find use that no HB edge

	IDType falsePositives = 0;

#ifdef UNIQUERACE
	bool raceForFree; // Flag to track if we saw a race for a given free
	bool falsePositiveForFree; // Flag to track if we saw a false positive for a given free
#endif
	for (map<IDType, freeOpDetails>::iterator freeIt = freeIDMap.begin(); freeIt != freeIDMap.end(); freeIt++) {
		raceType = UNKNOWN;
#ifdef UNIQUERACE
		raceForFree = false;
		falsePositiveForFree = false;
#endif
		IDType freeID = freeIt->first;
		IDType allocID = freeIt->second.allocOpID;
		IDType nodeAlloc = -1;

		IDType nodeFree = opIDMap[freeID].nodeID;
		if (nodeFree <= 0) {
			cout << "ERROR: Invalid node ID for op " << freeID << "\n";
			return -1;
		}

		if (allocID == -1) {
#ifdef GRAPHDEBUGFULL
			cout << "DEBUG: Cannot find alloc for free op " << freeID << endl;
#endif
		} else {
			nodeAlloc = opIDMap[allocID].nodeID;
			if (nodeAlloc <= 0) {
				cout << "ERROR: Invalid node ID for op " << allocID << "\n";
				return -1;
			}
		}

		for (set<IDType>::iterator readIt = freeIt->second.readOps.begin(); readIt != freeIt->second.readOps.end(); readIt++) {
			raceType = UNKNOWN;
			IDType readID = *readIt;

			IDType nodeRead = opIDMap[readID].nodeID;
			if (nodeRead <= 0) {
				cout << "ERROR: Invalid node ID for op " << readID << "\n";
				return -1;
			}
			if (nodeFree == nodeRead && freeID < readID) {
				cout << "Definite UAF between read op " << readID << " (read at address " << readSet[readID].startingAddress
					 << " in task " << opIDMap[readID].taskID << " in thread " << opIDMap[readID].threadID << ") "
					 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
					 << " in task " << opIDMap[freeID].taskID << " in thread " << opIDMap[freeID].threadID << ")\n";
				if (allocID > 0) {
					cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
						 << allocSet[allocID].startingAddress << " in task " << opIDMap[allocID].taskID
						 << " in thread " << opIDMap[allocID].threadID << ")\n";
				}

				// If free and read are in the same node, they are in the same thread
				raceType = SINGLETHREADED;

				log(readID, freeID, "read", "free", true);

				flag = true;
#ifdef UNIQUERACE
				raceForFree = true;
				break;
#else
				continue;
#endif
			} else if (nodeFree == nodeRead) {
#ifdef RACEDEBUG
				cout << "DEBUG: Free op " << freeID << " and read op " << readID << " in the same node, but read before free\n";
				cout << "DEBUG: Skipping this read while detecting races\n";
#endif
				continue;
			}
			if (graph->opEdgeExists(nodeFree, nodeRead) == 1) {
				cout << "Definite UAF between read op " << readID << " (read at address " << readSet[readID].startingAddress
					 << " in task " << opIDMap[readID].taskID << " in thread " << opIDMap[readID].threadID << ") "
					 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
					 << " in task " << opIDMap[freeID].taskID << " in thread " << opIDMap[freeID].threadID << ")\n";
				if (allocID > 0) {
					cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
						 << allocSet[allocID].startingAddress << " in task " << opIDMap[allocID].taskID
						 << " in thread " << opIDMap[allocID].threadID << ")\n";
				}

				if (opIDMap[readID].threadID == opIDMap[freeID].threadID)
					raceType = SINGLETHREADED;
				else
					raceType = MULTITHREADED;

				log(readID, freeID, "read", "free", true);
				flag = true;
#ifdef UNIQUERACE
				raceForFree = true;
				break;
#else
				continue;
#endif
			}

			if (graph->opEdgeExists(nodeRead, nodeFree) == 0) {

#ifdef ADDITIONS
				if (allocID > 0) {
					// Even if there is no edge between read and free,
					// if there is an edge from alloc to read (alloc happens before read), and the alloc is in the same task as the read and the task is atomic, this is a false positive.
					// This is true only if free is in the same thread as alloc and read.
					bool edgeExists = false;
					if (nodeAlloc == nodeRead && allocID < readID)
						edgeExists = true;
					else if (nodeAlloc != nodeRead && graph->opEdgeExists(nodeAlloc, nodeRead))
						edgeExists = true;
					if (edgeExists &&
							opIDMap[allocID].taskID.compare(opIDMap[readID].taskID) == 0
							&& taskIDMap[opIDMap[allocID].taskID].atomic) {
						if (opIDMap[freeID].threadID == opIDMap[readID].threadID) {
#ifdef UNIQUERACE
							if (!falsePositiveForFree) {
								falsePositives++;
								falsePositiveForFree = true;
							}
#else
							falsePositives++;
#endif
							raceType = SINGLETHREADED_ALLOC_MEMOP_IN_SAME_TASK_FP;

							continue;
						} else {
							// May be we should categorize these separately. Easy to reproduce
							raceType = MULTITHREADED_ALLOC_MEMOP_IN_SAME_TASK;
						}
					}
				}
#endif
				cout << "Potential UAF between read op " << readID << " (read at address " << readSet[readID].startingAddress
					 << " in task " << opIDMap[readID].taskID << " in thread " << opIDMap[readID].threadID << ") "
					 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
					 << " in task " << opIDMap[freeID].taskID << " in thread " << opIDMap[freeID].threadID << ")\n";
				if (allocID > 0) {
					cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
						 << allocSet[allocID].startingAddress << " in task " << opIDMap[allocID].taskID
						 << " in thread " << opIDMap[allocID].threadID << ")\n";
				}

				log(readID, freeID, "read", "free", true);

				flag = true;
#ifdef UNIQUERACE
				raceForFree = true;
				break;
#else
				continue;
#endif
			}
		}

#ifdef UNIQUERACE
		if (raceForFree)
			continue;
#endif

		for (set<IDType>::iterator writeIt = freeIt->second.writeOps.begin(); writeIt != freeIt->second.writeOps.end(); writeIt++) {
			IDType writeID = *writeIt;

			IDType nodeWrite = opIDMap[writeID].nodeID;
			if (nodeWrite <= 0) {
				cout << "ERROR: Invalid node ID for op " << writeID << "\n";
				return -1;
			}
			if (nodeFree == nodeWrite && freeID < writeID) {
				cout << "Definite UAF between write op " << writeID << " (write at address " << writeSet[writeID].startingAddress
					 << " in task " << opIDMap[writeID].taskID << " in thread " << opIDMap[writeID].threadID << ") "
					 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
					 << " in task " << opIDMap[freeID].taskID << " in thread " << opIDMap[freeID].threadID << ")\n";
				if (allocID > 0) {
					cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
						 << allocSet[allocID].startingAddress << " in task " << opIDMap[allocID].taskID << " in thread "
						 << opIDMap[allocID].threadID << ")\n";
				}

				// If free and read are in the same node, they are in the same thread
				raceType = SINGLETHREADED;

				log(writeID, freeID, "read", "free", true);

				flag = true;
#ifdef UNIQUERACE
				raceForFree = true;
				break;
#else
				continue;
#endif
			} else if (nodeFree == nodeWrite) {
#ifdef RACEDEBUG
				cout << "DEBUG: Free op " << freeID << " and write op " << writeID << " in the same node, but write before free\n";
				cout << "DEBUG: Skipping this write while detecting races\n";
#endif
				continue;
			}
			if (graph->opEdgeExists(nodeFree, nodeWrite) == 1) {
				cout << "Definite UAF between write op " << writeID << " (write at address " << writeSet[writeID].startingAddress
					 << " in task " << opIDMap[writeID].taskID << " in thread " << opIDMap[writeID].threadID << ") "
					 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
					 << " in task " << opIDMap[freeID].taskID << " in thread " << opIDMap[freeID].threadID << ")\n";
				if (allocID > 0) {
					cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
						 << allocSet[allocID].startingAddress << " in task " << opIDMap[allocID].taskID
						 << " in thread " << opIDMap[allocID].threadID << ")\n";
				}

				if (opIDMap[writeID].threadID == opIDMap[freeID].threadID)
					raceType = SINGLETHREADED;
				else
					raceType = MULTITHREADED;

				log(writeID, freeID, "read", "free", true);

				flag = true;
#ifdef UNIQUERACE
				raceForFree = true;
				break;
#else
				continue;
#endif
			}

			if (graph->opEdgeExists(nodeWrite, nodeFree) == 0) {

#ifdef ADDITIONS
				if (allocID > 0) {
					// Even if there is no edge between write and free,
					// if there is an edge from alloc to write (alloc happens before write), and the alloc is in the same task as the write and the task is atomic, this is a false positive.
					// This is true only if free is in the same thwrite as alloc and write.
					bool edgeExists = false;
					if (nodeAlloc == nodeWrite && allocID < writeID)
						edgeExists = true;
					else if (nodeAlloc != nodeWrite && graph->opEdgeExists(nodeAlloc, nodeWrite))
						edgeExists = true;
					if (edgeExists &&
							opIDMap[allocID].taskID.compare(opIDMap[writeID].taskID) == 0
							&& taskIDMap[opIDMap[allocID].taskID].atomic) {
						if (opIDMap[freeID].threadID == opIDMap[writeID].threadID) {
#ifdef UNIQUERACE
							if (!falsePositiveForFree) {
								falsePositives++;
								falsePositiveForFree = true;
							}
#else
							falsePositives++;
#endif
							raceType = SINGLETHREADED_ALLOC_MEMOP_IN_SAME_TASK_FP;
							continue;
						} else {
							raceType = MULTITHREADED_ALLOC_MEMOP_IN_SAME_TASK;
						}
					}
				}
#endif

				cout << "Potential UAF between write op " << writeID << " (write at address " << writeSet[writeID].startingAddress
					 << " in task " << opIDMap[writeID].taskID << " in thread " << opIDMap[writeID].threadID << ") "
					 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
					 << " in task " << opIDMap[freeID].taskID << " in thread " << opIDMap[freeID].threadID << ")\n";
				if (allocID > 0) {
					cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
						 << allocSet[allocID].startingAddress << " in task " << opIDMap[allocID].taskID
						 << " in thread " << opIDMap[allocID].threadID << ")\n";
				}

				log(writeID, freeID, "read", "free", true);

				flag = true;
#ifdef UNIQUERACE
				raceForFree = true;
				break;
#else
				continue;
#endif
			}
		}
	}

	if (flag) {
		cout << "OUTPUT: False positives = " << falsePositives << "\n";
		return uafCount;
	} else
		return 0;
}

#ifndef ACCESS
IDType UAFDetector::findDataRaces() {

	bool flag = false;

#ifdef UNIQUERACE
	bool raceForAlloc;
#endif
	for (map<IDType, allocOpDetails>::iterator allocIt = allocIDMap.begin(); allocIt != allocIDMap.end(); allocIt++) {
#ifdef UNIQUERACE
		raceForAlloc = false;
#endif
		for (set<IDType>::iterator writeIt = allocIt->second.writeOps.begin(); writeIt != allocIt->second.writeOps.end(); writeIt++) {

			string writeAddress1 = writeSet[*writeIt].startingAddress;

			IDType nodeWrite = opIDMap[*writeIt].nodeID;

			// write-write races
			for (set<IDType>::iterator write2It = allocIt->second.writeOps.begin(); write2It != allocIt->second.writeOps.end(); write2It++) {
				if (*write2It == *writeIt) continue;

				string writeAddress2 = writeSet[*write2It].startingAddress;
				if (writeAddress1.compare(writeAddress2) != 0) continue;

				IDType nodeWrite2 = opIDMap[*write2It].nodeID;
				if (nodeWrite <= 0) {
					cout << "ERROR: Invalid node ID for op " << *writeIt << "\n";
					return -1;
				}
				if (nodeWrite2 <= 0) {
					cout << "ERROR: Invalid node ID for op " << *write2It << "\n";
					return -1;
				}
				if (nodeWrite == nodeWrite2) {
#ifdef RACEDEBUG
					cout << "DEBUG: Write ops " << *writeIt << " and " *write2It << " are in the same node, so no race\n";
#endif
					continue;
				}
				if (graph->opEdgeExists(nodeWrite, nodeWrite2) == 0 && graph->opEdgeExists(nodeWrite2, nodeWrite) == 0) {
					cout << "Potential data race between write ops " << *writeIt << " (in task " << opIDMap[*writeIt].taskID
						 << " in thread " << opIDMap[*writeIt].threadID
						 << ") and " << *write2It << " (in task " << opIDMap[*write2It].taskID
						 << " in thread " << opIDMap[*write2It].threadID << ") on address "
						 << writeAddress1 << endl;

					if (opIDMap[*writeIt].threadID == opIDMap[*write2It].threadID)
						raceType = SINGLETHREADED;
					else
						raceType = MULTITHREADED;

					log(*writeIt, *write2It, "write", "write", false);

					flag = true;
#ifdef UNIQUERACE
					raceForAlloc = true;
					break;
#else
					continue;
#endif
				}
			}

#ifdef UNIQUERACE
			if (raceForAlloc)
				break;
#endif

			// write-read / read-write races
			for (set<IDType>::iterator readIt = allocIt->second.readOps.begin(); readIt != allocIt->second.readOps.end(); readIt++) {
				if (*readIt == *writeIt) continue;

				string readAddress = readSet[*readIt].startingAddress;
				if (writeAddress1.compare(readAddress) != 0) continue;

				IDType nodeRead = opIDMap[*readIt].nodeID;
				if (nodeRead <= 0) {
					cout << "ERROR: Invalid node ID for op " << *readIt << "\n";
					return -1;
				}
				if (nodeWrite == nodeRead) {
#ifdef RACEDEBUG
					cout << "DEBUG: Read op " << *readIt << " and write op " << *writeIt << " are in the same node, so no race\n";
#endif
					continue;
				}
				if (graph->opEdgeExists(nodeWrite, nodeRead) == 0 && graph->opEdgeExists(nodeRead, nodeWrite) == 0) {
					cout << "Potential data race between read op " << *readIt << " (in task " << opIDMap[*readIt].taskID
						 << " in thread " << opIDMap[*readIt].threadID
						 << ") and write op " << *writeIt << "(in task " << opIDMap[*writeIt].taskID
						 << " in thread " << opIDMap[*writeIt].threadID << ") on address "
						 << readAddress << "\n";

					if (opIDMap[*writeIt].threadID == opIDMap[*readIt].threadID)
						raceType = SINGLETHREADED;
					else
						raceType = MULTITHREADED;

					log(*readIt, *writeIt, "read", "write", false);

					flag = true;
#ifdef UNIQUERACE
					raceForAlloc = true;
					break;
#else
					continue;
#endif
				}
			}
		}
	}

	if (flag)
		return raceCount;
	else
		return 0;
}
#endif

void UAFDetector::initLog(std::string traceFileName) {
	uafCount = 0;
	raceCount = 0;

	std::string uafFileName = traceFileName + ".uaf.all";
	uafAllLogger.init(uafFileName);
	std::string raceFileName = traceFileName + ".race.all";
	raceAllLogger.init(raceFileName);

	uafFileName = traceFileName + ".uaf.withnesting";
	uafNestingLogger.init(uafFileName);
	raceFileName = traceFileName + ".race.withnesting";
	raceNestingLogger.init(raceFileName);

	uafFileName = traceFileName + ".uaf.withoutnesting";
	uafNoNestingLogger.init(uafFileName);
	raceFileName = traceFileName + ".race.withoutnesting";
	raceNoNestingLogger.init(raceFileName);

	uafFileName = traceFileName + ".uaf.withtask";
	uafTaskLogger.init(uafFileName);
	raceFileName = traceFileName + ".race.withtask";
	raceTaskLogger.init(raceFileName);

	uafFileName = traceFileName + ".uaf.withouttask";
	uafNoTaskLogger.init(uafFileName);
	raceFileName = traceFileName + ".race.withouttask";
	raceNoTaskLogger.init(raceFileName);

	uafFileName = traceFileName + ".uaf.enqpath";
	uafEnqPathLogger.init(uafFileName);
	raceFileName = traceFileName + ".race.enqpath";
	raceEnqPathLogger.init(raceFileName);

	uafFileName = traceFileName + ".uaf.allocmemopinsametask";
	uafAllocMemopSameTaskLogger.init(uafFileName);
	raceFileName = traceFileName + ".race.allocmemopinsametask";
	raceAllocMemopSameTaskLogger.init(raceFileName);
}

// uafOrRace: true if uaf, false if data race
void UAFDetector::log(IDType op1ID, IDType op2ID, std::string op1Type,
		std::string op2Type, bool uafOrRace) {

	Logger *taskLogger, *nestingLogger, *noNestingLogger, *noTaskLogger,
		*allLogger, *enqPathLogger, *allocMemopInSameTaskLogger;
	std::string raceTypeString;
	unsigned long long int count;
	if (uafOrRace) {
		uafCount++;

		allLogger = &uafAllLogger;
		taskLogger = &uafTaskLogger;
		nestingLogger = &uafNestingLogger;
		noNestingLogger = &uafNoNestingLogger;
		noTaskLogger = &uafNoTaskLogger;
		enqPathLogger = &uafEnqPathLogger;
		allocMemopInSameTaskLogger = &uafAllocMemopSameTaskLogger;
		raceTypeString = "UAF";
		count = uafCount;

	} else {
		raceCount++;

		allLogger = &raceAllLogger;
		taskLogger = &raceTaskLogger;
		nestingLogger = &raceNestingLogger;
		noNestingLogger = &raceNoNestingLogger;
		noTaskLogger = &raceNoTaskLogger;
		enqPathLogger = &raceEnqPathLogger;
		allocMemopInSameTaskLogger = &raceAllocMemopSameTaskLogger;
		raceTypeString = "Race";
		count = raceCount;
	}

	IDType op1ThreadID = opIDMap[op1ID].threadID;
	IDType op2ThreadID = opIDMap[op2ID].threadID;
	std::string op1TaskID = opIDMap[op1ID].taskID;
	std::string op2TaskID = opIDMap[op2ID].taskID;

	allLogger->streamObject << op1ID << " " << op1ThreadID
			<< " " << op2ID << " " << op2ThreadID << "\n";
	allLogger->writeLog();

	// Finding the enq path
	IDType enqID, threadID = -1;
	std::string tempTaskID = "";

	enqPathLogger->streamObject << "\n" << raceTypeString << " " << count << ":\n";
	// op1
	enqPathLogger->streamObject << "OP: " << op1ID << " " << op1TaskID
			<< " " << op1ThreadID << "\n";
	enqPathLogger->writeLog();

	enqID = taskIDMap[op1TaskID].enqOpID;
	if (enqID != -1) {
		tempTaskID = opIDMap[enqID].taskID;
		threadID = opIDMap[enqID].threadID;
		enqPathLogger->streamObject << "enq: " << enqID << " " << tempTaskID
			<< " " << threadID << "\n";
	} else {
		enqPathLogger->streamObject << "enq: " << enqID << "\n";
	}
	enqPathLogger->writeLog();

	allLogger->streamObject << enqID << " ";

	while (enqID != -1 && tempTaskID.compare("") != 0) {
		enqID = taskIDMap[tempTaskID].enqOpID;
		if (enqID != -1) {
			tempTaskID = opIDMap[enqID].taskID;
			threadID = opIDMap[enqID].threadID;
			enqPathLogger->streamObject << "enq: " << enqID << " " << tempTaskID
				<< " " << threadID << "\n";
		} else {
			enqPathLogger->streamObject << "enq: " << enqID << "\n";
		}

		allLogger->streamObject << enqID << " ";

		enqPathLogger->writeLog();
	}

	allLogger->streamObject << "\n";
	allLogger->writeLog();

	// op2
	enqPathLogger->streamObject << "OP: " << op2ID << " " << op2TaskID
			<< " " << op2ThreadID << "\n";
	enqPathLogger->writeLog();

	enqID = taskIDMap[op2TaskID].enqOpID;
	if (enqID != -1) {
		tempTaskID = opIDMap[enqID].taskID;
		threadID = opIDMap[enqID].threadID;
		enqPathLogger->streamObject << "enq: " << enqID << " " << tempTaskID
			<< " " << threadID << "\n";
	} else {
		enqPathLogger->streamObject << "enq: " << enqID << "\n";
	}
	enqPathLogger->writeLog();
	allLogger->streamObject << enqID << " ";
	while (enqID != -1 && tempTaskID.compare("") != 0) {
		enqID = taskIDMap[tempTaskID].enqOpID;
		if (enqID != -1) {
			tempTaskID = opIDMap[enqID].taskID;
			threadID = opIDMap[enqID].threadID;
			enqPathLogger->streamObject << "enq: " << enqID << " " << tempTaskID
				<< " " << threadID << "\n";
		} else {
			enqPathLogger->streamObject << "enq: " << enqID << "\n";
		}

		enqPathLogger->writeLog();

		allLogger->streamObject << enqID << " ";
	}

	allLogger->streamObject << "\n";
	allLogger->writeLog();

	if (op1TaskID.compare("") != 0 && op2TaskID.compare("") != 0) {
		taskLogger->streamObject << "\n" << raceTypeString << " " << count << ":\n";
		taskLogger->streamObject << op1Type << ": " << op1ID << " thread " << op1ThreadID
								 << " task " << op1TaskID << "\n"
								 << op2Type << ": "	<< op2ID << " thread " << op2ThreadID
								 << " task " << op2TaskID << "\n";
		taskLogger->writeLog();

		// It is not sufficient that the task of read/write (op1) is not atomic,
		// we need the read to happen after the (first) nesting loop.
		if (!taskIDMap[op1TaskID].atomic && taskIDMap[op1TaskID].firstPauseOpID < op1ID) {
			nestingLogger->streamObject << "\n" << raceTypeString << " " << count << ":\n";
			nestingLogger->streamObject << op1Type << ": " << op1ID << " thread " << op1ThreadID
									 << " task " << op1TaskID << "\n"
									 << op2Type << ": " << op2ID << " thread " << op2ThreadID
									 << " task " << op2TaskID << "\n";
			nestingLogger->writeLog();
		} else {
			noNestingLogger->streamObject << "\n" << raceTypeString << " " << count << ":\n";
			noNestingLogger->streamObject << op1Type << ": " << op1ID << " thread " << op1ThreadID
									 << " task " << op1TaskID << "\n"
									 << op2Type << ": " << op2ID << " thread " << op2ThreadID
									 << " task " << op2TaskID << "\n";
			noNestingLogger->writeLog();
		}

	} else {
		noTaskLogger->streamObject << "\n" << raceTypeString << " " << count << ":\n";
		noTaskLogger->streamObject << op1Type << ": " << op1ID << " thread " << op1ThreadID
								 << " task " << op1TaskID << "\n"
								 << op2Type << ": " << op2ID << " thread " << op2ThreadID
								 << " task " << op2TaskID << "\n";
		noTaskLogger->writeLog();
	}

	if (raceType == MULTITHREADED_ALLOC_MEMOP_IN_SAME_TASK) {
		allocMemopInSameTaskLogger->streamObject << "\n" << raceTypeString << " " << count << ":\n";
		allocMemopInSameTaskLogger->streamObject << op1Type << ": " << op1ID << " thread " << op1ThreadID
				<< " task " << op1TaskID << "\n"
				<< op2Type << ": " << op2ID << " thread " << op2ThreadID << " task " << op2TaskID << "\n";
		allocMemopInSameTaskLogger->writeLog();
	}
}
