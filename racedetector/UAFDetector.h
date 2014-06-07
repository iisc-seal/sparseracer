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
		long long threadID;
		string taskID;
		string opType;

		opDetails() {
			threadID = -1;
			taskID = "";
			opType = "";
		}

		void printOpDetails() {
			cout << "threadID " << threadID << " taskID " << taskID << " opType " << opType;
		}
	};

	// Maps operationID to its threadID, taskID and type.
	map<long long, opDetails> opIDMap;

	// Maps operation (as the string) to its ID.
	map<string, long long> opStringToOpIDMap;

	// Maps opID to next op's ID in the same task.
	map<long long, long long> opToNextOpInTask;

	// Maps opID to next op's ID in the same thread.
	map<long long, long long> opToNextOpInThread;


	// Stores threadID, first pause operation's ID and last resume operation's ID
	class taskDetails {
	public:
		long long threadID;
		long long firstPauseOpID;
		long long lastResumeOpID;
		// list<pair<long long, long long> > pauseResumepairs;

		long long deqOpID;
		long long endOpID;
		long long enqOpID;
		string callback;

		taskDetails() {
			threadID = -1;
			firstPauseOpID = -1;
			lastResumeOpID = -1;

			deqOpID = -1;
			endOpID = -1;
			enqOpID = -1;
			callback = "";
		}

		void printTaskDetails() {
			cout << "threadID " << threadID << " first-pause " << firstPauseOpID
				 << " last-resume " << lastResumeOpID << " deq " << deqOpID
				 << " end " << endOpID << " enq " << enqOpID << " callback " << callback;
		}
	};

	// Maps task ID to its threadID, first pause and last resume.
	map<string, taskDetails> taskIDMap;

	// Set of atomic tasks in the trace.
	set<string> atomicTasks;

	class threadDetails {
	public:
		long long threadinitOpID;
		long long enterloopOpID;
		long long exitloopOpID;
		long long threadexitOpID;
		long long forkOpID; // op that forked this thread
		long long joinOpID; // op that joined this thread
		// list<string> lockOps;

		threadDetails() {
			threadinitOpID = -1;
			enterloopOpID = -1;
			exitloopOpID = -1;
			threadexitOpID = -1;
			forkOpID = -1;
			joinOpID = -1;
		}

		void printThreadDetails() {
			cout << " threadinit " << threadinitOpID << " threadexit " << threadexitOpID
				 << " enterloop " << enterloopOpID << " exitloop " << exitloopOpID
				 << " fork " << forkOpID << " join " << joinOpID;
		}
	};

	map<long long, threadDetails> threadIDMap;

	set<long long> threadinitSet;
	set<long long> threadexitSet;
	set<long long> enterloopSet;
	set<long long> exitloopSet;
	set<long long> deqSet;
	set<long long> endSet;
	set<long long> resumeSet;
	set<long long> pauseSet;

	// Class to store 4 arguments of an enq operation.
	class enqOpDetails{
	public:
		long long currThreadID;
		string taskEnqueued;
		long long targetThreadID;
		string callback;

		enqOpDetails() {
			currThreadID = -1;
			taskEnqueued = "";
			targetThreadID = -1;
			callback = "";
		}
	};

	// Maps the op ID of an enq operation to its arguments.
	map<long long, enqOpDetails> enqSet;

	class forkAndJoinOpDetails {
	public:
		long long currThreadID;
		long long targetThreadID;

		forkAndJoinOpDetails() {
			currThreadID = -1;
			targetThreadID = -1;
		}
	};

	// Maps the op ID of a fork operation to its arguments.
	map<long long, forkAndJoinOpDetails> forkSet;

	// maps the op ID of a join operation to its arguments.
	map<long long, forkAndJoinOpDetails> joinSet;

	class acquireAndReleaseOpDetails {
	public:
		long long currThreadID;
		string lockID;

		acquireAndReleaseOpDetails() {
			currThreadID = -1;
			lockID = "";
		}
	};

	// maps the op ID of an acquire operation to its arguments.
	map<long long, acquireAndReleaseOpDetails> acquireSet;

	// maps the op ID of a release operation to its arguments.
	map<long long, acquireAndReleaseOpDetails> releaseSet;

	class memoryOpDetails {
	public:
		long long threadID;
		string startingAddress;
		long long range;

		memoryOpDetails() {
			threadID = -1;
			startingAddress = "";
			range = -1;
		}
	};

	// maps the op ID of a memory operation to its arguments.
	map<long long, memoryOpDetails> allocSet;
	map<long long, memoryOpDetails> freeSet;
	map<long long, memoryOpDetails> readSet;
	map<long long, memoryOpDetails> writeSet;
	map<long long, memoryOpDetails> incSet;
	map<long long, memoryOpDetails> decSet;

	HBGraph graph;

	void initGraph(long long countOfNodes);

	int addEdges(Logger &logger);

	void findUAF(Logger &logger);

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
	int addLockEdges();
	int addCallbackSTEdges();
	int addFifoAtomicEdges();
	int addNoPreEdges();
	int addFifoCallbackEdges();
	int addFifoNestedEdges();
	int addNoPrePrefixEdges();
	int addNoPreSuffixEdges();
	int addTransSTOrMTEdges();
};

#endif /* UAFDETECTOR_H_ */
