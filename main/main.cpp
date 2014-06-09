/*
 * main.cpp
 *
 *  Created on: 29-May-2014
 *      Author: shalini
 */

#include <iostream>
#include <cstdlib>
#include <parser/TraceParser.h>
#include <racedetector/UAFDetector.h>
#include <logging/Logger.h>

#include <debugconfig.h>

int main(int argc, char* argv[]) {
	if (argc < 3) {
		cout << "ERROR: Missing input\n";
		cout << "Input trace-file name as argument 1\n";
		cout << "Input 1 as argument 2 if you want to use alloc while finding UAF\n";
		return -1;
	}

	string logFileName = "UAF.log";

	Logger logger(logFileName);
	logger.initLog();

	TraceParser parser(argv[1], logger);
	UAFDetector detectorObj;

	if (parser.parse(detectorObj, logger) < 0) {
		cout << "ERROR while parsing the trace\n";
		return -1;
	}

#ifdef TRACEDEBUG
	cout << "map size: " << detectorObj.opIDMap.size() << endl;
#endif

	if (detectorObj.addEdges(logger) < 0) {
		cout << "ERROR while constructing HB Graph\n";
		return -1;
	}

#ifdef GRAPHDEBUG
	detectorObj.printEdges();
#endif

	int flag = atoi(argv[2]);
	int retfindUAF;
	if (flag == 1) {
		retfindUAF = detectorObj.findUAFusingAlloc(logger);
	} else {
		retfindUAF = detectorObj.findUAFwithoutAlloc(logger);
	}

	if (retfindUAF == -1) {
		cout << "ERROR: While finding UAF\n";
		return -1;
	} else if (retfindUAF == 0) {
		cout << "No UAF in the trace\n";
		return 0;
	}

	return 0;
}
