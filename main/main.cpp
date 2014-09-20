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

	string traceFileName = argv[1];

	string uafFileName = traceFileName + ".uaf.all";
	Logger uafAllLogger(uafFileName);
	string raceFileName = traceFileName + ".race.all";
	Logger raceAllLogger(raceFileName);

	uafFileName = traceFileName + ".uaf.withnesting";
	Logger uafNestingLogger(uafFileName);
	raceFileName = traceFileName + ".race.withnesting";
	Logger raceNestingLogger(raceFileName);

	uafFileName = traceFileName + ".uaf.withoutnesting";
	Logger uafNoNestingLogger(uafFileName);
	raceFileName = traceFileName + ".race.withoutnesting";
	Logger raceNoNestingLogger(raceFileName);

	uafFileName = traceFileName + ".uaf.withtask";
	Logger uafTaskLogger(uafFileName);
	raceFileName = traceFileName + ".race.withtask";
	Logger raceTaskLogger(raceFileName);

	uafFileName = traceFileName + ".uaf.withouttask";
	Logger uafNoTaskLogger(uafFileName);
	raceFileName = traceFileName + ".race.withouttask";
	Logger raceNoTaskLogger(raceFileName);

	TraceParser parser(traceFileName);
	UAFDetector detectorObj;

	if (parser.parse(detectorObj) < 0) {
		cout << "ERROR while parsing the trace\n";
		return -1;
	}

#ifdef TRACEDEBUG
	cout << "map size: " << detectorObj.opIDMap.size() << endl;
#endif

    //return 0;
	if (detectorObj.addEdges() < 0) {
		cout << "ERROR while constructing HB Graph\n";
		return -1;
	}

	cout << "\nFinding UAF\n";
	int retfindUAF = detectorObj.findUAF(&uafAllLogger,
			&uafNestingLogger, &uafNoNestingLogger, &uafTaskLogger,
			&uafNoTaskLogger);

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
	int retfindRace = detectorObj.findDataRaces(&raceAllLogger,
			&raceNestingLogger, &raceNoNestingLogger, &raceTaskLogger,
			&raceNoTaskLogger);

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
