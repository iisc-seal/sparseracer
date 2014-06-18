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

		string parentTask; // ID of immediate parent task

		taskDetails() {
			threadID = -1;
			firstPauseOpID = -1;
			lastResumeOpID = -1;

			deqOpID = -1;
			endOpID = -1;
			enqOpID = -1;
			callback = "";

			parentTask = "";
		}

		void printTaskDetails() {
			cout << "threadID " << threadID << " first-pause " << firstPauseOpID
				 << " last-resume " << lastResumeOpID << " deq " << deqOpID
				 << " end " << endOpID << " enq " << enqOpID << " callback " << callback
				 << " parent task " << parentTask;
		}
	};

	// Maps task ID to its threadID, first pause and last resume.
	map<string, taskDetails> taskIDMap;

	// Set of atomic tasks in the trace.
	set<string> atomicTasks;

	class threadDetails {
	public:
		long long firstOpID;
		long long threadinitOpID;
		long long enterloopOpID;
		long long exitloopOpID;
		long long threadexitOpID;
		long long forkOpID; // op that forked this thread
		long long joinOpID; // op that joined this thread
		// list<string> lockOps;

		threadDetails() {
			firstOpID = -1;
			threadinitOpID = -1;
			enterloopOpID = -1;
			exitloopOpID = -1;
			threadexitOpID = -1;
			forkOpID = -1;
			joinOpID = -1;
		}

		void printThreadDetails() {
			cout << " first-op " << firstOpID << " threadinit " << threadinitOpID << " threadexit " << threadexitOpID
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

#ifdef LOCKS
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

	// maps the op ID of an entermonitor operation to its arguments.
	map<long long, acquireAndReleaseOpDetails> entermonitorSet;

	// maps the op ID of an exitmonitor operation to its arguments.
	map<long long, acquireAndReleaseOpDetails> exitmonitorSet;

	map<long long, acquireAndReleaseOpDetails> waitSet;
	map<long long, acquireAndReleaseOpDetails> notifySet;
	map<long long, acquireAndReleaseOpDetails> notifyAllSet;

	class lockDetails {
	public:
		set<long long> acquireOps;
		set<long long> releaseOps;
		set<long long> entermonitorOps;
		set<long long> exitmonitorOps;
		set<long long> waitOps;
		set<long long> notifyOps;
		set<long long> notifyAllOps;

		lockDetails() {
			acquireOps = set<long long>();
			releaseOps = set<long long>();
			entermonitorOps = set<long long>();
			exitmonitorOps = set<long long>();
			waitOps = set<long long>();
			notifyOps = set<long long>();
			notifyAllOps = set<long long>();
		}

		void printDetails() {
			cout << "acquire ops: ";
			for (set<long long>::iterator it = acquireOps.begin(); it != acquireOps.end(); it++)
				cout << *it << " ";
			cout << "\nrelease ops: ";
			for (set<long long>::iterator it = releaseOps.begin(); it != releaseOps.end(); it++)
				cout << *it << " ";
			cout << "\nentermonitor ops: ";
			for (set<long long>::iterator it = entermonitorOps.begin(); it != entermonitorOps.end(); it++)
				cout << *it << " ";
			cout << "\nexitmonitor ops: ";
			for (set<long long>::iterator it = exitmonitorOps.begin(); it != exitmonitorOps.end(); it++)
				cout << *it << " ";
			cout << "\nwait ops: ";
			for (set<long long>::iterator it = waitOps.begin(); it != waitOps.end(); it++)
				cout << *it << " ";
			cout << "\nnotify ops: ";
			for (set<long long>::iterator it = notifyOps.begin(); it != notifyOps.end(); it++)
				cout << *it << " ";
			cout << "\nnotifyAll ops: ";
			for (set<long long>::iterator it = notifyAllOps.begin(); it != notifyAllOps.end(); it++)
				cout << *it << " ";
		}
	};

	map<string, lockDetails> lockIDMap;
#endif

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
#ifdef ACCESS
	map<long long, memoryOpDetails> accessSet;
#else
	map<long long, memoryOpDetails> readSet;
	map<long long, memoryOpDetails> writeSet;
#endif
	map<long long, memoryOpDetails> incSet;
	map<long long, memoryOpDetails> decSet;

	class allocOpDetails {
	public:
#ifdef ACCESS
		set<long long> accessOps;
#else
		set<long long> readOps;
		set<long long> writeOps;
#endif
		set<long long> freeOps;
		set<long long> incOps;
		set<long long> decOps;

		allocOpDetails() {
#ifdef ACCESS
			accessOps = set<long long>();
#else
			readOps = set<long long>();
			writeOps = set<long long>();
#endif
			freeOps = set<long long>();
			incOps = set<long long>();
			decOps = set<long long>();
		}

		void printDetails() {
#ifdef ACCESS
			cout << " access ops: ";
			for (set<long long>::iterator it = accessOps.begin(); it != accessOps.end(); it++)
				cout << *it << " ";
#else
			cout << " read ops: ";
			for (set<long long>::iterator it = readOps.begin(); it != readOps.end(); it++)
				cout << *it << " ";
			cout << "\n write ops: ";
			for (set<long long>::iterator it = writeOps.begin(); it != writeOps.end(); it++)
				cout << *it << " ";
#endif
			cout << "\n free ops: ";
			for (set<long long>::iterator it = freeOps.begin(); it != freeOps.end(); it++)
				cout << *it << " ";
			cout << "\n inc ops: ";
			for (set<long long>::iterator it = incOps.begin(); it != incOps.end(); it++)
				cout << *it << " ";
			cout << "\n dec ops: ";
			for (set<long long>::iterator it = decOps.begin(); it != decOps.end(); it++)
				cout << *it << " ";
		}
	};

	map<long long, allocOpDetails> allocIDMap;

	class freeOpDetails {
	public:
		long long allocOpID;
#ifdef ACCESS
		set<long long> accessOps;
#else
		set<long long> readOps;
		set<long long> writeOps;
#endif
		set<long long> incOps;
		set<long long> decOps;

		freeOpDetails() {
			allocOpID = -1;
#ifdef ACCESS
			accessOps = set<long long>();
#else
			readOps = set<long long>();
			writeOps = set<long long>();
#endif
			incOps = set<long long>();
			decOps = set<long long>();
		}

		void printDetails() {
			cout << "alloc op: " << allocOpID << endl;
#ifdef ACCESS
			cout << " access ops: ";
			for (set<long long>::iterator it = accessOps.begin(); it != accessOps.end(); it++)
				cout << *it << " ";
#else
			cout << " read ops: ";
			for (set<long long>::iterator it = readOps.begin(); it != readOps.end(); it++)
				cout << *it << " ";
			cout << "\n write ops: ";
			for (set<long long>::iterator it = writeOps.begin(); it != writeOps.end(); it++)
				cout << *it << " ";
#endif
			cout << "\n inc ops: ";
			for (set<long long>::iterator it = incOps.begin(); it != incOps.end(); it++)
				cout << *it << " ";
			cout << "\n dec ops: ";
			for (set<long long>::iterator it = decOps.begin(); it != decOps.end(); it++)
				cout << *it << " ";
		}
	};

	map<long long, freeOpDetails> freeIDMap;

	HBGraph graph;

	void initGraph(long long countOfNodes);

	int addEdges(Logger &logger);

//	int findUAFusingAlloc(Logger &logger);
	int findUAFwithoutAlloc(Logger &logger);

#ifndef ACCESS
	int findDataRaces(Logger &logger);
#endif

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
#ifdef LOCKS
	int addLockEdges();
#endif
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
