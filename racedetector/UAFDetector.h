/*
 * UAFDetector.h
 *
 *  Created on: 30-May-2014
 *      Author: shalini
 */

#include <map>
#include <string>
#include <set>
#include <iostream>
#include <utility>
#include <vector>
#include <cassert>

#include <config.h>
#include <debugconfig.h>
#include <logging/Logger.h>

#ifndef UAFDETECTOR_H_
#define UAFDETECTOR_H_

class HBGraph;

enum RaceKind {
	MULTITHREADED,
	MULTITHREADED_ALLOC_MEMOP_IN_SAME_TASK,
	MULTITHREADED_FROM_SAME_NESTING_LOOP,
	SINGLETHREADED,
	SINGLETHREADED_ALLOC_MEMOP_IN_SAME_TASK_FP,
	NESTED_NESTED,
	NESTED_PRIMARY,
	NESTED_WITH_TASKS_ORDERED,
	NONATOMIC_WITH_OTHER,
	NOTASKRACE,
	UNKNOWN
};

class UAFDetector {
public:
	UAFDetector();

	virtual ~UAFDetector();
	void ERRLOG(std::string error) {
		cout << error << endl;
	}

	// Stores threadID, taskID and opType of an operation.
	class opDetails {
	public:
		IDType threadID;
		std::string taskID;
		std::string opType;
		IDType blockID;
		IDType nodeID;

		IDType nextOpInThread;
		IDType nextOpInTask;
		IDType nextOpInBlock;
		IDType prevOpInBlock;

		opDetails() {
			threadID = -1;
			taskID = "";
			opType = "";
			blockID = -1;
			nodeID = -1;
			nextOpInThread = -1;
			nextOpInTask = -1;
			nextOpInBlock = -1;
			prevOpInBlock = -1;
		}

		void printOpDetails() {
			cout << "threadID " << threadID << " taskID " << taskID
				 << " blockID " << blockID << " opType " << opType << " node " << nodeID << "\n";
			cout << "next-op-in-thread " << nextOpInThread << " next-op-in-task "
				 << nextOpInTask << " next-op-in-block " << nextOpInBlock;
		}
	};

	// Maps operationID to its threadID, taskID and type.
	map<IDType, opDetails> opIDMap;

	class setOfOps {
	public:
		set<IDType> opSet;
		setOfOps() {
			opSet = set<IDType>();
		}
		void printDetails() {
			for (set<IDType>::iterator it = opSet.begin(); it != opSet.end(); it++) {
				cout << *it << " ";
			}
		}
	};
	// Maps node ID to set of opID
	map<IDType, setOfOps> nodeIDMap;

	// Stores pause-resume pair
	class pauseResumeResetTuple {
	public:
		IDType pauseOp;
		IDType resumeOp;
		IDType resetOp;

		pauseResumeResetTuple() {
			pauseOp = -1;
			resumeOp = -1;
			resetOp = -1;
		}

		void printPauseResumeResetTupleDetails() const {
			cout << "Pause: " << pauseOp << " Resume: " << resumeOp
			     << " Reset: " << resetOp;
		}
	};


	// Stores (1) enq, deq, first-pause, last-resume and end op IDs,
	//		  (2) parent task ID, (3) flag denoting whether the task is atomic,
	//		  (4) the first block and the last block IDs of a task.
	class taskDetails {
	public:
		IDType firstPauseOpID;
		IDType lastResumeOpID;
		IDType deqOpID;
		IDType endOpID;
		IDType enqOpID;

		std::string parentTask; // ID of immediate parent task
		bool atomic;

		vector<pauseResumeResetTuple> pauseResumeResetSequence;

		IDType firstBlockID; // ID of the first block in task
		IDType lastBlockID;  // ID of the last block in task

		taskDetails() {
			firstPauseOpID = -1;
			lastResumeOpID = -1;
			deqOpID = -1;
			endOpID = -1;
			enqOpID = -1;

			parentTask = "";
			atomic = true;

			firstBlockID = -1;
			lastBlockID = -1;
		}

		void printTaskDetails() {
			cout << "first-pause " << firstPauseOpID
				 << " last-resume " << lastResumeOpID << " deq " << deqOpID
				 << " end " << endOpID << " enq " << enqOpID
				 << " parent task " << parentTask << " atomic " << (atomic? "true": "false")
				 << " first-block " << firstBlockID << " last-block " << lastBlockID;

			cout << "\nPauseResumeResetSequence: ";
			for (vector<pauseResumeResetTuple>::iterator it = pauseResumeResetSequence.begin();
					it != pauseResumeResetSequence.end(); it++) {
				cout << "(";
				it->printPauseResumeResetTupleDetails();
				cout << ")\n";
			}
		}
	};

