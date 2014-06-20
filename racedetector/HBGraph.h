/*
 * HBGraph.h
 *
 *  Created on: 01-Jun-2014
 *      Author: shalini
 */

#include <vector>
#include <set>
#include <bitset>
#include <stddef.h>
#include <cassert>

using namespace std;

#ifndef HBGRAPH_H_
#define HBGRAPH_H_

class HBGraph {
public:
	struct adjListNode {
		long long sourceOp;
		long long destinationOp;
		long long destinationBlock;
		struct adjListNode* next;
	};

private:
	struct adjListType {
		struct adjListNode* head;
	};

	struct adjListNode* createNewNode(long long sourceOp, long long destinationOp, long long destinationBlock) {
		struct adjListNode* newNode = new adjListNode;
		newNode->sourceOp = sourceOp;
		newNode->destinationOp = destinationOp;
		newNode->destinationBlock = destinationBlock;
		newNode->next = NULL;
		return newNode;
	}

public:
	HBGraph();
	HBGraph(long long countOfOps, long long countOfNodes);
	virtual ~HBGraph();

	long long totalNodes;
	long long totalOps;
	bool** adjMatrix;
	struct adjListType* adjList;

	// Return -1 if error, 1 if the edge was newly added, 0 if edge already present.
	int addSingleEdge(long long sourceBlock, long long destinationBlock, long long sourceOp, long long destinationOp);

	// Return 1 if edge exists, 0 if not, -1 if adjMatrix and adjList are out of sync.
	int blockEdgeExists(long long source, long long destination);
	int opEdgeExists(long long sourceBlock, long long destinationBlock, long long sourceOp, long long destinationOp);

	void printGraph(bool flag);

private:
	bool blockEdgeExistsinList(long long source, long long destination) {
		struct adjListNode* currNode;
		currNode = adjList[source].head;
		while (currNode != NULL) {
			if (currNode->destinationBlock == destination)
				return true;
			currNode = currNode->next;
		}
		return false;
	}

	bool opEdgeExistsinList(long long sourceBlock, long long destinationBlock, long long sourceOp, long long destinationOp) {
		struct adjListNode* currNode;
		currNode = adjList[sourceBlock].head;
		while (currNode != NULL) {
			if (currNode->sourceOp == sourceOp && currNode->destinationOp == destinationOp)
				return true;
			currNode = currNode->next;
		}
		return false;
	}
};

#endif /* HBGRAPH_H_ */
