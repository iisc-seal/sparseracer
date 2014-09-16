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
	if (argc < 2) {
		cout << "ERROR: Missing input\n";
		cout << "Input trace-file name as argument 1\n";
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

    //return 0;
	if (detectorObj.addEdges(logger) < 0) {
		cout << "ERROR while constructing HB Graph\n";
		return -1;
	}

	cout << "\nFinding UAF\n";
	int retfindUAF = detectorObj.findUAFwithoutAlloc(logger);

	if (retfindUAF == -1) {
		cout << "ERROR: While finding UAF\n";
		return -1;
	} else if (retfindUAF == 0) {
		cout << "No UAF in the trace\n";
	} else {
		cout << "OUTPUT: Found " << retfindUAF << " UAFs\n";
	}

#ifdef DATARACE
	cout << "\nFinding data races\n";
	int retfindRace = detectorObj.findDataRaces(logger);

	if (retfindRace == -1) {
		cout << "ERROR: While finding Dataraces\n";
		return -1;
	} else if (retfindRace == 0) {
		cout << "No data races in the trace\n";
	} else {
		cout << "OUTPUT: Found " << retfindRace << " races\n";
	}
#endif
	return 0;
}
