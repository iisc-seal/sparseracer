/*
 * HBGraph.h
 *
 *  Created on: 01-Jun-2014
 *      Author: shalini
 */

#include <vector>
#include <set>
#include <stddef.h>
#include <cassert>
#include <map>

using namespace std;

#ifndef HBGRAPH_H_
#define HBGRAPH_H_

#include <config.h>

class HBGraph {
public:
	struct adjListNode {
		long long destination;
		struct adjListNode* next;
	};

private:
	struct adjListType {
		struct adjListNode* head;
	};

	struct adjListNode* createNewNode(long long destination) {
		struct adjListNode* newNode = new adjListNode;
		newNode->destination = destination;
		newNode->next = NULL;
		return newNode;
	}

public:
	HBGraph();
	HBGraph(IDType countOfOps, IDType countOfBlocks,
			map<IDType, UAFDetector::opDetails> opMap,
			map<IDType, UAFDetector::blockDetails> blockMap,
			map<IDType, UAFDetector::setOfOps> nodeMap);
	virtual ~HBGraph();

	long long totalBlocks;
	long long totalOps;
	bool** opAdjMatrix;
	bool** blockAdjMatrix;
	struct adjListType* opAdjList;
	struct adjListType* blockAdjList;

	unsigned long long numOfOpEdges;
	unsigned long long numOfBlockEdges;

	// Return -1 if error, 1 if the edge was newly added, 0 if edge already present.
	int addOpEdge(long long sourceOp, long long destinationOp);
	int addBlockEdge(long long sourceBlock, long long destinationBlock);

	// Return 1 if edge exists, 0 if not, -1 if adjMatrix and adjList are out of sync.
	int blockEdgeExists(long long sourceBlock, long long destinationBlock);
	int opEdgeExists(long long sourceOp, long long destinationOp);

	void printGraph();

private:
	map<IDType, UAFDetector::opDetails> opIDMap;
	map<IDType, UAFDetector::blockDetails> blockIDMap;
	map<IDType, UAFDetector::setOfOps> nodeIDMap;

	bool blockEdgeExistsinList(long long source, long long destination) {
		struct adjListNode* currNode;
		currNode = blockAdjList[source].head;
		while (currNode != NULL) {
			if (currNode->destination == destination)
				return true;
			currNode = currNode->next;
		}
		return false;
	}

	bool opEdgeExistsinList(long long sourceOp, long long destinationOp) {
		struct adjListNode* currNode;
		currNode = opAdjList[sourceOp].head;
		while (currNode != NULL) {
			if (currNode->destination == destinationOp)
				return true;
			currNode = currNode->next;
		}
		return false;
	}
};

#endif /* HBGRAPH_H_ */
