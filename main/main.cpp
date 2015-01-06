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
#include <cstring>

#include <debugconfig.h>

std::string convertTime(clock_t startTime, clock_t endTime) {
	std::string time;
	std::ostringstream stream;
	unsigned long long elapsed_secs = (endTime - startTime)/CLOCKS_PER_SEC;

	if (elapsed_secs == 0) {
		double secs = (double) (endTime - startTime)/CLOCKS_PER_SEC;

		double milliseconds = secs * 1000;

		stream << milliseconds << "ms";
		time = stream.str();
	} else {
		int hrs = elapsed_secs / 3600;
		unsigned long long remaining = elapsed_secs % 3600;

		int mins = remaining / 60;
		remaining = remaining % 60;

		int secs = remaining;

		stream << hrs << "h" << mins << "m" << secs << "s";
		time = stream.str();
	}

	return time;
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		cout << "ERROR: Missing input\n";
		cout << "Input trace-file name as argument 1\n";
		return -1;
	}

	string traceFileName = argv[1];

	TraceParser parser(traceFileName);

	bool outputAllConflictingOps = false;
	bool filterInput = false;
	bool runDetectorOnTrace = false;
	string outputAllOpsFileName, filterInputFileName, filterOutputFileName;
	for (int i = 2; i < argc; i++) {
		if (strcmp(argv[i], "-a") == 0) {
			outputAllConflictingOps = true;
			outputAllOpsFileName = traceFileName + ".allconflictingops";
		} else if (strcmp(argv[i], "-f") == 0) {
			filterInput = true;
			filterInputFileName = argv[i+1];
			filterOutputFileName = filterInputFileName + ".filtered";
			i++;
		} else if (strcmp(argv[i], "-r") == 0) {
			runDetectorOnTrace = true;
		} else {
			cout << "ERROR: Invalid argument " << argv[i] << "\n";
			exit(0);
		}
	}
	UAFDetector detectorObj;

	clock_t totalStart, totalEnd, tStart, tEnd;
	totalStart = clock();
	tStart = totalStart;
	int retParse = parser.parse(detectorObj);
	if (retParse == -2) {
		cout << "Giving up! More than 15k nodes\n";
		return -1;
	} else if (retParse < 0) {
		cout << "ERROR while parsing the trace\n";
		return -1;
	}

	if (outputAllConflictingOps) {
		detectorObj.outputAllConflictingOps(outputAllOpsFileName);
		return 0;
	}

	if (!filterInput && !runDetectorOnTrace)
		return 0;

#ifdef TRACEDEBUG
	cout << "map size: " << detectorObj.opIDMap.size() << endl;
#endif
	tEnd = clock();
	cout << "Time taken for parsing: " << convertTime(tStart, tEnd) << "\n";

        //return 0;

	tStart = clock();
	if (detectorObj.addEdges() < 0) {
		cout << "ERROR while constructing HB Graph\n";
		return -1;
	}
	tEnd = clock();
	cout << "Time taken for transitive closure: " << convertTime(tStart, tEnd) << "\n";

	detectorObj.initLog(traceFileName);
	if (runDetectorOnTrace) {
		tStart = clock();
		cout << "\nFinding UAF\n";
#ifdef NODERACES
		int retfindUAF = detectorObj.findUAFUsingNodes();
#else
		int retfindUAF = detectorObj.findUAF();
#endif

		if (retfindUAF == -1) {
			cout << "ERROR: While finding UAF\n";
			return -1;
		} else if (retfindUAF == 0) {
			cout << "No UAF in the trace\n";
		} else {
			cout << "OUTPUT: Found " << retfindUAF << " UAFs\n";
		}
		tEnd = clock();
		cout << "Time taken for finding UAF: " << convertTime(tStart, tEnd) << "\n";

#ifdef DATARACE
		tStart = clock();
		cout << "\nFinding data races\n";
#ifdef NODERACES
		int retfindRace = detectorObj.findDataRacesUsingNodes();
#else
		int retfindRace = detectorObj.findDataRaces();
#endif

		if (retfindRace == -1) {
			cout << "ERROR: While finding Data races\n";
			return -1;
		} else if (retfindRace == 0) {
			cout << "No data races in the trace\n";
		} else {
			cout << "OUTPUT: Found " << retfindRace << " races\n";
		}
		tEnd = clock();
		cout << "Time taken for finding races: " << convertTime(tStart, tEnd) << "\n";
#endif

		detectorObj.log();
	}

	if (filterInput) {
		if (detectorObj.filterInput(filterInputFileName, filterOutputFileName) < 0) {
			cout << "ERROR: While filtering races\n";
			return -1;
		}
	}

	totalEnd = clock();
	cout << "Total time taken: " << convertTime(totalStart, totalEnd) << "\n";
	return 0;
}
