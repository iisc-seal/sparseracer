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

HBGraph::HBGraph(){
	totalBlocks = 0;
	totalOps = 0;
	numOfOpEdges = 0;
	numOfBlockEdges = 0;
	opIDMap = map<IDType, UAFDetector::opDetails>();
	blockIDMap = map<IDType, UAFDetector::blockDetails>();
	nodeIDMap = map<IDType, UAFDetector::setOfOps>();

	opAdjList = NULL;
	opAdjMatrix = NULL;
	blockAdjList = NULL;
	blockAdjMatrix = NULL;
}

HBGraph::HBGraph(IDType countOfOps, IDType countOfBlocks,
		map<IDType, UAFDetector::opDetails> opMap,
		map<IDType, UAFDetector::blockDetails> blockMap,
		map<IDType, UAFDetector::setOfOps> nodeMap) {
	totalOps = countOfOps;
	totalBlocks = countOfBlocks;

	numOfOpEdges = 0;
	numOfBlockEdges = 0;

	opIDMap = opMap;
	blockIDMap = blockMap;
	nodeIDMap = nodeMap;

	opAdjMatrix = (bool**) malloc(sizeof(bool*) * (totalOps+1));
	blockAdjMatrix = (bool**) malloc(sizeof(bool*) * (totalBlocks+1));

	if (opAdjMatrix == NULL) {
		cout << "ERROR: Cannot allocate memory for opAdjMatrix\n";
	}
	if (blockAdjMatrix == NULL) {
		cout << "ERROR: Cannot allocate memory for blockAdjMatrix\n";
	}

	opAdjList = new HBGraph::adjListType [totalOps+1];
	blockAdjList = new HBGraph::adjListType [totalBlocks+1];

	for (IDType i=1; i <= totalOps; i++) {
		opAdjList[i].head = NULL;
		opAdjMatrix[i] = (bool*) calloc((totalOps+1) , sizeof(bool));
	}
	for (IDType i=1; i <= totalBlocks; i++) {
		blockAdjList[i].head = NULL;
		blockAdjMatrix[i] = (bool*) calloc((totalBlocks+1) , sizeof(bool));

		// There is always a self edge for each block since there are HB edges
		// (program-order) within the block
		blockAdjMatrix[i][i] = true;
		HBGraph::adjListNode *currNode = HBGraph::createNewNode(i);
		blockAdjList[i].head = currNode;
	}
}

HBGraph::~HBGraph() {
}

