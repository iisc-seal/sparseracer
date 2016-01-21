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
#include <climits>

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
	NOTASKRACE_SINGLETHREADED,
	NOTASKRACE_MULTITHREADED,
	UNKNOWN
};

enum RaceKindByThread {
	ONLY_MULTITHREADED,
	ONLY_SINGLETHREADED,
	BOTH_MULTITHREADED,
	BOTH_SINGLETHREADED
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
		IDType nodeID;

		IDType nextOpInThread;
		IDType prevOpInThread;
		IDType nextOpInTask;

		opDetails() {
			threadID = -1;
			taskID = "";
			opType = "";
			nodeID = -1;
			nextOpInThread = -1;
			nextOpInTask = -1;
			prevOpInThread = -1;
		}

		void printOpDetails() {
			cout << "threadID " << threadID << " taskID " << taskID
				 << " opType " << opType << " node " << nodeID << "\n";
			cout << "next-op-in-thread " << nextOpInThread << " prev-op-in-thread " << prevOpInThread
				 << " next-op-in-task " << nextOpInTask;
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

		IDType firstOpInTaskID;
		IDType lastOpInTaskID;

		std::string parentTask; // ID of immediate parent task
		bool atomic;
		IDType priority;

		vector<pauseResumeResetTuple> pauseResumeResetSequence;

		taskDetails() {
			firstPauseOpID = -1;
			lastResumeOpID = -1;
			deqOpID = -1;
			endOpID = -1;
			enqOpID = -1;

			firstOpInTaskID = -1;

			parentTask = "";
			atomic = true;
			priority = INT_MAX;

		}

		void printTaskDetails() {
			cout << "priority " << priority << " first-pause " << firstPauseOpID
				 << " last-resume " << lastResumeOpID << " deq " << deqOpID
				 << " end " << endOpID << " enq " << enqOpID << " first-op-in-task " << firstOpInTaskID
				 << " parent task " << parentTask << " atomic " << (atomic? "true": "false");

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
		IDType enterloopID;
		IDType exitloopID;

		IDType lastOpInThreadID;

		threadDetails() {
			firstOpID = -1;
			threadinitOpID = -1;
			threadexitOpID = -1;
			forkOpID = -1;
			joinOpID = -1;
			enterloopID = -1;
			exitloopID = -1;
			lastOpInThreadID = -1;
		}

		void printThreadDetails() {
			cout << " first-op " << firstOpID << " threadinit " << threadinitOpID << " threadexit " << threadexitOpID
				 << " fork " << forkOpID << " join " << joinOpID << " enterloop " << enterloopID << " exitloop " << exitloopID
				 << " lastOpInThread " << lastOpInThreadID;
		}
	};

	map<IDType, threadDetails> threadIDMap;



	class enqOpDetails {
	public:
		std::string taskEnqueued;
		IDType targetThread;
		IDType priority;

		enqOpDetails() {
			taskEnqueued = "";
			targetThread = -1;
			priority = INT_MAX;
		}

		void printEnqDetails() {
			cout << "Task enqueued " << taskEnqueued << " (priority "
				 << priority << ") to target thread " << targetThread;
		}
	};
	// Maps the op ID of an enq operation to its arguments
	map<IDType, enqOpDetails> enqToTaskEnqueued;

	// Maps the op ID of pause/resume/reset ops to the shared variable
	// not used!
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

	// Change this! Keep only a map from free to alloc
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

	// not used!
	map<IDType, lockOpDetails> waitSet;
	// not used!
	map<IDType, lockOpDetails> notifySet;
	// not used!
	map<IDType, lockOpDetails> notifyAllSet;


	// shared variable to set of notify ops
	// not used!
	map<std::string, setOfOps> lockToNotify;
	// shared variable to set of notifyall ops
	// not used!
	map<std::string, setOfOps> lockToNotifyAll;

	// map notify op to corresponding wait
	map<IDType, IDType> notifyToWait;
	// map notifyall op to corresponding set of wait ops
	map<IDType, setOfOps> notifyAllToWaitSet;
	// map wait op to corresponding notify/notifyall
	// not used!
	map<IDType, IDType> waitToNotify;

	HBGraph* graph;

	void initGraph(IDType countOfOps);
	void outputAllConflictingOps(string outUAFFileName, string outUAFUniqueFileName,
			string outRaceFileName, string outRaceUniqueFileName);
	int filterInput(string inFileName, string outFileName);

//	int addEdges(Logger &logger);
	int addEdges();

	IDType findUAF();
	IDType findUAFUsingNodes();

	IDType findDataRaces();
	IDType findDataRacesUsingNodes();

	void initLog(std::string traceFileName);

	void log(bool mt); // true if we are running mtHB, false if richHB

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
	Logger uafAllDebugLogger, raceAllDebugLogger;
	Logger uafAllUniqueLogger, raceAllUniqueLogger;
	Logger uafNestingLogger, raceNestingLogger;
	Logger uafNoNestingLogger, raceNoNestingLogger;
	Logger uafTaskLogger, raceTaskLogger;
	Logger uafNoTaskLogger, raceNoTaskLogger;
	Logger uafEnqPathLogger, raceEnqPathLogger;
	Logger uafAllocMemopSameTaskLogger, raceAllocMemopSameTaskLogger;
	Logger uafAllocMemopSameTaskSameThreadLogger, raceAllocMemopSameTaskSameThreadLogger;
	Logger uafNestedNestedLogger, raceNestedNestedLogger;
	Logger uafNestedPrimaryLogger, raceNestedPrimaryLogger;
	Logger uafNestedOrderedLogger, raceNestedOrderedLogger;
	Logger uafOtherLogger, raceOtherLogger;
	Logger uafNonAtomicOtherLogger, raceNonAtomicOtherLogger;
	Logger uafMultithreadedSameNestingLoopLogger, raceMultithreadedSameNestingLoopLogger;

	Logger uafOnlyMultiLogger, raceOnlyMultiLogger;
	Logger uafOnlySingleLogger, raceOnlySingleLogger;
	Logger uafBothMultiLogger, raceBothMultiLogger;
	Logger uafBothSingleLogger, raceBothSingleLogger;

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

		void printRaceKind() const {
			switch(raceType) {
			case MULTITHREADED:
				cout << "MULTITHREADED";
				break;
			case MULTITHREADED_ALLOC_MEMOP_IN_SAME_TASK:
				cout << "MULTITHREADED_ALLOC_MEMOP_IN_SAME_TASK";
				break;
			case MULTITHREADED_FROM_SAME_NESTING_LOOP:
				cout << "MULTITHREADED_FROM_SAME_NESTING_LOOP";
				break;
			case SINGLETHREADED:
				cout << "SINGLETHREADED";
				break;
			case SINGLETHREADED_ALLOC_MEMOP_IN_SAME_TASK_FP:
				cout << "SINGLETHREADED_ALLOC_MEMOP_IN_SAME_TASK_FP";
				break;
			case NESTED_NESTED:
				cout << "NESTED_NESTED";
				break;
			case NESTED_PRIMARY:
				cout << "NESTED_PRIMARY";
				break;
			case NESTED_WITH_TASKS_ORDERED:
				cout << "NESTED_WITH_TASKS_ORDERED";
				break;
			case NONATOMIC_WITH_OTHER:
				cout << "NONATOMIC_WITH_OTHER";
				break;
			case NOTASKRACE_MULTITHREADED:
				cout << "NOTASKRACE_MULTITHREADED";
				break;
			case NOTASKRACE_SINGLETHREADED:
				cout << "NOTASKRACE_SINGLETHREADED";
				break;
			default:
				cout << "UNKNOWN";
			}
		}

		void printDetails() const {
			cout << "Op1: " << op1 << " op2: " << op2 << " op1Task: " << op1Task
				 << " op2Task: " << op2Task << " allocID: " << allocID
				 << " uafOrRace: " << uafOrRace << " raceType: ";
			printRaceKind();
			cout << "\n";
		}

	};

//	map<IDType, std::vector<raceDetails> > allocToRaceMap;
	map<IDType, std::multiset<raceDetails> > allocToRaceMap;

