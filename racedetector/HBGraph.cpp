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
	totalBlocks = 0;
	totalOps = 0;
	numOfOpEdges = 0;
	numOfBlockEdges = 0;
	numOfOpEdgesRemoved = 0;
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
	numOfOpEdgesRemoved = 0;

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

	opAdjList = new std::multiset<HBGraph::adjListNode> [totalOps+1];
	blockAdjList = new std::multiset<HBGraph::adjListNode> [totalBlocks+1];

	for (IDType i=1; i <= totalOps; i++) {
		opAdjMatrix[i] = (bool*) calloc((totalOps+1) , sizeof(bool));
	}
	for (IDType i=1; i <= totalBlocks; i++) {
		blockAdjMatrix[i] = (bool*) calloc((totalBlocks+1) , sizeof(bool));

		// There is always a self edge for each block since there are HB edges
		// (program-order) within the block
		blockAdjMatrix[i][i] = true;
		HBGraph::adjListNode blockNode(i);
		blockAdjList[i].insert(blockNode);
	}
}

HBGraph::~HBGraph() {
}

int HBGraph::addOpEdge(IDType sourceNode, IDType destinationNode, IDType sourceBlock, IDType destinationBlock) {

	assert(1 <= sourceNode      && sourceNode <= totalOps);
	assert(1 <= destinationNode && destinationNode <= totalOps);
	assert(sourceNode != destinationNode);

	if (sourceBlock == 0) {
		IDType sourceOp = *(nodeIDMap[sourceNode].opSet.begin());
		if (sourceOp <= 0) {
			cout << "ERROR: Invalid op ID for node " << sourceNode << "\n";
			return -1;
		}
		sourceBlock = opIDMap[sourceOp].blockID;
	}

	if (destinationBlock == 0) {
		IDType destinationOp = *(nodeIDMap[destinationNode].opSet.begin());
		if (destinationOp <= 0) {
			cout << "ERROR: Invalid op ID for node " << destinationNode << "\n";
			return -1;
		}
		destinationBlock = opIDMap[destinationOp].blockID;
	}

	assert(sourceBlock > 0);
	assert(destinationBlock > 0);


	int retOpValue = opEdgeExists(sourceNode, destinationNode, sourceBlock, destinationBlock);
	if (retOpValue == 0) {
		if (opEdgeExists(destinationNode, sourceNode, destinationBlock, sourceBlock) == 1) {
			cout << "ERROR: Edge exists from " << destinationNode << " to " << sourceNode 	   << endl;
			cout << "Trying to add edge from " << sourceNode		<< " to " << destinationNode << endl;
			return -1;
		}

		assert(sourceBlock != destinationBlock);

		opAdjMatrix[sourceNode][destinationNode] = true;

		adjListNode destOpNode(destinationNode, destinationBlock);
		opAdjList[sourceNode].insert(destOpNode);

		numOfOpEdges++;

		int retBlockValue = blockEdgeExists(sourceBlock, destinationBlock);
		if (retBlockValue == 0) {
			blockAdjMatrix[sourceBlock][destinationBlock] = true;

			adjListNode destBlockNode(destinationBlock);
			blockAdjList[sourceBlock].insert(destBlockNode);

			numOfBlockEdges++;
		} else if (retBlockValue == -1)
			return -1;
		return 1;
	} else if (retOpValue == 1)
		return 0;
	else
		return -1;

	return -1;
}

// currNode is the adjListNode containing destinationOp in the adjacency list of sourceOp
int HBGraph::removeOpEdge(IDType sourceNode, IDType destinationNode, IDType sourceBlock, IDType destinationBlock) {
	assert(1 <= sourceNode && sourceNode <= totalOps);
	assert(1 <= destinationNode && destinationNode <= totalOps);

	if (destinationBlock == 0) {
		IDType destinationOp = *(nodeIDMap[destinationNode].opSet.begin());
		if (destinationOp <= 0) {
			cout << "ERROR: Invalid op for node ID " << destinationNode << "\n";
			return -1;
		}
		destinationBlock = opIDMap[destinationOp].blockID;
	}

	adjListNode destNode(destinationNode, destinationBlock);
	std::pair<std::multiset<adjListNode>::iterator, std::multiset<adjListNode>::iterator>
		ret = opAdjList[sourceNode].equal_range(destNode);
	if (ret.first != ret.second) {
		std::multiset<adjListNode>::iterator it;
		for (it = ret.first; it != ret.second; it++) {
#ifdef SANITYCHECK
			assert(it->blockID == destinationBlock);
#endif
			if (it->nodeID == destinationNode)
				break;
		}
		if (it != ret.second)
			opAdjList[sourceNode].erase(it);
		else {
			cout << "ERROR: Cannot find the op edge to remove\n";
			cout << "ERROR: Edge: (" << sourceNode << ", " << destinationNode << ")\n";
			return -1;
		}
	} else {
		cout << "ERROR: Cannot find the op edge to remove\n";
		cout << "ERROR: Edge: (" << sourceNode << ", " << destinationNode << ")\n";
		return -1;
	}

	opAdjMatrix[sourceNode][destinationNode] = false;
	numOfOpEdges--;
	numOfOpEdgesRemoved++;
	return 0;
}