int HBGraph::addOpEdge(IDType sourceNode, IDType destinationNode) {

	assert(1 <= sourceNode      && sourceNode <= totalOps);
	assert(1 <= destinationNode && destinationNode <= totalOps);
	assert(sourceNode != destinationNode);

	IDType sourceOp = *(nodeIDMap[sourceNode].opSet.begin());
	if (sourceOp <= 0) {
		cout << "ERROR: Invalid op ID for node " << sourceNode << "\n";
		return -1;
	}
	IDType sourceBlock = opIDMap[sourceOp].blockID;
	IDType destinationOp = *(nodeIDMap[destinationNode].opSet.begin());
	if (destinationOp <= 0) {
		cout << "ERROR: Invalid op ID for node " << destinationNode << "\n";
		return -1;
	}
	IDType destinationBlock = opIDMap[destinationOp].blockID;

	assert(sourceBlock > 0);
	assert(destinationBlock > 0);
	assert(sourceBlock != destinationBlock);

	IDType i = opIDMap[sourceOp].nextOpInBlock;
	IDType lastOp1 = blockIDMap[sourceBlock].lastOpInBlock;

	while (i > 0 && i <= lastOp1) {
		IDType j = blockIDMap[destinationBlock].firstOpInBlock;
		while (j > 0 && j < destinationOp) {
			IDType nodei = opIDMap[i].nodeID;
			IDType nodej = opIDMap[j].nodeID;
			if (nodei <= 0) {
				cout << "ERROR: Invalid node ID for op " << i << "\n";
				return -1;
			}
			if (nodej <= 0) {
				cout << "ERROR: Invalid node ID for op " << j << "\n";
				return -1;
			}
			if (opEdgeExists(nodei, nodej))
				// Required edge already implied transitively
				return 0;

			j = opIDMap[j].nextOpInBlock;
		}

		i = opIDMap[i].nextOpInBlock;
	}

	if (opEdgeExists(destinationNode, sourceNode) == 1) {
		cout << "ERROR: Edge exists from " << destinationNode << " to " << sourceNode 	   << endl;
		cout << "Trying to add edge from " << sourceNode		<< " to " << destinationNode << endl;
		return -1;
	}

	int retBlockValue = blockEdgeExists(sourceBlock, destinationBlock);
	if (retBlockValue == 0) {
		blockAdjMatrix[sourceBlock][destinationBlock] = true;

		struct adjListNode* destBlockNode = createNewNode(destinationBlock);
		destBlockNode->next = blockAdjList[sourceBlock].head;

		if (blockAdjList[sourceBlock].head != NULL)
			blockAdjList[sourceBlock].head->prev = destBlockNode;

		blockAdjList[sourceBlock].head = destBlockNode;

		numOfBlockEdges++;

		int retOpValue = opEdgeExists(sourceNode, destinationNode);
		if (retOpValue == 0) {
			opAdjMatrix[sourceNode][destinationNode] = true;

			struct adjListNode* destOpNode = createNewNode(destinationNode);
			destOpNode->next = opAdjList[sourceNode].head;

			if (opAdjList[sourceNode].head != NULL)
				opAdjList[sourceNode].head->prev = destOpNode;

			opAdjList[sourceNode].head = destOpNode;

			numOfOpEdges++;

			return 1;
		} else if (retOpValue == 1)
			return 0;
		else
			return -1;
	} else if (retBlockValue == 1) {
		int retOpValue = opEdgeExists(sourceNode, destinationNode);
		if (retOpValue == 0) {
			opAdjMatrix[sourceNode][destinationNode] = true;

			struct adjListNode* destOpNode = createNewNode(destinationNode);
			destOpNode->next = opAdjList[sourceNode].head;

			if (opAdjList[sourceNode].head != NULL)
				opAdjList[sourceNode].head->prev = destOpNode;

			opAdjList[sourceNode].head = destOpNode;

			numOfOpEdges++;

			return 1;
		} else if (retOpValue == 1)
			return 0;
		else
			return -1;
	}
	else
		return -1;

	return -1;
}

// currNode is the adjListNode containing destinationOp in the adjacency list of sourceOp
void HBGraph::removeOpEdge(HBGraph::adjListNode* currNode, IDType sourceNode, IDType destinationNode) {
	assert(currNode != NULL);
	assert(1 <= sourceNode && sourceNode <= totalOps);
	assert(1 <= destinationNode && destinationNode <= totalOps);

	if (currNode->next == NULL && currNode->prev == NULL) {
		// This is the only node in the list.
		// Set adjacency list of sourceNode to NULL
		opAdjList[sourceNode].head = NULL;
	} else if (currNode->next == NULL) {
		// No node after this node in the list. Make the previous node point to NULL
		currNode->prev->next = NULL;
	} else if (currNode->prev == NULL) {
		// This is the first node in the list. Make next node the head of adjacency list
		currNode->next->prev = NULL;
		opAdjList[sourceNode].head = currNode->next;
	} else {
		currNode->prev->next = currNode->next;
		currNode->next->prev = currNode->prev;
	}

	opAdjMatrix[sourceNode][destinationNode] = false;
	delete currNode;
}

