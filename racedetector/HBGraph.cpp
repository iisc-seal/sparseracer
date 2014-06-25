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
	totalOps = 0;
}

HBGraph::HBGraph(IDType countOfOps, IDType countOfNodes) {
	totalOps = countOfOps;
	totalNodes = countOfNodes;

	adjMatrix = (bool**) malloc(sizeof(bool*) * (totalNodes+1));
	if (adjMatrix == NULL) {
		cout << "ERROR: Cannot allocate memory for adjMatrix\n";
	}
	adjList = new HBGraph::adjListType [totalNodes+1];

	for (long long i=1; i <= totalNodes; i++) {
		adjList[i].head = NULL;
		adjMatrix[i] = (bool*) calloc((totalNodes+1) , sizeof(bool));
	}
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

int HBGraph::addSingleEdge(long long sourceBlock, long long destinationBlock, long long sourceOp, long long destinationOp) {

	assert(sourceBlock >= 1 && sourceBlock <= totalNodes);
	assert(destinationBlock >= 1 && destinationBlock <= totalNodes);
	assert(sourceOp >= 1 && sourceOp <= totalOps);
	assert(destinationOp >= 1 && destinationOp <= totalOps);

	if (opEdgeExists(destinationBlock, sourceBlock, destinationOp, sourceOp) == 1) {
		cout << "ERROR: Edge exists from block " << destinationBlock << " op " << destinationOp << " to block " << sourceBlock << " op " << sourceOp << endl;
		cout << "We are trying to edge from block " << sourceBlock << " op " << sourceOp << " to block " << destinationBlock << " op " << destinationOp << endl;
		return -1;
	}
	if (sourceBlock == destinationBlock && sourceOp == destinationOp) {
		cout << "ERROR: Adding self-loop edge on " << sourceOp << endl;
		return -1;
	}
	int retValue = opEdgeExists(sourceBlock, destinationBlock, sourceOp, destinationOp);
	if (retValue == 0) {
		adjMatrix[sourceBlock][destinationBlock] = true;
			
		struct adjListNode* destNode = createNewNode(destinationBlock, sourceOp, destinationOp);
		destNode->next = adjList[sourceBlock].head;
		adjList[sourceBlock].head = destNode;
		return 1;
	} else if (retValue == 1)
		return 0;
	else
		return -1;

	return 0;
}

int HBGraph::opEdgeExists(long long sourceBlock, long long destinationBlock, long long sourceOp, long long destinationOp) {

	assert(adjMatrix[sourceBlock][destinationBlock] == true || adjMatrix[sourceBlock][destinationBlock] == false);
	// Checking if adjMatrix and adjList are in sync.
	if (!adjMatrix[sourceBlock][destinationBlock] && !opEdgeExistsinList(sourceBlock, destinationBlock, sourceOp, destinationOp))
		return 0;
	else if (!adjMatrix[sourceBlock][destinationBlock]) {
		cout << "ERROR: Edge exists in adjList, not in adjMatrix: " << sourceBlock << " -> " << destinationBlock << endl;
		return -1;
	} else if (!opEdgeExistsinList(sourceBlock, destinationBlock, sourceOp, destinationOp)) {
		cout << "ERROR: Edge exists in adjMatrix, not in adjList: " << sourceOp << " -> " << destinationOp << endl;
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
			cout << currNode->sourceOp << "->" << currNode->destinationOp << "   ";
			currNode = currNode->next;
		}
	}
	cout << "\n";
}
