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

	// PAUSE-ST/MT OR RESUME-ST/MT
	if (add_PauseSTMT_ResumeSTMT_Edges() < 0) {
		cout << "ERROR: While adding PAUSE-ST/MT or RESUME-ST/MT edges\n";
		return -1;
	}

	bool edgeAdded = false;
	while (true) {
		int retValue;

#ifdef GRAPHDEBUGFULL
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

#ifdef GRAPHDEBUGFULL
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

#ifdef GRAPHDEBUGFULL
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

#ifdef GRAPHDEBUGFULL
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
				cout << "DEBUG: Cannot find fork op of thread " << it->first << endl;
				cout << "DEBUG: Skipping FORK-edge for thread " << it->first << endl;
			}
			if (opJ <= 0) {
				cout << "DEBUG: Cannot find threadinit of thread " << it->first << endl;
				cout << "DEBUG: Skipping FORK-edge for thread " << it->first << endl;
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
#ifdef GRAPHDEBUG
				cout << "DEBUG: Cannot find threadexit op of thread " << it->first << endl;
				cout << "DEBUG: Skipping JOIN-edge for thread " << it->first << endl;
#endif
			}
			if (opJ <= 0) {
#ifdef GRAPHDEBUG
				cout << "DEBUG: Cannot find join op of thread " << it->first << endl;
				cout << "DEBUG: Skipping JOIN-edge for thread " << it->first << endl;
#endif
			}
		}


		// Adding LOOP-PO edges

		IDType blockI = it->second.firstBlockID;
		IDType enterloopBlock = it->second.enterloopBlockID;
		IDType lastBlockInThread = it->second.lastBlockID;