	// Maps task ID to its enq-op, deq-op, etc.
	map<std::string, taskDetails> taskIDMap;

	class nestingLoopDetails {
	public:
		vector<pauseResumeResetTuple> pauseResumeResetSet;

		nestingLoopDetails() {
			pauseResumeResetSet = vector<pauseResumeResetTuple>();
		}

		void printNestingLoopDetails() {
			cout << "Pause-Resume-Reset: ";
			for (vector<pauseResumeResetTuple>::iterator it =
			    pauseResumeResetSet.begin(); it != pauseResumeResetSet.end(); it++) {
				it->printPauseResumeResetTupleDetails();
				cout << "\n";
			}
		}
	};

	// Maps address of a shared variable to the corresponding pause/resume/reset ops.
	map<std::string, nestingLoopDetails> nestingLoopMap;

	class threadDetails {
	public:
		IDType firstOpID;
		IDType threadinitOpID;
		IDType threadexitOpID;
		IDType forkOpID; // op that forked this thread
		IDType joinOpID; // op that joined this thread

		IDType firstBlockID;
		IDType lastBlockID;
		IDType enterloopBlockID;
		IDType exitloopBlockID;

		threadDetails() {
			firstOpID = -1;
			threadinitOpID = -1;
			threadexitOpID = -1;
			forkOpID = -1;
			joinOpID = -1;

			firstBlockID = -1;
			lastBlockID = -1;
			enterloopBlockID = -1;
			exitloopBlockID = -1;
		}

		void printThreadDetails() {
			cout << " first-op " << firstOpID << " threadinit " << threadinitOpID << " threadexit " << threadexitOpID
				 << " enterloop " << enterloopBlockID << " exitloop " << exitloopBlockID
				 << " first-block " << firstBlockID << " last-block " << lastBlockID
				 << " fork " << forkOpID << " join " << joinOpID;
		}
	};

	map<IDType, threadDetails> threadIDMap;

	class blockDetails {
	public:
		IDType threadID;
		std::string taskID;

		IDType firstOpInBlock;
		IDType lastOpInBlock;

		IDType nextBlockInTask;
		IDType nextBlockInThread;
		IDType prevBlockInThread;

		set<IDType> enqSet; // Set of enqs in the block

		blockDetails() {
			threadID = -1;
			taskID = "";

			firstOpInBlock = -1;
			lastOpInBlock = -1;

			nextBlockInTask = -1;
			nextBlockInThread = -1;
			prevBlockInThread = -1;

			enqSet = set<IDType>();
		}

		void printBlockDetails() {
			cout << "ThreadID " << threadID << " Task ID " << taskID
				 << " First-op " << firstOpInBlock << " Last-op " << lastOpInBlock
				 << " next-block-in-task " << nextBlockInTask
				 << " next-block-in-thread " << nextBlockInThread
				 << " prev-block-in-thread " << prevBlockInThread;

			cout << "\nEnqs: ";
			for (set<IDType>::iterator it = enqSet.begin(); it != enqSet.end(); it++) {
				cout << *it << " ";
			}
		}
	};

	map<IDType, blockDetails> blockIDMap;

	class enqOpDetails {
	public:
		std::string taskEnqueued;
		IDType targetThread;

		enqOpDetails() {
			taskEnqueued = "";
			targetThread = -1;
		}

		void printEnqDetails() {
			cout << "Task enqueued " << taskEnqueued << " to target thread " << targetThread;
		}
	};
	// Maps the op ID of an enq operation to its arguments
	map<IDType, enqOpDetails> enqToTaskEnqueued;

	// Maps the op ID of pause/resume/reset ops to the shared variable
	map<IDType, std::string> pauseResumeResetOps;

	class memoryOpDetails {
	public:
		std::string startingAddress;
		IDType range;

		memoryOpDetails() {
			startingAddress = "";
			range = -1;
		}

		void printMemOpDetails() {
			cout << "starting address: " << startingAddress << " range: " << range;
		}
	};

	// maps the op ID of a memory operation to the starting address and range of memory block involved.
	map<IDType, memoryOpDetails> allocSet;
	map<IDType, memoryOpDetails> freeSet;
	map<IDType, memoryOpDetails> readSet;
	map<IDType, memoryOpDetails> writeSet;

	class allocOpDetails {
	public:
		set<IDType> readOps;
		set<IDType> writeOps;
		set<IDType> freeOps;
		set<IDType> nodes;

		allocOpDetails() {
			readOps = set<IDType>();
			writeOps = set<IDType>();
			freeOps = set<IDType>();
			nodes = set<IDType>();
		}

