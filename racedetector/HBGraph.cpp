/*
 * HBGraph.cpp
 *
 *  Created on: 01-Jun-2014
 *      Author: shalini
 */

#include <racedetector/UAFDetector.h>
#include <iostream>
#include <list>
#include <cstdlib>
#include <cstring>
#include <cassert>
typedef std::multiset<HBGraph::adjListNode>::iterator nodeIterator;

HBGraph::HBGraph(){
	totalOps = 0;
	numOfOpEdges = 0;
	opIDMap = map<IDType, UAFDetector::opDetails>();
	nodeIDMap = map<IDType, UAFDetector::setOfOps>();

	opAdjList = std::map<IDType, adjListType*>();
	opAdjMatrix = NULL;
	opEdgeTypeMatrix = NULL;
}

HBGraph::HBGraph(IDType countOfOps,
		map<IDType, UAFDetector::opDetails> opMap,
		map<IDType, UAFDetector::setOfOps> nodeMap) {
	totalOps = countOfOps;
	numOfOpEdges = 0;

	opIDMap = opMap;
	nodeIDMap = nodeMap;

	opAdjMatrix = (bool**) malloc(sizeof(bool*) * (totalOps+1));
	opEdgeTypeMatrix = (bool**) malloc(sizeof(bool*) * (totalOps+1));

	if (opAdjMatrix == NULL) {
		cout << "ERROR: Cannot allocate memory for opAdjMatrix\n";
	}
	if (opEdgeTypeMatrix == NULL) {
		cout << "ERROR: Cannot allocate memory for opEdgeTypeMatrix\n";
	}

	opAdjList = std::map<IDType, adjListType*>();

	for (IDType i=1; i <= totalOps; i++) {
		opAdjMatrix[i] = (bool*) calloc((totalOps+1) , sizeof(bool));
		opEdgeTypeMatrix[i] = (bool*) calloc((totalOps+1), sizeof(bool));
	}
}

HBGraph::~HBGraph() {
}

int HBGraph::addOpEdge(IDType sourceNode, IDType destinationNode, bool edgeType) {

#ifdef SANITYCHECK
	assert(1 <= sourceNode      && sourceNode <= totalOps);
	assert(1 <= destinationNode && destinationNode <= totalOps);
	assert(sourceNode != destinationNode);
#endif

	int retOpValue = opEdgeExists(sourceNode, destinationNode);
	if (retOpValue == 0) {
#ifdef SANITYCHECK
		if (opEdgeExists(destinationNode, sourceNode) == 1) {
			cout << "ERROR: Edge exists from " << destinationNode << " to " << sourceNode 	   << endl;
			cout << "Trying to add edge from " << sourceNode		<< " to " << destinationNode << endl;
			return -1;
		}

#endif

		opAdjMatrix[sourceNode][destinationNode] = true;
		opEdgeTypeMatrix[sourceNode][destinationNode] = edgeType;

		if (opAdjList.find(sourceNode) == opAdjList.end()) {
			adjListNode* newNode = new adjListNode(destinationNode);
			adjListType* newList = new adjListType;
			newList->head = newNode;
			opAdjList[sourceNode] = newList;
		} else {
			adjListNode* newNode = new adjListNode(destinationNode);
			newNode->next = opAdjList[sourceNode]->head;
			opAdjList[sourceNode]->head = newNode;
		}

		numOfOpEdges++;

		return 1;
	} else if (retOpValue == 1)
		return 0;
	else
		return -1;

	return -1;
}

// currNode is the adjListNode containing destinationOp in the adjacency list of sourceOp
int HBGraph::removeOpEdge(IDType sourceNode, IDType destinationNode) {
#ifdef SANITYCHECK
	assert(1 <= sourceNode && sourceNode <= totalOps);
	assert(1 <= destinationNode && destinationNode <= totalOps);
#endif

	if (opAdjList.find(sourceNode) != opAdjList.end()) {
		adjListNode* currNode = opAdjList[sourceNode]->head;
		adjListNode* prevNode = NULL;
		while (currNode != NULL) {
			if (currNode->nodeID == destinationNode) {
				if (!prevNode) {
					opAdjList[sourceNode]->head = currNode->next;
					delete currNode;
					break;
				} else {
					prevNode->next = currNode->next;
					delete currNode;
					break;
				}
			}
			prevNode = currNode;
			currNode = currNode->next;
		}
	}

	opAdjMatrix[sourceNode][destinationNode] = false;
	opEdgeTypeMatrix[sourceNode][destinationNode] = false;
	numOfOpEdges--;
	return 0;
}

int HBGraph::opEdgeExists(IDType sourceNode, IDType destinationNode) {
#ifdef SANITYCHECK
	assert(sourceNode > 0);
	assert(destinationNode > 0);
	if (sourceNode == destinationNode) {
		cout << "ERROR: sourceNode == destinationNode: " << sourceNode << "\n";
	}
	assert(sourceNode != destinationNode);
	assert(opAdjMatrix[sourceNode][destinationNode] == true || opAdjMatrix[sourceNode][destinationNode] == false);
	//assert(opAdjMatrix[sourceNode][destinationNode] == opEdgeExistsinList(sourceNode, destinationNode));
#endif

	if (opAdjMatrix[sourceNode][destinationNode])
		return 1;
	else
		return 0;

}

void HBGraph::printGraph() {

	cout << "\nOp Edges:";
	for (IDType i=1; i <= totalOps; i++) {
		cout << endl << i << ": ";
		if (opAdjList.find(i) == opAdjList.end())
			cout << "";
		else {
			adjListNode* currNode = opAdjList[i]->head;
			while (currNode != NULL) {
				cout << currNode->nodeID << " ";
				currNode = currNode->next;
			}
		}
	}
	cout << "\n";
}

bool HBGraph::isSTEdge(IDType sourceNode, IDType destinationNode) {
	return opEdgeTypeMatrix[sourceNode][destinationNode];
}