#ifdef SANITYCHECK
		if (blockI <= 0) {
#ifdef GRAPHDEBUG
			cout << "DEBUG: Cannot find first block of thread " << it->first << endl;
			cout << "DEBUG: Skipping LOOP-PO edges for this thread\n";
#endif
			continue;
//			return -1;
		}
		if (enterloopBlock <= 0) {
#ifdef GRAPHDEBUG
			cout << "DEBUG: Cannot find enterloop block of thread " << it->first << endl;
			cout << "DEBUG: Skipping LOOP-PO edges for thread " << it->first << endl;
#endif
			continue;
		}
		if (lastBlockInThread <= 0) {
#ifdef GRAPHDEBUG
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
#ifdef GRAPHDEBUG
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
				int retOpValue = graph->opEdgeExists(opI, opJ);
				if (retOpValue == 0) {
					int addEdgeRetValue = graph->addOpEdge(opI, opJ);
					if (addEdgeRetValue == 1) {
						flag = true;
#ifdef GRAPHDEBUG
						cout << "LOOP-PO edge (" << opI << ", " << opJ << ") -- #op-edges "   << graph->numOfOpEdges
																	   << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
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
#ifdef GRAPHDEBUG
			cout << "DEBUG: Cannot find next block of block " << enterloopBlock << " in thread " << it->first << endl;
			cout << "DEBUG: Block " << enterloopBlock << ": ops (" << blockIDMap[enterloopBlock].firstOpInBlock
				 << " - " << blockIDMap[enterloopBlock].lastOpInBlock << ")\n";
			cout << "DEBUG: Skipping LOOP-PO edge for this block\n";
#endif
			continue;
		}
		if (exitloopBlock <= 0) {
#ifdef GRAPHDEBUG
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
#ifdef GRAPHDEBUG
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

#if 0
		if (it->second.enqOpID > 0 && it->second.deqOpID <= 0) {
			// Saw an enq of the task, but no deq
			cout << "DEBUG: Saw an enq of task " << it->first << " at " << it->second.enqOpID << "\n";
			cout << "DEBUG: Skipping TASK-PO/ENQUEUE-ST/MT edges for this task\n";
			continue;
		}
#endif

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
#ifdef GRAPHDEBUG
				cout << "DEBUG: Cannot find enq op of task " << it->first << endl;
				cout << "DEBUG: Skipping ENQUEUE-ST/MT edge for this task\n";
#endif
			}
			if (deqOp <= 0) {
#ifdef GRAPHDEBUG
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
#ifdef GRAPHDEBUG
			cout << "DEBUG: Cannot find first block of task " << it->first << endl;
			cout << "DEBUG: Skipping TASK-PO edges for this task\n";
#endif
//			return -1;
			continue;
		}
		if (lastBlockInTask <= 0) {
#ifdef GRAPHDEBUG
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

int UAFDetector::add_FifoAtomic_NoPre_Edges() {

	bool flag = false; // To keep track of whether edges were added.


	for (map<string, taskDetails>::iterator it = taskIDMap.begin(); it != taskIDMap.end(); it++) {

		// If the task is not atomic, the rule does not apply
		if (it->second.atomic == false) {
#ifdef GRAPHDEBUG
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
#ifdef GRAPHDEBUG
			cout << "DEBUG: Saw enq of task " << it->first << " at " << it->second.enqOpID << ", but no end op\n";
			cout << "DEBUG: Skipping FIFO-ATOMIC/NO-PRE edges for this task\n";
#endif
			continue;
		}

		// Adding FIFO-ATOMIC edges.
		IDType enqOp = it->second.enqOpID;
		if (enqOp <= 0) {
#ifdef GRAPHDEBUG
			cout << "DEBUG: Cannot find enq of task " << it->first << endl;
			cout << "DEBUG: Skipping FIFO-ATOMIC edge for this task\n";
#endif
		} else {
			IDType enqOpBlock = opIDMap[enqOp].blockID;
			if (enqOpBlock <= 0) {
#ifdef GRAPHDEBUG
				cout << "DEBUG: Cannot find block of enq op " << enqOp << endl;
				cout << "DEBUG: Skipping FIFO-ATOMIC edge for task " << it->first << "\n";
#endif
			} else {
				IDType opI = it->second.endOpID;
				if (opI <= 0) {
#ifdef GRAPHDEBUG
					cout << "DEBUG: Cannot find end of task " << opI << endl;
					cout << "DEBUG: Skipping FIFO-ATOMIC edge for task " << it->first << "\n";
#endif
				} else {
					for (HBGraph::adjListNode* currNode = graph->blockAdjList[enqOpBlock].head; currNode != NULL; currNode = currNode->next) {
						IDType tempBlock = currNode->destination;
						if (tempBlock <= 0) {
#ifdef GRAPHDEBUG
							cout << "DEBUG: Found invalid block edge from " << enqOpBlock << endl;
#endif
							continue;
						}

						for (set<IDType>::iterator enqIt = blockIDMap[tempBlock].enqSet.begin(); enqIt != blockIDMap[tempBlock].enqSet.end(); enqIt++) {
							IDType tempenqOp = *enqIt;

							// If we are looking at enqs within the same block,
							// make sure we do not have the same enq as both
							// source and destination for the HB edge we check.
							if (enqOpBlock == tempBlock && enqOp == tempenqOp) continue;

							string taskName = enqToTaskEnqueued[tempenqOp].taskEnqueued;
							if (taskName.compare("") == 0) {
#ifdef GRAPHDEBUG
								cout << "DEBUG: Cannot find task enqueued in op " << tempenqOp << endl;
#endif
								continue;
							}

							// FIFO-ATOMIC does not apply if the two tasks are not posted to the same thread
							if (enqToTaskEnqueued[enqOp].targetThread != enqToTaskEnqueued[tempenqOp].targetThread) continue;

							int retOpValue1 = graph->opEdgeExists(enqOp, tempenqOp);
							if (retOpValue1 == 1) {
								IDType opJ = taskIDMap[taskName].deqOpID;
								if (opJ <= 0) {
#ifdef GRAPHDEBUG
									cout << "DEBUG: Cannot find deq op of task " << taskName << endl;
#endif
									continue;
								}

								int retOpValue2 = graph->opEdgeExists(opI, opJ);
								if (retOpValue2 == 0) {
									int addEdgeRetValue = graph->addOpEdge(opI, opJ);
									if (addEdgeRetValue == 1) {
										flag = true;
#ifdef GRAPHDEBUG
										cout << "FIFO-ATOMIC edge (" << opI << ", " << opJ << ") -- #op-edges "   << graph->numOfOpEdges
																	   	   	   	   	   	   << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
									} else if (addEdgeRetValue == -1) {
										cout << "ERROR: While adding FIFO-ATOMIC edge from " << opI << " to " << opJ << endl;
										return -1;
									}
								} else if (retOpValue2 == -1) {
									cout << "ERROR: While checking for op-edge (" << opI << ", " << opJ << ")\n";
									return -1;
								}
							} else if (retOpValue1 == -1) {
								cout << "ERROR: While checking for op-edge (" << enqOp << ", " << tempenqOp << ")\n";
								return -1;
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
#ifdef GRAPHDEBUG
			cout << "DEBUG: Cannot find deq op of task " << it->first << endl;
			cout << "DEBUG: If there is no deq op, find the first op of task\n";
#endif
			IDType firstBlockOfTask = taskIDMap[it->first].firstBlockID;
			if (firstBlockOfTask <= 0) {
#ifdef GRAPHDEBUG
				cout << "DEBUG: Cannot find first block of task " << it->first << "\n";
				cout << "DEBUG: This means there are no ops in the task\n";
				cout << "DEBUG: Skipping NOPRE edges for this task\n";
#endif
			} else {
				i = blockIDMap[firstBlockOfTask].firstOpInBlock;
#ifdef GRAPHDEBUG
				cout << "DEBUG: Using op " << i << " in place of deq op, since it is the "
					 << "first op in the task " << it->first << "\n";
#endif
			}
		}
		if (opI <= 0) {
#ifdef GRAPHDEBUG
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
					for (HBGraph::adjListNode* currNode = graph->blockAdjList[blockI].head; currNode != NULL; currNode = currNode->next) {
#ifdef SANITYCHECK
						assert(currNode->destination > 0);
#endif

						for (set<IDType>::iterator enqIt = blockIDMap[currNode->destination].enqSet.begin(); enqIt != blockIDMap[currNode->destination].enqSet.end(); enqIt++) {
#ifdef SANITYCHECK
							assert(*enqIt > 0);
#endif
							IDType tempenqOp = *enqIt;

							// If we are looking at enqs within the same block,
							// make sure we do not have the same enq as both
							// source and destination for the HB edge we check.
							if (blockI == currNode->destination && i == tempenqOp) continue;

							string taskName = enqToTaskEnqueued[tempenqOp].taskEnqueued;
							if (taskName.compare("") == 0) {
#ifdef GRAPHDEBUG
								cout << "ERROR: Cannot find task enqueued in op " << tempenqOp << endl;
#endif
//								continue;
								return -1;
							}

							// NO-PRE does not apply if the two tasks are in different threads
							if (opIDMap[i].threadID != enqToTaskEnqueued[tempenqOp].targetThread) continue;

							int retOpValue1 = graph->opEdgeExists(i, tempenqOp);
							if (retOpValue1 == 1) {
								IDType opJ = taskIDMap[taskName].deqOpID;
								if (opJ <= 0) {
#ifdef GRAPHDEBUG
									cout << "DEBUG: Cannot find deq op of task " << taskName << endl;
									cout << "DEBUG: Skipping NOPRE edge from end op " << opI
										 << " of task " << it->first << " to deq op of task " << taskName << "\n";
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

								int retOpValue2 = graph->opEdgeExists(opI, opJ);
								if (retOpValue2 == 0) {
									int addEdgeRetValue = graph->addOpEdge(opI, opJ);
									if (addEdgeRetValue == 1) {
										flag = true;
#ifdef GRAPHDEBUG
										cout << "NO-PRE edge (" << opI << ", " << opJ << ") -- #op-edges "   << graph->numOfOpEdges
																					  << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
									} else if (addEdgeRetValue == -1) {
										cout << "ERROR: While adding NO-PRE edge from " << opI << " to " << opJ << endl;
										return -1;
									}
								} else if (retOpValue2 == -1) {
									cout << "ERROR: While checking for op-edge (" << opI << ", " << opJ << ")\n";
									return -1;
								}
							} else if (retOpValue1 == -1) {
								cout << "ERROR: While checking for op-edge (" << i << ", " << tempenqOp << ")\n";
								return -1;
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
					int retValue = graph->opEdgeExists(opI, opJ);
					if (retValue == 0) {
#ifdef GRAPHDEBUG
						cout << "DEBUG: Adding PAUSE-MT edge (" << opI << ", " << opJ << ")\n";
#endif
						int addEdgeRetValue = graph->addOpEdge(opI, opJ);
						if (addEdgeRetValue == 1) {
							flag = true;
#ifdef GRAPHDEBUG
							cout << "PAUSE-MT edge (" << opI << ", " << opJ << ") -- #op-edges "   << graph->numOfOpEdges
																		    << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
						} else if (addEdgeRetValue == -1) {
							cout << "ERROR: While adding PAUSE-MT edge from " << opI << " to " << opJ << endl;
							return -1;
						}
					} else if (retValue == -1) {
						cout << "ERROR: While checking PAUSE-MT edge from " << opI << " to " << opJ << endl;
						return -1;
					}
				} else {
					std::string taskOfResetOp = opIDMap[resetOp].taskID;
					std::string taskOfPauseOp = opIDMap[pauseOp].taskID;
					if (taskOfResetOp.compare("") != 0 && taskOfPauseOp.compare("") != 0 &&
							taskOfResetOp.compare(taskOfPauseOp) != 0) {
						opJ = taskIDMap[taskOfResetOp].deqOpID;
						if (opJ <= 0) {
#ifdef GRAPHDEBUG
							cout << "DEBUG: Cannot find deq of task " << taskOfResetOp << "\n";
							cout << "DEBUG: Skipping PAUSE-ST/MT for this task\n";
#endif
						} else {
							int retValue = graph->opEdgeExists(opI, opJ);
							if (retValue == 0) {
#ifdef GRAPHDEBUG
								cout << "DEBUG: Adding PAUSE-ST edge (" << opI << ", " << opJ << ")\n";
#endif
								int addEdgeRetValue = graph->addOpEdge(opI, opJ);
								if (addEdgeRetValue == 1) {
									flag = true;
#ifdef GRAPHDEBUG
									cout << "PAUSE-ST edge (" << opI << ", " << opJ << ") -- #op-edges "   << graph->numOfOpEdges
																				    << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
								} else if (addEdgeRetValue == -1) {
									cout << "ERROR: While adding PAUSE-ST edge from " << opI << " to " << opJ << endl;
									return -1;
								}
							} else if (retValue == -1) {
								cout << "ERROR: While checking PAUSE-ST edge from " << opI << " to " << opJ << endl;
								return -1;
							}
						}
					} else if (taskOfResetOp.compare("") == 0) {
#ifdef GRAPHDEBUG
						cout << "DEBUG: Cannot find task of reset op " << resetOp << "\n";
						cout << "DEBUG: Skipping PAUSE-ST/MT for this task\n";
#endif
					} else if (taskOfPauseOp.compare("") == 0) {
						cout << "ERROR: Cannot find task of pause op " << pauseOp << "\n";
						return -1;
					} else if (taskOfResetOp.compare(taskOfPauseOp) == 0) {
						cout << "DEBUG: Pause op " << pauseOp << " and reset op " << resetOp
							 << " are in the same task " << taskOfResetOp << "\n";
#ifdef EXTRARULES
						cout << "DEBUG: Adding edge from pause op to reset op\n";

						opJ = resetOp;
						int retValue = graph->opEdgeExists(opI, opJ);
						if (retValue == 0) {
#ifdef GRAPHDEBUG
							cout << "DEBUG: Adding PAUSE-ST edge (" << opI << ", " << opJ << ")\n";
#endif
							int addEdgeRetValue = graph->addOpEdge(opI, opJ);
							if (addEdgeRetValue == 1) {
								flag = true;
#ifdef GRAPHDEBUG
								cout << "PAUSE-ST edge (" << opI << ", " << opJ << ") -- #op-edges "   << graph->numOfOpEdges
																			    << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
							} else if (addEdgeRetValue == -1) {
								cout << "ERROR: While adding PAUSE-ST edge from " << opI << " to " << opJ << endl;
								return -1;
							}
						} else if (retValue == -1) {
							cout << "ERROR: While checking PAUSE-ST edge from " << opI << " to " << opJ << endl;
							return -1;
						}
#else
						cout << "DEBUG: Skipping PAUSE-ST edge for this task\n";
#endif
					}
				}

			} else if (opI <= 0) {
#ifdef GRAPHDEBUG
				cout << "DEBUG: Cannot find pause for shared variable " << it->first << "\n";
				cout << "DEBUG: Skipping PAUSE-ST/MT\n";
#endif
			} else if (resetOp <= 0) {
#ifdef GRAPHDEBUG
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
					int retValue = graph->opEdgeExists(opI, opJ);
					if (retValue == 0) {
#ifdef GRAPHDEBUG
						cout << "DEBUG: Adding RESUME-MT edge (" << opI << ", " << opJ << ")\n";
#endif
						int addEdgeRetValue = graph->addOpEdge(opI, opJ);
						if (addEdgeRetValue == 1) {
							flag = true;
#ifdef GRAPHDEBUG
							cout << "RESUME-MT edge (" << opI << ", " << opJ << ") -- #op-edges "   << graph->numOfOpEdges
																		     << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
						} else if (addEdgeRetValue == -1) {
							cout << "ERROR: While adding RESUME-MT edge from " << opI << " to " << opJ << endl;
							return -1;
						}
					} else if (retValue == -1) {
						cout << "ERROR: While checking RESUME-MT edge from " << opI << " to " << opJ << endl;
						return -1;
					}
				} else {
					std::string taskOfResetOp = opIDMap[resetOp].taskID;
					std::string taskOfResumeOp = opIDMap[resumeOp].taskID;
					if (taskOfResetOp.compare("") != 0 && taskOfResumeOp.compare("") != 0 &&
							taskOfResetOp.compare(taskOfResumeOp) != 0) {
						opI = taskIDMap[taskOfResetOp].endOpID;
						if (opI <= 0) {
							cout << "DEBUG: Cannot find end of task " << taskOfResetOp << "\n";
							cout << "DEBUG: Skipping RESUME-ST/MT for this task\n";
						} else {
							int retValue = graph->opEdgeExists(opI, opJ);
							if (retValue == 0) {
#ifdef GRAPHDEBUG
								cout << "DEBUG: Adding RESUME-ST edge (" << opI << ", " << opJ << ")\n";
#endif
								int addEdgeRetValue = graph->addOpEdge(opI, opJ);
								if (addEdgeRetValue == 1) {
									flag = true;
#ifdef GRAPHDEBUG
									cout << "RESUME-ST edge (" << opI << ", " << opJ << ") -- #op-edges "   << graph->numOfOpEdges
																				    << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
								} else if (addEdgeRetValue == -1) {
									cout << "ERROR: While adding RESUME-ST edge from " << opI << " to " << opJ << endl;
									return -1;
								}
							} else if (retValue == -1) {
								cout << "ERROR: While checking RESUME-ST edge from " << opI << " to " << opJ << endl;
								return -1;
							}
						}
					} else if (taskOfResetOp.compare("") == 0) {
#ifdef GRAPHDEBUG
						cout << "DEBUG: Cannot find task of reset op " << resetOp << "\n";
						cout << "DEBUG: Skipping PAUSE-ST/MT for this task\n";
#endif
					} else if (taskOfResumeOp.compare("") == 0) {
						cout << "ERROR: Cannot find task of resume op " << resumeOp << "\n";
						return -1;
					} else if (taskOfResetOp.compare(taskOfResumeOp) == 0) {
						cout << "DEBUG: Resume op " << resumeOp << " and reset op " << resetOp
							 << " are in the same task " << taskOfResetOp << "\n";
#ifdef EXTRARULES
						cout << "DEBUG: Adding edge from reset op to resume op\n";

						opI = resetOp;
						opJ = resumeOp;
						int retValue = graph->opEdgeExists(opI, opJ);
						if (retValue == 0) {
#ifdef GRAPHDEBUG
							cout << "DEBUG: Adding RESUME-ST edge (" << opI << ", " << opJ << ")\n";
#endif
							int addEdgeRetValue = graph->addOpEdge(opI, opJ);
							if (addEdgeRetValue == 1) {
								flag = true;
#ifdef GRAPHDEBUG
								cout << "RESUME-ST edge (" << opI << ", " << opJ << ") -- #op-edges "   << graph->numOfOpEdges
																			    << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
							} else if (addEdgeRetValue == -1) {
								cout << "ERROR: While adding RESUME-ST edge from " << opI << " to " << opJ << endl;
								return -1;
							}
						} else if (retValue == -1) {
							cout << "ERROR: While checking RESUME-ST edge from " << opI << " to " << opJ << endl;
							return -1;
						}
#else
						cout << "DEBUG: Skipping RESUME-ST edge for this task\n";
#endif
					}
				}
			} else if (resetOp <= 0) {
#ifdef GRAPHDEBUG
				cout << "DEBUG: Cannot find reset for shared variable " << it->first << "\n";
				cout << "DEBUG: Skipping RESUME-ST/MT\n";
#endif
			} else if (resumeOp <= 0) {
#ifdef GRAPHDEBUG
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
			for (HBGraph::adjListNode* currNode = graph->blockAdjList[blockI].head; currNode != NULL; currNode = currNode->next) {
#ifdef SANITYCHECK
				assert(currNode->destination > 0);
#endif

				IDType blockJ = currNode->destination;

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

					int retValue = graph->opEdgeExists(enqI, enqJ);
					if (retValue == 1) {
						std::string taskJ = enqToTaskEnqueued[enqJ].taskEnqueued;
#ifdef SANITYCHECK
						assert(taskJ.compare("") != 0);
#endif
						opJ = taskIDMap[taskJ].deqOpID;
#ifdef SANITYCHECK
						if (opJ <= 0) {
							cout << "DEBUG: Cannot find deq of task " << taskJ << endl;
							cout << "DEBUG: Skipping FIFO-NESTED-1 edge from pause op " << opI << "\n";
							continue;
						}
#endif
						int edgeRetValue = graph->opEdgeExists(opI, opJ);
						if (edgeRetValue == 0) {
							int addEdgeRetValue = graph->addOpEdge(opI, opJ);
							if (addEdgeRetValue == 1) {
								flag = true;
#ifdef GRAPHDEBUG
								cout << "FIFO-NESTED-1 edge (" << opI << ", " << opJ << ") -- #op-edges "   << graph->numOfOpEdges
																	     	 	 	 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
							} else if (addEdgeRetValue == 0) {
								cout << "ERROR: Edge (" << opI << ", " << opJ << ") was not present when checked but addOpEdge returns 0\n";
								return -1;
							} else if (addEdgeRetValue == -1) {
								cout << "ERROR: While adding FIFO-NESTED-1 edge from " << opI << " to " << opJ << endl;
								return -1;
							}
						} else if (edgeRetValue == -1) {
							cout << "ERROR: While checking FIFO-NESTED-1 edge from " << opI << " to " << opJ << endl;
							return -1;
						}
					} else if (retValue == -1) {
						cout << "ERROR: While checking edge from " << enqI << " to " << enqJ << endl;
						return -1;
					}
				}
			}
		} else if (opI <= 0) {
#ifdef GRAPHDEBUG
			cout << "DEBUG: Cannot find first pause of task " << it->first << "\n";
			cout << "DEBUG: Skipping FIFO-NESTED-1 edge for this task\n";
#endif
		} else if (enqI <= 0) {
#ifdef GRAPHDEBUG
			cout << "DEBUG: Cannot find enq of task " << it->first << "\n";
			cout << "DEBUG: Skipping FIFO-NESTED-1 edge for this task\n";
#endif
		}


		// FIFO-NESTED-2
		opI = it->second.endOpID;
		IDType opL = it->second.lastResumeOpID;

		if (opL > 0) {
			if (opI <= 0) {
#ifdef GRAPHDEBUG
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
					for (HBGraph::adjListNode* currNode = graph->blockAdjList[blockL].head; currNode != NULL; currNode = currNode->next) {
#ifdef SANITYCHECK
						assert(currNode->destination > 0);
#endif

						IDType blockJ = currNode->destination;
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

							int retValue = graph->opEdgeExists(opL, enqJ);
							if (retValue == 1) {
								std::string taskJ = enqToTaskEnqueued[enqJ].taskEnqueued;
#ifdef SANITYCHECK
								assert(taskJ.compare("") != 0);
#endif
								opJ = taskIDMap[taskJ].deqOpID;
#ifdef SANITYCHECK
								if (opJ <= 0) {
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
								}
#endif
								if (opJ > 0) {
									int edgeRetValue = graph->opEdgeExists(opI, opJ);
									if (edgeRetValue == 0) {
										int addEdgeRetValue = graph->addOpEdge(opI, opJ);
										if (addEdgeRetValue == 1) {
											flag = true;
#ifdef GRAPHDEBUG
											cout << "FIFO-NESTED-2 edge (" << opI << ", " << opJ << ") -- #op-edges "   << graph->numOfOpEdges
												 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
										} else if (addEdgeRetValue == 0) {
											cout << "ERROR: Edge (" << opI << ", " << opJ << ") was not present when checked but addOpEdge returns 0\n";
											return -1;
										} else if (addEdgeRetValue == -1) {
											cout << "ERROR: While adding FIFO-NESTED-2 edge from " << opI << " to " << opJ << endl;
											return -1;
										}
									} else if (edgeRetValue == -1) {
										cout << "ERROR: While checking FIFO-NESTED-2 edge from " << opI << " to " << opJ << endl;
										return -1;
									}
								} else {
#ifdef GRAPHDEBUG
									cout << "DEBUG: Cannot find deq op (or first op) of task " << taskJ << "\n";
									cout << "DEBUG: Skipping FIFO-NESTED-2 edge from op " << opI << " to task " << taskJ << "\n";
#endif
								}
							} else if (retValue == -1) {
								cout << "ERROR: While checking edge from " << opL << " to " << enqJ << endl;
								return -1;
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
#ifdef GRAPHDEBUG
				cout << "DEBUG: Cannot find end op (or last op) of task " << it->first << "\n";
				cout << "DEBUG: Skipping FIFO-NESTED-2 edge for this task\n";
#endif
			}
		} else {
#ifdef GRAPHDEBUG
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
				for (HBGraph::adjListNode* currNode = graph->blockAdjList[blockOfResumeOp].head; currNode != NULL; currNode = currNode->next) {
#ifdef SANITYCHECK
					assert(currNode->destination > 0);
#endif

					IDType blockJ = currNode->destination;
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

						int retValue = graph->opEdgeExists(resumeOp, enqJ);
						if (retValue == 1) {

							if (prIt +1 == taskIDMap[it->first].pauseResumeResetSequence.end())
								continue;

							opI = (prIt+1)->pauseOp;
#ifdef SANITYCHECK
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
#ifdef SANITYCHECK
							if (opJ <= 0) {
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
							}
#endif
							if (opJ > 0) {
								int edgeRetValue = graph->opEdgeExists(opI, opJ);
								if (edgeRetValue == 0) {
									int addEdgeRetValue = graph->addOpEdge(opI, opJ);
									if (addEdgeRetValue == 1) {
										flag = true;
#ifdef GRAPHDEBUG
										cout << "FIFO-NESTED-GEN edge (" << opI << ", " << opJ << ") -- #op-edges "   << graph->numOfOpEdges
											 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
									} else if (addEdgeRetValue == -1) {
										cout << "ERROR: While adding FIFO-NESTED-GEN edge from " << opI << " to " << opJ << endl;
										return -1;
									}
								} else if (edgeRetValue == -1) {
									cout << "ERROR: While checking FIFO-NESTED-GEN edge from " << opI << " to " << opJ << endl;
									return -1;
								}
							} else {
								cout << "DEBUG: Cannot find deq op (or first op) of task " << taskEnqueued << "\n";
								cout << "DEBUG: Skipping FIFO-NESTED-GEN edge from op " << opI << "\n";
							}
						} else if (retValue == -1) {
							cout << "ERROR: While checking edge from " << resumeOp << " to " << enqJ << endl;
							return -1;
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
#ifdef SANITYCHECK
				if (taskOfResetOp.compare("") == 0) {
					cout << "DEBUG: Cannot find task of op " << resetOp << endl;
					cout << "DEBUG: Skipping ENQRESET-ST-1 edge for resume op " << resumeOp << "\n";
					continue;
				}
#endif

				IDType enqN = taskIDMap[taskOfResetOp].enqOpID;
#ifdef SANITYCHECK
				if (enqN <= 0) {
					cout << "DEBUG: Cannot find enq of reset task " << taskOfResetOp << endl;
					cout << "DEBUG: Skipping ENQRESET-ST-1 edge for resume op " << resumeOp << "\n";
					continue;
				}
#endif

				if (enqK == enqN) {
					cout << "DEBUG: Same task found to reset and resume a nesting loop\n";
					cout << "DEBUG: Skipping ENQRESET-ST-1 edge for resume op " << resumeOp << "\n";
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

				for (HBGraph::adjListNode *currNode = graph->blockAdjList[blockK].head; currNode != NULL; currNode = currNode->next) {
#ifdef SANITYCHECK
					assert(currNode->destination > 0);
#endif

					IDType blockJ = currNode->destination;
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

						int retValue1 = graph->opEdgeExists(enqK, enqL);
						if (retValue1 == 1) {
							int retValue2 = graph->opEdgeExists(enqL, enqN);
							if (retValue2 == 1) {
								std::string taskEnqueuedInL = enqToTaskEnqueued[enqL].taskEnqueued;
#ifdef SANITYCHECK
								assert(taskEnqueuedInL.compare("") != 0);
#endif
								opI = taskIDMap[taskEnqueuedInL].endOpID;

								if (opI <= 0) {
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
								}

								if (opI > 0) {
									opJ = resumeOp;

									int retValue3 = graph->opEdgeExists(opI, opJ);
									if (retValue3 == 0) {
										int addEdgeRetValue = graph->addOpEdge(opI, opJ);
										if (addEdgeRetValue == 1) {
											flag = true;
#ifdef GRAPHDEBUG
											cout << "ENQRESET-ST-1 edge (" << opI << ", " << opJ << ") -- #op-edges "   << graph->numOfOpEdges
																	       << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
										} else if (addEdgeRetValue == -1) {
											cout << "ERROR: While adding ENQRESET-ST-1 edge from " << opI << " to " << opJ << endl;
											return -1;
										}
									} else if (retValue3 == -1) {
										cout << "ERROR: While checking ENQRESET-ST-1 edge from " << opI << " to " << opJ << endl;
										return -1;
									}
								} else {
									cout << "DEBUG: Cannot find end op (or last op) of task " << taskEnqueuedInL << "\n";
									cout << "DEBUG: Skipping ENQRESET-ST-1 edge from end of task " << taskEnqueuedInL << "\n";
								}
							} else if (retValue2 == -1) {
								cout << "ERROR: While checking edge from " << enqL << " to " << enqN << endl;
								return -1;
							}
						} else if (retValue1 == -1) {
							cout << "ERROR: While checking edge from " << enqK << " to " << enqL << endl;
							return -1;
						}
					}
				}
			} else if (enqK <= 0) {
				cout << "DEBUG: Cannot find enq op of task " << it->first << "\n";
				cout << "DEBUG: Skipping ENQRESET-ST-1 edge for this task\n";
			} else if (resetOp <= 0) {
				cout << "DEBUG: Cannot find reset op for loop with pause " << prIt->pauseOp << " and resume " << prIt->resumeOp << "\n";
				cout << "DEBUG: Skipping ENQRESET-ST-1 edge\n";
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
#ifdef GRAPHDEBUG
				cout << "DEBUG: Cannot find reset op of nesting loop of shared variable " << it->first
					 << " with pause " << prrIt->pauseOp << " and resume " << prrIt->resumeOp << "\n";
				cout << "DEBUG: Skipping ENQRESET-ST-2/3 edges for this loop\n";
#endif
				continue;
			}

			IDType opM = prrIt->resumeOp;

			if (opM <= 0) {
#ifdef GRAPHDEBUG
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
				cout << "DEBUG: Reset op " << opK << " is not contained in a task\n";
				cout << "DEBUG: Skipping ENQRESET-ST-2/3 edges concerning this reset op\n";
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

			for (HBGraph::adjListNode *currNode = graph->blockAdjList[blockK].head;
					currNode != NULL; currNode = currNode->next) {
#ifdef SANITYCHECK
				assert(currNode->destination > 0);
#endif

				IDType blockJ = currNode->destination;
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

#ifdef GRAPHDEBUG
					cout << "DEBUG: checking op edge (" << opK << ", " << opL << ")\n";
#endif
					int retValue = graph->opEdgeExists(opK, opL);
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
						}

						// ENQRESET-ST-2
						opI = taskIDMap[taskM].endOpID;

						if (opI <= 0) {
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
						}

						if (opI > 0 && opJ > 0) {
#ifdef GRAPHDEBUG
							cout << "DEBUG: checking op edge (" << opI << ", " << opJ << ")\n";
#endif
							int edgeRetValue = graph->opEdgeExists(opI, opJ);
							if (edgeRetValue == 0) {
								int addEdgeRetValue = graph->addOpEdge(opI, opJ);
								if (addEdgeRetValue == 1) {
									flag = true;
#ifdef GRAPHDEBUG
									cout << "ENQRESET-ST-2 edge (" << opI << ", " << opJ << ") -- #op-edges "   << graph->numOfOpEdges
										 << " -- #block-edges " << graph->numOfBlockEdges << endl;
#endif
								} else if (addEdgeRetValue == -1) {
									cout << "ERROR: While adding ENQRESET-ST-2 edge from " << opI << " to " << opJ << endl;
									return -1;
								}
							} else if (edgeRetValue == -1) {
								cout << "ERROR: While checking ENQRESET-ST-2 edge from " << opI << " to " << opJ << endl;
								return -1;
							}
						} else if (opI <= 0) {
							cout << "DEBUG: Cannot find end of task " << taskM << "\n";
							cout << "DEBUG: Skipping ENQRESET-ST-2 edge from end of task\n";
						} else if (opJ <= 0) {
							cout << "DEBUG: Cannot find deq of task " << taskL << "\n";
							cout << "DEBUG: Skipping ENQRESET-ST-2 edge from deq of task\n";
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
									cout << "DEBUG: Cannot find next pause after the nesting loop of shared variable " << sharedVariable
										 << " with pause " << prIt->pauseOp << ", resume " << prIt->resumeOp << ", reset " << prIt->resetOp
										 << "\n";
//									continue;
									break;
								}

								opI = nextPause;
								foundFlag = true;
								break;
							}
						}

						if (foundFlag && opJ > 0) {
#ifdef GRAPHDEBUG
							cout << "DEBUG: checking op edge (" << opI << ", " << opJ << ")\n";
#endif
							int edgeRetValue = graph->opEdgeExists(opI, opJ);
							if (edgeRetValue == 0) {
								int addEdgeRetValue = graph->addOpEdge(opI, opJ);
								if (addEdgeRetValue == 1) {
									flag = true;
	#ifdef GRAPHDEBUG
									cout << "ENQRESET-ST-3 edge (" << opI << ", " << opJ << ") -- #op-edges "   << graph->numOfOpEdges
															     	 	 	   	   	     << " -- #block-edges " << graph->numOfBlockEdges << endl;
	#endif
								} else if (addEdgeRetValue == -1) {
									cout << "ERROR: While adding ENQRESET-ST-3 edge from " << opI << " to " << opJ << endl;
									return -1;
								}
							} else if (edgeRetValue == -1) {
								cout << "ERROR: While checking ENQRESET-ST-3 edge from " << opI << " to " << opJ << endl;
								return -1;
							}
						} else if (opJ <= 0) {
							cout << "DEBUG: Cannot find deq op (or first op) of task " << taskL << "\n";
							cout << "DEBUG: Skipping ENQRESET-ST-3 edge from op " << opI << " to this task\n";
						}
					} else if (retValue == -1) {
						cout << "ERROR: While checking edge from " << opK << " to " << opL << endl;
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
					cout << "ERROR: Invalid block edge from " << blockK << endl;
					return -1;
				}
#endif

				// If blockK == blockJ, we are trying to infer the same edges from blockI to blockK/blockJ
				// Redundant
				// Similarly, other cases
				if (blockI == blockK || blockK == blockJ || blockI == blockJ)
					continue;

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
				int addEdgeRetValue;
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
				while (opI > 0 && opI >= firstOp) {
					// Find the earliest op in blockK such that there exists edge (opI, op)
					IDType minOpInK = -1;
					opJ = -1;

					// Loop through adjacency list of opI
					for (HBGraph::adjListNode* currNode = graph->opAdjList[opI].head; currNode != NULL; currNode = currNode->next) {
						if (opIDMap[currNode->destination].blockID != blockK)
							continue;
						if (minOpInK == -1 || minOpInK > currNode->destination) {
							minOpInK = currNode->destination;
							// This is to remove multiple edges from opI to different ops in blockK
							graph->removeOpEdge(currNode, opI, currNode->destination);
						}
					}

					if (minOpInK > 0) {
						// Restore the edge from opI to the earliest op in blockK
						addEdgeRetValue = graph->addOpEdge(opI, minOpInK);
#ifdef GRAPHDEBUGFULL
						if (addEdgeRetValue == 1) {
							cout << "Restored edge from " << opI << " to " << minOpInK << endl;
						} else if (addEdgeRetValue == 0) {
							cout << "Did not add restoration edge from " << opI << " to " << minOpInK << endl;
						} else {
							cout << "ERROR: While adding restoration edge from " << opI << " to " << minOpInK << endl;
							return -1;
						}
#endif
					}

					for (IDType tempOp = minOpInK; tempOp > 0 && tempOp <= blockIDMap[blockK].lastOpInBlock; tempOp = opIDMap[tempOp].nextOpInBlock) {
						// Find the earliest op in blockJ such that there exists edge (tempOp, op)
						IDType minOpInJ = -1;

						// Loop through adjacency list of tempOp
						for (HBGraph::adjListNode* currNode = graph->opAdjList[tempOp].head; currNode != NULL; currNode = currNode->next) {
							if (opIDMap[currNode->destination].blockID != blockJ)
								continue;
							if (minOpInJ == -1 || minOpInJ > currNode->destination) {
								minOpInJ = currNode->destination;
								// This is to remove multiple edges from tempOp to different ops in blockJ
								graph->removeOpEdge(currNode, tempOp, currNode->destination);
							}
						}

						if (minOpInJ > 0) {
							// Restore the edge from tempOp to the earliest op in blockJ
							addEdgeRetValue = graph->addOpEdge(tempOp, minOpInJ);
#ifdef GRAPHDEBUGFULL
							if (addEdgeRetValue == 1) {
								cout << "Restored edge from " << tempOp << " to " << minOpInJ << endl;
							} else if (addEdgeRetValue == 0) {
								cout << "Did not add restoration edge from " << tempOp << " to " << minOpInJ << endl;
							} else {
								cout << "ERROR: While adding restoration edge from " << tempOp << " to " << minOpInJ << endl;
								return -1;
							}
#endif

							if (opJ == -1 || opJ > minOpInJ)
								opJ = minOpInJ;
						}
					}

					if (opJ > 0) {
						int retValue = graph->opEdgeExists(opI, opJ);
						if (retValue == 0) {
							addEdgeRetValue = graph->addOpEdge(opI, opJ);
							if (addEdgeRetValue == 1) {
								flag = true;
#ifdef SANITYCHECK
								cout << "TRANS-ST/MT Edge (" << opI << ", " << opJ << ") -- #opEdges " << graph->numOfOpEdges
																			   << " -- #blockEdges " << graph->numOfBlockEdges << endl;
#endif
								if (opI == blockIDMap[blockI].lastOpInBlock && opJ == blockIDMap[blockJ].firstOpInBlock)
									break;
							} else if (addEdgeRetValue == -1) {
								cout << "ERROR: While adding TRANS-ST/MT edge " << opI << " to " << opJ << endl;
								return -1;
							}
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

IDType  UAFDetector::findUAFwithoutAlloc(Logger &logger){

	bool flag = false;
	IDType uafCount = 0;

	// Loop through freeIDMap, for each free, find use that no HB edge

	for (map<IDType, freeOpDetails>::iterator freeIt = freeIDMap.begin(); freeIt != freeIDMap.end(); freeIt++) {
		IDType freeID = freeIt->first;
		IDType allocID = freeIt->second.allocOpID;

		if (allocID == -1) {
			cout << "DEBUG: Cannot find alloc for free op " << freeID << endl;
//			continue;
		}
#ifdef ACCESS
		for (set<IDType>::iterator accessIt = freeIt->second.accessOps.begin(); accessIt != freeIt->second.accessOps.end(); accessIt++) {
			IDType accessID = *accessIt;

			if (graph->edgeExists(freeID, accessID) == 1) {
				cout << "Definite UAF between access op " << accessID << " (access at address " << accessSet[accessID].startingAddress << ") "
					 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
					 << ")\n";
				cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
					 << allocSet[allocID].startingAddress << ")\n";
				uafCount++;
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
				uafCount++;
				flag = true;
				continue;
			}
		}

#else
		for (set<IDType>::iterator readIt = freeIt->second.readOps.begin(); readIt != freeIt->second.readOps.end(); readIt++) {
			IDType readID = *readIt;

			if (graph->opEdgeExists(freeID, readID) == 1) {
				cout << "Definite UAF between read op " << readID << " (read at address " << readSet[readID].startingAddress << ") "
					 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
					 << ")\n";
				if (allocID > 0) {
					cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
						 << allocSet[allocID].startingAddress << ")\n";
				}
				uafCount++;
				flag = true;
				continue;
			}

			if (graph->opEdgeExists(readID, freeID) == 0) {

#ifdef ADDITIONS
				if (allocID > 0) {
					// Even if there is no edge between read and free,
					// if there is an edge from alloc to read (alloc happens before read), and the alloc is in the same task as the read and the task is atomic, this is a false positive.
					// This is true only if free is in the same thread as alloc and read.
					if (graph->opEdgeExists(allocID, readID) && opIDMap[allocID].taskID.compare(opIDMap[readID].taskID) == 0
							&& taskIDMap[opIDMap[allocID].taskID].atomic && opIDMap[freeID].threadID == opIDMap[readID].threadID)
						continue;
				}
#endif
				cout << "Potential UAF between read op " << readID << " (read at address " << readSet[readID].startingAddress << ") "
					 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
					 << ")\n";
				if (allocID > 0) {
					cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
						 << allocSet[allocID].startingAddress << ")\n";
				}
				uafCount++;
				flag = true;
				continue;
			}
		}

		for (set<IDType>::iterator writeIt = freeIt->second.writeOps.begin(); writeIt != freeIt->second.writeOps.end(); writeIt++) {
			IDType writeID = *writeIt;

			if (graph->opEdgeExists(freeID, writeID) == 1) {
				cout << "Definite UAF between write op " << writeID << " (write at address " << writeSet[writeID].startingAddress << ") "
					 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
					 << ")\n";
				cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
					 << allocSet[allocID].startingAddress << ")\n";
				uafCount++;
				flag = true;
				continue;
			}

			if (graph->opEdgeExists(writeID, freeID) == 0) {

#ifdef ADDITIONS
				if (allocID > 0) {
					// Even if there is no edge between write and free,
					// if there is an edge from alloc to write (alloc happens before write), and the alloc is in the same task as the write and the task is atomic, this is a false positive.
					// This is true only if free is in the same thwrite as alloc and write.
					if (graph->opEdgeExists(allocID, writeID) && opIDMap[allocID].taskID.compare(opIDMap[writeID].taskID) == 0
							&& taskIDMap[opIDMap[allocID].taskID].atomic && opIDMap[freeID].threadID == opIDMap[writeID].threadID)
						continue;
				}
#endif

				cout << "Potential UAF between write op " << writeID << " (write at address " << writeSet[writeID].startingAddress << ") "
					 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
					 << ")\n";
				if (allocID > 0) {
					cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
						 << allocSet[allocID].startingAddress << ")\n";
				}
				uafCount++;
				flag = true;
				continue;
			}
		}
#endif
	}

	if (flag)
		return uafCount;
	else
		return 0;
}

#ifndef ACCESS
IDType UAFDetector::findDataRaces(Logger &logger){

	bool flag = false;
	IDType raceCount = 0;

	for (map<IDType, allocOpDetails>::iterator allocIt = allocIDMap.begin(); allocIt != allocIDMap.end(); allocIt++) {
		for (set<IDType>::iterator writeIt = allocIt->second.writeOps.begin(); writeIt != allocIt->second.writeOps.end(); writeIt++) {

			string writeAddress1 = writeSet[*writeIt].startingAddress;

			// write-write races
			for (set<IDType>::iterator write2It = allocIt->second.writeOps.begin(); write2It != allocIt->second.writeOps.end(); write2It++) {
				if (*write2It == *writeIt) continue;

				string writeAddress2 = writeSet[*write2It].startingAddress;
				if (writeAddress1.compare(writeAddress2) != 0) continue;

				if (graph->opEdgeExists(*writeIt, *write2It) == 0 && graph->opEdgeExists(*write2It, *writeIt) == 0) {
					cout << "Potential data race between write ops " << *writeIt << " and " << *write2It << " on address "
						 << writeAddress1 << endl;
					raceCount++;
					flag = true;
					continue;
				}
			}

			// write-read / read-write races
			for (set<IDType>::iterator readIt = allocIt->second.readOps.begin(); readIt != allocIt->second.readOps.end(); readIt++) {
				if (*readIt == *writeIt) continue;

				string readAddress = readSet[*readIt].startingAddress;
				if (writeAddress1.compare(readAddress) != 0) continue;

				if (graph->opEdgeExists(*writeIt, *readIt) == 0 && graph->opEdgeExists(*readIt, *writeIt) == 0) {
					cout << "Potential data race between read op " << *readIt << " and write op " << *writeIt << " on address "
						 << readAddress << endl;
					raceCount++;
					flag = true;
					continue;
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