	void getRaceKind(raceDetails &race);


	void log(IDType op1ID, IDType op2ID, IDType opAllocID,
			bool uafOrRace, RaceKind raceType, bool logAll=false, RaceKindByThread raceTypeByThread=BOTH_MULTITHREADED);

	std::string findPreviousTaskOfOp(IDType op);
	void insertRace(raceDetails race);
};

class HBGraph {
public:

	class adjListNode {
	public:
		IDType nodeID;
		adjListNode* next;

		adjListNode() {
			nodeID = -1;
			next = NULL;
		}

		adjListNode(IDType op) {
			nodeID = op;
			next = NULL;
		}
	};

private:
	class adjListType {
	public:
		adjListNode* head;

		adjListType() {
			head = NULL;
		}
	};

public:
	HBGraph();
	HBGraph(IDType countOfOps,
			map<IDType, UAFDetector::opDetails> opMap,
			map<IDType, UAFDetector::setOfOps> nodeMap);
	virtual ~HBGraph();

	IDType totalOps;
	bool** opAdjMatrix;
	bool** opEdgeTypeMatrix; // true if edge is st-edge, false if edge is dt-edge
	std::map<IDType, adjListType*> opAdjList;


	unsigned long long numOfOpEdges;

	// Return -1 if error, 1 if the edge was newly added, 0 if edge already present.
	int addOpEdge(IDType sourceOp, IDType destinationOp, bool edgeType);
	// True if ST edge, false if MT edge
	bool isSTEdge(IDType sourceNode, IDType destinationNode);
	int removeOpEdge(IDType sourceOp, IDType destinationOp);

	// Return 1 if edge exists, 0 if not, -1 if adjMatrix and adjList are out of sync.
	int opEdgeExists(IDType sourceOp, IDType destinationOp);

	void printGraph();

private:
	map<IDType, UAFDetector::opDetails> opIDMap;
	map<IDType, UAFDetector::setOfOps> nodeIDMap;

	bool opEdgeExistsinList(IDType source, IDType destination) {
		// The arguments are nodes, not ops!

		if (opAdjList.find(source) == opAdjList.end()) return false;
		adjListNode* currNode = opAdjList[source]->head;
		while (currNode != NULL) {
			if (currNode->nodeID == destination)
				return true;
			currNode = currNode->next;
		}
		return false;
	}
};

#endif /* UAFDETECTOR_H_ */
