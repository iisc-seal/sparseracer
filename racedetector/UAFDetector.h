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
#include "HBGraph.h"

#include <config.h>
#include <debugconfig.h>
#include <logging/Logger.h>
using namespace std;

#ifndef UAFDETECTOR_H_
#define UAFDETECTOR_H_

class UAFDetector {
public:
	UAFDetector();
	virtual ~UAFDetector();
	void ERRLOG(string error) {
		cout << error << endl;
	}

	// Stores threadID, taskID and opType of an operation.
	class opDetails {
	public:
		IDType threadID;
		string taskID;
		string opType;
		IDType blockID;

		IDType nextOpInThread;
		IDType nextOpInTask;
		IDType nextOpInBlock;
		IDType prevOpInBlock;

		opDetails() {
			threadID = -1;
			taskID = "";
			opType = "";
			blockID = -1;
			nextOpInThread = -1;
			nextOpInTask = -1;
			nextOpInBlock = -1;
			prevOpInBlock = -1;
		}

		void printOpDetails() {
			cout << "threadID " << threadID << " taskID " << taskID
				 << " blockID " << blockID << " opType " << opType << endl;
			cout << "next-op-in-thread " << nextOpInThread << " next-op-in-task "
				 << nextOpInTask << " next-op-in-block " << nextOpInBlock;
		}
	};

	// Maps operationID to its threadID, taskID and type.
	map<IDType, opDetails> opIDMap;

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

		string parentTask; // ID of immediate parent task
		bool atomic;

		IDType firstBlockID; // ID of the first block in task
		IDType lastBlockID;  // ID of the last block in task

		// Set of pause/reset/resume ops of the task
		set<IDType> pauseSet;
		set<IDType> resetSet;
		set<IDType> resumeSet;

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

			pauseSet = set<IDType>();
			resetSet = set<IDType>();
			resumeSet = set<IDType>();
		}

		void printTaskDetails() {
			cout << "first-pause " << firstPauseOpID
				 << " last-resume " << lastResumeOpID << " deq " << deqOpID
				 << " end " << endOpID << " enq " << enqOpID
				 << " parent task " << parentTask << " atomic " << atomic
				 << " first-block " << firstBlockID << " last-block " << lastBlockID;
			cout << "\n Pause ops: ";
			for (set<IDType>::iterator it = pauseSet.begin(); it != pauseSet.end(); it++)
				cout << *it << " ";
			cout << "\n Reset ops: ";
			for (set<IDType>::iterator it = resetSet.begin(); it != resetSet.end(); it++)
				cout << *it << " ";
			cout << "\n Resume ops: ";
			for (set<IDType>::iterator it = resumeSet.begin(); it != resumeSet.end(); it++)
				cout << *it << " ";
		}
	};

	// Maps task ID to its enq-op, deq-op, etc.
	map<string, taskDetails> taskIDMap;

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
		string taskID;

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
		string taskEnqueued;
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
	map<IDType, string> pauseResumeResetOps;

	class memoryOpDetails {
	public:
		string startingAddress;
		IDType range;

		memoryOpDetails() {
			startingAddress = "";
			range = -1;
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

		allocOpDetails() {
			readOps = set<IDType>();
			writeOps = set<IDType>();
			freeOps = set<IDType>();
		}

		void printDetails() {
			cout << " read ops: ";
			for (set<IDType>::iterator it = readOps.begin(); it != readOps.end(); it++)
				cout << *it << " ";
			cout << "\n write ops: ";
			for (set<IDType>::iterator it = writeOps.begin(); it != writeOps.end(); it++)
				cout << *it << " ";
			cout << "\n free ops: ";
			for (set<IDType>::iterator it = freeOps.begin(); it != freeOps.end(); it++)
				cout << *it << " ";
		}
	};

	map<IDType, allocOpDetails> allocIDMap;

	class freeOpDetails {
	public:
		IDType allocOpID;
		set<IDType> readOps;
		set<IDType> writeOps;

		freeOpDetails() {
			allocOpID = -1;
			readOps = set<IDType>();
			writeOps = set<IDType>();
		}

		void printDetails() {
			cout << "alloc op: " << allocOpID << endl;
			cout << " read ops: ";
			for (set<IDType>::iterator it = readOps.begin(); it != readOps.end(); it++)
				cout << *it << " ";
			cout << "\n write ops: ";
			for (set<IDType>::iterator it = writeOps.begin(); it != writeOps.end(); it++)
				cout << *it << " ";
		}
	};

	map<IDType, freeOpDetails> freeIDMap;

	HBGraph graph;

	void initGraph(IDType countOfOps, IDType countOfNodes);

	int addEdges(Logger &logger);

	int findUAFwithoutAlloc(Logger &logger);

	int findDataRaces(Logger &logger);

#ifdef GRAPHDEBUG
	void printEdges();
#endif

private:
	// All the following functions: Return -1 if some error, 1 if atleast one edge is added, 0 if no edges are added.
	int addLoopPOEdges();
	int addTaskPOEdges();
	int addEnqueueSTorMTEdges();
	int addForkEdges();
	int addJoinEdges();
	int addCallbackSTEdges();
	int addFifoAtomicEdges();
	int addNoPreEdges();
	int addFifoCallbackEdges();
	int addFifoCallback2Edges();
	int addFifoNestedEdges();
	int addNoPrePrefixEdges();
	int addNoPreSuffixEdges();
	int addTransSTOrMTEdges();

	unsigned long long numOfEdges;
};

#endif /* UAFDETECTOR_H_ */
