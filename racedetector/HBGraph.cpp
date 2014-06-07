/*
 * HBGraph.cpp
 *
 *  Created on: 01-Jun-2014
 *      Author: shalini
 */

#include <racedetector/HBGraph.h>
#include <iostream>
#include <list>

HBGraph::HBGraph(){
	totalNodes = 0;
}

HBGraph::HBGraph(long long countOfNodes) {
	totalNodes = countOfNodes;

	adjMatrix = new bool* [totalNodes+1];
	adjList = new HBGraph::adjListType [totalNodes+1];

	// Initialze adjMatrix to false;
	for (long long i=1; i <= totalNodes; i++) {
		adjList[i].head = NULL;
		adjMatrix[i] = new bool [totalNodes+1];
		for (long long j=1; j <= totalNodes; j++) {
			adjMatrix[i][j] = false;
		}
	}
}

HBGraph::~HBGraph() {
}

int HBGraph::addSingleEdge(long long source, long long destination) {

	if (source >= 1 && source <= totalNodes && destination >=1 && destination <= totalNodes) {
		if (edgeExists(destination, source) == 1) {
			cout << "ERROR: Edge exists from " << destination << " to " << source << " and we are edding edge from " << source << " to "
				 << destination << endl;
			return -1;
		}
		if (source == destination) {
			cout << "ERROR: Adding self-loop edge on " << source << endl;
			return -1;
		}
		int retValue = edgeExists(source, destination);
		if (retValue == 0) {
			adjMatrix[source][destination] = 1;
			struct adjListNode* destNode = createNewNode(destination);
			destNode->next = adjList[source].head;
			adjList[source].head = destNode;
			return 1;
		} else if (retValue == 1)
			return 0;
		else
			return -1;
	} else  {
		cout << "ERROR: Invalid source/destination for adding edge - source " << source << " destination " << destination << endl;
		return -1;
	}

	return 0;
}

int HBGraph::edgeExists(long long source, long long destination) {
	// Checking if adjMatrix and adjList are in sync.
	if (!adjMatrix[source][destination] && !edgeExistsinList(source, destination))
		return 0;
	else if (!adjMatrix[source][destination]) {
		cout << "ERROR: Edge exists in adjList, not in adjMatrix: " << source << " -> " << destination << endl;
		return -1;
	} else if (!edgeExistsinList(source, destination)) {
		cout << "ERROR: Edge exists in adjMatrix, not in adjList: " << source << " -> " << destination << endl;
		return -1;
	} else
		return 1;
}

void HBGraph::printGraph(bool flag) {
//	cout << endl << totalNodes;
	if (flag) {
	cout << endl << "Matrix";
	for (long long i=1; i <= totalNodes; i++) {
		cout << endl << i << ": ";
		for (long long j=1; j <= totalNodes; j++) {
			cout << adjMatrix[i][j] << " ";
		}
	}
	cout << "\n";
	}

//	cout << "List";
	for (long long i=1; i <= totalNodes; i++) {
		cout << endl << i << ": ";
		struct adjListNode* currNode = adjList[i].head;
		while (currNode != NULL) {
			cout << currNode->destination << " ";
			currNode = currNode->next;
		}
	}
	cout << "\n";
}