		void printDetails() {
			cout << "read ops: ";
			for (set<IDType>::iterator it = readOps.begin(); it != readOps.end(); it++)
				cout << *it << " ";
			cout << "\nwrite ops: ";
			for (set<IDType>::iterator it = writeOps.begin(); it != writeOps.end(); it++)
				cout << *it << " ";
			cout << "\nfree ops: ";
			for (set<IDType>::iterator it = freeOps.begin(); it != freeOps.end(); it++)
				cout << *it << " ";
			cout << "\nnodes: ";
			for (set<IDType>::iterator it = nodes.begin(); it != nodes.end(); it++)
				cout << *it << " ";
		}
	};

	map<IDType, allocOpDetails> allocIDMap;

	class freeOpDetails {
	public:
		IDType allocOpID;
		set<IDType> readOps;
		set<IDType> writeOps;
		set<IDType> nodes;

		freeOpDetails() {
			allocOpID = -1;
			readOps = set<IDType>();
			writeOps = set<IDType>();
			nodes = set<IDType>();
		}

		void printDetails() {
			cout << "alloc op: " << allocOpID << endl;
			cout << "read ops: ";
			for (set<IDType>::iterator it = readOps.begin(); it != readOps.end(); it++)
				cout << *it << " ";
			cout << "\nwrite ops: ";
			for (set<IDType>::iterator it = writeOps.begin(); it != writeOps.end(); it++)
				cout << *it << " ";
			cout << "\nnodes: ";
			for (set<IDType>::iterator it = nodes.begin(); it != nodes.end(); it++)
				cout << *it << " ";
		}
	};

	map<IDType, freeOpDetails> freeIDMap;

	class lockOpDetails {
	public:
		IDType threadID;
		std::string lockID;

		lockOpDetails() {
			threadID = -1;
			lockID = "";
		}

		void printDetails() {
			cout << "thread: " << threadID << " lockID: " << lockID;
		}
	};

	map<IDType, lockOpDetails> waitSet;
	map<IDType, lockOpDetails> notifySet;
	map<IDType, lockOpDetails> notifyAllSet;


	// shared variable to set of notify ops
	map<std::string, setOfOps> lockToNotify;
	// shared variable to set of notifyall ops
	map<std::string, setOfOps> lockToNotifyAll;

	// map notify op to corresponding wait
	map<IDType, IDType> notifyToWait;
	// map notifyall op to corresponding set of wait ops
	map<IDType, setOfOps> notifyAllToWaitSet;
	// map wait op to corresponding notify/notifyall
	map<IDType, IDType> waitToNotify;

	HBGraph* graph;

	void initGraph(IDType countOfOps, IDType countOfNodes);

//	int addEdges(Logger &logger);
	int addEdges();

	IDType findUAF();
	IDType findUAFUsingNodes();

	IDType findDataRaces();
	IDType findDataRacesUsingNodes();

	void initLog(std::string traceFileName);

	void log();

#ifdef GRAPHDEBUG
	void printEdges();
#endif

private:
	// All the following functions: Return -1 if some error, 1 if atleast one edge is added, 0 if no edges are added.
	int add_LoopPO_Fork_Join_Edges();
	int add_WaitNotify_Edges();
	int add_TaskPO_EnqueueSTOrMT_Edges();
	int add_PauseSTMT_ResumeSTMT_Edges();
	int add_FifoAtomic_NoPre_Edges();
	int add_FifoNested_1_2_Gen_EnqResetST_1_Edges();
	int add_EnqReset_ST_2_3_Edges();
	int addTransSTOrMTEdges();

	unsigned long long uafCount, raceCount;
	unsigned long long uniqueUafCount, uniqueRaceCount;

	Logger uafAllLogger, raceAllLogger;
	Logger uafNestingLogger, raceNestingLogger;
	Logger uafNoNestingLogger, raceNoNestingLogger;
	Logger uafTaskLogger, raceTaskLogger;
	Logger uafNoTaskLogger, raceNoTaskLogger;
	Logger uafEnqPathLogger, raceEnqPathLogger;
	Logger uafAllocMemopSameTaskLogger, raceAllocMemopSameTaskLogger;
	Logger uafNestedNestedLogger, raceNestedNestedLogger;
	Logger uafNestedPrimaryLogger, raceNestedPrimaryLogger;
	Logger uafNestedOrderedLogger, raceNestedOrderedLogger;
	Logger uafOtherLogger, raceOtherLogger;
	Logger uafNonAtomicOtherLogger, raceNonAtomicOtherLogger;
	Logger uafMultithreadedSameNestingLoopLogger, raceMultithreadedSameNestingLoopLogger;

