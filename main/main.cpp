/*
 * main.cpp
 *
 *  Created on: 29-May-2014
 *      Author: shalini
 */

#include <iostream>
#include <parser/TraceParser.h>
#include <racedetector/UAFDetector.h>
#include <logging/Logger.h>

#include <debugconfig.h>

int main(int argc, char* argv[]) {
	if (argc < 2) {
		cout << "ERROR: Missing input:- trace-file name\n";
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

//#ifdef GRAPHDEBUG
#ifdef TRACEDEBUG
	detectorObj.printEdges();
#endif

	detectorObj.findUAF(logger);

	return 0;
}
