/*
 * UAFDetector.cpp
 *
 *  Created on: 30-May-2014
 *      Author: shalini
 */

#include "UAFDetector.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <boost/regex.hpp>

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
	uniqueRaceCount = 0;
	uniqueUafCount = 0;
}

UAFDetector::~UAFDetector() {
}

void UAFDetector::initGraph(IDType countOfNodes, IDType countOfBlocks) {
	graph = new HBGraph(countOfNodes, countOfBlocks, opIDMap, blockIDMap, nodeIDMap);
	assert(graph != NULL);
}

struct field {
	std::string address;
	IDType minUseOp;
	std::string minMsg;
};

bool isFieldPresent(std::vector<field> fieldsVector, std::string address) {
	for (std::vector<field>::iterator it = fieldsVector.begin();
			it != fieldsVector.end(); it++) {
		if (it->address.compare(address) == 0)
			return true;
	}

	return false;
}

void UAFDetector::outputAllConflictingOps(std::string outUAFFileName, std::string outUAFUniqueFileName,
		std::string outRaceFileName, std::string outRaceUniqueFileName) {
	Logger outUAF, outRace;
	outUAF.init(outUAFFileName);
	outRace.init(outRaceFileName);

	Logger outUAFUniq, outRaceUniq;
	outUAFUniq.init(outUAFUniqueFileName);
	outRaceUniq.init(outRaceUniqueFileName);

	IDType totalUAFCount = 0, totalRacesCount = 0;
	IDType totalObjsWithUAFCount = 0, totalObjsWithRacesCount = 0;
	std::stringstream str;

	for (std::map<IDType, allocOpDetails>::iterator allocIt = allocIDMap.begin();
			allocIt != allocIDMap.end(); allocIt++) {
		IDType allocID = allocIt->first;
		IDType uafsCount = 0, racesCount = 0;

		str.str("");
		str.clear();
		str << "Object " << allocID << "\n";
		std::string objmsg = str.str();

		std::string msg, minMsg;
		IDType minUse = -1;

		std::vector<field> fieldsVector;

		bool firstWrite = true;

		for (std::set<IDType>::iterator freeIt = allocIt->second.freeOps.begin();
				freeIt != allocIt->second.freeOps.end(); freeIt++) {
			IDType freeID = *freeIt;
			IDType threadFree = opIDMap[freeID].threadID;
			for (std::set<IDType>::iterator readIt = allocIt->second.readOps.begin();
					readIt != allocIt->second.readOps.end(); readIt++) {
				IDType readID = *readIt;
				IDType threadRead = opIDMap[readID].threadID;
				std::string address = readSet[readID].startingAddress;

				if (threadFree == threadRead) continue;

//				if (isFieldPresent(fieldsVector, address));

				if (firstWrite) {
					outUAF.writeLog(objmsg);
					firstWrite = false;
				}

				str.str("");
				str.clear();
				str << readID << " " << threadRead << " " << freeID << " " << threadFree << "\n";
				msg = str.str();
				outUAF.writeLog(msg);

				if (minUse == -1 || minUse > readID) {
					minUse = readID;
					minMsg = msg;
				}

				uafsCount++;
			}

			for (std::set<IDType>::iterator writeIt = allocIt->second.writeOps.begin();
					writeIt != allocIt->second.writeOps.end(); writeIt++) {
				IDType writeID = *writeIt;
				IDType threadWrite = opIDMap[writeID].threadID;

				if (threadFree == threadWrite) continue;

				if (firstWrite) {
					outUAF.writeLog(objmsg);
					firstWrite = false;
				}

				str.str("");
				str.clear();
				str << writeID << " " << threadWrite << " " << freeID << " " << threadFree << "\n";
				msg = str.str();
				outUAF.writeLog(msg);

				if (minUse == -1 || minUse > writeID) {
					minUse = writeID;
					minMsg = msg;
				}

				uafsCount++;
			}
		}

		if (minUse != -1) {
			outUAFUniq.writeLog(minMsg);
			totalObjsWithUAFCount++;
		}

		firstWrite = true;
		minUse = -1;
		for (std::set<IDType>::iterator wIt = allocIt->second.writeOps.begin();
				wIt != allocIt->second.writeOps.end(); wIt++) {
			IDType writeOp = *wIt;
			IDType threadWrite = opIDMap[writeOp].threadID;
			std::string writeAddress = writeSet[writeOp].startingAddress;
			for (std::set<IDType>::iterator rIt = allocIt->second.readOps.begin();
					rIt != allocIt->second.readOps.end(); rIt++) {
				IDType readOp = *rIt;
				IDType threadRead = opIDMap[readOp].threadID;
				std::string readAddress = readSet[readOp].startingAddress;

				if (threadWrite == threadRead) continue;
				if (writeAddress.compare(readAddress) != 0) continue;

				if (firstWrite) {
					outRace.writeLog(objmsg);
					firstWrite = false;
				}

				str.str("");
				str.clear();
				str << readOp << " " << threadRead << " " << writeOp << " " << threadWrite << "\n";
				msg = str.str();
				outRace.writeLog(msg);

				if (minUse == -1 || minUse > readOp) {
					minUse = readOp;
					minMsg = msg;
				}
				if (minUse > writeOp) {
					minUse = writeOp;
					minMsg = msg;
				}

				racesCount++;
			}
			for (std::set<IDType>::iterator w2It = allocIt->second.writeOps.begin();
					w2It != allocIt->second.writeOps.end(); w2It++) {
				IDType write2Op = *w2It;
				IDType threadWrite2 = opIDMap[write2Op].threadID;
				std::string write2Address = writeSet[write2Op].startingAddress;

				if (writeOp == write2Op) continue;
				if (threadWrite == threadWrite2) continue;
				if (writeAddress.compare(write2Address) != 0) continue;

				if (firstWrite) {
					outRace.writeLog(objmsg);
					firstWrite = false;
				}

				str.str("");
				str.clear();
				str << writeOp << " " << threadWrite << " " << write2Op << " " << threadWrite2 << "\n";
				msg = str.str();
				outRace.writeLog(msg);

				if (minUse == -1 || minUse > writeOp) {
					minUse = writeOp;
					minMsg = msg;
				}
				if (minUse > write2Op) {
					minUse = write2Op;
					minMsg = msg;
				}

				racesCount++;
			}
		}
		if (minUse != -1) {
			outRaceUniq.writeLog(minMsg);
			totalObjsWithRacesCount++;
		}

		cout << "Object-" << allocID << ":uafs-" << uafsCount << ":races-"
			 << racesCount << "\n";
		totalUAFCount += uafsCount;
		totalRacesCount += racesCount;
	}

	cout << "Total objects: " << allocIDMap.size() << "\n";
	cout << "Objects with uafs: " << totalObjsWithUAFCount << "\n";
	cout << "Total uafs: " << totalUAFCount << "\n";
	cout << "Objects with races: " << totalObjsWithRacesCount << "\n";
	cout << "Total races: " << totalRacesCount << "\n";
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
//		cout << "Transitive with criterion";
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
				int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ, false);
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
				int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ, false);
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
					int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ, true);
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
					int addEdgeRetValue = graph->addOpEdge(nodeJ, nodeI, true);
					if (addEdgeRetValue == 1) {
						flag = true;
//#ifdef GRAPHDEBUG
						cout << "LOOP-PO edge (" << nodeJ << ", " << nodeI << ") -- #op-edges "   << graph->numOfOpEdges
							 << " -- #block-edges " << graph->numOfBlockEdges << endl;
//#endif
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

	for (map<std::string, UAFDetector::taskDetails>::iterator it = taskIDMap.begin(); it != taskIDMap.end(); it++) {

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
				IDType threadEnq = opIDMap[enqOp].threadID;
				IDType threadDeq = opIDMap[deqOp].threadID;
				if (threadEnq < 0) {
					cout << "ERROR: Invalid thread ID for op " << enqOp << "\n";
					return -1;
				}
				if (threadDeq < 0) {
					cout << "ERROR: Invalid thread ID for op " << deqOp << "\n";
					return -1;
				}
				bool edgeType;
				if (threadEnq == threadDeq)
					edgeType = true;
				else
					edgeType = false;
				int addEdgeRetValue = graph->addOpEdge(nodeEnq, nodeDeq, edgeType);
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
//		IDType lastBlockInTask = it->second.lastBlockID;

#ifdef SANITYCHECK
		if (firstBlockInTask <= 0) {
#ifdef GRAPHDEBUGFULL
			cout << "DEBUG: Cannot find first block of task " << it->first << endl;
			cout << "DEBUG: Skipping TASK-PO edges for this task\n";
#endif
//			return -1;
			continue;
		}
#if 0
		if (lastBlockInTask <= 0) {
#ifdef GRAPHDEBUGFULL
			cout << "DEBUG: Cannot find last block of task " << it->first << endl;
			cout << "DEBUG: Skipping TASK-PO edges for this task\n";
#endif
//			return -1;
			continue;
		}
#endif
#endif

//		for (IDType b1 = firstBlockInTask; (b1 > 0 && b1 <= lastBlockInTask); b1 = blockIDMap[b1].nextBlockInTask) {
		for (IDType b1 = firstBlockInTask; (b1 > 0); b1 = blockIDMap[b1].nextBlockInTask) {
#ifdef SANITYCHECK
			assert(b1 > 0);
#endif
			IDType nextBlock = blockIDMap[b1].nextBlockInTask;
//			for (IDType b2 = nextBlock; (b2 > 0 && b2 <= lastBlockInTask); b2 = blockIDMap[b2].nextBlockInTask) {
			for (IDType b2 = nextBlock; b2 > 0; b2 = blockIDMap[b2].nextBlockInTask) {
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
					int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ, true);
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


	for (map<std::string, taskDetails>::iterator it = taskIDMap.begin(); it != taskIDMap.end(); it++) {

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

							std::string taskName = enqToTaskEnqueued[tempenqOp].taskEnqueued;
							if (taskName.compare("") == 0) {
#ifdef GRAPHDEBUGFULL
								cout << "DEBUG: Cannot find task enqueued in op " << tempenqOp << endl;
#endif
								continue;
							}

							// FIFO-ATOMIC does not apply if the two tasks are not posted to the same thread
							if (enqToTaskEnqueued[enqOp].targetThread != enqToTaskEnqueued[tempenqOp].targetThread) continue;

							// If the enqs are not of the same priority, FIFO-ATOMIC does not apply.
							if (enqToTaskEnqueued.find(tempenqOp) == enqToTaskEnqueued.end()) {
								cout << "ERROR: Cannot find entry for op " << tempenqOp << " in enqToTaskEnqueued\n";
								return -1;
							}
							if (it->second.priority == INT_MAX) {
								cout << "ERROR: Priority is not set for task " << it->first << "\n";
								return -1;
							}
							if (enqToTaskEnqueued[tempenqOp].priority == INT_MAX) {
								cout << "ERROR: Priority is not set in enq op " << tempenqOp << "\n";
								return -1;
							}
							if (it->second.priority != enqToTaskEnqueued[tempenqOp].priority) continue;

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
										int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ, true);
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

							std::string taskName = enqToTaskEnqueued[tempenqOp].taskEnqueued;
							if (taskName.compare("") == 0) {
#ifdef GRAPHDEBUGFULL
								cout << "DEBUG: Cannot find task enqueued in op " << tempenqOp << endl;
#endif
//								continue;
								return -1;
							}

							// NO-PRE does not apply if the two tasks are in different threads
							if (opIDMap[i].threadID != enqToTaskEnqueued[tempenqOp].targetThread) continue;

							// NO-PRE does not apply if the first op is an enq and it does not have the same priority
							// as the second enqop.
							if (opIDMap[i].opType.compare("enq") == 0) {
								if (enqToTaskEnqueued.find(i) == enqToTaskEnqueued.end()) {
									cout << "ERROR: Cannot find entry for op " << i << " in enqToTaskEnqueued\n";
									return -1;
								}
								if (enqToTaskEnqueued.find(tempenqOp) == enqToTaskEnqueued.end()) {
									cout << "ERROR: Cannot find entry for op " << tempenqOp << " in enqToTaskEnqueued\n";
									return -1;
								}
								if (enqToTaskEnqueued[i].priority == INT_MAX) {
									cout << "ERROR: Priority not set in enq op " << i << "\n";
									return -1;
								}
								if (enqToTaskEnqueued[tempenqOp].priority == INT_MAX) {
									cout << "ERROR: Priority not set in enq op " << tempenqOp << "\n";
									return -1;
								}
								if (enqToTaskEnqueued[i].priority != enqToTaskEnqueued[tempenqOp].priority)
									continue;
							}

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
										int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ, true);
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
						int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ, false);
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
								int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ, true);
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
						int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ, false);
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
								int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ, true);
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
			IDType threadI = opIDMap[opI].threadID;
			IDType threadJ = opIDMap[opJ].threadID;
			if (threadI < 0) {
				cout << "ERROR: Cannot find threadID of op " << opI << "\n";
				return -1;
			}
			if (threadJ < 0) {
				cout << "ERROR: Cannot find threadID of op " << opJ << "\n";
				return -1;
			}

			if (threadI == threadJ) continue;

			IDType nodeI = opIDMap[opI].nodeID;
			IDType nodeJ = opIDMap[opJ].nodeID;
			if (nodeI <= 0) {
				cout << "ERROR: Invalid node ID for op " << opI << "\n";
				return -1;
			} else if (nodeJ <= 0) {
				cout << "ERROR: Invalid node ID for op" << opJ << "\n";
				return -1;
			} else {
				int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ, false);
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
				IDType threadI = opIDMap[opI].threadID;
				IDType threadJ = opIDMap[opJ].threadID;
				if (threadI < 0) {
					cout << "ERROR: Cannot find threadID of op " << opI << "\n";
					return -1;
				}
				if (threadJ < 0) {
					cout << "ERROR: Cannot find threadID of op " << opJ << "\n";
					return -1;
				}

				if (threadI == threadJ) continue;

				IDType nodeI = opIDMap[opI].nodeID;
				IDType nodeJ = opIDMap[opJ].nodeID;
				if (nodeI <= 0) {
					cout << "ERROR: Invalid node ID for op " << opI << "\n";
					return -1;
				} else if (nodeJ <= 0) {
					cout << "ERROR: Invalid node ID for op " << opJ << "\n";
					return -1;
				} else {
#ifdef GRAPHDEBUG
					cout << "DEBUG: Adding edge from " << nodeI << " to " << nodeJ << "\n";
#endif
					int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ, false);
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
			for (std::multiset<HBGraph::adjListNode>::iterator blockIt = graph->blockAdjList[blockI].begin();
					blockIt != graph->blockAdjList[blockI].end(); blockIt++) {
#ifdef SANITYCHECK
				assert(blockIt->blockID > 0);
#endif

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

					// If the first enq does not have the same priority as the second, then the rule does not apply.
					if (enqToTaskEnqueued.find(enqI) == enqToTaskEnqueued.end()) {
						cout << "ERROR: Cannot find entry for op " << enqI << " in enqToTaskEnqueued\n";
						return -1;
					}
					if (enqToTaskEnqueued.find(enqJ) == enqToTaskEnqueued.end()) {
						cout << "ERROR: Cannot find entry for op " << enqJ << " in enqToTaskEnqueued\n";
						return -1;
					}
					if (enqToTaskEnqueued[enqI].priority == INT_MAX) {
						cout << "ERROR: Priority not set in enq op " << enqI << "\n";
						return -1;
					}
					if (enqToTaskEnqueued[enqJ].priority == INT_MAX) {
						cout << "ERROR: Priority not set in enq op " << enqJ << "\n";
						return -1;
					}
					if (enqToTaskEnqueued[enqI].priority != enqToTaskEnqueued[enqJ].priority)
						continue;

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
								int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ, true);
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
					for (std::multiset<HBGraph::adjListNode>::iterator blockIt = graph->blockAdjList[blockL].begin();
							blockIt != graph->blockAdjList[blockL].end(); blockIt++) {
#ifdef SANITYCHECK
						assert(blockIt->blockID > 0);
#endif

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
											int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ, true);
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
				for (std::multiset<HBGraph::adjListNode>::iterator blockIt = graph->blockAdjList[blockOfResumeOp].begin();
						blockIt != graph->blockAdjList[blockOfResumeOp].end(); blockIt++) {
#ifdef SANITYCHECK
					assert(blockIt->blockID > 0);
#endif

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
								if (opI <= 0) {
#ifdef GRAPHDEBUGFULL
									cout << "DEBUG: Cannot find pause op after resume op " << resumeOp << endl;
									cout << "DEBUG: Skipping FIFO-NESTED-GEN edge from resume op " << resumeOp << "\n";
#endif
									continue;
								}

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
										int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ, true);
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

				for (std::multiset<HBGraph::adjListNode>::iterator blockKIt = graph->blockAdjList[blockK].begin();
						blockKIt != graph->blockAdjList[blockK].end(); blockKIt++) {
#ifdef SANITYCHECK
					assert(blockKIt->blockID > 0);
#endif

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

						// If priority of enqK is not the same as priority of enqL or
						// priority of enqL is not the same as priority of enqN, then the rule does not apply
						if (enqToTaskEnqueued.find(enqK) == enqToTaskEnqueued.end()) {
							cout << "ERROR: Cannot find entry for op " << enqK << " in enqToTaskEnqueued\n";
							return -1;
						}
						if (enqToTaskEnqueued.find(enqL) == enqToTaskEnqueued.end()) {
							cout << "ERROR: Cannot find entry for op " << enqL << " in enqToTaskEnqueued\n";
							return -1;
						}
						if (enqToTaskEnqueued.find(enqN) == enqToTaskEnqueued.end()) {
							cout << "ERROR: Cannot find entry for op " << enqN << " in enqToTaskEnqueued\n";
							return -1;
						}
						if (enqToTaskEnqueued[enqK].priority == INT_MAX) {
							cout << "ERROR: Priority not set in enq op " << enqK << "\n";
							return -1;
						}
						if (enqToTaskEnqueued[enqL].priority == INT_MAX) {
							cout << "ERROR: Priority not set in enq op " << enqL << "\n";
							return -1;
						}
						if (enqToTaskEnqueued[enqN].priority == INT_MAX) {
							cout << "ERROR: Priority not set in enq op " << enqN << "\n";
							return -1;
						}
						if (enqToTaskEnqueued[enqK].priority != enqToTaskEnqueued[enqL].priority ||
								enqToTaskEnqueued[enqL].priority != enqToTaskEnqueued[enqN].priority)
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
											int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ, true);
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

			// taskM needs to be the parent of taskK
			if (taskM.compare(taskIDMap[taskK].parentTask) != 0)
				continue;

			IDType enqOfReset = taskIDMap[taskK].enqOpID;
			if (enqOfReset <= 0) {
#ifdef GRAPHDEBUG
				cout << "ERROR: Cannot find enq of task " << taskK << "\n";
#endif
				continue;
			}

