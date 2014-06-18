/*
 * HBGraph.cpp
 *
 *  Created on: 01-Jun-2014
 *      Author: shalini
 */

#include <racedetector/HBGraph.h>
#include <iostream>
#include <list>
#include <cstdlib>
#include <cstring>

HBGraph::HBGraph(){
	totalNodes = 0;
}

HBGraph::HBGraph(long long countOfNodes) {
	cout << "Start HBGraph constructor\n";
	totalNodes = countOfNodes;

	//adjMatrix = new bool* [totalNodes+1];
	adjMatrix = (bool**) malloc(sizeof(bool*) * (totalNodes+1));
//	adjMatrix = (bool*) malloc(sizeof(bool) * totalNodes * totalNodes);
	if (adjMatrix == NULL) {
		cout << "ERROR: Cannot allocate memory for adjMatrix\n";
	}
	adjList = new HBGraph::adjListType [totalNodes+1];

	// Initialze adjMatrix to false;
	cout << "Before for loop\n";
	for (long long i=1; i <= totalNodes; i++) {
		adjList[i].head = NULL;
//		adjMatrix[i] = new bitset<1> [totalNodes+1];
		//adjMatrix[i] = (bitset<1>*) malloc(sizeof(bitset<1>) * (totalNodes +1));
		//adjMatrix[i] = (bitset<1>*) calloc((totalNodes + 1), sizeof(bitset<1>));
		adjMatrix[i] = (bool*) calloc((totalNodes+1) , sizeof(bool));
		//adjMatrix[i] = (bool*) malloc((totalNodes)*sizeof(bool));
	}
	cout << "After for loop\n";
	if (adjMatrix[1][1] == true) {
//	if (matrixElement(0,0) == true) {
		cout << "True\n";
	} else if (adjMatrix[1][1] == false) {
//	} else if (matrixElement(0,0) == false) {
		cout << "False\n";
	} else {
		cout << "Neither True nor False\n";
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
			adjMatrix[source][destination] = true;
//			assignMatrixElement(source-1, destination-1, true);
			
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

	assert(adjMatrix[source][destination] == true || adjMatrix[source][destination] == false);
	// Checking if adjMatrix and adjList are in sync.
//	if (matrixElement(source-1, destination-1) == false && !edgeExistsinList(source, destination))
	if (!adjMatrix[source][destination] && !edgeExistsinList(source, destination))
	//if (!adjMatrix[source][destination].test(0) && !edgeExistsinList(source, destination))
		return 0;
//	else if (matrixElement(source-1, destination-1) == false) {
	else if (!adjMatrix[source][destination]) {
	//else if (!adjMatrix[source][destination].test(0)) {
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
//			cout << matrixElement(i, j) << " ";
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
