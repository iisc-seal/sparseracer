/*
 * HBGraph.h
 *
 *  Created on: 01-Jun-2014
 *      Author: shalini
 */

#include <vector>
#include <set>
#include <stddef.h>

using namespace std;

#ifndef HBGRAPH_H_
#define HBGRAPH_H_

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
	HBGraph(long long countOfNodes);
	virtual ~HBGraph();

	long long totalNodes;
	bool** adjMatrix;
	struct adjListType* adjList;

	// Return -1 if error, 1 if the edge was newly added, 0 if edge already present.
	int addSingleEdge(long long source, long long destination);

	// Return 1 if edge exists, 0 if not, -1 if adjMatrix and adjList are out of sync.
	int edgeExists(long long source, long long destination);

	void printGraph(bool flag);

private:
	bool edgeExistsinList(long long source, long long destination) {
		struct adjListNode* currNode;
		currNode = adjList[source].head;
		while (currNode != NULL) {
			if (currNode->destination == destination)
				return true;
			currNode = currNode->next;
		}
		return false;
	}
};

#endif /* HBGRAPH_H_ */