//			IDType blockK = opIDMap[opK].blockID;
			IDType blockK = opIDMap[enqOfReset].blockID;
#ifdef SANITYCHECK
			if (blockK <= 0) {
//				cout << "ERROR: Cannot find block of op " << opK << "\n";
				cout << "ERROR: Cannot find block of op " << enqOfReset << "\n";
				return -1;
			}
#endif

			for (std::multiset<HBGraph::adjListNode>::iterator blockKIt = graph->blockAdjList[blockK].begin();
					blockKIt != graph->blockAdjList[blockK].end(); blockKIt++) {
#ifdef SANITYCHECK
				assert(blockKIt->blockID > 0);
#endif

				IDType blockJ = blockKIt->blockID;
				for (set<IDType>::iterator enqIt = blockIDMap[blockJ].enqSet.begin(); enqIt != blockIDMap[blockJ].enqSet.end();
						enqIt++) {
#ifdef SANITYCHECK
					assert(*enqIt > 0);
#endif
					IDType opL = *enqIt;
					if (opL == enqOfReset)
						continue;
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

					// If reset task does not have the same the priority as the second enq, the rule does not apply
					if (enqToTaskEnqueued.find(enqOfReset) == enqToTaskEnqueued.end()) {
						cout << "ERROR: Cannot find entry for op " << enqOfReset << " in enqToTaskEnqueued\n";
						return -1;
					}
					if (enqToTaskEnqueued.find(opL) == enqToTaskEnqueued.end()) {
						cout << "ERROR: Cannot find entry for op " << opL << " in enqToTaskEnqueued\n";
						return -1;
					}
					if (enqToTaskEnqueued[enqOfReset].priority == INT_MAX) {
						cout << "ERROR: Priority not set in enq op " << enqOfReset << "\n";
						return -1;
					}
					if (enqToTaskEnqueued[opL].priority == INT_MAX) {
						cout << "ERROR: Priority not set in enq op " << opL << "\n";
						return -1;
					}
					if (enqToTaskEnqueued[enqOfReset].priority != enqToTaskEnqueued[opL].priority)
						continue;

#ifdef GRAPHDEBUGFULL
					cout << "DEBUG: checking op edge (" << opK << ", " << opL << ")\n";
#endif
//					IDType nodeK = opIDMap[opK].nodeID;
					IDType nodeK = opIDMap[enqOfReset].nodeID;
					IDType nodeL = opIDMap[opL].nodeID;
					if (nodeK <= 0) {
//						cout << "ERROR: Invalid node ID for op " << opK << "\n";
						cout << "ERROR: Invalid node ID for op " << enqOfReset << "\n";
						return -1;
					} else if (nodeL <= 0) {
						cout << "ERROR: Invalid node ID for op " << opL << "\n";
						return -1;
					} else if (nodeK == nodeL) {
#ifdef GRAPHDEBUG
						cout << "DEBUG: Comparing same enq ops: " << opL << "\n";
#endif
						continue;
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

							if (opM == lastResumeOftaskM) {
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
										int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ, true);
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

							} else {
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
//											continue;
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
										int addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ, true);
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

		for (std::multiset<HBGraph::adjListNode>::iterator blockIIt = graph->blockAdjList[blockI].begin();
				blockIIt != graph->blockAdjList[blockI].end(); blockIIt++) {
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
				int edgeType1, edgeType2; // used to check the transitivity criterion
				opI = blockIDMap[blockI].lastOpInBlock;
				firstOp = blockIDMap[blockI].firstOpInBlock;
#ifdef SANITYCHECK
				if (opI <= 0) {
					cout << "ERROR: Cannot find last op of block " << blockI << endl;
					return -1;
				}
				if (firstOp <= 0) {
					cout << "ERROR: Cannot find first op of block " << blockI << endl;
					return -1;
				}
#endif
				IDType nodeI = 0;
				IDType prevNodeI = 0;

//				bool validTransEdge = true;
				while (opI > 0 && opI >= firstOp) {
					// Find the earliest op in blockK such that there exists edge (opI, op)
					IDType minOpInK = -1, minNodeInK = -1;
					bool edgeTypeOfMinNodeInK;
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
								edgeTypeOfMinNodeInK = graph->isSTEdge(nodeI, minNodeInK);
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
								addEdgeRetValue = graph->addOpEdge(nodeI, minNodeInK, edgeTypeOfMinNodeInK);
								if (addEdgeRetValue == 1) {
#ifdef GRAPHDEBUGFULL
									cout << "Restored edge from " << nodeI
										 << " to " << minNodeInK << "\n";
#endif
								} else if (addEdgeRetValue == 0) {
#ifdef GRAPHDEBUGFULL
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

					edgeType1 = edgeTypeOfMinNodeInK;

					IDType nodeTempOp = 0;
					IDType prevNodeTempOp = 0;

					for (IDType tempOp = minOpInK;
							tempOp > 0 && tempOp <= blockIDMap[blockK].lastOpInBlock;
							tempOp = opIDMap[tempOp].nextOpInBlock) {
						// Find the earliest op in blockJ such that there exists edge (tempOp, op)
						IDType minOpInJ = -1, minNodeInJ = -1;
						bool edgeTypeOfMinNodeInJ;

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
									edgeTypeOfMinNodeInJ = graph->isSTEdge(nodeTempOp, minNodeInJ);
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
									addEdgeRetValue = graph->addOpEdge(nodeTempOp, minNodeInJ, edgeTypeOfMinNodeInJ);
									if (addEdgeRetValue == 1) {
#ifdef GRAPHDEBUGFULL
										cout << "Restored edge from " << nodeTempOp << " to " << minNodeInJ << endl;
#endif
									} else if (addEdgeRetValue == 0) {
#ifdef GRAPHDEBUGFULL
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

						edgeType2 = edgeTypeOfMinNodeInJ;
					}

					if (opJ > 0) {
						IDType nodeJ = opIDMap[opJ].nodeID;
						if (nodeJ <= 0) {
							cout << "ERROR: Invalid node ID for op " << opJ << "\n";
							return -1;
						}
						bool transEdgeType;
						if (threadI != threadJ)
							transEdgeType = false;
						else if (edgeType1 && edgeType2)
							transEdgeType = true;
						else {
							// threadI == threadJ and the contributing edges are not ST
//							validTransEdge = false;
							// Not a valid transitive edge
							cout << "Not a valid transitive edge " << nodeI << ", " << nodeJ << "\n";
							opI = opIDMap[opI].prevOpInBlock;
							continue;
						}
						addEdgeRetValue = graph->addOpEdge(nodeI, nodeJ, transEdgeType);
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

int UAFDetector::filterInput(std::string inFileName, std::string outFileName) {
	ifstream inFile;
	inFile.open(inFileName.c_str(), ios_base::in);
	if (!inFile.is_open()) {
		cout << "ERROR: Cannot open input file: " << inFileName << "\n";
		return -1;
	}

	Logger out;
	out.init(outFileName);

	std::string line;
	std::string lineRegEx = "^ *([0-9]+) *([0-9]+) *([0-9]+) *([0-9]+) *$";
	std::string objRegEx = "^ *Object *([0-9]+) *$";
	boost::regex reg, regObj;
	boost::cmatch matches;

	std::stringstream str;

	try {
		reg.assign(lineRegEx, boost::regex_constants::icase);
	} catch (boost::regex_error &e) {
		cout << lineRegEx << " is not a valid regular expression: \""
			 << e.what() << "\"\n";
		return -1;
	}
	try {
		regObj.assign(objRegEx, boost::regex_constants::icase);
	} catch (boost::regex_error &e) {
		cout << objRegEx << " is not a valid regular expression: \""
			 << e.what() << "\"\n";
		return -1;
	}

//	bool newAlloc = false;
	IDType allocID = -1;
	long int allocRacesCount = 0;
	long int totalRacesCount = 0;
	while (getline(inFile, line)) {
		if (boost::regex_match(line.c_str(), matches, regObj)) {
			// Print out the stats for previous alloc
			if (allocID != -1)
				cout << "Object-" << allocID << ":races-" << allocRacesCount << "\n";
			allocID = -1;
			for (unsigned i=1; i < matches.size(); i++) {
				std::string match(matches[i].first, matches[i].second);
				if (!match.empty() && match.compare("") != 0) {
					allocID = atoi(match.c_str());
					break;
				}
			}
			if (allocID == -1) {
				cout << "ERROR: Cannot obtain allocID in line " << line << "\n";
				return -1;
			}
//			newAlloc = true;
			totalRacesCount += allocRacesCount;
			allocRacesCount = 0;

			str.clear();
			str << "Object " << allocID << "\n";
			out.writeLog(str.str());
			continue;
		} //else
//			newAlloc = false;

		if (!boost::regex_match(line.c_str(), matches, reg)) {
			cout << "ERROR: Line in input file is not valid\n";
			cout << line << "\n";
			return -1;
		}

		IDType useOp  = -1;
		IDType freeOp = -1;
		IDType threadUse = -1;
		IDType threadFree = -1;

		unsigned i;
		for (i=1; i < matches.size(); i++) {
			std::string match(matches[i].first, matches[i].second);
			if (!match.empty() && match.compare("") != 0) {
				useOp = atoi(match.c_str());
				break;
			}
		}

		for (i=i+1; i < matches.size(); i++) {
			std::string match(matches[i].first, matches[i].second);
			if (!match.empty() && match.compare("") != 0) {
				threadUse = atoi(match.c_str());
				break;
			}
		}

		for (i=i+1; i < matches.size(); i++) {
			std::string match(matches[i].first, matches[i].second);
			if (!match.empty() && match.compare("") != 0) {
				freeOp = atoi(match.c_str());
				break;
			}
		}

		for (i=i+1; i < matches.size(); i++) {
			std::string match(matches[i].first, matches[i].second);
			if (!match.empty() && match.compare("") != 0) {
				threadFree = atoi(match.c_str());
				break;
			}
		}

		if (useOp == -1) {
			cout << "Cannot find use op\n";
			return -1;
		}
		if (freeOp == -1) {
			cout << "Cannot find free op\n";
			return -1;
		}

		IDType useNode = opIDMap[useOp].nodeID;
		IDType freeNode = opIDMap[freeOp].nodeID;
		bool useToFree = false;
		bool freeToUse = false;
		if (useNode == freeNode && freeOp < useOp) {
			useToFree = false;
			freeToUse = true;
		} else if (useNode == freeNode) {
			useToFree = true;
			freeToUse = false;
		} else {
			useToFree = graph->opEdgeExists(useNode, freeNode);
			freeToUse = graph->opEdgeExists(freeNode, useNode);
		}

		if (!useToFree && freeToUse) {
			str.str("");
			str.clear();
			str << useOp << " " << threadUse << " " << freeOp << " " << threadFree << "\n";
			out.writeLog(str.str());
			allocRacesCount++;
			continue;
		}

		if (!useToFree && !freeToUse) {
			str.str("");
			str.clear();
			str << useOp << " " << threadUse << " " << freeOp << " " << threadFree << "\n";
			out.writeLog(str.str());
			allocRacesCount++;
			continue;
		}
	}

//	cout << "TotalRaces: " << totalRacesCount << "\n";

	return 0;
}

void UAFDetector::getRaceKind(UAFDetector::raceDetails &race) {
	assert(race.op1 > 0 && race.op2 > 0);

	race.op1Task = opIDMap[race.op1].taskID;
	race.op2Task = opIDMap[race.op2].taskID;

	if (race.op1Task.compare("") == 0 || race.op2Task.compare("") == 0) {
		if (opIDMap[race.op1].threadID != opIDMap[race.op2].threadID)
			race.raceType = NOTASKRACE_MULTITHREADED;
		else
			race.raceType = NOTASKRACE_SINGLETHREADED;
		return;
	}

	if (race.op1Task.compare(race.op2Task) == 0) {
		cout << "ERROR: Racing ops are in the same task\n";
		cout << "ERROR: Op: " << race.op1 << ", " << race.op2 << "\n";
		return;
	}

	if (opIDMap[race.op1].threadID != opIDMap[race.op2].threadID) {
		race.raceType = MULTITHREADED;
		return;
	}

	if ((taskIDMap[race.op1Task].parentTask.compare("") != 0) && (taskIDMap[race.op2Task].parentTask.compare("") != 0)
			&& (taskIDMap[race.op1Task].parentTask.compare(taskIDMap[race.op2Task].parentTask) == 0)) {
		race.raceType = NESTED_NESTED;
	} else if (taskIDMap[race.op1Task].parentTask.compare("") != 0 &&
			taskIDMap[race.op1Task].parentTask.compare(taskIDMap[race.op2Task].parentTask) != 0) {
		race.raceType = NESTED_PRIMARY;
	} else if (taskIDMap[race.op2Task].parentTask.compare("") != 0 &&
			taskIDMap[race.op2Task].parentTask.compare(taskIDMap[race.op1Task].parentTask) != 0) {
		race.raceType = NESTED_PRIMARY;
	} else if (!(taskIDMap[race.op1Task].atomic)) {
		race.raceType = NONATOMIC_WITH_OTHER;
	} else if (!(taskIDMap[race.op2Task].atomic)) {
		race.raceType = NONATOMIC_WITH_OTHER;
	}

	IDType op1Deq = taskIDMap[race.op1Task].deqOpID;
	IDType op2Deq = taskIDMap[race.op2Task].deqOpID;

	if (op1Deq <= 0) {
		cout << "ERROR: Cannot find deq of task " << race.op1Task << "\n";
		return;
	}
	if (op2Deq <= 0) {
		cout << "ERROR: Cannot find deq of task " << race.op2Task << "\n";
		return;
	}

	if (op1Deq == op2Deq) {
		cout << "ERROR: Tasks " << race.op1Task << " and " << race.op2Task
			 << " have same deq op " << op1Deq << "\n";
		return;
	}

	IDType nodeDeq1 = opIDMap[op1Deq].nodeID;
	IDType nodeDeq2 = opIDMap[op2Deq].nodeID;
	if (nodeDeq1 <= 0) {
		cout << "ERROR: Cannot find node for deq op " << op1Deq << "\n";
		return;
	}
	if (nodeDeq2 <= 0) {
		cout << "ERROR: Cannot find node for deq op " << op2Deq << "\n";
		return;
	}

	if (nodeDeq1 == nodeDeq2) {
		cout << "ERROR: Nodes of deq ops " << op1Deq << " and " << op2Deq
			 << " are the same: " << nodeDeq1 << "\n";
		return;
	}

	if (graph->opEdgeExists(nodeDeq1, nodeDeq2) ||
			graph->opEdgeExists(nodeDeq2, nodeDeq1)) {
		race.raceType = NESTED_WITH_TASKS_ORDERED;
	}

	if (opIDMap[race.op1].threadID != opIDMap[race.op2].threadID)
		race.raceType = MULTITHREADED;
	else
		race.raceType = SINGLETHREADED;

	return;
}

void UAFDetector::insertRace(raceDetails race) {
	if (allocToRaceMap.find(race.allocID) == allocToRaceMap.end()) {
#if 0
		std::vector<raceDetails> allocRaceVector;
		allocRaceVector.push_back(race);
		allocToRaceMap[race.allocID] = allocRaceVector;
#endif
		std::multiset<raceDetails> allocRaceSet;
		allocRaceSet.insert(race);
		allocToRaceMap[race.allocID] = allocRaceSet;
	} else {
		std::multiset<raceDetails> existingEntry = allocToRaceMap[race.allocID];
		existingEntry.insert(race);
		allocToRaceMap.erase(allocToRaceMap.find(race.allocID));
		allocToRaceMap[race.allocID] = existingEntry;
	}
}

#if 0
bool isInteresting(RaceKind type) {
	if (type == NESTED_NESTED || type == NESTED_PRIMARY ||
			type == NESTED_WITH_TASKS_ORDERED ||
			type == NONATOMIC_WITH_OTHER)
		return true;
	else
		return false;
}
#endif

IDType  UAFDetector::findUAF() {

	bool flag = false;

	// Loop through freeIDMap, for each free, find use that no HB edge

	IDType falsePositives = 0;

#ifdef UNIQUERACE
	bool raceForFree; // Flag to track if we saw a race for a given free
	bool falsePositiveForFree; // Flag to track if we saw a false positive for a given free
#endif
	for (map<IDType, freeOpDetails>::iterator freeIt = freeIDMap.begin(); freeIt != freeIDMap.end(); freeIt++) {
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
			IDType readID = *readIt;

			raceDetails uaf;
			uaf.allocID = allocID;

			IDType nodeRead = opIDMap[readID].nodeID;
			if (nodeRead <= 0) {
				cout << "ERROR: Invalid node ID for op " << readID << "\n";
				return -1;
			}
			if (nodeFree == nodeRead && freeID < readID) {
#if 0
				cout << "Definite UAF between read op " << readID << " (read at address " << readSet[readID].startingAddress
					 << " in task " << opIDMap[readID].taskID << " in thread " << opIDMap[readID].threadID << ") "
					 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
					 << " in task " << opIDMap[freeID].taskID << " in thread " << opIDMap[freeID].threadID << ")\n";
				if (allocID > 0) {
					cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
						 << allocSet[allocID].startingAddress << " in task " << opIDMap[allocID].taskID
						 << " in thread " << opIDMap[allocID].threadID << ")\n";
				}
#endif

				// If free and read are in the same node, they are in the same thread
				uaf.op1 = readID;
				uaf.op2 = freeID;
				uaf.uafOrRace = true;

				getRaceKind(uaf);
				uaf.raceType = SINGLETHREADED;
				insertRace(uaf);

				uafCount++;
//				log(readID, freeID, allocID, "read", "free", true);

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
#if 0
				cout << "Definite UAF between read op " << readID << " (read at address " << readSet[readID].startingAddress
					 << " in task " << opIDMap[readID].taskID << " in thread " << opIDMap[readID].threadID << ") "
					 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
					 << " in task " << opIDMap[freeID].taskID << " in thread " << opIDMap[freeID].threadID << ")\n";
				if (allocID > 0) {
					cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
						 << allocSet[allocID].startingAddress << " in task " << opIDMap[allocID].taskID
						 << " in thread " << opIDMap[allocID].threadID << ")\n";
				}
#endif

				uaf.op1 = readID;
				uaf.op2 = freeID;
				uaf.uafOrRace = true;

				getRaceKind(uaf);
				insertRace(uaf);

				uafCount++;
//				log(readID, freeID, allocID, "read", "free", true);
				flag = true;
#ifdef UNIQUERACE
				raceForFree = true;
				break;
#else
				continue;
#endif
			}

			if (graph->opEdgeExists(nodeRead, nodeFree) == 0) {

				uaf.op1 = readID;
				uaf.op2 = freeID;
				uaf.uafOrRace = true;
				getRaceKind(uaf);

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
							uaf.raceType = SINGLETHREADED_ALLOC_MEMOP_IN_SAME_TASK_FP;

//							continue;
						} else {
							// May be we should categorize these separately. Easy to reproduce
							uaf.raceType = MULTITHREADED_ALLOC_MEMOP_IN_SAME_TASK;
						}
					}
				}
#endif
#if 0
				cout << "Potential UAF between read op " << readID << " (read at address " << readSet[readID].startingAddress
					 << " in task " << opIDMap[readID].taskID << " in thread " << opIDMap[readID].threadID << ") "
					 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
					 << " in task " << opIDMap[freeID].taskID << " in thread " << opIDMap[freeID].threadID << ")\n";
				if (allocID > 0) {
					cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
						 << allocSet[allocID].startingAddress << " in task " << opIDMap[allocID].taskID
						 << " in thread " << opIDMap[allocID].threadID << ")\n";
				}
#endif

				insertRace(uaf);

				uafCount++;
//				log(readID, freeID, allocID, "read", "free", true);

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

			raceDetails uaf;
			uaf.allocID = allocID;

			IDType nodeWrite = opIDMap[writeID].nodeID;
			if (nodeWrite <= 0) {
				cout << "ERROR: Invalid node ID for op " << writeID << "\n";
				return -1;
			}
			if (nodeFree == nodeWrite && freeID < writeID) {
#if 0
				cout << "Definite UAF between write op " << writeID << " (write at address " << writeSet[writeID].startingAddress
					 << " in task " << opIDMap[writeID].taskID << " in thread " << opIDMap[writeID].threadID << ") "
					 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
					 << " in task " << opIDMap[freeID].taskID << " in thread " << opIDMap[freeID].threadID << ")\n";
				if (allocID > 0) {
					cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
						 << allocSet[allocID].startingAddress << " in task " << opIDMap[allocID].taskID << " in thread "
						 << opIDMap[allocID].threadID << ")\n";
				}
#endif

				// If free and read are in the same node, they are in the same thread
				uaf.op1 = writeID;
				uaf.op2 = freeID;
				uaf.uafOrRace = true;
				getRaceKind(uaf);
				uaf.raceType = SINGLETHREADED;

				insertRace(uaf);

				uafCount++;

//				log(writeID, freeID, allocID, "read", "free", true);

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
#if 0
				cout << "Definite UAF between write op " << writeID << " (write at address " << writeSet[writeID].startingAddress
					 << " in task " << opIDMap[writeID].taskID << " in thread " << opIDMap[writeID].threadID << ") "
					 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
					 << " in task " << opIDMap[freeID].taskID << " in thread " << opIDMap[freeID].threadID << ")\n";
				if (allocID > 0) {
					cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
						 << allocSet[allocID].startingAddress << " in task " << opIDMap[allocID].taskID
						 << " in thread " << opIDMap[allocID].threadID << ")\n";
				}
#endif

				uaf.op1 = writeID;
				uaf.op2 = freeID;
				uaf.uafOrRace = true;

				getRaceKind(uaf);
				insertRace(uaf);

//				log(writeID, freeID, allocID, "read", "free", true);

				uafCount++;

				flag = true;
#ifdef UNIQUERACE
				raceForFree = true;
				break;
#else
				continue;
#endif
			}

			if (graph->opEdgeExists(nodeWrite, nodeFree) == 0) {

				uaf.op1 = writeID;
				uaf.op2 = freeID;
				uaf.uafOrRace = true;
				getRaceKind(uaf);

#ifdef ADDITIONS
				if (allocID > 0) {
					// Even if there is no edge between write and free,
					// if there is an edge from alloc to write (alloc happens before write), and the alloc is in the same task as the write and the task is atomic, this is a false positive.
					// This is true only if free is in the same thread as alloc and write.
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
							uaf.raceType = SINGLETHREADED_ALLOC_MEMOP_IN_SAME_TASK_FP;
//							continue;
						} else {
							uaf.raceType = MULTITHREADED_ALLOC_MEMOP_IN_SAME_TASK;
						}
					}
				}
#endif

#if 0
				cout << "Potential UAF between write op " << writeID << " (write at address " << writeSet[writeID].startingAddress
					 << " in task " << opIDMap[writeID].taskID << " in thread " << opIDMap[writeID].threadID << ") "
					 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
					 << " in task " << opIDMap[freeID].taskID << " in thread " << opIDMap[freeID].threadID << ")\n";
				if (allocID > 0) {
					cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
						 << allocSet[allocID].startingAddress << " in task " << opIDMap[allocID].taskID
						 << " in thread " << opIDMap[allocID].threadID << ")\n";
				}
#endif

				insertRace(uaf);
				uafCount++;
//				log(writeID, freeID, allocID, "read", "free", true);

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

#ifdef DATARACE
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

			std::string writeAddress1 = writeSet[*writeIt].startingAddress;

			IDType nodeWrite = opIDMap[*writeIt].nodeID;

			// write-write races
			for (set<IDType>::iterator write2It = allocIt->second.writeOps.begin(); write2It != allocIt->second.writeOps.end(); write2It++) {
				if (*write2It == *writeIt) continue;

				std::string writeAddress2 = writeSet[*write2It].startingAddress;
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
				UAFDetector::raceDetails dataRace;
				dataRace.allocID = allocIt->first;

				if (graph->opEdgeExists(nodeWrite, nodeWrite2) == 0 && graph->opEdgeExists(nodeWrite2, nodeWrite) == 0) {
#if 0
					cout << "Potential data race between write ops " << *writeIt << " (in task " << opIDMap[*writeIt].taskID
						 << " in thread " << opIDMap[*writeIt].threadID
						 << ") and " << *write2It << " (in task " << opIDMap[*write2It].taskID
						 << " in thread " << opIDMap[*write2It].threadID << ") on address "
						 << writeAddress1 << endl;
#endif

					dataRace.op1 = *writeIt;
					dataRace.op2 = *write2It;
					dataRace.uafOrRace = false;

					getRaceKind(dataRace);
					insertRace(dataRace);
					raceCount++;
//					log(*writeIt, *write2It, allocIt->first, "write", "write", false);


					flag = true;
#ifdef UNIQUERACE
					if (isInteresting(dataRace.raceType))
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

				std::string readAddress = readSet[*readIt].startingAddress;
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
				UAFDetector::raceDetails dataRace;
				dataRace.allocID = allocIt->first;

				if (graph->opEdgeExists(nodeWrite, nodeRead) == 0 && graph->opEdgeExists(nodeRead, nodeWrite) == 0) {
#if 0
					cout << "Potential data race between read op " << *readIt << " (in task " << opIDMap[*readIt].taskID
						 << " in thread " << opIDMap[*readIt].threadID
						 << ") and write op " << *writeIt << "(in task " << opIDMap[*writeIt].taskID
						 << " in thread " << opIDMap[*writeIt].threadID << ") on address "
						 << readAddress << "\n";
#endif

					dataRace.op1 = *writeIt;
					dataRace.op2 = *readIt;
					dataRace.uafOrRace = false;

					getRaceKind(dataRace);
					insertRace(dataRace);

					raceCount++;
//					log(*readIt, *writeIt, allocIt->first, "read", "write", false);

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
#endif

IDType  UAFDetector::findUAFUsingNodes() {

	bool flag = false;

	// Loop through freeIDMap, for each free, find use that no HB edge

	IDType falsePositives = 0;

	for (map<IDType, freeOpDetails>::iterator freeIt = freeIDMap.begin(); freeIt != freeIDMap.end(); freeIt++) {
		IDType freeID = freeIt->first;
		IDType allocID = freeIt->second.allocOpID;
		IDType nodeAlloc = -1;

		IDType nodeFree = opIDMap[freeID].nodeID;
		if (nodeFree <= 0) {
			cout << "ERROR: Invalid node ID for op " << freeID << "\n";
			return -1;
		}

		long long freeStartAddress, freeEndAddress;
		std::stringstream freeStream;
		freeStream << freeSet[freeID].startingAddress;
		freeStream >> std::hex >> freeStartAddress;
		freeEndAddress = freeStartAddress + freeSet[freeID].range - 1;

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

		for (set<IDType>::iterator nodeIt = freeIt->second.nodes.begin(); nodeIt != freeIt->second.nodes.end(); nodeIt++) {
			if (nodeIDMap.find(*nodeIt) == nodeIDMap.end()) {
				cout << "ERROR: Invalid node ID in freeIDMap\n";
				return -1;
			}

			IDType nodeAccess = *nodeIt;
			if (nodeAccess <= 0) {
				cout << "ERROR: Invalid node in freeIDMap\n";
				cout << "ERROR: free ID: " << freeID << "\n";
				freeIt->second.printDetails();
				cout << "\n";
			}

			for (set<IDType>::iterator accessIt = nodeIDMap[nodeAccess].opSet.begin();
					accessIt != nodeIDMap[nodeAccess].opSet.end(); accessIt++) {
				IDType accessID = *accessIt;

				if (opIDMap[accessID].opType.compare("read") == 0) {
					std::string readAddress = readSet[accessID].startingAddress;
					long long readAddressInt;
					std::stringstream readStream;
					readStream << readAddress;
					readStream >> std::hex >> readAddressInt;

					if (readAddressInt < freeStartAddress || freeEndAddress < readAddressInt)
						continue;

				} else if (opIDMap[accessID].opType.compare("write") == 0) {
					std::string writeAddress = writeSet[accessID].startingAddress;
					long long writeAddressInt;
					std::stringstream writeStream;
					writeStream << writeAddress;
					writeStream >> std::hex >> writeAddressInt;

					if (writeAddressInt < freeStartAddress || freeEndAddress < writeAddressInt)
						continue;

				} else {
#ifdef RACEDEBUG
					cout << "DEBUG: Op " << accessID << " is neither read nor write\n";
#endif
					continue;
				}

				raceDetails uaf;
				uaf.allocID = allocID;

				if (nodeFree == nodeAccess && freeID < accessID) {
#if 0
					cout << "Definite UAF between read op " << accessID << " (read at address " << readSet[accessID].startingAddress
						 << " in task " << opIDMap[accessID].taskID << " in thread " << opIDMap[accessID].threadID << ") "
						 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
						 << " in task " << opIDMap[freeID].taskID << " in thread " << opIDMap[freeID].threadID << ")\n";
					if (allocID > 0) {
						cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
							 << allocSet[allocID].startingAddress << " in task " << opIDMap[allocID].taskID
							 << " in thread " << opIDMap[allocID].threadID << ")\n";
					}
#endif

					// If free and read are in the same node, they are in the same thread
					uaf.op1 = accessID;
					uaf.op2 = freeID;
					uaf.uafOrRace = true;

					getRaceKind(uaf);
					uaf.raceType = SINGLETHREADED;
					insertRace(uaf);

					uafCount++;
					log (accessID, freeID, allocID, true, uaf.raceType, true);

					flag = true;
					continue;

				} else if (nodeFree == nodeAccess) {
#ifdef RACEDEBUG
					cout << "DEBUG: Free op " << freeID << " and read op " << readID << " in the same node, but read before free\n";
					cout << "DEBUG: Skipping this read while detecting races\n";
#endif
					continue;
				}
				if (graph->opEdgeExists(nodeFree, nodeAccess) == 1) {
#if 0
					cout << "Definite UAF between read op " << accessID << " (read at address " << readSet[accessID].startingAddress
						 << " in task " << opIDMap[accessID].taskID << " in thread " << opIDMap[accessID].threadID << ") "
						 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
						 << " in task " << opIDMap[freeID].taskID << " in thread " << opIDMap[freeID].threadID << ")\n";
					if (allocID > 0) {
						cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
							 << allocSet[allocID].startingAddress << " in task " << opIDMap[allocID].taskID
							 << " in thread " << opIDMap[allocID].threadID << ")\n";
					}
#endif

					uaf.op1 = accessID;
					uaf.op2 = freeID;
					uaf.uafOrRace = true;

					getRaceKind(uaf);
					insertRace(uaf);

					uafCount++;
					log (accessID, freeID, allocID, true, uaf.raceType, true);
					flag = true;
					continue;
				}

				if (graph->opEdgeExists(nodeAccess, nodeFree) == 0) {

					uaf.op1 = accessID;
					uaf.op2 = freeID;
					uaf.uafOrRace = true;
					getRaceKind(uaf);

#ifdef ADDITIONS
					if (allocID > 0) {
						// Even if there is no edge between read and free,
						// if there is an edge from alloc to read (alloc happens before read), and the alloc is in the same task as the read and the task is atomic, this is a false positive.
						// This is true only if free is in the same thread as alloc and read.
						bool edgeExists = false;
						if (nodeAlloc == nodeAccess && allocID < accessID)
							edgeExists = true;
						else if (nodeAlloc != nodeAccess && graph->opEdgeExists(nodeAlloc, nodeAccess))
							edgeExists = true;
						if (edgeExists &&
								opIDMap[allocID].taskID.compare(opIDMap[accessID].taskID) == 0
								&& taskIDMap[opIDMap[allocID].taskID].atomic) {

							if (opIDMap[freeID].threadID == opIDMap[accessID].threadID) {
								falsePositives++;
								uaf.raceType = SINGLETHREADED_ALLOC_MEMOP_IN_SAME_TASK_FP;
//								cout << "False positive!!\n";
							} else {
								// May be we should categorize these separately. Easy to reproduce
								uaf.raceType = MULTITHREADED_ALLOC_MEMOP_IN_SAME_TASK;
							}
						}
					}
#endif
#if 0
					cout << "Potential UAF between read op " << accessID << " (read at address " << readSet[accessID].startingAddress
						 << " in task " << opIDMap[accessID].taskID << " in thread " << opIDMap[accessID].threadID << ") "
						 << " and free op " << freeID << " (freed " << freeSet[freeID].range << " bytes from address " << freeSet[freeID].startingAddress
						 << " in task " << opIDMap[freeID].taskID << " in thread " << opIDMap[freeID].threadID << ")\n";
					if (allocID > 0) {
						cout << "Memory originally allocated at " << allocID << " (allocated " << allocSet[allocID].range << " bytes from address "
							 << allocSet[allocID].startingAddress << " in task " << opIDMap[allocID].taskID
							 << " in thread " << opIDMap[allocID].threadID << ")\n";
					}
#endif

					insertRace(uaf);

					uafCount++;
					log (accessID, freeID, allocID, true, uaf.raceType, true);

					flag = true;
					continue;
				}
			}
		}
	}

	if (flag) {
		cout << "OUTPUT: False positives = " << falsePositives << "\n";
		return uafCount;
	} else
		return 0;
}

#ifdef DATARACE
#ifndef ACCESS
IDType UAFDetector::findDataRacesUsingNodes() {

	bool flag = false;

	for (map<IDType, allocOpDetails>::iterator allocIt = allocIDMap.begin(); allocIt != allocIDMap.end(); allocIt++) {
		if (allocIt->second.writeOps.size() == 0)
			continue;

		long long allocStartAddress, allocEndAddress;
		std::stringstream allocStream;
		allocStream << allocSet[allocIt->first].startingAddress;
		allocStream >> std::hex >> allocStartAddress;
		allocEndAddress = allocStartAddress + allocSet[allocIt->first].range - 1;

		for (set<IDType>::iterator nodeIt1 = allocIt->second.nodes.begin(); nodeIt1 != allocIt->second.nodes.end(); nodeIt1++) {
			if (nodeIDMap.find(*nodeIt1) == nodeIDMap.end()) {
				cout << "ERROR: Invalid node ID in allocIDMap\n";
				return -1;
			}
			IDType node1 = *nodeIt1;

			for (set<IDType>::iterator nodeIt2 = allocIt->second.nodes.begin(); nodeIt2 != allocIt->second.nodes.end(); nodeIt2++) {
				if (*nodeIt1 == *nodeIt2) continue;

				if (nodeIDMap.find(*nodeIt2) == nodeIDMap.end()) {
					cout << "ERROR: Invalid node ID in allocIDMap\n";
					return -1;
				}
				IDType node2 = *nodeIt2;

				if (graph->opEdgeExists(node1, node2) != 0 ||
						graph->opEdgeExists(node2, node1) != 0)
					continue;

				for (set<IDType>::iterator op1It = nodeIDMap[node1].opSet.begin();
						op1It != nodeIDMap[node1].opSet.end(); op1It++) {

					std::string op1Address;
					IDType op1 = *op1It;
					if (opIDMap[op1].opType.compare("write") == 0) {
						op1Address = writeSet[op1].startingAddress;
					} else if (opIDMap[op1].opType.compare("read") == 0) {
						op1Address = readSet[op1].startingAddress;
					} else {
//						cout << "DEBUG: Op for node " << *nodeIt1 << " is neither read nor write\n";
						continue;
					}

					long long op1AddressInt;
					std::stringstream op1Stream;
					op1Stream << op1Address;
					op1Stream >> std::hex >> op1AddressInt;

					if (op1AddressInt < allocStartAddress || allocEndAddress < op1AddressInt)
						continue;

					for (set<IDType>::iterator op2It = nodeIDMap[*nodeIt2].opSet.begin();
							op2It != nodeIDMap[*nodeIt2].opSet.end(); op2It++) {

						std::string op2Address;
						IDType op2 = *op2It;
						if (opIDMap[op2].opType.compare("write") == 0)
							op2Address = writeSet[op2].startingAddress;
						else if (opIDMap[op2].opType.compare("read") == 0)
							op2Address = readSet[op2].startingAddress;
						else {
//							cout << "DEBUG: Op for node " << *nodeIt2 << " is neither read nor write\n";
							continue;
						}
						long long op2AddressInt;
						std::stringstream op2Stream;
						op2Stream << op2Address;
						op2Stream >> std::hex >> op2AddressInt;

						if (op2AddressInt < allocStartAddress || allocEndAddress < op2AddressInt)
							continue;

						if (op1Address.compare(op2Address) != 0) continue;

						if (opIDMap[*op1It].opType.compare("read") == 0 &&
								opIDMap[*op2It].opType.compare("read") == 0)
							continue;

						UAFDetector::raceDetails dataRace;
						dataRace.allocID = allocIt->first;

						dataRace.op1 = *op1It;
						dataRace.op2 = *op2It;
						dataRace.uafOrRace = false;

						getRaceKind(dataRace);
						insertRace(dataRace);
						raceCount++;
						log (*op1It, *op2It, allocIt->first, false, dataRace.raceType, true);
//						cout << "Data Race: ";
//						dataRace.printDetails();

						flag = true;

					}
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
#endif

void UAFDetector::initLog(std::string traceFileName) {
	uafCount = 0;
	raceCount = 0;

	std::string uafFileName = traceFileName + ".uaf.all";
	uafAllLogger.init(uafFileName);
#ifdef DATARACE
	std::string raceFileName = traceFileName + ".race.all";
	raceAllLogger.init(raceFileName);
#endif

	uafFileName = traceFileName + ".uaf.all.debug";
	uafAllDebugLogger.init(uafFileName);
#ifdef DATARACE
	raceFileName = traceFileName + ".race.all.debug";
	raceAllDebugLogger.init(raceFileName);
#endif

	uafFileName = traceFileName + ".uaf.unique.all";
	uafAllUniqueLogger.init(uafFileName);
#ifdef DATARACE
	raceFileName = traceFileName + ".race.unique.all";
	raceAllUniqueLogger.init(raceFileName);
#endif

#if 0
	uafFileName = traceFileName + ".uaf.withouttask";
	uafNoTaskLogger.init(uafFileName);
#ifdef DATARACE
	raceFileName = traceFileName + ".race.withouttask";
	raceNoTaskLogger.init(raceFileName);
#endif

	uafFileName = traceFileName + ".uaf.enqpath";
	uafEnqPathLogger.init(uafFileName);
#ifdef DATARACE
	raceFileName = traceFileName + ".race.enqpath";
	raceEnqPathLogger.init(raceFileName);
#endif

	uafFileName = traceFileName + ".uaf.allocmemopinsametaskdiffthread";
	uafAllocMemopSameTaskLogger.init(uafFileName);
#ifdef DATARACE
	raceFileName = traceFileName + ".race.allocmemopinsametaskdiffthread";
	raceAllocMemopSameTaskLogger.init(raceFileName);
#endif
#endif

	uafFileName = traceFileName + ".uaf.allocmemopinsametaskinsamethread";
	uafAllocMemopSameTaskSameThreadLogger.init(uafFileName);
#ifdef DATARACE
	raceFileName = traceFileName + ".race.allocmemopinsametaskinsamethread";
	raceAllocMemopSameTaskSameThreadLogger.init(raceFileName);
#endif

#if 0
	uafFileName = traceFileName + ".uaf.nestednested";
	uafNestedNestedLogger.init(uafFileName);
#ifdef DATARACE
	raceFileName = traceFileName + ".race.nestednested";
	raceNestedNestedLogger.init(raceFileName);
#endif

	uafFileName = traceFileName + ".uaf.nestedprimary";
	uafNestedPrimaryLogger.init(uafFileName);
#ifdef DATARACE
	raceFileName = traceFileName + ".race.nestedprimary";
	raceNestedPrimaryLogger.init(raceFileName);
#endif

	uafFileName = traceFileName + ".uaf.nestedordered";
	uafNestedOrderedLogger.init(uafFileName);
#ifdef DATARACE
	raceFileName = traceFileName + ".race.nestedordered";
	raceNestedOrderedLogger.init(raceFileName);
#endif

	uafFileName = traceFileName + ".uaf.nonatomicwithother";
	uafNonAtomicOtherLogger.init(uafFileName);
#ifdef DATARACE
	raceFileName = traceFileName + ".race.nonatomicwithother";
	raceNonAtomicOtherLogger.init(raceFileName);
#endif

	uafFileName = traceFileName + ".uaf.multithreadedwithsamenestingloop";
	uafMultithreadedSameNestingLoopLogger.init(uafFileName);
#ifdef DATARACE
	raceFileName = traceFileName + ".race.multithreadedwithsamenestingloop";
	raceMultithreadedSameNestingLoopLogger.init(raceFileName);
#endif

	uafFileName = traceFileName + ".uaf.other";
	uafOtherLogger.init(uafFileName);
#ifdef DATARACE
	raceFileName = traceFileName + ".race.other";
	raceOtherLogger.init(raceFileName);
#endif
#endif

	uafFileName = traceFileName + ".uaf.only.multithreaded";
	uafOnlyMultiLogger.init(uafFileName);
#ifdef DATARACE
	raceFileName = traceFileName + ".race.only.multithreaded";
	raceOnlyMultiLogger.init(raceFileName);
#endif

	uafFileName = traceFileName + ".uaf.only.singlethreaded";
	uafOnlySingleLogger.init(uafFileName);
#ifdef DATARACE
	raceFileName = traceFileName + ".race.only.singlethreaded";
	raceOnlyMultiLogger.init(raceFileName);
#endif

	uafFileName = traceFileName + ".uaf.both.multithreaded";
	uafBothMultiLogger.init(uafFileName);
#ifdef DATARACE
	raceFileName = traceFileName + ".race.both.multithreaded";
	raceBothMultiLogger.init(raceFileName);
#endif

	uafFileName = traceFileName + ".uaf.both.singlethreaded";
	uafBothSingleLogger.init(uafFileName);
#ifdef DATARACE
	raceFileName = traceFileName + ".race.both.singlethreaded";
	raceBothSingleLogger.init(raceFileName);
#endif
}

std::string UAFDetector::findPreviousTaskOfOp(IDType op) {
	IDType blockID = opIDMap[op].blockID;
	if (blockID == -1) {
		cout << "ERROR: Cannot find blockID of op " << op << "\n";
		return "";
	}

	while (blockIDMap[blockID].taskID.compare("") == 0) {
		blockID = blockIDMap[blockID].prevBlockInThread;
		if (blockID == -1)
			break;
	}

	if (blockID == -1) {
		//cout << "DEBUG: Cannot find previous task of op\n";
		return "";
	}

	return blockIDMap[blockID].taskID;
}

#if 0
void UAFDetector::log() {
#if 0
	for (map<IDType, std::multiset<raceDetails> >::iterator it = allocToRaceMap.begin();
			it != allocToRaceMap.end(); it++) {
		cout << "Alloc ID: " << it->first << "\n";
		for (std::multiset<raceDetails>::iterator rIt = it->second.begin();
				rIt != it->second.end(); rIt++) {
			rIt->printDetails();
		}
	}
#endif

	IDType allocCount = 0;
	std::set<IDType> allocSeen;
	// Find UAFs
	uniqueUafCount = 0;
	for (map<IDType, UAFDetector::allocOpDetails>::iterator allocIt = allocIDMap.begin();
			allocIt != allocIDMap.end(); allocIt++) {
		if (allocToRaceMap.find(allocIt->first) == allocToRaceMap.end())
			continue;

		bool foundNested = false;
		raceDetails tempRace;
		tempRace.uafOrRace = true;
		tempRace.raceType = NESTED_WITH_TASKS_ORDERED;
		std::pair<std::multiset<raceDetails>::iterator, std::multiset<raceDetails>::iterator>
			ret = allocToRaceMap[allocIt->first].equal_range(tempRace);
		if (ret.first != ret.second) {
			for (std::multiset<raceDetails>::iterator raceIt = ret.first;
					raceIt != ret.second; raceIt++) {
//				raceIt->printDetails();
				log (raceIt->op1, raceIt->op2, raceIt->allocID,
						raceIt->uafOrRace, raceIt->raceType);
				uniqueUafCount++;

				if (allocSeen.find(allocIt->first) == allocSeen.end()) {
					allocCount++;
					allocSeen.insert(allocIt->first);
				}

				foundNested = true;
				break;
			}

			if (foundNested)
				continue;
		}

		foundNested = false;
		tempRace.raceType = NESTED_NESTED;
		ret = allocToRaceMap[allocIt->first].equal_range(tempRace);
		if (ret.first != ret.second) {
			for (std::multiset<raceDetails>::iterator raceIt = ret.first;
					raceIt != ret.second; raceIt++) {
//				raceIt->printDetails();
				log (raceIt->op1, raceIt->op2, raceIt->allocID,
						raceIt->uafOrRace, raceIt->raceType);
				uniqueUafCount++;

				if (allocSeen.find(allocIt->first) == allocSeen.end()) {
					allocCount++;
					allocSeen.insert(allocIt->first);
				}

				foundNested = true;
				break;
			}

			if (foundNested)
				continue;
		}

		foundNested = false;
		tempRace.raceType = NESTED_PRIMARY;
		ret = allocToRaceMap[allocIt->first].equal_range(tempRace);
		if (ret.first != ret.second) {
			for (std::multiset<raceDetails>::iterator raceIt = ret.first;
					raceIt != ret.second; raceIt++) {
//				raceIt->printDetails();
				log (raceIt->op1, raceIt->op2, raceIt->allocID,
						raceIt->uafOrRace, raceIt->raceType);
				uniqueUafCount++;

				if (allocSeen.find(allocIt->first) == allocSeen.end()) {
					allocCount++;
					allocSeen.insert(allocIt->first);
				}

				foundNested = true;
				break;
			}

			if (foundNested)
				continue;
		}

		for (std::multiset<raceDetails>::iterator raceIt = allocToRaceMap[allocIt->first].begin();
				raceIt != allocToRaceMap[allocIt->first].end(); raceIt++) {
			if (raceIt->raceType == SINGLETHREADED_ALLOC_MEMOP_IN_SAME_TASK_FP)
				continue;
			if (raceIt->uafOrRace) {
//				raceIt->printDetails();
				log (raceIt->op1, raceIt->op2, raceIt->allocID,
						raceIt->uafOrRace, raceIt->raceType);
				uniqueUafCount++;

				if (allocSeen.find(allocIt->first) == allocSeen.end()) {
					allocCount++;
					allocSeen.insert(allocIt->first);
				}

				break;
			}
		}
	}

#ifdef DATARACE
	// Find data races
	uniqueRaceCount = 0;
	for (map<IDType, UAFDetector::allocOpDetails>::iterator allocIt = allocIDMap.begin();
			allocIt != allocIDMap.end(); allocIt++) {
		if (allocToRaceMap.find(allocIt->first) == allocToRaceMap.end())
			continue;

		bool foundNested = false;
		raceDetails tempRace;
		tempRace.uafOrRace = false;
		tempRace.raceType = NESTED_WITH_TASKS_ORDERED;
		std::pair<std::multiset<raceDetails>::iterator, std::multiset<raceDetails>::iterator>
			ret = allocToRaceMap[allocIt->first].equal_range(tempRace);
		if (ret.first != ret.second) {
			for (std::multiset<raceDetails>::iterator raceIt = ret.first;
					raceIt != ret.second; raceIt++) {
				log (raceIt->op1, raceIt->op2, raceIt->allocID,
						raceIt->uafOrRace, raceIt->raceType);
				uniqueRaceCount++;

				if (allocSeen.find(allocIt->first) == allocSeen.end()) {
					allocCount++;
					allocSeen.insert(allocIt->first);
				}

				foundNested = true;
				break;
			}

			if (foundNested)
				continue;
		}

		foundNested = false;
		tempRace.raceType = NESTED_NESTED;
		ret = allocToRaceMap[allocIt->first].equal_range(tempRace);
		if (ret.first != ret.second) {
			for (std::multiset<raceDetails>::iterator raceIt = ret.first;
					raceIt != ret.second; raceIt++) {
				log (raceIt->op1, raceIt->op2, raceIt->allocID,
						raceIt->uafOrRace, raceIt->raceType);
				uniqueRaceCount++;

				if (allocSeen.find(allocIt->first) == allocSeen.end()) {
					allocCount++;
					allocSeen.insert(allocIt->first);
				}

				foundNested = true;
				break;
			}

			if (foundNested)
				continue;
		}

		foundNested = false;
		tempRace.raceType = NESTED_PRIMARY;
		ret = allocToRaceMap[allocIt->first].equal_range(tempRace);
		if (ret.first != ret.second) {
			for (std::multiset<raceDetails>::iterator raceIt = ret.first;
					raceIt != ret.second; raceIt++) {
				log (raceIt->op1, raceIt->op2, raceIt->allocID,
						raceIt->uafOrRace, raceIt->raceType);
				uniqueRaceCount++;

				if (allocSeen.find(allocIt->first) == allocSeen.end()) {
					allocCount++;
					allocSeen.insert(allocIt->first);
				}

				foundNested = true;
				break;
			}

			if (foundNested)
				continue;
		}

		for (std::multiset<raceDetails>::iterator raceIt = allocToRaceMap[allocIt->first].begin();
				raceIt != allocToRaceMap[allocIt->first].end(); raceIt++) {
			if (raceIt->raceType == SINGLETHREADED_ALLOC_MEMOP_IN_SAME_TASK_FP)
				continue;
			if (!raceIt->uafOrRace) {
				uniqueRaceCount++;
				log (raceIt->op1, raceIt->op2, raceIt->allocID,
						raceIt->uafOrRace, raceIt->raceType);

				if (allocSeen.find(allocIt->first) == allocSeen.end()) {
					allocCount++;
					allocSeen.insert(allocIt->first);
				}

				break;
			}
		}
	}
#endif

  	cout << "\n#Allocation sites with races = " << allocCount << "\n";
	cout << "OUTPUT: #Unique UAFs: " << uniqueUafCount << "\n";
#ifdef DATARACE
	cout << "OUTPUT: #Unique Races: " << uniqueRaceCount << "\n";
#endif
}
#endif

// uafOrRace: true if uaf, false if data race
// logAll: log all races
// raceTypeByThread = only multithreaded, only singlethreaded, both multithreaded,
// 					  both singlethreaded
void UAFDetector::log(IDType op1ID, IDType op2ID, IDType opAllocID,
		bool uafOrRace, RaceKind raceType, bool logAll, RaceKindByThread raceTypeByThread) {

//	cout << "Enter log(.): race: " << op1ID << " & " << op2ID << "\n";

	IDType op1ThreadID = opIDMap[op1ID].threadID;
	IDType op2ThreadID = opIDMap[op2ID].threadID;
	std::string op1TaskID = opIDMap[op1ID].taskID;
	std::string op2TaskID = opIDMap[op2ID].taskID;

	if (op1ThreadID < 0) {
		cout << "ERROR: Cannot find threadID of op " << op1ID << "\n";
		return;
	}
	if (op2ThreadID < 0) {
		cout << "ERROR: Cannot find threadID of op " << op2ID << "\n";
		return;
	}
#ifdef SINGLETHREADEDRACES
	if (op1ThreadID != op2ThreadID)
		return;
#endif

	if (op1TaskID.compare("") == 0)
		op1TaskID = findPreviousTaskOfOp(op1ID);
	if (op2TaskID.compare("") == 0)
		op2TaskID = findPreviousTaskOfOp(op2ID);

	IDType allocThreadID = -1;
	if (opAllocID > 0) {
		allocThreadID = opIDMap[opAllocID].threadID;
	}

	std::string line1, lines23, line4, line5;
	std::stringstream str;

	str << op1ID << " " << op1ThreadID
		<< " " << op2ID << " " << op2ThreadID << "\n";
	line1 = str.str();

	str.str("");
	str.clear();

	str << opAllocID << "\n" << allocThreadID << "\n";
	lines23 = str.str();

	str.str("");
	str.clear();
	str << op1ID << " " << op1ThreadID << " " << op2ID << " " << op2ThreadID
		<< " " << opAllocID << " " << allocThreadID << "\n";
	std::string mtLine = str.str();

	str.str("");
	str.clear();

	// Finding the enq path
	IDType enqID;
	std::string tempTaskID = "";

	std::set<std::string> enqPathTasks1, enqPathTasks2;
	// op1
	enqID = taskIDMap[op1TaskID].enqOpID;
	if (enqID != -1) {
		tempTaskID = opIDMap[enqID].taskID;

		if (tempTaskID.compare("") != 0 &&
				taskIDMap[tempTaskID].parentTask.compare("") != 0) {
			enqPathTasks1.insert(taskIDMap[tempTaskID].parentTask);
		}
	}

	str << enqID << " ";

	while (enqID != -1 && tempTaskID.compare("") != 0) {
		enqID = taskIDMap[tempTaskID].enqOpID;
		if (enqID != -1) {
			tempTaskID = opIDMap[enqID].taskID;

			if (tempTaskID.compare("") != 0 &&
					taskIDMap[tempTaskID].parentTask.compare("") != 0) {
				enqPathTasks1.insert(taskIDMap[tempTaskID].parentTask);
			}
		}

		str << enqID << " ";

	}

	str << "\n";
	line4 = str.str();

	str.str("");
	str.clear();

	// op2
	enqID = taskIDMap[op2TaskID].enqOpID;
	if (enqID != -1) {
		tempTaskID = opIDMap[enqID].taskID;

		if (tempTaskID.compare("") != 0 &&
				taskIDMap[tempTaskID].parentTask.compare("") != 0) {
			enqPathTasks2.insert(taskIDMap[tempTaskID].parentTask);
		}
	}

	str << enqID << " ";
	while (enqID != -1 && tempTaskID.compare("") != 0) {
		enqID = taskIDMap[tempTaskID].enqOpID;
		if (enqID != -1) {
			tempTaskID = opIDMap[enqID].taskID;

			if (tempTaskID.compare("") != 0 &&
					taskIDMap[tempTaskID].parentTask.compare("") != 0) {
				enqPathTasks2.insert(taskIDMap[tempTaskID].parentTask);
			}
		}

		str << enqID << " ";
	}

	str << "\n";

	line5 = str.str();

	if (logAll) {
//		cout << "logAll\n";
		std::string accessAddress, allocAddress;
		if (opIDMap[op1ID].opType.compare("read") == 0) {
			accessAddress = readSet[op1ID].startingAddress;
		} else if (opIDMap[op1ID].opType.compare("write") == 0) {
			accessAddress = writeSet[op1ID].startingAddress;
		} else {
			cout << "ERROR: Racing op " << op1ID << " is neither read nor write\n";
			opIDMap[op1ID].printOpDetails();
			return;
		}

		allocAddress = allocSet[opAllocID].startingAddress;
		long long allocAddressInt, accessAddressInt;
		str << allocAddress;
		str >> std::hex >> allocAddressInt;
		str.str("");
		str.clear();

		str << accessAddress;
		str >> std::hex >> accessAddressInt;
		str.str("");
		str.clear();

//		int offset = accessAddressInt - allocAddressInt;

		if (uafOrRace) {
//			cout << "logall uaf: " << op1ID << " & " << op2ID << "\n";
#if 0
			uafAllLogger.streamObject << op1ID << " "
					<< op1ThreadID << " " << op2ID << " " << op2ThreadID
					<< " " << opAllocID << " " << accessAddress << " "
					<< offset << "\n";
			uafAllLogger.writeLog();

			uafAllDebugLogger.writeLog(line1);
			uafAllDebugLogger.writeLog(lines23);
			uafAllDebugLogger.writeLog(line4);
			uafAllDebugLogger.writeLog(line5);

			if (raceType == SINGLETHREADED_ALLOC_MEMOP_IN_SAME_TASK_FP) {
				uafAllocMemopSameTaskSameThreadLogger.writeLog(line1);
			}
#endif
			uafAllLogger.writeLog(mtLine);
		} else {
#ifdef DATARACE
			raceAllLogger.streamObject << op1ID << " "
					<< op1ThreadID << " " << op2ID << " " << op2ThreadID
					<< " " << opAllocID << " " << accessAddress << " "
					<< offset << "\n";
			raceAllLogger.writeLog();

			raceAllDebugLogger.writeLog(line1);
			raceAllDebugLogger.writeLog(lines23);
			raceAllDebugLogger.writeLog(line4);
			raceAllDebugLogger.writeLog(line5);

			if (raceType == SINGLETHREADED_ALLOC_MEMOP_IN_SAME_TASK_FP) {
				raceAllocMemopSameTaskSameThreadLogger.writeLog(line1);
			}
#endif
		}
		return;
	}

	Logger *onlyMultiLogger, *onlySingleLogger;
	Logger *bothMultiLogger, *bothSingleLogger;
	Logger *uniqueAllLogger;

	if (uafOrRace) {
		onlyMultiLogger = &uafOnlyMultiLogger;
	} else {
#ifdef DATARACE
		onlyMultiLogger = &raceOnlyMultiLogger;
#endif
	}

	if (uafOrRace) {
		onlySingleLogger = &uafOnlySingleLogger;
	} else {
#ifdef DATARACE
		onlySingleLogger = &raceOnlySingleLogger;
#endif
	}

	if (uafOrRace) {
		bothMultiLogger = &uafBothMultiLogger;
	} else {
#ifdef DATARACE
		bothMultiLogger = &raceBothMultiLogger;
#endif
	}

	if (uafOrRace) {
		bothSingleLogger = &uafBothSingleLogger;
	} else {
#ifdef DATARACE
		bothSingleLogger = &raceBothSingleLogger;
#endif
	}

	if (uafOrRace) {
		uniqueAllLogger = &uafAllUniqueLogger;
	} else {
#ifdef DATARACE
		uniqueAllLogger = &raceAllUniqueLogger;
#endif
	}

	uniqueAllLogger->writeLog(line1);
	uniqueAllLogger->writeLog(lines23);
	uniqueAllLogger->writeLog(line4);
	uniqueAllLogger->writeLog(line5);

	if (raceTypeByThread == ONLY_MULTITHREADED) {
		onlyMultiLogger->writeLog(line1);
		onlyMultiLogger->writeLog(lines23);
		onlyMultiLogger->writeLog(line4);
		onlyMultiLogger->writeLog(line5);
	} else if (raceTypeByThread == ONLY_SINGLETHREADED) {
		onlySingleLogger->writeLog(line1);
		onlySingleLogger->writeLog(lines23);
		onlySingleLogger->writeLog(line4);
		onlySingleLogger->writeLog(line5);
	} else if (raceTypeByThread == BOTH_MULTITHREADED) {
		bothMultiLogger->writeLog(line1);
		bothMultiLogger->writeLog(lines23);
		bothMultiLogger->writeLog(line4);
		bothMultiLogger->writeLog(line5);
	} else if (raceTypeByThread == BOTH_SINGLETHREADED) {
		bothSingleLogger->writeLog(line1);
		bothSingleLogger->writeLog(lines23);
		bothSingleLogger->writeLog(line4);
		bothSingleLogger->writeLog(line5);
	} else {
		cout << "ERROR: race type is not ONLY_MULTITHREADED, ONLY_SINGLETHREADED, BOTH_MULTITHREADED, BOTH_SINGLETHREADED\n";
	}

	return;

	Logger *allLogger, *raceLogger;
	std::string raceTypeString;



	if (uafOrRace) {
		allLogger = &uafAllUniqueLogger;

	} else {
#ifdef DATARACE
		allLogger = &raceAllUniqueLogger;
#endif
	}

	if (raceType == NESTED_NESTED) {
		if (uafOrRace)
			raceLogger = &uafNestedNestedLogger;
#ifdef DATARACE
		else
			raceLogger = &raceNestedNestedLogger;
#endif
	} else if (raceType == NESTED_PRIMARY) {
		if (uafOrRace)
			raceLogger = &uafNestedPrimaryLogger;
#ifdef DATARACE
		else
			raceLogger = &raceNestedPrimaryLogger;
#endif
//		IDType deq1 = -1;
//		IDType deq2 = -1;
//		if (taskIDMap[opIDMap[op1ID].taskID].parentTask.compare("") != 0)
//			deq1 = taskIDMap[taskIDMap[opIDMap[op1ID].taskID].parentTask].deqOpID;
//		if (taskIDMap[opIDMap[op2ID].taskID].parentTask.compare("") != 0)
//			deq2 = taskIDMap[taskIDMap[opIDMap[op2ID].taskID].parentTask].deqOpID;

	} else if (raceType == NESTED_WITH_TASKS_ORDERED) {
		if (uafOrRace)
			raceLogger = &uafNestedOrderedLogger;
#ifdef DATARACE
		else
			raceLogger = &raceNestedOrderedLogger;
#endif
	} else if (raceType == NOTASKRACE_MULTITHREADED || raceType == NOTASKRACE_SINGLETHREADED) {
		if (uafOrRace)
			raceLogger = &uafNoTaskLogger;
#ifdef DATARACE
		else
			raceLogger = &raceNoTaskLogger;
#endif
	} else if (raceType == MULTITHREADED_ALLOC_MEMOP_IN_SAME_TASK) {
		if (uafOrRace)
			raceLogger = &uafAllocMemopSameTaskLogger;
#ifdef DATARACE
		else
			raceLogger = &raceAllocMemopSameTaskLogger;
#endif
	} else if (raceType == SINGLETHREADED_ALLOC_MEMOP_IN_SAME_TASK_FP) {
		return;
//		if (uafOrRace)
//			raceLogger = &uafAllocMemopSameTaskSameThreadLogger;
//		else
//			raceLogger = &raceAllocMemopSameTaskSameThreadLogger;
	} else if (raceType == NONATOMIC_WITH_OTHER) {
		if (uafOrRace)
			raceLogger = &uafNonAtomicOtherLogger;
#ifdef DATARACE
		else
			raceLogger = &raceNonAtomicOtherLogger;
#endif
//		IDType deq1 = -1;
//		IDType deq2 = -1;
//		if (!(taskIDMap[opIDMap[op1ID].taskID].atomic))
//			deq1 = taskIDMap[opIDMap[op1ID].taskID].deqOpID;
//		if (!(taskIDMap[opIDMap[op2ID].taskID].atomic))
//			deq2 = taskIDMap[opIDMap[op2ID].taskID].deqOpID;

	} else {
		if (uafOrRace)
			raceLogger = &uafOtherLogger;
#ifdef DATARACE
		else
			raceLogger = &raceOtherLogger;
#endif
	}


	allLogger->writeLog(line1);
	allLogger->writeLog(lines23);
	allLogger->writeLog(line4);
	allLogger->writeLog(line5);

	if (opIDMap[op1ID].threadID != opIDMap[op2ID].threadID) {
		for (std::set<std::string>::iterator it1 = enqPathTasks1.begin();
				it1 != enqPathTasks1.end(); it1++) {
			for (std::set<std::string>::iterator it2 = enqPathTasks2.begin();
					it2 != enqPathTasks2.end(); it2++) {
				if ((*it1).compare(*it2) == 0) {
					raceType = MULTITHREADED_FROM_SAME_NESTING_LOOP;
					if (uafOrRace)
						raceLogger = &uafMultithreadedSameNestingLoopLogger;
#ifdef DATARACE
					else
						raceLogger = &raceMultithreadedSameNestingLoopLogger;
#endif
					break;
				}
			}
			if (raceType == MULTITHREADED_FROM_SAME_NESTING_LOOP)
				break;
		}
	}

	raceLogger->writeLog(line1);
	raceLogger->writeLog(lines23);
	raceLogger->writeLog(line4);
	raceLogger->writeLog(line5);
}

void UAFDetector::log(bool mt) {

	IDType onlyMultithreadedAllUAFs = 0;
	IDType onlyMultithreadedAllRaces = 0;
	IDType onlyMultithreadedUniqueObjects = 0;
	IDType onlySinglethreadedAllUAFs = 0;
	IDType onlySinglethreadedAllRaces = 0;
	IDType onlySinglethreadedUniqueObjects = 0;
	IDType bothMultithreadedAllUAFs = 0;
	IDType bothMultithreadedAllRaces = 0;
	IDType bothMultithreadedUniqueObjects = 0;
	IDType bothSinglethreadedAllUAFs = 0;
	IDType bothSinglethreadedAllRaces = 0;
	IDType bothSinglethreadedUniqueObjects = 0;

	IDType totalAllocs = allocIDMap.size();

	for (map<IDType, std::multiset<raceDetails> >::iterator allocIt = allocToRaceMap.begin();
			allocIt != allocToRaceMap.end(); allocIt++) {
		std::multiset<raceDetails> raceSet = allocIt->second;

		if (raceSet.size() == 0) continue;

		IDType multithreadedUAFCount = 0, multithreadedRaceCount = 0, multithreadedCount = 0;
		IDType multithreadedUAFAllocMemopInSameTaskCount = 0, multithreadedRaceAllocMemopInSameTaskCount = 0,
				multithreadedAllocMemopInSameTaskCount = 0;
		IDType multithreadedUAFSameNestingLoopCount = 0, multithreadedRaceSameNestingLoopCount = 0,
				multithreadedSameNestingLoopCount = 0;
		IDType multithreadedUAFNoTaskCount = 0, multithreadedRaceNoTaskCount = 0,
				multithreadedNoTaskCount = 0;

		raceDetails tempRace;
		// check for MULTITHREADED uaf
		tempRace.uafOrRace = true;
		tempRace.raceType = MULTITHREADED;
		std::pair<std::multiset<raceDetails>::iterator, std::multiset<raceDetails>::iterator>
			retMultithreadedUAF = raceSet.equal_range(tempRace);
		if (retMultithreadedUAF.first != retMultithreadedUAF.second)
			multithreadedUAFCount = std::distance(retMultithreadedUAF.first, retMultithreadedUAF.second);
		else
			multithreadedUAFCount = 0;
		// check for MULTITHREADED race
		tempRace.uafOrRace = false;
		tempRace.raceType = MULTITHREADED;
		std::pair<std::multiset<raceDetails>::iterator, std::multiset<raceDetails>::iterator>
			retMultithreadedRace = raceSet.equal_range(tempRace);
		if (retMultithreadedRace.first != retMultithreadedRace.second)
			multithreadedRaceCount = std::distance(retMultithreadedRace.first, retMultithreadedRace.second);
		else
			multithreadedRaceCount = 0;
		multithreadedCount = multithreadedUAFCount + multithreadedRaceCount;

		// check for MULTITHREADED_ALLOC_MEMOP_IN_SAME_TASK uaf
		tempRace.uafOrRace = true;
		tempRace.raceType = MULTITHREADED_ALLOC_MEMOP_IN_SAME_TASK;
		std::pair<std::multiset<raceDetails>::iterator, std::multiset<raceDetails>::iterator>
			retMultithreadedUAFAllocMemopInSameTask = raceSet.equal_range(tempRace);
		if (retMultithreadedUAFAllocMemopInSameTask.first != retMultithreadedUAFAllocMemopInSameTask.second)
			multithreadedUAFAllocMemopInSameTaskCount = std::distance(retMultithreadedUAFAllocMemopInSameTask.first,
					retMultithreadedUAFAllocMemopInSameTask.second);
		else
			multithreadedUAFAllocMemopInSameTaskCount = 0;
		// check for MULTITHREADED_ALLOC_MEMOP_IN_SAME_TASK race
		tempRace.uafOrRace = false;
		tempRace.raceType = MULTITHREADED_ALLOC_MEMOP_IN_SAME_TASK;
		std::pair<std::multiset<raceDetails>::iterator, std::multiset<raceDetails>::iterator>
			retMultithreadedRaceAllocMemopInSameTask = raceSet.equal_range(tempRace);
		if (retMultithreadedRaceAllocMemopInSameTask.first != retMultithreadedRaceAllocMemopInSameTask.second)
			multithreadedRaceAllocMemopInSameTaskCount = std::distance(retMultithreadedRaceAllocMemopInSameTask.first,
					retMultithreadedRaceAllocMemopInSameTask.second);
		else
			multithreadedRaceAllocMemopInSameTaskCount = 0;
		multithreadedAllocMemopInSameTaskCount = multithreadedUAFAllocMemopInSameTaskCount + multithreadedRaceAllocMemopInSameTaskCount;

		// check for MULTITHREADED_FROM_SAME_NESTING_LOOP uaf
		tempRace.uafOrRace = true;
		tempRace.raceType = MULTITHREADED_FROM_SAME_NESTING_LOOP;
		std::pair<std::multiset<raceDetails>::iterator, std::multiset<raceDetails>::iterator>
			retMultithreadedUAFSameNestingLoop = raceSet.equal_range(tempRace);
		if (retMultithreadedUAFSameNestingLoop.first != retMultithreadedUAFSameNestingLoop.second)
			multithreadedUAFSameNestingLoopCount = std::distance(retMultithreadedUAFSameNestingLoop.first,
					retMultithreadedUAFSameNestingLoop.second);
		else
			multithreadedUAFSameNestingLoopCount = 0;
		// check for MULTITHREADED_FROM_SAME_NESTING_LOOP race
		tempRace.uafOrRace = false;
		tempRace.raceType = MULTITHREADED_FROM_SAME_NESTING_LOOP;
		std::pair<std::multiset<raceDetails>::iterator, std::multiset<raceDetails>::iterator>
			retMultithreadedRaceSameNestingLoop = raceSet.equal_range(tempRace);
		if (retMultithreadedRaceSameNestingLoop.first != retMultithreadedRaceSameNestingLoop.second)
			multithreadedRaceSameNestingLoopCount = std::distance(retMultithreadedRaceSameNestingLoop.first,
					retMultithreadedRaceSameNestingLoop.second);
		else
			multithreadedRaceSameNestingLoopCount = 0;
		multithreadedSameNestingLoopCount = multithreadedUAFSameNestingLoopCount + multithreadedRaceSameNestingLoopCount;

		// check for NOTASKRACE_MULTITHREADED uaf
		tempRace.uafOrRace = true;
		tempRace.raceType = NOTASKRACE_MULTITHREADED;
		std::pair<std::multiset<raceDetails>::iterator, std::multiset<raceDetails>::iterator>
			retMultithreadedUAFNoTask = raceSet.equal_range(tempRace);
		if (retMultithreadedUAFNoTask.first != retMultithreadedUAFNoTask.second)
			multithreadedUAFNoTaskCount = std::distance(retMultithreadedUAFNoTask.first,
					retMultithreadedUAFNoTask.second);
		else
			multithreadedUAFNoTaskCount = 0;
		// check for NOTASKRACE_MULTITHREADED race
		tempRace.uafOrRace = false;
		tempRace.raceType = NOTASKRACE_MULTITHREADED;
		std::pair<std::multiset<raceDetails>::iterator, std::multiset<raceDetails>::iterator>
			retMultithreadedRaceNoTask = raceSet.equal_range(tempRace);
		if (retMultithreadedRaceNoTask.first != retMultithreadedRaceNoTask.second)
			multithreadedRaceNoTaskCount = std::distance(retMultithreadedRaceNoTask.first,
					retMultithreadedRaceNoTask.second);
		else
			multithreadedRaceNoTaskCount = 0;
		multithreadedNoTaskCount = multithreadedUAFNoTaskCount + multithreadedRaceNoTaskCount;

		IDType singlethreadedUAFCount = 0, singlethreadedRaceCount = 0, singlethreadedCount = 0;
		IDType singlethreadedUAFNestedNestedCount = 0, singlethreadedRaceNestedNestedCount = 0,
				singlethreadedNestedNestedCount = 0;
		IDType singlethreadedUAFNestedPrimaryCount = 0, singlethreadedRaceNestedPrimaryCount = 0,
				singlethreadedNestedPrimaryCount = 0;
		IDType singlethreadedUAFNestedTasksOrderedCount = 0, singlethreadedRaceNestedTasksOrderedCount = 0,
				singlethreadedNestedTasksOrderedCount = 0;
		IDType singlethreadedUAFNonAtomicWithOtherCount = 0, singlethreadedRaceNonAtomicWithOtherCount = 0,
				singlethreadedNonAtomicWithOtherCount = 0;
		IDType singlethreadedUAFNoTaskCount = 0, singlethreadedRaceNoTaskCount = 0,
				singlethreadedNoTaskCount = 0;

		// check for SINGLETHREADED uaf
		tempRace.uafOrRace = true;
		tempRace.raceType = SINGLETHREADED;
		std::pair<std::multiset<raceDetails>::iterator, std::multiset<raceDetails>::iterator>
			retSinglethreadedUAF = raceSet.equal_range(tempRace);
		if (retSinglethreadedUAF.first != retSinglethreadedUAF.second)
			singlethreadedUAFCount = std::distance(retSinglethreadedUAF.first,
					retSinglethreadedUAF.second);
		else
			singlethreadedUAFCount = 0;
		// check for SINGLETHREADED race
		tempRace.uafOrRace = false;
		tempRace.raceType = SINGLETHREADED;
		std::pair<std::multiset<raceDetails>::iterator, std::multiset<raceDetails>::iterator>
			retSinglethreadedRace = raceSet.equal_range(tempRace);
		if (retSinglethreadedRace.first != retSinglethreadedRace.second)
			singlethreadedRaceCount = std::distance(retSinglethreadedRace.first,
					retSinglethreadedRace.second);
		else
			singlethreadedRaceCount = 0;
		singlethreadedCount = singlethreadedUAFCount + singlethreadedRaceCount;

		// check for NESTED_NESTED uaf
		tempRace.uafOrRace = true;
		tempRace.raceType = NESTED_NESTED;
		std::pair<std::multiset<raceDetails>::iterator, std::multiset<raceDetails>::iterator>
			retSinglethreadedUAFNestedNested = raceSet.equal_range(tempRace);
		if (retSinglethreadedUAFNestedNested.first != retSinglethreadedUAFNestedNested.second)
			singlethreadedUAFNestedNestedCount = std::distance(retSinglethreadedUAFNestedNested.first,
					retSinglethreadedUAFNestedNested.second);
		else
			singlethreadedUAFNestedNestedCount = 0;
		// check for NESTED_NESTED race
		tempRace.uafOrRace = false;
		tempRace.raceType = NESTED_NESTED;
		std::pair<std::multiset<raceDetails>::iterator, std::multiset<raceDetails>::iterator>
			retSinglethreadedRaceNestedNested = raceSet.equal_range(tempRace);
		if (retSinglethreadedRaceNestedNested.first != retSinglethreadedRaceNestedNested.second)
			singlethreadedRaceNestedNestedCount = std::distance(retSinglethreadedRaceNestedNested.first,
					retSinglethreadedRaceNestedNested.second);
		else
			singlethreadedRaceNestedNestedCount = 0;
		singlethreadedNestedNestedCount = singlethreadedUAFNestedNestedCount + singlethreadedRaceNestedNestedCount;

		// check for NESTED_PRIMARY uaf
		tempRace.uafOrRace = true;
		tempRace.raceType = NESTED_PRIMARY;
		std::pair<std::multiset<raceDetails>::iterator, std::multiset<raceDetails>::iterator>
			retSinglethreadedUAFNestedPrimary = raceSet.equal_range(tempRace);
		if (retSinglethreadedUAFNestedPrimary.first != retSinglethreadedUAFNestedPrimary.second)
			singlethreadedUAFNestedPrimaryCount = std::distance(retSinglethreadedUAFNestedPrimary.first,
					retSinglethreadedUAFNestedPrimary.second);
		else
			singlethreadedUAFNestedPrimaryCount = 0;
		// check for NESTED_PRIMARY race
		tempRace.uafOrRace = false;
		tempRace.raceType = NESTED_PRIMARY;
		std::pair<std::multiset<raceDetails>::iterator, std::multiset<raceDetails>::iterator>
			retSinglethreadedRaceNestedPrimary = raceSet.equal_range(tempRace);
		if (retSinglethreadedRaceNestedPrimary.first != retSinglethreadedRaceNestedPrimary.second)
			singlethreadedRaceNestedPrimaryCount = std::distance(retSinglethreadedRaceNestedPrimary.first,
					retSinglethreadedRaceNestedPrimary.second);
		else
			singlethreadedRaceNestedPrimaryCount = 0;
		singlethreadedNestedPrimaryCount = singlethreadedUAFNestedPrimaryCount + singlethreadedRaceNestedPrimaryCount;

		// check for NESTED_WITH_TASKS_ORDERED uaf
		tempRace.uafOrRace = true;
		tempRace.raceType = NESTED_WITH_TASKS_ORDERED;
		std::pair<std::multiset<raceDetails>::iterator, std::multiset<raceDetails>::iterator>
			retSinglethreadedUAFNestedTasksOrdered = raceSet.equal_range(tempRace);
		if (retSinglethreadedUAFNestedTasksOrdered.first != retSinglethreadedUAFNestedTasksOrdered.second)
			singlethreadedUAFNestedTasksOrderedCount = std::distance(retSinglethreadedUAFNestedTasksOrdered.first,
					retSinglethreadedUAFNestedTasksOrdered.second);
		else
			singlethreadedUAFNestedTasksOrderedCount = 0;
		// check for NESTED_WITH_TASKS_ORDERED race
		tempRace.uafOrRace = false;
		tempRace.raceType = NESTED_WITH_TASKS_ORDERED;
		std::pair<std::multiset<raceDetails>::iterator, std::multiset<raceDetails>::iterator>
			retSinglethreadedRaceNestedTasksOrdered = raceSet.equal_range(tempRace);
		if (retSinglethreadedRaceNestedTasksOrdered.first != retSinglethreadedRaceNestedTasksOrdered.second)
			singlethreadedRaceNestedTasksOrderedCount = std::distance(retSinglethreadedRaceNestedTasksOrdered.first,
					retSinglethreadedRaceNestedTasksOrdered.second);
		else
			singlethreadedRaceNestedTasksOrderedCount = 0;
		singlethreadedNestedTasksOrderedCount = singlethreadedUAFNestedTasksOrderedCount + singlethreadedRaceNestedTasksOrderedCount;

		// check for NONATOMIC_WITH_OTHER uaf
		tempRace.uafOrRace = true;
		tempRace.raceType = NONATOMIC_WITH_OTHER;
		std::pair<std::multiset<raceDetails>::iterator, std::multiset<raceDetails>::iterator>
			retSinglethreadedUAFNonAtomicWithOther = raceSet.equal_range(tempRace);
		if (retSinglethreadedUAFNonAtomicWithOther.first != retSinglethreadedUAFNonAtomicWithOther.second)
			singlethreadedUAFNonAtomicWithOtherCount = std::distance(retSinglethreadedUAFNonAtomicWithOther.first,
					retSinglethreadedUAFNonAtomicWithOther.second);
		else
			singlethreadedUAFNonAtomicWithOtherCount = 0;
		// check for NONATOMIC_WITH_OTHER race
		tempRace.uafOrRace = false;
		tempRace.raceType = NONATOMIC_WITH_OTHER;
		std::pair<std::multiset<raceDetails>::iterator, std::multiset<raceDetails>::iterator>
			retSinglethreadedRaceNonAtomicWithOther = raceSet.equal_range(tempRace);
		if (retSinglethreadedRaceNonAtomicWithOther.first != retSinglethreadedRaceNonAtomicWithOther.second)
			singlethreadedRaceNonAtomicWithOtherCount = std::distance(retSinglethreadedRaceNonAtomicWithOther.first,
					retSinglethreadedRaceNonAtomicWithOther.second);
		else
			singlethreadedRaceNonAtomicWithOtherCount = 0;
		singlethreadedNonAtomicWithOtherCount = singlethreadedUAFNonAtomicWithOtherCount + singlethreadedRaceNonAtomicWithOtherCount;

		// check for NOTASKRACE_SINGLETHREADED uaf
		tempRace.uafOrRace = true;
		tempRace.raceType = NOTASKRACE_SINGLETHREADED;
		std::pair<std::multiset<raceDetails>::iterator, std::multiset<raceDetails>::iterator>
			retSinglethreadedUAFNoTask = raceSet.equal_range(tempRace);
		if (retSinglethreadedUAFNoTask.first != retSinglethreadedUAFNoTask.second)
			singlethreadedUAFNoTaskCount = std::distance(retSinglethreadedUAFNoTask.first,
					retSinglethreadedUAFNoTask.second);
		else
			singlethreadedUAFNoTaskCount = 0;
		// check for NOTASKRACE_SINGLETHREADED race
		tempRace.uafOrRace = false;
		tempRace.raceType = NOTASKRACE_SINGLETHREADED;
		std::pair<std::multiset<raceDetails>::iterator, std::multiset<raceDetails>::iterator>
			retSinglethreadedRaceNoTask = raceSet.equal_range(tempRace);
		if (retSinglethreadedRaceNoTask.first != retSinglethreadedRaceNoTask.second)
			singlethreadedRaceNoTaskCount = std::distance(retSinglethreadedRaceNoTask.first,
					retSinglethreadedRaceNoTask.second);
		else
			singlethreadedRaceNoTaskCount = 0;
		singlethreadedNoTaskCount = singlethreadedUAFNoTaskCount + singlethreadedRaceNoTaskCount;

		IDType fpCount = 0;
		tempRace.uafOrRace = true;
		tempRace.raceType = SINGLETHREADED_ALLOC_MEMOP_IN_SAME_TASK_FP;
		std::pair<std::multiset<raceDetails>::iterator, std::multiset<raceDetails>::iterator>
			retFP = raceSet.equal_range(tempRace);
		if (retFP.first != retFP.second)
			fpCount = std::distance(retFP.first, retFP.second);
		else
			fpCount = 0;

		bool logAll;
		if (mt)
			logAll = true;
		else
			logAll = false;

		if (singlethreadedCount == 0 && singlethreadedNestedNestedCount == 0 &&
				singlethreadedNestedPrimaryCount == 0 && singlethreadedNestedTasksOrderedCount == 0 &&
				singlethreadedNoTaskCount == 0 && singlethreadedNonAtomicWithOtherCount == 0) {
			onlyMultithreadedAllUAFs += multithreadedUAFAllocMemopInSameTaskCount +
					multithreadedUAFCount + multithreadedUAFNoTaskCount +
					multithreadedUAFSameNestingLoopCount;
			onlyMultithreadedAllRaces += multithreadedRaceAllocMemopInSameTaskCount +
					multithreadedRaceCount + multithreadedRaceNoTaskCount +
					multithreadedRaceSameNestingLoopCount;

			bool objFound = false;
			if (multithreadedSameNestingLoopCount != 0) {
				objFound = true;
				for (std::multiset<raceDetails>::iterator it = retMultithreadedUAFSameNestingLoop.first;
						it != retMultithreadedUAFSameNestingLoop.second; it++)
					log (it->op1, it->op2, it->allocID, true, it->raceType, logAll, ONLY_MULTITHREADED);
				for (std::multiset<raceDetails>::iterator it = retMultithreadedRaceSameNestingLoop.first;
						it != retMultithreadedRaceSameNestingLoop.second; it++)
					log (it->op1, it->op2, it->allocID, false, it->raceType, logAll, ONLY_MULTITHREADED);
			} else if (multithreadedAllocMemopInSameTaskCount != 0) {
				objFound = true;
				for (std::multiset<raceDetails>::iterator it = retMultithreadedUAFAllocMemopInSameTask.first;
						it != retMultithreadedUAFAllocMemopInSameTask.second; it++)
					log (it->op1, it->op2, it->allocID, true, it->raceType, logAll, ONLY_MULTITHREADED);
				for (std::multiset<raceDetails>::iterator it = retMultithreadedRaceAllocMemopInSameTask.first;
						it != retMultithreadedRaceAllocMemopInSameTask.second; it++)
					log (it->op1, it->op2, it->allocID, false, it->raceType, logAll, ONLY_MULTITHREADED);
			} else if (multithreadedCount != 0) {
				objFound = true;
				for (std::multiset<raceDetails>::iterator it = retMultithreadedUAF.first;
						it != retMultithreadedUAF.second; it++)
					log (it->op1, it->op2, it->allocID, true, it->raceType, logAll, ONLY_MULTITHREADED);
				for (std::multiset<raceDetails>::iterator it = retMultithreadedRace.first;
						it != retMultithreadedRace.second; it++)
					log (it->op1, it->op2, it->allocID, false, it->raceType, logAll, ONLY_MULTITHREADED);
			} else if (multithreadedNoTaskCount != 0) {
				objFound = true;
				for (std::multiset<raceDetails>::iterator it = retMultithreadedUAFNoTask.first;
						it != retMultithreadedUAFNoTask.second; it++)
					log (it->op1, it->op2, it->allocID, true, it->raceType, logAll, ONLY_MULTITHREADED);
				for (std::multiset<raceDetails>::iterator it = retMultithreadedRaceNoTask.first;
						it != retMultithreadedRaceNoTask.second; it++)
					log (it->op1, it->op2, it->allocID, false, it->raceType, logAll, ONLY_MULTITHREADED);
			} else if (fpCount == 0) {
				cout << "ERROR: Identified only MULTITHREADED object, but all counts are zero!\n";
				return;
			}

			if (objFound)
				onlyMultithreadedUniqueObjects++;
		} else if (multithreadedAllocMemopInSameTaskCount == 0 && multithreadedCount == 0 &&
				multithreadedNoTaskCount == 0 && multithreadedSameNestingLoopCount == 0) {
			onlySinglethreadedAllUAFs += singlethreadedUAFCount + singlethreadedUAFNestedNestedCount +
					singlethreadedUAFNestedPrimaryCount + singlethreadedUAFNestedTasksOrderedCount +
					singlethreadedUAFNoTaskCount + singlethreadedUAFNonAtomicWithOtherCount;
			onlySinglethreadedAllRaces += singlethreadedRaceCount + singlethreadedRaceNestedNestedCount +
					singlethreadedRaceNestedPrimaryCount + singlethreadedRaceNestedTasksOrderedCount +
					singlethreadedRaceNoTaskCount + singlethreadedRaceNonAtomicWithOtherCount;

			bool objFound = false;
			if (singlethreadedNestedTasksOrderedCount != 0) {
				objFound = true;
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedUAFNestedTasksOrdered.first;
						it != retSinglethreadedUAFNestedTasksOrdered.second; it++)
					log (it->op1, it->op2, it->allocID, true, it->raceType, logAll, ONLY_SINGLETHREADED);
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedRaceNestedTasksOrdered.first;
						it != retSinglethreadedRaceNestedTasksOrdered.second; it++)
					log (it->op1, it->op2, it->allocID, false, it->raceType, logAll, ONLY_SINGLETHREADED);
			} else if (singlethreadedNestedNestedCount != 0) {
				objFound = true;
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedUAFNestedNested.first;
						it != retSinglethreadedUAFNestedNested.second; it++)
					log (it->op1, it->op2, it->allocID, true, it->raceType, logAll, ONLY_SINGLETHREADED);
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedRaceNestedNested.first;
						it != retSinglethreadedRaceNestedNested.second; it++)
					log (it->op1, it->op2, it->allocID, false, it->raceType, logAll, ONLY_SINGLETHREADED);
			} else if (singlethreadedNestedPrimaryCount != 0) {
				objFound = true;
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedUAFNestedPrimary.first;
						it != retSinglethreadedUAFNestedPrimary.second; it++)
					log (it->op1, it->op2, it->allocID, true, it->raceType, logAll, ONLY_SINGLETHREADED);
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedRaceNestedPrimary.first;
						it != retSinglethreadedRaceNestedPrimary.second; it++)
					log (it->op1, it->op2, it->allocID, false, it->raceType, logAll, ONLY_SINGLETHREADED);
			} else if (singlethreadedNonAtomicWithOtherCount != 0) {
				objFound = true;
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedUAFNonAtomicWithOther.first;
						it != retSinglethreadedUAFNonAtomicWithOther.second; it++)
					log (it->op1, it->op2, it->allocID, true, it->raceType, logAll, ONLY_SINGLETHREADED);
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedRaceNonAtomicWithOther.first;
						it != retSinglethreadedRaceNonAtomicWithOther.second; it++)
					log (it->op1, it->op2, it->allocID, false, it->raceType, logAll, ONLY_SINGLETHREADED);
			} else if (singlethreadedCount != 0) {
				objFound = true;
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedUAF.first;
						it != retSinglethreadedUAF.second; it++)
					log (it->op1, it->op2, it->allocID, true, it->raceType, logAll, ONLY_SINGLETHREADED);
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedRace.first;
						it != retSinglethreadedRace.second; it++)
					log (it->op1, it->op2, it->allocID, false, it->raceType, logAll, ONLY_SINGLETHREADED);
			} else if (singlethreadedNoTaskCount != 0) {
				objFound = true;
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedUAFNoTask.first;
						it != retSinglethreadedUAFNoTask.second; it++)
					log (it->op1, it->op2, it->allocID, true, it->raceType, logAll, ONLY_SINGLETHREADED);
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedRaceNoTask.first;
						it != retSinglethreadedRaceNoTask.second; it++)
					log (it->op1, it->op2, it->allocID, false, it->raceType, logAll, ONLY_SINGLETHREADED);
			} else if (fpCount == 0) {
				cout << "ERROR: Identified only SINGLETHREADED object, but all counts are zero!\n";
				return;
			}

			if (objFound)
				onlySinglethreadedUniqueObjects++;
		} else {
			bothMultithreadedAllUAFs += multithreadedUAFAllocMemopInSameTaskCount +
					multithreadedUAFCount + multithreadedUAFNoTaskCount +
					multithreadedUAFSameNestingLoopCount;
			bothSinglethreadedAllUAFs += singlethreadedUAFCount + singlethreadedUAFNestedNestedCount +
					singlethreadedUAFNestedPrimaryCount + singlethreadedUAFNestedTasksOrderedCount +
					singlethreadedUAFNoTaskCount + singlethreadedUAFNonAtomicWithOtherCount;
			bothMultithreadedAllRaces += multithreadedRaceAllocMemopInSameTaskCount +
					multithreadedRaceCount + multithreadedRaceNoTaskCount +
					multithreadedRaceSameNestingLoopCount;
			bothSinglethreadedAllRaces += singlethreadedRaceCount + singlethreadedRaceNestedNestedCount +
					singlethreadedRaceNestedPrimaryCount + singlethreadedRaceNestedTasksOrderedCount +
					singlethreadedRaceNoTaskCount + singlethreadedRaceNonAtomicWithOtherCount;

			bool objFound = false;
			if (multithreadedSameNestingLoopCount != 0) {
				objFound = true;
				for (std::multiset<raceDetails>::iterator it = retMultithreadedUAFSameNestingLoop.first;
						it != retMultithreadedUAFSameNestingLoop.second; it++)
					log (it->op1, it->op2, it->allocID, true, it->raceType, logAll, BOTH_MULTITHREADED);
				for (std::multiset<raceDetails>::iterator it = retMultithreadedRaceSameNestingLoop.first;
						it != retMultithreadedRaceSameNestingLoop.second; it++)
					log (it->op1, it->op2, it->allocID, false, it->raceType, logAll, BOTH_MULTITHREADED);
			} else if (multithreadedAllocMemopInSameTaskCount != 0) {
				objFound = true;
				for (std::multiset<raceDetails>::iterator it = retMultithreadedUAFAllocMemopInSameTask.first;
						it != retMultithreadedUAFAllocMemopInSameTask.second; it++)
					log (it->op1, it->op2, it->allocID, true, it->raceType, logAll, BOTH_MULTITHREADED);
				for (std::multiset<raceDetails>::iterator it = retMultithreadedRaceAllocMemopInSameTask.first;
						it != retMultithreadedRaceAllocMemopInSameTask.second; it++)
					log (it->op1, it->op2, it->allocID, false, it->raceType, logAll, BOTH_MULTITHREADED);
			} else if (multithreadedCount != 0) {
				objFound = true;
				for (std::multiset<raceDetails>::iterator it = retMultithreadedUAF.first;
						it != retMultithreadedUAF.second; it++)
					log (it->op1, it->op2, it->allocID, true, it->raceType, logAll, BOTH_MULTITHREADED);
				for (std::multiset<raceDetails>::iterator it = retMultithreadedRace.first;
						it != retMultithreadedRace.second; it++)
					log (it->op1, it->op2, it->allocID, false, it->raceType, logAll, BOTH_MULTITHREADED);
			} else if (multithreadedNoTaskCount != 0) {
				objFound = true;
				for (std::multiset<raceDetails>::iterator it = retMultithreadedUAFNoTask.first;
						it != retMultithreadedUAFNoTask.second; it++)
					log (it->op1, it->op2, it->allocID, true, it->raceType, logAll, BOTH_MULTITHREADED);
				for (std::multiset<raceDetails>::iterator it = retMultithreadedRaceNoTask.first;
						it != retMultithreadedRaceNoTask.second; it++)
					log (it->op1, it->op2, it->allocID, false, it->raceType, logAll, BOTH_MULTITHREADED);
			} else if (fpCount == 0) {
				cout << "ERROR: Identified both MULTITHREADED object, but all counts are zero!\n";
				return;
			}

			if (objFound)
				bothMultithreadedUniqueObjects++;

			objFound = false;
			if (singlethreadedNestedTasksOrderedCount != 0) {
				objFound = true;
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedUAFNestedTasksOrdered.first;
						it != retSinglethreadedUAFNestedTasksOrdered.second; it++)
					log (it->op1, it->op2, it->allocID, true, it->raceType, logAll, BOTH_SINGLETHREADED);
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedRaceNestedTasksOrdered.first;
						it != retSinglethreadedRaceNestedTasksOrdered.second; it++)
					log (it->op1, it->op2, it->allocID, false, it->raceType, logAll, BOTH_SINGLETHREADED);
			} else if (singlethreadedNestedNestedCount != 0) {
				objFound = true;
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedUAFNestedNested.first;
						it != retSinglethreadedUAFNestedNested.second; it++)
					log (it->op1, it->op2, it->allocID, true, it->raceType, logAll, BOTH_SINGLETHREADED);
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedRaceNestedNested.first;
						it != retSinglethreadedRaceNestedNested.second; it++)
					log (it->op1, it->op2, it->allocID, false, it->raceType, logAll, BOTH_SINGLETHREADED);
			} else if (singlethreadedNestedPrimaryCount != 0) {
				objFound = true;
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedUAFNestedPrimary.first;
						it != retSinglethreadedUAFNestedPrimary.second; it++)
					log (it->op1, it->op2, it->allocID, true, it->raceType, logAll, BOTH_SINGLETHREADED);
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedRaceNestedPrimary.first;
						it != retSinglethreadedRaceNestedPrimary.second; it++)
					log (it->op1, it->op2, it->allocID, false, it->raceType, logAll, BOTH_SINGLETHREADED);
			} else if (singlethreadedNonAtomicWithOtherCount != 0) {
				objFound = true;
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedUAFNonAtomicWithOther.first;
						it != retSinglethreadedUAFNonAtomicWithOther.second; it++)
					log (it->op1, it->op2, it->allocID, true, it->raceType, logAll, BOTH_SINGLETHREADED);
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedRaceNonAtomicWithOther.first;
						it != retSinglethreadedRaceNonAtomicWithOther.second; it++)
					log (it->op1, it->op2, it->allocID, false, it->raceType, logAll, BOTH_SINGLETHREADED);
			} else if (singlethreadedCount != 0) {
				objFound = true;
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedUAF.first;
						it != retSinglethreadedUAF.second; it++)
					log (it->op1, it->op2, it->allocID, true, it->raceType, logAll, BOTH_SINGLETHREADED);
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedRace.first;
						it != retSinglethreadedRace.second; it++)
					log (it->op1, it->op2, it->allocID, false, it->raceType, logAll, BOTH_SINGLETHREADED);
			} else if (singlethreadedNoTaskCount != 0) {
				objFound = true;
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedUAFNoTask.first;
						it != retSinglethreadedUAFNoTask.second; it++)
					log (it->op1, it->op2, it->allocID, true, it->raceType, logAll, BOTH_SINGLETHREADED);
				for (std::multiset<raceDetails>::iterator it = retSinglethreadedRaceNoTask.first;
						it != retSinglethreadedRaceNoTask.second; it++)
					log (it->op1, it->op2, it->allocID, false, it->raceType, logAll, BOTH_SINGLETHREADED);
			} else if (fpCount == 0) {
				cout << "ERROR: Identified both SINGLETHREADED object, but all counts are zero!\n";
				return;
			}

			if (objFound)
				bothSinglethreadedUniqueObjects++;
		}

	}

	cout << "OUTPUT: Total allocs in trace: " << totalAllocs << "\n";
	cout << "OUTPUT: Allocs with uafs/races: " << onlyMultithreadedUniqueObjects + onlySinglethreadedUniqueObjects +
			bothMultithreadedUniqueObjects << "\n";
	cout << "OUTPUT: UAFs - only multithreaded: " << onlyMultithreadedAllUAFs << "\n";
	cout << "OUTPUT: Races - only multithreaded: " << onlyMultithreadedAllRaces << "\n";
	cout << "OUTPUT: Objects - only multithreaded: " << onlyMultithreadedUniqueObjects << "\n";
	cout << "OUTPUT: UAFs - only singlethreaded: " << onlySinglethreadedAllUAFs << "\n";
	cout << "OUTPUT: Races - only singlethreaded: " << onlySinglethreadedAllRaces << "\n";
	cout << "OUTPUT: Objects - only singlethreaded: " << onlySinglethreadedUniqueObjects << "\n";
	cout << "OUTPUT: UAFs - both multithreaded: " << bothMultithreadedAllUAFs << "\n";
	cout << "OUTPUT: Races - both multithreaded: " << bothMultithreadedAllRaces << "\n";
	cout << "OUTPUT: Objects - both multithreaded: " << bothMultithreadedUniqueObjects << "\n";
	cout << "OUTPUT: UAFs - both singlethreaded: " << bothSinglethreadedAllUAFs << "\n";
	cout << "OUTPUT: Races - both singlethreaded: " << bothSinglethreadedAllRaces << "\n";
	cout << "OUTPUT: Objects - both singlethreaded: " << bothSinglethreadedUniqueObjects << "\n";

//	cout << "README: UAFs - only multithreaded - are all the UAFs reported on objects that only had multithreaded races\n";
//	cout << "README: Objects - only multithreaded - are all the objects that only had multithreaded races (i.e. uniqued count)\n";
//	cout << "README: UAFs - only singlethreaded - are all the UAFs reported on objects that only had singlethreaded races\n";
//	cout << "README: Objects - only singlethreaded - are all the objects that only had singlethreaded races (i.e. uniqued count)\n";
//	cout << "README: UAFs - both multithreaded - are all the multithreaded UAFs reported on objects that had both kinds of races\n";
//	cout << "README: Objects - both multithreaded - are all the objects that had both kinds of races\n";
//	cout << "README: UAFs - both singlethreaded - are all the singlethreaded UAFs reported on objects that had both kinds of races\n";
//	cout << "README: Objects - both singlethreaded - are all the objects that had both kinds of races (should be same as Objects - both multithreaded)\n";
}