	class raceDetails {
	public:
		IDType op1;
		IDType op2;
		std::string op1Task, op2Task;
		IDType allocID;
		bool uafOrRace; // true if uaf, false if race
		RaceKind raceType;

		raceDetails() {
			op1 = -1; op2 = -1;
			op1Task = ""; op2Task = "";
			allocID = -1;
			uafOrRace = true;
			raceType = UNKNOWN;
		}

		bool operator < (const raceDetails& param) const {
			if (this->uafOrRace && !param.uafOrRace)
				return true;
			else if (!(this->uafOrRace) && param.uafOrRace)
				return false;
			else
				return (this->raceType < param.raceType);
		}
	};

//	map<IDType, std::vector<raceDetails> > allocToRaceMap;
	map<IDType, std::multiset<raceDetails> > allocToRaceMap;

	void getRaceKind(raceDetails &race);


	void log(IDType op1ID, IDType op2ID, IDType opAllocID,
			bool uafOrRace, RaceKind raceType);

	std::string findPreviousTaskOfOp(IDType op);
	void insertRace(raceDetails race);
};

class HBGraph {
public:

	class adjListNode {
	public:
		IDType nodeID;
		IDType blockID;

		adjListNode() {
			nodeID = -1;
			blockID = -1;
		}

		adjListNode(IDType block) {
			nodeID = -1;
			blockID = block;
		}

		adjListNode(IDType op, IDType block) {
			nodeID = op;
			blockID = block;
		}

		bool operator < (const adjListNode & param) const {
			return (this->blockID < param.blockID);
		}
	};

public:
	HBGraph();
	HBGraph(IDType countOfOps, IDType countOfBlocks,
			map<IDType, UAFDetector::opDetails> opMap,
			map<IDType, UAFDetector::blockDetails> blockMap,
			map<IDType, UAFDetector::setOfOps> nodeMap);
	virtual ~HBGraph();

	IDType totalBlocks;
	IDType totalOps;
	bool** opAdjMatrix;
	bool** blockAdjMatrix;
	std::multiset<adjListNode>* opAdjList;
	std::multiset<adjListNode>* blockAdjList;


	unsigned long long numOfOpEdges;
	unsigned long long numOfBlockEdges;
	unsigned long long numOfOpEdgesRemoved;

	// Return -1 if error, 1 if the edge was newly added, 0 if edge already present.
	int addOpEdge(IDType sourceOp, IDType destinationOp, IDType sourceBlock=0, IDType destinationBlock=0);
	//int addBlockEdge(IDType sourceBlock, IDType destinationBlock);
	int removeOpEdge(IDType sourceOp, IDType destinationOp, IDType sourceBlock=0, IDType destinationBlock=0);
	int removeOpEdgesToBlock(std::multiset<HBGraph::adjListNode>::iterator first,
			std::multiset<HBGraph::adjListNode>::iterator last,
			IDType sourceNode, IDType destinationBlock);

	// Return 1 if edge exists, 0 if not, -1 if adjMatrix and adjList are out of sync.
	int blockEdgeExists(IDType sourceBlock, IDType destinationBlock);
	int opEdgeExists(IDType sourceOp, IDType destinationOp, IDType sourceBlock=0, IDType destinationBlock=0);

	void printGraph();

private:
	map<IDType, UAFDetector::opDetails> opIDMap;
	map<IDType, UAFDetector::blockDetails> blockIDMap;
	map<IDType, UAFDetector::setOfOps> nodeIDMap;

	bool blockEdgeExistsinList(IDType source, IDType destination) {
		adjListNode destNode(destination);
		std::multiset<adjListNode>::iterator it = blockAdjList[source].find(destNode);
		if (it != blockAdjList[source].end()) {
#ifdef SANITYCHECK
			assert(it->blockID == destination);
#endif
			return true;
		} else
			return false;
	}

	bool opEdgeExistsinList(IDType source, IDType destination, IDType sourceBlock=0, IDType destinationBlock=0) {
		// The arguments are nodes, not ops!
		if (destinationBlock == 0) {
			IDType destinationOp = *(nodeIDMap[destination].opSet.begin());
			destinationBlock = opIDMap[destinationOp].blockID;
		}
		adjListNode destNode(destination, destinationBlock);
		std::pair<std::multiset<adjListNode>::iterator, std::multiset<adjListNode>::iterator>
			ret = opAdjList[source].equal_range(destNode);
		if (ret.first != ret.second) {
			for (std::multiset<adjListNode>::iterator it = ret.first; it != ret.second; it++) {
#ifdef SANITYCHECK
				assert(it->blockID == destinationBlock);
#endif
				if (it->nodeID == destination)
					return true;
			}
			return false;
		} else
			return false;

	}
};

#endif /* UAFDETECTOR_H_ */