int HBGraph::removeOpEdgesToBlock(std::multiset<HBGraph::adjListNode>::iterator first,
		std::multiset<HBGraph::adjListNode>::iterator last,
		IDType sourceNode, IDType destinationBlock) {
	// Assuming this function is called with valid arguments
	for (std::multiset<HBGraph::adjListNode>::iterator it = first; it != last; it++) {
		opAdjMatrix[sourceNode][it->nodeID] = false;
		numOfOpEdges--;
		numOfOpEdgesRemoved++;
	}

	opAdjList[sourceNode].erase(first, last);
	return 0;
}

int HBGraph::opEdgeExists(IDType sourceNode, IDType destinationNode, IDType sourceBlock, IDType destinationBlock) {

	assert(sourceNode > 0);
	assert(destinationNode > 0);
	assert(sourceNode != destinationNode);
	assert(opAdjMatrix[sourceNode][destinationNode] == true || opAdjMatrix[sourceNode][destinationNode] == false);

	if (opAdjMatrix[sourceNode][destinationNode])
		return 1;

	IDType sourceOp = *(nodeIDMap[sourceNode].opSet.begin());
	if (sourceOp <= 0) {
		cout << "ERROR: Invalid op ID for node " << sourceNode << "\n";
		return -1;
	}
	if (sourceBlock == 0)
		sourceBlock = opIDMap[sourceOp].blockID;

	IDType destinationOp = *(nodeIDMap[destinationNode].opSet.begin());
	if (destinationOp <= 0) {
		cout << "ERROR: Invalid op ID for node " << destinationNode << "\n";
		return -1;
	}
	if (destinationBlock == 0)
		destinationBlock = opIDMap[destinationOp].blockID;

	assert(sourceBlock > 0);
	assert(destinationBlock > 0);

	// If both ops are in the same block: then program order edge if sourceNode is before destinationNode
	if (sourceBlock == destinationBlock && sourceNode < destinationNode)
		return 1;
	else if (sourceBlock == destinationBlock && sourceNode >= destinationNode)
		return 0;

	// Check if the edge is implied transitively
	int retValue = blockEdgeExists(sourceBlock, destinationBlock);
	if (retValue == 0)
		return 0;
	else if (retValue == 1) {
		IDType i = blockIDMap[sourceBlock].lastOpInBlock;
		IDType nodei = 0, prevnodei = 0;
		IDType prevminNode = -1;
		while (i > 0 && i >= sourceOp) {
			nodei = opIDMap[i].nodeID;
			if (nodei <= 0) {
				cout << "ERROR: Invalid node ID for op " << i << "\n";
				return -1;
			}
			if (prevnodei != 0) {
				if (prevnodei == nodei) {
					i = opIDMap[i].prevOpInBlock;
					continue;
				}
			}
			prevnodei = nodei;

			HBGraph::adjListNode tempNode(destinationBlock);
			std::pair<nodeIterator, nodeIterator> ret = opAdjList[nodei].equal_range(destinationBlock);
			if (ret.first != ret.second) {
				IDType count = 0;
				IDType minNode = -1;
				for (nodeIterator retIt = ret.first; retIt != ret.second; retIt++) {
					if (minNode == -1 || minNode < retIt->nodeID) {
						minNode = retIt->nodeID;
						count++;
					}
				}

				if (count > 1) {
					removeOpEdgesToBlock(ret.first, ret.second, nodei, destinationBlock);
					if (minNode > 0) {
						int addEdgeRetValue = addOpEdge(nodei, minNode);
						if (addEdgeRetValue == -1) {
							cout << "ERROR: While adding restoration edge from "
								 << nodei << " to " << minNode << "\n";
							return -1;
						}
					}
				} else if (count == 1) {
					if (minNode < destinationNode)
						return 1;
				} else if (count < 1) {
					cout << "ERROR: While finding edges from node " << nodei
						 << " to nodes in block " << destinationBlock << "\n";
					cout << "ERROR: equal_range() gives a non-empty range, but count < 1\n";
					return -1;
				}

				if (prevnodei != 0 && prevminNode != -1) {
					if (minNode != -1 && prevminNode > minNode) {
						int removeEdgeRetValue =
								removeOpEdge(prevnodei, prevminNode, sourceBlock, destinationBlock);
						if (removeEdgeRetValue == -1) {
							cout << "ERROR: While removing edge from node "
								 << prevnodei << " to " << prevminNode << "\n";
							return -1;
						}
					}

					if (minNode < destinationNode)
						return 1;

					prevnodei = nodei;
					prevminNode = minNode;
				}
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

	if (blockAdjMatrix[sourceBlock][destinationBlock])
		return 1;
	return 0;
}

void HBGraph::printGraph() {

	cout << "\nBlock Edges:";
	for (IDType i=1; i <= totalBlocks; i++) {
		cout << endl << i << ": ";
		for (std::multiset<adjListNode>::iterator it = blockAdjList[i].begin();
				it != blockAdjList[i].end(); it++)
			cout << it->blockID << " ";
	}
	cout << "\n";

	cout << "\nOp Edges:";
	for (IDType i=1; i <= totalOps; i++) {
		cout << endl << i << ": ";
		for (std::multiset<adjListNode>::iterator it = opAdjList[i].begin();
				it != opAdjList[i].end(); it++)
			cout << it->nodeID << " ";
	}
	cout << "\n";
}
