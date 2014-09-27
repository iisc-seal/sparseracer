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
#include <time.h>

#include <debugconfig.h>

int main(int argc, char* argv[]) {
	if (argc < 2) {
		cout << "ERROR: Missing input\n";
		cout << "Input trace-file name as argument 1\n";
		return -1;
	}

	string traceFileName = argv[1];

	TraceParser parser(traceFileName);
	UAFDetector detectorObj;
	detectorObj.initLog(traceFileName);

	clock_t tStart = clock();
	if (parser.parse(detectorObj) < 0) {
		cout << "ERROR while parsing the trace\n";
		return -1;
	}

#ifdef TRACEDEBUG
	cout << "map size: " << detectorObj.opIDMap.size() << endl;
#endif

	cout << "Time taken for parsing: " << (double)(clock() - tStart)/CLOCKS_PER_SEC << "\n";

    //return 0;

	tStart = clock();
	if (detectorObj.addEdges() < 0) {
		cout << "ERROR while constructing HB Graph\n";
		return -1;
	}

	cout << "Time taken for transitive closure: " << (double)(clock() - tStart)/CLOCKS_PER_SEC << "\n";

	tStart = clock();
	cout << "\nFinding UAF\n";
	int retfindUAF = detectorObj.findUAF();

	if (retfindUAF == -1) {
		cout << "ERROR: While finding UAF\n";
		return -1;
	} else if (retfindUAF == 0) {
		cout << "No UAF in the trace\n";
	} else {
		cout << "OUTPUT: Found " << retfindUAF << " UAFs\n";
	}
	cout << "Time taken for finding UAF: " << (double)(clock() - tStart)/CLOCKS_PER_SEC << "\n";

#ifdef DATARACE
	tStart = clock();
	cout << "\nFinding data races\n";
	int retfindRace = detectorObj.findDataRaces();

	if (retfindRace == -1) {
		cout << "ERROR: While finding Dataraces\n";
		return -1;
	} else if (retfindRace == 0) {
		cout << "No data races in the trace\n";
	} else {
		cout << "OUTPUT: Found " << retfindRace << " races\n";
	}
	cout << "Time taken for finding races: " << (double)(clock() - tStart)/CLOCKS_PER_SEC << "\n";
#endif
	return 0;
}