int HBGraph::opEdgeExists(IDType sourceNode, IDType destinationNode) {

	assert(sourceNode > 0);
	assert(destinationNode > 0);
	assert(sourceNode != destinationNode);
	assert(opAdjMatrix[sourceNode][destinationNode] == true || opAdjMatrix[sourceNode][destinationNode] == false);

	IDType sourceOp = *(nodeIDMap[sourceNode].opSet.begin());
	if (sourceOp <= 0) {
		cout << "ERROR: Invalid op ID for node " << sourceNode << "\n";
		return -1;
	}
	IDType sourceBlock = opIDMap[sourceOp].blockID;
	IDType destinationOp = *(nodeIDMap[destinationNode].opSet.begin());
	if (destinationOp <= 0) {
		cout << "ERROR: Invalid op ID for node " << destinationNode << "\n";
		return -1;
	}
	IDType destinationBlock = opIDMap[destinationOp].blockID;
	assert(sourceBlock > 0);
	assert(destinationBlock > 0);

	// If both ops are in the same block: then program order edge if sourceNode is before destinationNode
	if (sourceBlock == destinationBlock && sourceNode < destinationNode)
		return 1;
	else if (sourceBlock == destinationBlock && sourceNode >= destinationNode)
		return 0;

	// Check if there is an explicit edge in the opAdjMatrix and opAdjList
	bool edgeExistsInList = opEdgeExistsinList(sourceNode, destinationNode);
	if (opAdjMatrix[sourceNode][destinationNode] == true && edgeExistsInList)
		return 1;
	else if (opAdjMatrix[sourceNode][destinationNode] == true && !edgeExistsInList) {
		cout << "ERROR: Op-Edge (" << sourceNode << ", " << destinationNode << ") exists in opAdjMatrix but not in opAdjList\n";
		return -1;
	} else if (opAdjMatrix[sourceNode][destinationNode] == false && edgeExistsInList) {
		cout << "ERROR: Op-Edge (" << sourceNode << ", " << destinationNode << ") exists in opAdjList but not in opAdjMatrix\n";
		return -1;
	}

	// Check if the edge is implied transitively
	int retValue = blockEdgeExists(sourceBlock, destinationBlock);
	if (retValue == 0)
		return 0;
	else if (retValue == 1) {
		IDType i = blockIDMap[sourceBlock].lastOpInBlock;
		while (i > 0 && i >= sourceOp) {
			IDType j = blockIDMap[destinationBlock].firstOpInBlock;
			while (j > 0 && j <= destinationOp) {

				IDType nodei = opIDMap[i].nodeID;
				IDType nodej = opIDMap[j].nodeID;
				if (nodei <= 0) {
					cout << "ERROR: Invalid node ID for op " << i << "\n";
					return -1;
				}
				if (nodej <= 0) {
					cout << "ERROR: Invalid node ID for op " << j << "\n";
					return -1;
				}
				edgeExistsInList = opEdgeExistsinList(nodei, nodej);
				if (opAdjMatrix[nodei][nodej] == true && edgeExistsInList)
					return 1;
				else if (opAdjMatrix[nodei][nodej] == true && !edgeExistsInList) {
					cout << "ERROR: Op-Edge (" << nodei << ", " << nodej << ") exists in opAdjMatrix but not in opAdjList\n";
					return -1;
				} else if (opAdjMatrix[nodei][nodej] == false && edgeExistsInList) {
					cout << "ERROR: Op-Edge (" << nodei << ", " << nodej << ") exists in opAdjList but not in opAdjMatrix\n";
					return -1;
				}

				j = opIDMap[j].nextOpInBlock;
			}
			i = opIDMap[i].prevOpInBlock;
		}

		return 0;
	} else
		return -1;

	return 0;
}

int HBGraph::blockEdgeExists(IDType sourceBlock, IDType destinationBlock) {

	assert(sourceBlock > 0);
	assert(destinationBlock > 0);
	assert(sourceBlock != destinationBlock);
	assert(blockAdjMatrix[sourceBlock][destinationBlock] == true || blockAdjMatrix[sourceBlock][destinationBlock] == false);

	bool edgeExistsInList = blockEdgeExistsinList(sourceBlock, destinationBlock);
	if (blockAdjMatrix[sourceBlock][destinationBlock] == true && edgeExistsInList)
		return 1;
	else if (blockAdjMatrix[sourceBlock][destinationBlock] == true && !edgeExistsInList) {
		cout << "ERROR: Block-Edge (" << sourceBlock << ", " << destinationBlock << ") exists in blockAdjMatrix but not in blockAdjList\n";
		return -1;
	} else if (blockAdjMatrix[sourceBlock][destinationBlock] == false && edgeExistsInList) {
		cout << "ERROR: Block-Edge (" << sourceBlock << ", " << destinationBlock << ") exists in blockAdjList but not in blockAdjMatrix\n";
		return -1;
	}

	return 0;
}

void HBGraph::printGraph() {

	cout << "\nBlock Edges:";
	for (IDType i=1; i <= totalBlocks; i++) {
		cout << endl << i << ": ";
		struct adjListNode* currNode = blockAdjList[i].head;
		while (currNode != NULL) {
			cout << currNode->destination << "   ";
			currNode = currNode->next;
		}
	}
	cout << "\n";

	cout << "\nOp Edges:";
	for (IDType i=1; i <= totalOps; i++) {
		cout << endl << i << ": ";
		struct adjListNode* currNode = opAdjList[i].head;
		while (currNode != NULL) {
			cout << currNode->destination << "   ";
			currNode = currNode->next;
		}
	}
	cout << "\n";
}
